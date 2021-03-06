/*********************************************************************
 *
 * Copyright (C) 2005,  University of Karlsruhe
 *
 * File path:     afterburn-wedge/l4-common/user.cc
 * Description:   House keeping code for mapping L4 threads to guest threads.
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
 * $Id: user.cc,v 1.8 2005/04/13 14:35:54 joshua Exp $
 *
 ********************************************************************/
#include <bind.h>
#include <debug.h>
#include <l4/kip.h>

#include INC_ARCH(bitops.h)
#include INC_WEDGE(vcpulocal.h)
#include INC_WEDGE(l4privileged.h)
#include INC_WEDGE(l4thread.h)
#include INC_WEDGE(backend.h)

static const bool debug_thread_exit=0;

L4_Word_t task_info_t::utcb_area_size = 0;
L4_Word_t task_info_t::utcb_size = 0;
L4_Word_t task_info_t::utcb_area_base = 0;

static L4_KernelInterfacePage_t *kip = NULL;

task_info_t::task_info_t()
{
    space_tid = L4_nilthread;
    page_dir = 0;

    for( L4_Word_t i = 0; i < sizeof(utcb_mask)/sizeof(utcb_mask[0]); i++ )
	utcb_mask[i] = 0;
}

void task_info_t::init()
{
    if (!kip)
	kip = (L4_KernelInterfacePage_t *) L4_GetKernelInterface();
    
    if( utcb_size == 0 )
    {
	if (vcpu_t::nr_vcpus * L4_UtcbSize(kip) > L4_UtcbAreaSize( kip ))
	    utcb_area_size = vcpu_t::nr_vcpus;
	else
	    utcb_area_size = L4_UtcbAreaSize( kip );
	
	utcb_area_base = get_vcpu().get_kernel_vaddr();
    }
    
    space_tid = L4_nilthread;

    for( L4_Word_t i = 0; i < sizeof(utcb_mask)/sizeof(utcb_mask[0]); i++ )
	utcb_mask[i] = 0;
}

bool task_info_t::utcb_allocate( L4_Word_t & utcb, L4_Word_t & uidx )
{
    uidx = bit_allocate_atomic( this->utcb_mask, this->max_threads );
    if( uidx >= this->max_threads )
	return false;

    utcb = uidx*this->utcb_size + this->utcb_area_base;
    return true;
}

void task_info_t::utcb_release( L4_Word_t uidx )
{
    static const word_t bits_per_word = sizeof(word_t)*8;
    ASSERT( uidx < this->max_threads );
    bit_clear_atomic( uidx % bits_per_word,
	    this->utcb_mask[uidx / bits_per_word] );
}

bool task_info_t::has_one_thread()
    // If the space has only one thread, then it should be the very
    // first utcb index (the space thread).
{
    bool has_one = utcb_mask[0] == 1;
    if( !has_one )
	return false;

    for( word_t i = 1; i < sizeof(utcb_mask)/sizeof(utcb_mask[0]); i++ )
	if( utcb_mask[i] )
	    return false;

    return true;
}


thread_info_t *allocate_thread()
{
    L4_Word_t errcode;
    vcpu_t &vcpu = get_vcpu();
    L4_ThreadId_t controller_tid = vcpu.main_gtid;

    // Allocate a thread ID.
    L4_ThreadId_t tid = get_l4thread_manager()->thread_id_allocate();
    if( L4_IsNilThread(tid) )
	PANIC( "Out of thread ID's." );

    // Lookup the address space's management structure.
    L4_Word_t page_dir = vcpu.cpu.cr3.get_pdir_addr();
    task_info_t *task_info = get_task_manager().find_by_page_dir( page_dir );
    if( !task_info )
    {
	// New address space.
	task_info = get_task_manager().allocate( page_dir );
	if( !task_info )
	    PANIC( "Hit task limit." );
	task_info->init();
    }

    // Choose a UTCB in the address space.
    L4_Word_t utcb, utcb_index;
    if( !task_info->utcb_allocate(utcb, utcb_index) )
	PANIC( "Hit task thread limit." );

    // Configure the TID.
    tid = L4_GlobalId( L4_ThreadNo(tid), task_info_t::encode_gtid_version(utcb_index) );
    if( task_info_t::decode_gtid_version(tid) != utcb_index )
	PANIC( "L4 thread version wrap-around." );
    if( !task_info->has_space_tid() ) {
	ASSERT( task_info_t::decode_gtid_version(tid) == 0 );
	task_info->set_space_tid( tid );
	ASSERT( task_info->get_space_tid() == tid );
    }

    // Allocate a thread info structure.
    thread_info_t *thread_info = get_thread_manager().allocate( tid );
    if( !thread_info )
	PANIC( "Hit thread limit." );
    thread_info->init();
    thread_info->ti = task_info;

    // Init the L4 address space if necessary.
    if( task_info->get_space_tid() == tid )
    {
	// Create the L4 thread.
	errcode = ThreadControl( tid, task_info->get_space_tid(), 
		controller_tid, L4_nilthread, utcb );
	if( errcode != L4_ErrOk )
	    PANIC( "Failed to create initial user thread, TID %t, L4error %s\n", 
		    tid, L4_ErrorCode_String(errcode));

	// Create an L4 address space + thread.
	ASSERT(kip);
	L4_Fpage_t kip_fp, utcb_fp;
	utcb_fp = L4_Fpage( task_info_t::utcb_area_base, task_info_t::utcb_area_size);
	kip_fp = L4_Fpage( L4_Address(utcb_fp) + L4_Size(utcb_fp), L4_KipAreaSize(kip));

	errcode = SpaceControl( tid, 0, kip_fp, utcb_fp, L4_nilthread );
	if( errcode != L4_ErrOk )
	    PANIC( "Failed to create an address space, TID %x, L4error %s\n", 
		    tid, L4_ErrorCode_String(errcode));
    }

    // Create the L4 thread.
    errcode = ThreadControl( tid, task_info->get_space_tid(), 
	    controller_tid, controller_tid, utcb );
    if( errcode != L4_ErrOk )
	PANIC( "Failed to create user thread, TID %t, , space TID"
		", utcb %x, L4 error %s\n", tid, task_info->get_space_tid(), 
		utcb, L4_ErrorCode_String(errcode) );

    // Set the thread priority.
    L4_Word_t prio = vcpu.get_vcpu_max_prio() + CONFIG_PRIO_DELTA_USER;
    if( !L4_Set_Priority(tid, prio) )
	PANIC( "Failed to set user thread's priority to %d, L4 error %s\n", prio, L4_ErrorCode_String(L4_ErrorCode()) );

    // Assign the thread info to the guest OS's thread.
    afterburn_thread_assign_handle( thread_info );

    return thread_info;
}

void delete_thread( thread_info_t *thread_info )
{
    if( !thread_info )
	return;
    ASSERT( thread_info->ti );

    L4_ThreadId_t tid = thread_info->get_tid();

    if( thread_info->is_space_thread() ) {
	// Keep the space thread alive, until the address space is empty.
	// Just flip a flag to say that the space thread is invalid.
	if( debug_thread_exit )
	    printf( "Space thread invalidate, TID %t\n", tid);
	thread_info->ti->invalidate_space_tid();
    }
    else
    {
	// Delete the L4 thread.
	thread_info->ti->utcb_release( task_info_t::decode_gtid_version(tid) );
	if( debug_thread_exit )
	    printf( "Thread delete, TID %t\n", tid);
	ThreadControl( tid, L4_nilthread, L4_nilthread, L4_nilthread, ~0UL );
	get_l4thread_manager()->thread_id_release( tid );
    }

    if( thread_info->ti->has_one_thread() 
	    && !thread_info->ti->is_space_tid_valid() )
    {
	// Retire the L4 address space.
	tid = thread_info->ti->get_space_tid();
	if( debug_thread_exit )
	    printf( "Space delete, TID %t\n", tid);
	ThreadControl( tid, L4_nilthread, L4_nilthread, L4_nilthread, ~0UL );
	get_l4thread_manager()->thread_id_release( tid );

	// Release the task info structure.
	get_task_manager().deallocate( thread_info->ti );
    }

    // Release the thread info structure.
    get_thread_manager().deallocate( thread_info );
}
