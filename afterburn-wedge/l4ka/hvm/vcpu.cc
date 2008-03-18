/*********************************************************************
 *
 * Copyright (C) 2005,2007-2008  University of Karlsruhe
 *
 * File path:     afterburn-wedge/l4-common/vm.cc
 * Description:   The L4 state machine for implementing the idle loop,
 *                and the concept of "returning to user".  When returning
 *                to user, enters a dispatch loop, for handling L4 messages.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 ********************************************************************/

#include <debug.h>
#include <console.h>
#include <l4/schedule.h>
#include <l4/ipc.h>
#include <l4/ipc.h>
#include <l4/schedule.h>
#include <l4/kip.h>
#include <l4/arch.h>
#include <device/portio.h>

#include INC_ARCH(ia32.h)
#include INC_WEDGE(vcpu.h)
#include INC_WEDGE(monitor.h)
#include INC_WEDGE(l4privileged.h)
#include INC_WEDGE(backend.h)
#include INC_WEDGE(vcpulocal.h)
#include INC_WEDGE(memory.h)
#include INC_WEDGE(hthread.h)
#include INC_WEDGE(message.h)
#include INC_WEDGE(module_manager.h)
#include INC_WEDGE(irq.h)
#include INC_ARCH(hvm_vmx.h)

bool mr_save_t::append_irq(L4_Word_t vector, L4_Word_t irq)
{
    flags_t eflags;
    hvm_vmx_gs_ias_t ias;
    
    // assert that we don't already have a pending exception
    ASSERT (exc_item.item.num_regs == 0);

    eflags.x.raw = gpr_item.regs.eflags;
    ias.raw = nonreg_item.regs.ias;

    if (get_cpu().interrupts_enabled() && ias.raw == 0)
    {
	ASSERT(!get_cpu().cr0.real_mode());
	
	dprintf(irq_dbg_level(irq)+1,
		"INTLOGIC sync irq %d vec %d efl %x (%c) ias %x\n", irq, 
		vector, get_cpu().flags, (get_cpu().interrupts_enabled() ? 'I' : 'i'), 
		ias.raw);
	
	exc_info_t exc;
	exc.raw = 0;
	exc.hvm.type = hvm_vmx_int_t::ext_int;
	exc.hvm.vector = vector;
	exc.hvm.err_code_valid = 0;
	exc.hvm.valid = 1;
	append_exc_item(exc.raw, 0, 0);
	return true;
    }
    
    dprintf(irq_dbg_level(irq)+1,
	    "INTLOGIC sync iwe irq %d vec %d efl %x (%c) ias %x\n", 
	    irq, vector, eflags, (eflags.interrupts_enabled() ? 'I' : 'i'), ias.raw);
    
    // inject IWE 
    hvm_vmx_exectr_cpubased_t cpubased;
    cpubased.raw = execctrl_item.regs.cpu;
    cpubased.iw = 1;
    append_execctrl_item(L4_CTRLXFER_EXEC_CPU, cpubased.raw);

    return false;
}


void vcpu_t::load_dispatch_exit_msg(L4_Word_t vector, L4_Word_t irq)
{
    mr_save_t *vcpu_mrs = &main_info.mr_save;

    dispatch_ipc_exit();
    if (!vcpu_mrs->append_irq(vector, irq))
	get_intlogic().reraise_vector(vector, irq);

    vcpu_mrs->load_vfault_reply();

    
    //Inject IRQ or IWE
    vcpu_mrs->set_propagated_reply(monitor_gtid);
    vcpu_mrs->load();
}

bool vcpu_t::handle_wedge_pfault(thread_info_t *ti, map_info_t &map_info, bool &nilmapping)
{
    const word_t wedge_paddr = get_wedge_paddr();
    const word_t wedge_end_paddr = get_wedge_end_paddr();
    vcpu_t &vcpu = get_vcpu();
    
    if (ti->get_tid() == vcpu.irq_gtid || ti->get_tid() == vcpu.irq_ltid)
    {
	idl4_fpage_t fp;
	CORBA_Environment ipc_env = idl4_default_environment;

	dprintf(debug_pfault, "Wedge pfault from IRQ thread %x\n", map_info.addr);
	idl4_set_rcv_window( &ipc_env, L4_CompleteAddressSpace );
	IResourcemon_pagefault( L4_Pager(), map_info.addr, map_info.addr, map_info.rwx, &fp, &ipc_env);
	nilmapping = true;
	return true;
    }	
    
    if ((map_info.addr >= wedge_paddr) && (map_info.addr < wedge_end_paddr))
    {
	extern word_t dummy_wedge_page;
	printf( "Wedge (Phys) pfault, subsitute with scratch page %x\n", dummy_wedge_page);
	map_info.addr = dummy_wedge_page;
	return true;
    }
    return false;
	
}

bool vcpu_t::resolve_paddr(thread_info_t *ti, map_info_t &map_info, word_t &paddr, bool &nilmapping)
{
    paddr = map_info.addr;
    return false;
}


static void prepare_startup(L4_Word_t cs, L4_Word_t ss, bool real_mode)
{
    mr_save_t *vcpu_mrs = &get_vcpu().main_info.mr_save;

    if( real_mode ) 
    {
	printf("untested switch real mode");
	
	vcpu_mrs->gpr_item.regs.eflags |= (X86_FLAGS_VM | X86_FLAGS_IOPL(3));
	
	// if the disk is larger than 3 MB, assume it is a hard disk
	
	if( resourcemon_shared.ramdisk_size >= MB(3) ) 
	    vcpu_mrs->gpr_item.regs.edx = 0x80;
	else 
	    vcpu_mrs->gpr_item.regs.edx = 0;
	
	
	//Segment registers (VM8086 mode)
	vcpu_mrs->append_seg_item(L4_CTRLXFER_CSREGS_ID, cs, cs << 4, 0xffff, 0xf3);
	vcpu_mrs->append_seg_item(L4_CTRLXFER_SSREGS_ID, ss, ss << 4, 0xffff, 0xf4);
	vcpu_mrs->append_seg_item(L4_CTRLXFER_DSREGS_ID, 0, 0, 0xffff, 0xf3);
	vcpu_mrs->append_seg_item(L4_CTRLXFER_ESREGS_ID, 0, 0, 0xffff, 0xf3);
	vcpu_mrs->append_seg_item(L4_CTRLXFER_FSREGS_ID, 0, 0, 0xffff, 0xf3);
	vcpu_mrs->append_seg_item(L4_CTRLXFER_GSREGS_ID, 0, 0, 0xffff, 0xf3);
	
	hvm_vmx_segattr_t attr;
	attr.raw		= 0;
	attr.type		= 0x3;
	attr.p			= 1;
	// Mark tss as unused, to consistently generate #GPs
	attr.raw		&= ~(1<<16);
	printf("tr attr %x\n", attr.raw);
	vcpu_mrs->append_seg_item(L4_CTRLXFER_TRREGS_ID, 0, 0, ~0, attr.raw);
	vcpu_mrs->append_seg_item(L4_CTRLXFER_LDTRREGS_ID, 0, 0, ~0, 0x10082);
	
	get_cpu().cr0.x.raw = 0x00000000;
	get_cpu().cr4.x.raw = 0x000023d0;
	DEBUGGER_ENTER("UNTESTED");				

    } 
    else 
    { 
	// protected mode
	vcpu_mrs->gpr_item.regs.esi = 0x9022;
	
	vcpu_mrs->append_seg_item(L4_CTRLXFER_CSREGS_ID, 1<<3, 0, ~0, 0x0c099);
	vcpu_mrs->append_seg_item(L4_CTRLXFER_SSREGS_ID, 0, 0, 0, 0x10000);
	vcpu_mrs->append_seg_item(L4_CTRLXFER_DSREGS_ID, 2<<3, 0, ~0, 0x0c093);
	vcpu_mrs->append_seg_item(L4_CTRLXFER_ESREGS_ID, 0, 0, 0, 0x10000);
	vcpu_mrs->append_seg_item(L4_CTRLXFER_FSREGS_ID, 0, 0, 0, 0x10000);
	vcpu_mrs->append_seg_item(L4_CTRLXFER_GSREGS_ID, 0, 0, 0, 0x10000);
	vcpu_mrs->append_seg_item(L4_CTRLXFER_TRREGS_ID, 0, 0, ~0, 0x0808b);
	vcpu_mrs->append_seg_item(L4_CTRLXFER_LDTRREGS_ID, 0, 0, 0, 0x18003);
	
	get_cpu().cr0.x.raw = 0x00000031;
	get_cpu().cr4.x.raw = 0x000023d0;
    }
    vcpu_mrs->append_cr_item(L4_CTRLXFER_CREGS_CR0, get_cpu().cr0.x.raw);
    vcpu_mrs->append_cr_item(L4_CTRLXFER_CREGS_CR0_SHADOW, get_cpu().cr0.x.raw);
    vcpu_mrs->append_cr_item(L4_CTRLXFER_CREGS_CR4_SHADOW, get_cpu().cr4.x.raw);

}

bool main_init( L4_Word_t prio, L4_ThreadId_t pager_tid, hthread_func_t start_func, vcpu_t *vcpu)
{
    // Allocate main thread
    L4_ThreadId_t scheduler, pager;
    L4_Error_t errcode;
    mr_save_t *vcpu_mrs = &get_vcpu().main_info.mr_save;
    L4_Word_t mr = 1;

    vcpu->main_gtid = get_hthread_manager()->thread_id_allocate();
    ASSERT( vcpu->main_gtid != L4_nilthread );

    scheduler = vcpu->monitor_gtid;
    pager = vcpu->monitor_gtid;

    // create thread
    errcode = ThreadControl( vcpu->main_gtid, vcpu->main_gtid, L4_Myself(), L4_nilthread, 0xeffff000 );
    if( errcode != L4_ErrOk )
    {
	printf( "Error: failure creating main thread, tid %t scheduler tid %d L4 errcode: %d\n",
		vcpu->main_gtid, scheduler, errcode);
	return false;
    }
    
    // create address space
    errcode = SpaceControl( vcpu->main_gtid, 1 << 30, L4_Fpage( 0xefffe000, 0x1000 ), 
			       L4_Fpage( 0xeffff000, 0x1000 ), L4_nilthread );
    
    if( errcode != L4_ErrOk )
    {
	printf( "Error: failure creating vcpu space, tid %t, L4 errcode code %d\n", 
		vcpu->main_gtid, errcode);
	goto err;
    }

    // set the thread's priority
    if( !L4_Set_Priority( vcpu->main_gtid, prio) )
    {
	printf( "Error: failure setting vcpus priority, tid %t, L4 error code %d\n", 
		vcpu->main_gtid, errcode);
	goto err;
    }

    // make the thread valid
    errcode = ThreadControl( vcpu->main_gtid, vcpu->main_gtid, scheduler, pager, -1UL);
    if( errcode != L4_ErrOk ) {
	printf( "Error: failure starting thread guest's priority, tid %t, L4 error code %d\n", 
		vcpu->main_gtid, errcode);
	goto err;
    }

    if (vcpu->init_info.vcpu_bsp)
    {   
	printf("VCPU load and preeboot\n");
	resourcemon_init_complete();
	// Load the kernel into memory and rewrite its instructions.
	if( !backend_load_vcpu(*vcpu) )
	    panic();
	// Prepare the emulated CPU and environment.  
	if( !backend_preboot(*vcpu) )
	    panic();
    }
    
    //Read execution control fields
    vcpu_mrs->init_msg();
    vcpu_mrs->append_execctrl_item(L4_CTRLXFER_EXEC_PIN, 0);
    vcpu_mrs->append_execctrl_item(L4_CTRLXFER_EXEC_CPU, 0);
    vcpu_mrs->append_execctrl_item(L4_CTRLXFER_EXEC_EXC_BITMAP, 0);
    vcpu_mrs->load_execctrl_item();
    L4_ReadCtrlXferItems(vcpu->main_gtid);
    vcpu_mrs->store_excecctrl_item(mr);
    dprintf(debug_startup, "VCPU execution control pin %x cpu %x exb_bmp %x\n", 
	    vcpu_mrs->execctrl_item.regs.pin,vcpu_mrs->execctrl_item.regs.cpu, 
	    vcpu_mrs->execctrl_item.regs.exc_bitmap);
    vcpu_mrs->dump(debug_startup, true);

    prepare_startup(vcpu->init_info.entry_cs, vcpu->init_info.entry_ss, vcpu->init_info.real_mode);
    setup_thread_faults(vcpu->main_gtid, true);

    return true;

err:
    
    // delete thread and space
    ThreadControl(vcpu->main_gtid, L4_nilthread, L4_nilthread, L4_nilthread, -1UL);
    return false;
}
	