
/*********************************************************************
 *
 * Copyright (C) 2005,  University of Karlsruhe
 *
 * File path:     afterburn-wedge/include/l4-common/irq.h
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
 * $Id: irq.h,v 1.6 2005/04/13 15:47:32 joshua Exp $
 *
 ********************************************************************/
#ifndef __AFTERBURN_WEDGE__INCLUDE__L4_COMMON__IRQ_H__
#define __AFTERBURN_WEDGE__INCLUDE__L4_COMMON__IRQ_H__

#include <bitmap.h>
#include INC_WEDGE(vcpulocal.h)
#include INC_WEDGE(vcpu.h)
#include INC_WEDGE(sync.h)
#include INC_ARCH(intlogic.h)

const L4_Word_t vtimer_timeouts = L4_Timeouts(L4_Never, L4_Never);
const L4_Word_t default_timeouts = L4_Timeouts(L4_ZeroTime, L4_Never);

extern bool irq_init( L4_Word_t prio, L4_ThreadId_t pager_tid, vcpu_t *vcpu);

extern cpu_lock_t irq_lock;

typedef struct 
{
    bitmap_t<INTLOGIC_MAX_HWIRQS> * bitmap;
    L4_Word_t vtimer_irq;
} virq_t;
extern virq_t virq;

#endif	/* __AFTERBURN_WEDGE__INCLUDE__L4_COMMON__IRQ_H__ */
