/*********************************************************************
 *
 * Copyright (C) 2005,  University of Karlsruhe
 *
 * File path:     afterburn-wedge/include/l4-common/monitor.h
 * Description:   
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
 * $Id: monitor.h,v 1.5 2005/04/13 15:47:32 joshua Exp $
 *
 ********************************************************************/
#ifndef __AFTERBURN_WEDGE__INCLUDE__L4_COMMON__MONITOR_H__
#define __AFTERBURN_WEDGE__INCLUDE__L4_COMMON__MONITOR_H__

#include INC_WEDGE(vcpu.h)
#include INC_WEDGE(vcpulocal.h)
#include INC_WEDGE(backend.h)

static const bool debug_pfault=0;
static const bool debug_preemption=0;

extern void monitor_loop( vcpu_t & vcpu, vcpu_t &activator );

INLINE thread_info_t *handle_pagefault( L4_MsgTag_t tag, L4_ThreadId_t tid )
{
    word_t map_addr;
    L4_MapItem_t map_item;
    word_t map_rwx, map_page_bits;
    vcpu_t &vcpu = get_vcpu();
    thread_info_t *ti = NULL;
    
    if( L4_UntypedWords(tag) != 2 ) {
	con << "Bogus page fault message from TID " << tid << '\n';
	return NULL;
    }
    
    if (tid == vcpu.main_gtid)
	ti = &vcpu.main_info;
    else if (tid == vcpu.irq_gtid)
	ti = &vcpu.irq_info;
#if defined(CONFIG_VSMP)
    else if (vcpu.is_bootstrapping_other_vcpu()
	    && tid == get_vcpu(vcpu.get_bootstrapped_cpu_id()).monitor_gtid)
	    ti = &get_vcpu(vcpu.get_bootstrapped_cpu_id()).monitor_info;
#endif
    else 
    {
	con << "Invalid page fault message from bogus TID " << tid << '\n';
	return NULL;
    }
    ti->mr_save.store_mrs(tag);
    
    if (debug_pfault)
    { 
	con << "pfault, VCPU " << vcpu.cpu_id  
	    << " addr: " << (void *) ti->mr_save.get_pfault_addr()
	    << ", ip: " << (void *) ti->mr_save.get_pfault_ip()
	    << ", rwx: " << (void *)  ti->mr_save.get_pfault_rwx()
	    << ", TID: " << tid << '\n'; 
    }  
    
    bool complete = 
	backend_handle_pagefault(tid, map_addr, map_page_bits, map_rwx, ti);
		
    if (complete)
	map_item = L4_MapItem( L4_Nilpage, 0 );
    else
	map_item = L4_MapItem( 
	    L4_FpageAddRights(L4_FpageLog2(map_addr, map_page_bits), map_rwx),
	    ti->mr_save.get_pfault_addr());

    ti->mr_save.load_pfault_reply(map_item);
    return ti;
}

#endif	/* __AFTERBURN_WEDGE__INCLUDE__L4_COMMON__MONITOR_H__ */
