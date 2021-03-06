/*********************************************************************
 *                
 * Copyright (C) 2008,  Karlsruhe University
 *                
 * File path:     vm8086.cc
 * Description:   
 *                
 * @LICENSE@
 *                
 * $Id:$
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
#include INC_ARCH(ops.h)
#include INC_WEDGE(vcpu.h)
#include INC_WEDGE(monitor.h)
#include INC_WEDGE(l4privileged.h)
#include INC_WEDGE(backend.h)
#include INC_WEDGE(vcpulocal.h)
#include INC_WEDGE(memory.h)
#include INC_WEDGE(l4thread.h)
#include INC_WEDGE(message.h)
#include INC_WEDGE(irq.h)
#include INC_ARCH(hvm_vmx.h)

extern bool handle_cr_fault();
extern bool handle_io_fault();
extern bool handle_hlt_fault();

typedef struct
{
    u16_t ip;
    u16_t cs;
} ia32_ive_t;

bool vm8086_sync_deliver_exception( exc_info_t exc, L4_Word_t eec)
{
    u16_t *stack;
    ia32_ive_t *int_vector;
    bool ext_int = (exc.hvm.type == hvm_vmx_int_t::ext_int);
    mrs_t *vcpu_mrs = &get_vcpu().main_info.mrs;

    if(!(get_cpu().interrupts_enabled()) && ext_int) 
    {
	printf( "ext int injection with disabled interrupts.\n");
	DEBUGGER_ENTER("VM8086 BUG");
    }
    
    debug_id_t id = (exc.hvm.vector == 9) ? irq_dbg_level(1) : debug_hvm_vm8086;
    

    /*    dprintf(id, "hvm: vm8086 deliver exception %x (t %d vec %d eecv %c), eec %d, ilen %d\n", 
	    exc.raw, exc.hvm.type, exc.hvm.vector, exc.hvm.err_code_valid ? 'y' : 'n', 
	    eec, vcpu_mrs->nonregexc_item.regs.entry_ilen);*/
    if(!ext_int)
	dprintf(id, "hvm: vm8086 deliver exception at %x, vec %d\n", vcpu_mrs->gpr_item.regs.eip, exc.hvm.vector);
    
   
    word_t eesp;
    bool mbt = backend_async_read_eaddr(L4_CTRLXFER_SSREGS_ID, vcpu_mrs->gpr_item.regs.esp & 0xffff, eesp);
    ASSERT(mbt);
    stack = (u16_t *) eesp;
   
    // push eflags, cs and ip onto the stack

    *(--stack) = vcpu_mrs->gpr_item.regs.eflags & 0xffff;
    *(--stack) = vcpu_mrs->seg_item[0].regs.segreg & 0xffff;

    if (ext_int)
	*(--stack) = vcpu_mrs->gpr_item.regs.eip & 0xffff;
    else
	*(--stack) = (vcpu_mrs->gpr_item.regs.eip + vcpu_mrs->hvm.ilen) & 0xffff;

    // get entry in interrupt vector table from guest
    int_vector = (ia32_ive_t*) ( exc.hvm.vector * 4 );

    //printf("exc ive %x %x @%x\n", int_vector->cs, int_vector->ip, int_vector);

    if( vcpu_mrs->gpr_item.regs.esp-6 < 0 )
	DEBUGGER_ENTER("stackpointer below segment");

    vcpu_mrs->gpr_item.regs.eip = int_vector->ip;
    vcpu_mrs->gpr_item.regs.esp -= 6;
    vcpu_mrs->gpr_item.regs.eflags &= ~X86_FLAGS_IF;

    vcpu_mrs->append_gpr_item();
    vcpu_mrs->append_seg_item(L4_CTRLXFER_CSREGS_ID, int_vector->cs, (int_vector->cs << 4 ), 
			      vcpu_mrs->seg_item[0].regs.limit, vcpu_mrs->seg_item[0].regs.attr);
    
    // make exc_item valid (but exc itself invalid, to prevent other exceptions
    // to be appended 
    exc.hvm.valid = 0;
    vcpu_mrs->append_exc_item(exc.raw, eec, vcpu_mrs->hvm.ilen);

    return true;
}



extern bool handle_vm8086_gp(exc_info_t exc, word_t eec, word_t cr2)
{
    word_t data_size		= 16;
    word_t addr_size		= 16;
    word_t addr			= -1UL;
    word_t data			= 0UL;
    bool rep			= false;
    bool seg_ovr		= false;
    word_t seg_id		= 0;
    word_t ereg			= 0;
    u8_t *linear_ip;
    ia32_modrm_t modrm;
    
    vcpu_t &vcpu = get_vcpu();
    mrs_t *vcpu_mrs = &vcpu.main_info.mrs;
    hvm_vmx_ei_qual_t qual;
    exc_info_t int_exc;

    bool mbt = backend_async_read_eaddr(L4_CTRLXFER_CSREGS_ID, vcpu_mrs->gpr_item.regs.eip, ereg);
    ASSERT(mbt);
    
    dprintf(debug_hvm_vm8086, 
	    "hvm: vm8086 exc %x (type %d vec %d eecv %c), eec %d ip %x ilen %d\n", 
	    exc.raw, exc.hvm.type, exc.hvm.vector, exc.hvm.err_code_valid ? 'y' : 'n', 
	    vcpu_mrs->nonregexc_item.regs.idt_eec, ereg, vcpu_mrs->hvm.ilen);
    
    linear_ip = (u8_t *) ereg;
    
    // Read the faulting instruction.

    // Strip prefixes.
    while (*linear_ip == 0x26 
	   || *linear_ip == 0x2e
	   || *linear_ip == 0x66 
	   || *linear_ip == 0x67 
	   || *linear_ip == 0xf3
	   || *linear_ip == 0xf2)
    {
	switch (*(linear_ip++))
	{
	case 0x26:
	    seg_id = L4_CTRLXFER_ESREGS_ID;
	    seg_ovr = true;
	    break;
	case 0x2e:
	    seg_id = L4_CTRLXFER_CSREGS_ID;
	    seg_ovr = true;
	    break;
	case 0x66:
	    data_size = 32;
	    break;
	case 0x67:
	    addr_size = 32;
	    break;
	case 0xf2:
	case 0xf3:
	    rep = true;
	    break;
	}
    }
    
    // Decode instruction.
    switch (*linear_ip)
    {
    case OP_2BYTE:				// mov, etc.
	switch (*(linear_ip + 1))
	{
	case OP_LLTL:
	{
	    printf("hvm: vm8086 lldt\n");
	    DEBUGGER_ENTER("UNIMPLEMENTED");
	    return false;
	    break;
	}
	case OP_LDTL:			// sgdt/sidt/lgdt/lidt/lmsw/....
	{
	    // default data size 24 bit
	    data_size = 24;
	    modrm.x.raw = *(linear_ip + 2);
	    
	    dprintf(debug_hvm_vm8086, "hvm: vm8086 lgdt/lidt/lmsw modrm %x <%x:%x:%x> size a%d/d%d\n", 
		    modrm.x.raw, modrm.get_mode(), modrm.get_reg(), modrm.get_rm(),
		    addr_size, data_size, modrm.get_rm());

	    // Find out operand
	    if (modrm.get_mode() == ia32_modrm_t::mode_indirect)
	    {
		if (addr_size == 32)
		{
		    if (modrm.get_rm() == 5)
			ereg = *((u32_t *) (linear_ip + 3));
		    else
			ereg = vcpu_mrs->get(OFS_MR_SAVE_EAX - modrm.get_rm());
		}
		else
		{
		    switch (modrm.get_rm())
		    {
		    case 0x4:
			ereg = vcpu_mrs->gpr_item.regs.esi;
			break;
		    case 0x5:
			ereg = vcpu_mrs->gpr_item.regs.edi;
			break;
		    case 0x7:
			ereg = vcpu_mrs->gpr_item.regs.ebx;
			break;
		    case 0x6:
			ereg = *((u16_t *) (linear_ip + 3));
			break;
		    default:
			// Other operands not supported yet.
			return false;
		    }
		    ereg &= 0xffff;
		}
		
		if (!seg_ovr) seg_id = L4_CTRLXFER_DSREGS_ID;
		backend_async_read_eaddr(seg_id, ereg, addr, true);

	    }
	    else if (modrm.get_mode() == ia32_modrm_t::mode_reg)
	    {
		data = vcpu_mrs->get(OFS_MR_SAVE_EAX - modrm.get_rm());
	    }
	    else 
	    {
		// Other operands not supported yet.
		printf("hvm: vm8086 lgdt/lidt/lmsw operand mode unimplemented \n");
		DEBUGGER_ENTER("UNIMPLEMENTED");
	    }


	    switch (modrm.get_opcode())
	    {
	    case 0x2:			// lgdt.
	    {
		word_t base  = (*((u32_t *) (addr + 2))) & ((2 << data_size-1) - 1);
		word_t limit = *((u16_t *) addr);
		vcpu_mrs->append_dtr_item(L4_CTRLXFER_GDTRREGS_ID, base, limit); 
		dprintf(debug_dtr, 
			"hvm: vm8086 lgdt @ %x base %x limit %x dsize %d rm %d\n", 
			addr, base, limit, data_size, modrm.get_rm());
	    }
	    break;
	    case 0x3:			// lidt.
	    {
		
		word_t base  = (*((u32_t *) (addr + 2))) & ((2 << data_size-1) - 1);
		word_t limit = *((u16_t *) addr);
		vcpu_mrs->append_dtr_item(L4_CTRLXFER_IDTRREGS_ID, base, limit); 
		dprintf(debug_dtr, "hvm: vm8086 lidt @ %x base  %x limit %x dsize %d rm %d\n", 
			addr, base, limit, data_size, modrm.get_rm());
	    }
	    break;
	    case 0x6:			// lmsw.
		qual.raw = 0;
		qual.mov_cr.access_type = hvm_vmx_ei_qual_t::lmsw;
		//qual.mov_cr.cr_num = modrm.get_reg();
		//qual.mov_cr.mov_cr_gpr = (hvm_vmx_ei_qual_t::gpr_e) modrm.get_rm();
		if (addr != -1UL)
		{
		    qual.mov_cr.lmsw_op_type = (hvm_vmx_ei_qual_t::mem_reg_e) 1;
			data = *((u16_t *) addr);
		}
		qual.mov_cr.lmsw_src_data = data & 0xffff;
		vcpu_mrs->hvm.qual = qual.raw;
		vcpu_mrs->hvm.ai_info = addr;
		//if (addr != -1UL) data = *((u16_t *) addr);
		//vcpu_mrs->hvm.ai_info = data;
		printf("hvm: vm8086 lmsw %x\n", data);
		return handle_cr_fault();
		break;
	    default:
		printf("hvm: vm8086 unhandled sgdt/sidt/lidt/lgdt/lmsw\n");
		UNIMPLEMENTED();
		return false;
	    }
	    vcpu_mrs->next_instruction();
	    return true;
	}
	break;
	case OP_MOV_FROMCREG:		// mov cr, x.
	case OP_MOV_TOCREG:		// mov x, cr.
	{
	    modrm.x.raw = *(linear_ip + 2);

	    ASSERT(modrm.get_mode() == ia32_modrm_t::mode_reg);
	    
	    dprintf(debug_hvm_vm8086, "hvm: vm8086 mov %c CR val %x (rm %x)\n",
		    ((*(linear_ip+1) == OP_MOV_FROMCREG) ? 'f' : 't'),
		    vcpu_mrs->get(OFS_MR_SAVE_EAX - modrm.get_rm()), modrm.get_rm());
	    
	    qual.raw = 0;
	    qual.mov_cr.access_type = (*(linear_ip+1) == OP_MOV_FROMCREG) ?
		hvm_vmx_ei_qual_t::from_cr : hvm_vmx_ei_qual_t::to_cr;
	    qual.mov_cr.cr_num = modrm.get_reg();
	    qual.mov_cr.mov_cr_gpr = (hvm_vmx_ei_qual_t::gpr_e) modrm.get_rm();
	    vcpu_mrs->hvm.qual = qual.raw;
	    return handle_cr_fault();
	}
	break;
	
	case OP_MOV_FROMDREG:
	case OP_MOV_TODREG:
	{
	    printf("hvm: vm8086 mov to/from dreg unimplemented\n");
	    DEBUGGER_ENTER("UNIMPLEMENTED");
	    return false;
	}
	break;
	default:
	{
	    printf("hvm: vm8086 failure to decode mov instruction\n");
	    DEBUGGER_ENTER("UNIMPLEMENTED");
	    return false;
	}
	break;
	}
    case OP_INSB_DX:			// insb	 dx, mem	
    case OP_OUTSB_DX:			// outsb  dx,mem      
	data_size = 8;
	// fall through
    case OP_INS_DX:		
    case OP_OUTS_DX:	
	qual.raw = 0;
	qual.io.rep = rep;
	qual.io.string = true;
	qual.io.port_num = vcpu_mrs->gpr_item.regs.edx & 0xffff; 
	qual.io.soa = (hvm_vmx_ei_qual_t::soa_e) ((data_size / 8)-1);
	
	if (*linear_ip >= 0x6e)
	{
	    // out
	    qual.io.dir = hvm_vmx_ei_qual_t::out;
	    
	    if (!seg_ovr) seg_id = L4_CTRLXFER_DSREGS_ID;
	    ereg = vcpu_mrs->gpr_item.regs.esi & 0xffff;
	}
	else
	{
	    // in
	    qual.io.dir = hvm_vmx_ei_qual_t::in;
    
	    if (!seg_ovr) seg_id = L4_CTRLXFER_ESREGS_ID;
	    ereg = vcpu_mrs->gpr_item.regs.edi & 0xffff;
	}
	backend_async_read_eaddr(seg_id, ereg, (word_t &)vcpu_mrs->hvm.ai_info, true);
	
	vcpu_mrs->hvm.qual = qual.raw;
	return handle_io_fault();
	
    case OP_INB:			// inb n.
    case OP_OUTB:			// outb n.
	data_size = 8;
	// fall through
    case OP_IN:				// in n.
    case OP_OUT:			// out n.
	qual.raw = 0;
	qual.io.rep = rep;
	qual.io.port_num =		*(linear_ip + 1);
	qual.io.soa = (hvm_vmx_ei_qual_t::soa_e) ((data_size / 8)-1);
	qual.io.dir = (*linear_ip >= 0xe6) ? hvm_vmx_ei_qual_t::out :
	    hvm_vmx_ei_qual_t::in;
	vcpu_mrs->hvm.qual = qual.raw;
	return handle_io_fault();
    case OP_INB_DX:			// inb dx.
    case OP_OUTB_DX:			// outb dx.
	data_size = 8;
	// fall through
    case OP_IN_DX:			// in dx.
    case OP_OUT_DX:			// out dx.
	qual.raw = 0;
	qual.io.rep = rep;
	qual.io.port_num = vcpu_mrs->gpr_item.regs.edx & 0xffff;
	qual.io.soa = (hvm_vmx_ei_qual_t::soa_e) ((data_size / 8)-1);
	qual.io.dir = (*linear_ip >= 0xee) ? hvm_vmx_ei_qual_t::out :
	    hvm_vmx_ei_qual_t::in;
	vcpu_mrs->hvm.qual = qual.raw;
	return handle_io_fault();
    case OP_HLT:				// hlt
	return handle_hlt_fault();
    case OP_STI:
    case OP_CLI:
	DEBUGGER_ENTER("UNIMPLEMENTED STI/CLI");
	break;

    case OP_INT3:
    case OP_INT:
	int_exc.hvm.raw = 0;
	int_exc.hvm.type = hvm_vmx_int_t::sw_int;
	int_exc.hvm.vector = *(linear_ip + 1);
	int_exc.hvm.valid = 1;
	return vm8086_sync_deliver_exception( int_exc, 0);

    }
    
    dprintf(debug_hvm_vm8086, "hvm: vm8086 forward exc to guest\n");
    return backend_sync_deliver_exception(exc, eec);
}

