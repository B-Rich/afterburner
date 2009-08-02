/*********************************************************************
 *                
 * Copyright (C) 2002, 2003, 2005-2009,  Karlsruhe University
 *                
 * File path:     l4archvm.h
 * Description:   x86 specific functions and defines
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
 * $Id: arch.h,v 1.2 2006/06/08 11:15:17 skoglund Exp $
 *                
 ********************************************************************/
#ifndef __L4__IA32__ARCHVM_H__
#define __L4__IA32__ARCHVM_H__


#include <l4/thread.h>
#include <l4/kip.h>

L4_INLINE int __L4_Lsb (L4_Word_t w) __attribute__ ((const));

L4_INLINE int __L4_Lsb (L4_Word_t w)
{
    L4_Word_t bitnum;

    __asm__ (
	"/* l4_lsb()		*/			\n"
	"bsf	%1, %0					\n"
	: /* outputs */
	"=r" (bitnum)
	: /* inputs */
	"rm" (w)
	);

    return bitnum;
}

L4_INLINE L4_Word_t L4_HS_Schedule (L4_ThreadId_t tid, L4_Word_t control, L4_ThreadId_t domain, L4_Word_t prio, L4_Word_t stride, L4_Word_t *old_control)
{
    L4_Word_t preemption_control = ((control & 0x3f) << 26) | (1 << 25);
    L4_Word_t time_control = domain.raw;
    L4_Word_t prio_control = (stride << 16) | (prio & 0x1ff);
    
    return L4_Schedule(tid, time_control, ~0UL, prio_control, preemption_control, old_control);
}


#define L4_EXREGS_CTRLXFER_CONF_FLAG    	(1UL <<  9)  
#define L4_EXREGS_CTRLXFER_READ_FLAG    	(1UL << 10)
#define L4_EXREGS_CTRLXFER_WRITE_FLAG    	(1UL << 11)
#define L4_EXREGS_EXCHANDLER_FLAG		(1UL << 12)
#define L4_EXREGS_SCHEDULER_FLAG		(1UL << 13)

#define L4_CTRLXFER_TSTATE_ID		(0)
#define L4_CTRLXFER_GPREGS_ID		(1)
#define L4_CTRLXFER_FPUREGS_ID		(2)
#define L4_CTRLXFER_PMCREGS_ID		(3)
#define L4_CTRLXFER_CREGS_ID		(4)
#define L4_CTRLXFER_DREGS_ID		(5)
#define L4_CTRLXFER_CSREGS_ID		(6)
#define L4_CTRLXFER_SSREGS_ID		(7)
#define L4_CTRLXFER_DSREGS_ID		(8)
#define L4_CTRLXFER_ESREGS_ID		(9)
#define L4_CTRLXFER_FSREGS_ID		(10)
#define L4_CTRLXFER_GSREGS_ID		(11)
#define L4_CTRLXFER_TRREGS_ID		(12)
#define L4_CTRLXFER_LDTRREGS_ID		(13)
#define L4_CTRLXFER_IDTRREGS_ID		(14)
#define L4_CTRLXFER_GDTRREGS_ID		(15)
#define L4_CTRLXFER_NONREGEXC_ID	(16)
#define L4_CTRLXFER_EXECCTRL_ID		(17)
#define L4_CTRLXFER_OTHERREGS_ID	(18)
#define L4_CTRLXFER_INVALID_ID		(255)
#define L4_CTRLXFER_FAULT_MASK(id)	(1 << id)

#define L4_CTRLXFER_TSTATE_SIZE		(7)
#define L4_CTRLXFER_GPREGS_SIZE		(10)
#define L4_CTRLXFER_FPUREGS_SIZE	(1)
#define L4_CTRLXFER_PMCREGS_SIZE	(8)
#define L4_CTRLXFER_CREGS_SIZE		(8)
#define L4_CTRLXFER_DREGS_SIZE		(6)
#define L4_CTRLXFER_SEGREG_SIZE		(4)
#define L4_CTRLXFER_DTRREG_SIZE		(2)
#define L4_CTRLXFER_NONREGEXC_SIZE	(10)
#define L4_CTRLXFER_EXECCTRL_SIZE	(3)
#define L4_CTRLXFER_OTHERREGS_SIZE	(9)

#define L4_CTRLXFER_PMCREGS_SHIFT	3

#define L4_FAULT_PAGEFAULT		2
#define L4_FAULT_EXCEPTION		3
#define L4_FAULT_PREEMPTION		5
#define L4_CTRLXFER_HVM_FAULT(id)	(9+id)
#define L4_CTRLXFER_FAULT_ID_MASK(id)	(1ULL << id)

#define L4_CTRLXFER_GPREGS_EIP		(0)
#define L4_CTRLXFER_GPREGS_EFLAGS	(1)
#define L4_CTRLXFER_GPREGS_EDI		(2)
#define L4_CTRLXFER_GPREGS_ESI		(3)
#define L4_CTRLXFER_GPREGS_EBP		(4)
#define L4_CTRLXFER_GPREGS_ESP		(5)
#define L4_CTRLXFER_GPREGS_EBX		(6)
#define L4_CTRLXFER_GPREGS_EDX		(7)
#define L4_CTRLXFER_GPREGS_ECX		(8)
#define L4_CTRLXFER_GPREGS_EAX		(9)

#define L4_CTRLXFER_CREGS_CR0		(0)
#define L4_CTRLXFER_CREGS_CR0_SHADOW	(1)
#define L4_CTRLXFER_CREGS_CR0_MASK	(2)
#define L4_CTRLXFER_CREGS_CR2		(3)
#define L4_CTRLXFER_CREGS_CR3		(4)
#define L4_CTRLXFER_CREGS_CR4		(5)
#define L4_CTRLXFER_CREGS_CR4_SHADOW	(6)
#define L4_CTRLXFER_CREGS_CR4_MASK	(7)

#define L4_CTRLXFER_DREGS_DR0		(0)
#define L4_CTRLXFER_DREGS_DR1		(1)
#define L4_CTRLXFER_DREGS_DR2		(2)
#define L4_CTRLXFER_DREGS_DR3		(3)
#define L4_CTRLXFER_DREGS_DR6		(4)
#define L4_CTRLXFER_DREGS_DR7		(5)

#define L4_CTRLXFER_CSREGS_CS		(0)
#define L4_CTRLXFER_CSREGS_CS_BASE	(1)
#define L4_CTRLXFER_CSREGS_CS_LIMIT	(2)
#define L4_CTRLXFER_CSREGS_CS_ATTR	(3)

#define L4_CTRLXFER_NONREGEXC_ACTIVITY	(0)
#define L4_CTRLXFER_NONREGEXC_INT	(1)
#define L4_CTRLXFER_NONREGEXC_DBG_EXC	(2)
#define L4_CTRLXFER_NONREGEXC_ENT_INFO	(3)
#define L4_CTRLXFER_NONREGEXC_ENT_EEC	(4)
#define L4_CTRLXFER_NONREGEXC_ENT_ILEN	(5)
#define L4_CTRLXFER_NONREGEXC_EX_INFO	(6)
#define L4_CTRLXFER_NONREGEXC_EX_EEC	(7)
#define L4_CTRLXFER_NONREGEXC_IDT_INFO	(8)
#define L4_CTRLXFER_NONREGEXC_IDT_EEC	(9)

#define L4_CTRLXFER_EXEC_PIN		(0)
#define L4_CTRLXFER_EXEC_CPU		(1)
#define L4_CTRLXFER_EXEC_EXC_BITMAP	(2)

#define L4_CTRLXFER_OTHERREGS_SYS_CS	 (0)
#define L4_CTRLXFER_OTHERREGS_SYS_EIP	 (1)
#define L4_CTRLXFER_OTHERREGS_SYS_ESP	 (2)
#define L4_CTRLXFER_OTHERREGS_DBG_LOW	 (3)
#define L4_CTRLXFER_OTHERREGS_DBG_HIGH	 (4)
#define L4_CTRLXFER_OTHERREGS_RDTSC_LOW	 (5)
#define L4_CTRLXFER_OTHERREGS_RDTSC_HIGH (6)
#define L4_CTRLXFER_OTHERREGS_VAPIC	 (7)
#define L4_CTRLXFER_OTHERREGS_TPR_THRSH	 (8)


#define L4_CTRLXFER_REG_ID(id, reg)	((id * 0x10) + reg)


/*
 * CtrlXfer Item
 */

typedef union {
    L4_Word_t		raw[0];
    struct {
	L4_Word_t	C:1;
	L4_Word_t	__type:3;
	L4_Word_t	id:8;
	L4_Word_t	mask:20;
    };
} L4_CtrlXferItem_t;	


#if defined(__cplusplus)
/*
 * IA32 CtrlXfer Item
 */

L4_INLINE void L4_CtrlXferItemInit (L4_CtrlXferItem_t *c, L4_Word_t id)
{
    c->raw[0] = 0;
    c->__type = 0x06;
    c->id = id;
    c->mask = 0;

}

L4_INLINE void L4_MsgAppendCtrlXferItem (L4_Msg_t * msg, L4_CtrlXferItem_t *c)
{
    L4_Word_t reg=0, num=0, mask = c->mask;
    
    /* 
     * Add regs according to mask 
     * */
    for (reg+=__L4_Lsb(mask); mask!=0; mask>>=__L4_Lsb(mask)+1,reg+=__L4_Lsb(mask)+1,num++)
	msg->msg[msg->tag.X.u + msg->tag.X.t + 2 + num] = c->raw[reg+1];
   
    /* Add item */
    if (num)
    {
	msg->msg[msg->tag.X.u + msg->tag.X.t + 1] = c->raw[0];
	msg->tag.X.t += 1 + num;
    }
}

L4_INLINE L4_Word_t L4_MsgStoreCtrlXferItem (L4_Msg_t * msg, L4_Word_t mr, L4_CtrlXferItem_t *c)
{
    L4_Word_t reg=0, num=0, mask=0;

    /* Store item */
    c->raw[0] = msg->msg[mr];
    mask = c->mask;
    /* 
     * Store regs according to mask 
     * */
    for (reg+=__L4_Lsb(mask); mask!=0; mask>>=__L4_Lsb(mask)+1,reg+=__L4_Lsb(mask)+1,num++)
	c->raw[reg+1] = msg->msg[mr + 1 + num];
    
    return num + 1;
}


L4_INLINE L4_Word_t L4_CtrlXferItemId(L4_Msg_t * msg, L4_Word_t mr)
{
    
    L4_CtrlXferItem_t *c = (L4_CtrlXferItem_t *) &msg->msg[mr];
    
    return (c->__type == 0x6) ? c->id : L4_CTRLXFER_INVALID_ID;
       
}
				      
/* 
 * IA32 Fault Configuration item 
 */
L4_INLINE void L4_FaultConfCtrlXferItemInit (L4_CtrlXferItem_t *c, L4_Word_t fault_id, L4_Word_t fault_mask)
{
    c->raw[0] = 0;
    c->__type = 0x06;
    c->id = fault_id;
    c->mask = fault_mask;
}

L4_INLINE void L4_AppendFaultConfCtrlXferItems(L4_Msg_t *msg, L4_Word64_t fault_id_mask, L4_Word_t fault_mask, L4_Word_t C)
{
    L4_CtrlXferItem_t item; 
    L4_Word_t fault_id = 0;
    L4_Word_t fault_id_mask_low = fault_id_mask;
    L4_Word_t fault_id_mask_high = (fault_id_mask >> 32);
    
    for (fault_id+=__L4_Lsb(fault_id_mask_low); fault_id_mask_low != 0;  
	 fault_id_mask_low>>=__L4_Lsb(fault_id_mask_low)+1,fault_id+=__L4_Lsb(fault_id_mask_low)+1)
    {
	L4_FaultConfCtrlXferItemInit(&item, fault_id, fault_mask); 
	item.C = 1;
	L4_Append(msg, item.raw[0]);
    }
    
    fault_id = 32;
    for (fault_id+=__L4_Lsb(fault_id_mask_high); fault_id_mask_high != 0;  
	 fault_id_mask_high>>=__L4_Lsb(fault_id_mask_high)+1,fault_id+=__L4_Lsb(fault_id_mask_high)+1)
    {
	L4_FaultConfCtrlXferItemInit(&item, fault_id, fault_mask); 
	item.C = 1;
	L4_Append(msg, item.raw[0]);
    }

    item.C = C;
    msg->msg[msg->tag.X.u + msg->tag.X.t] = item.raw[0];
}



/*
 * IA32 Single Reg CtrlXfer Item
 */

typedef union { 
    L4_Word_t   raw[2]; 
    struct { 
	L4_CtrlXferItem_t item; 
	L4_Word_t   reg; 
    }; 
} L4_RegCtrlXferItem_t; 


L4_INLINE void L4_RegCtrlXferItemInit(L4_RegCtrlXferItem_t *c, L4_Word_t id)
{
    L4_CtrlXferItemInit(&c->item, id);
}

L4_INLINE void L4_RegCtrlXferItemSet(L4_RegCtrlXferItem_t *c, L4_Word_t reg, L4_Word_t val)
{
    c->reg = val;
    c->item.mask |= (1<<reg);	   
}

L4_INLINE void L4_MsgAppendRegCtrlXferItem(L4_Msg_t * msg, L4_RegCtrlXferItem_t *c)
{
    L4_MsgAppendCtrlXferItem(msg, &c->item);
}

L4_INLINE L4_Word_t L4_MsgStoreRegCtrlXferItem(L4_Msg_t *msg, L4_Word_t mr, L4_RegCtrlXferItem_t *c)
{
    return L4_MsgStoreCtrlXferItem(msg, mr, &c->item);
}

/*
 * V4 Thread State 
 */

typedef union
{
    L4_Word_t reg[L4_CTRLXFER_TSTATE_SIZE];
} L4_TState_t;


typedef union {
    L4_Word_t	raw[L4_CTRLXFER_TSTATE_SIZE+1];
    struct {
	L4_CtrlXferItem_t  item;
	L4_TState_t   regs;
    };
} L4_TStateCtrlXferItem_t;

L4_INLINE void L4_TStateCtrlXferItemInit(L4_TStateCtrlXferItem_t *c)
{
    L4_Word_t i;
    L4_CtrlXferItemInit(&c->item, L4_CTRLXFER_TSTATE_ID);
    for (i=0; i < L4_CTRLXFER_TSTATE_SIZE; i++)
	c->regs.reg[i] = 0;
    
}

L4_INLINE void L4_TStateCtrlXferItemSet(L4_TStateCtrlXferItem_t *c, 
					L4_Word_t reg, L4_Word_t val)
{
    c->regs.reg[reg] = val;
    c->item.mask |= (1<<reg);    
}

L4_INLINE void L4_MsgAppendTStateCtrlXferItem (L4_Msg_t * msg, L4_TStateCtrlXferItem_t *c)
{
    L4_MsgAppendCtrlXferItem(msg, &c->item);
}

L4_INLINE L4_Word_t L4_MsgStoreTStateCtrlXferItem (L4_Msg_t *msg, L4_Word_t mr, L4_TStateCtrlXferItem_t *c)
{
    return L4_MsgStoreCtrlXferItem(msg, mr, &c->item);
}

/*
 * IA32 GPRegs 
 */

typedef union
{
    struct
    {
	L4_Word_t eip;
	L4_Word_t eflags;
	L4_Word_t edi;
	L4_Word_t esi; 
	L4_Word_t ebp;
	L4_Word_t esp;
	L4_Word_t ebx;
	L4_Word_t edx;
	L4_Word_t ecx;
	L4_Word_t eax;
    };
    L4_Word_t reg[L4_CTRLXFER_GPREGS_SIZE];
} L4_IA32_GPRegs_t;


/*
 * IA32 GPRegs CtrlXfer Item
 */

typedef union {
    L4_Word_t	raw[L4_CTRLXFER_GPREGS_SIZE+1];
    struct {
	L4_CtrlXferItem_t  item;
	L4_IA32_GPRegs_t   regs;
    };
} L4_GPRegsCtrlXferItem_t;

L4_INLINE void L4_GPRegsCtrlXferItemInit(L4_GPRegsCtrlXferItem_t *c)
{
    L4_Word_t i;
    L4_CtrlXferItemInit(&c->item, L4_CTRLXFER_GPREGS_ID);
    for (i=0; i < L4_CTRLXFER_GPREGS_SIZE; i++)
	c->regs.reg[i] = 0;
    
}

L4_INLINE void L4_GPRegsCtrlXferItemSet(L4_GPRegsCtrlXferItem_t *c, 
					L4_Word_t reg, L4_Word_t val)
{
    c->regs.reg[reg] = val;
    c->item.mask |= (1<<reg);    
}

L4_INLINE void L4_MsgAppendGPRegsCtrlXferItem (L4_Msg_t * msg, L4_GPRegsCtrlXferItem_t *c)
{
    L4_MsgAppendCtrlXferItem(msg, &c->item);
}

L4_INLINE L4_Word_t L4_MsgStoreGPRegsCtrlXferItem (L4_Msg_t *msg, L4_Word_t mr, L4_GPRegsCtrlXferItem_t *c)
{
    return L4_MsgStoreCtrlXferItem(msg, mr, &c->item);
}


/*
 * IA32 PMC Regs
 */

typedef union
{
    L4_Word_t reg[L4_CTRLXFER_PMCREGS_SIZE];
} L4_IA32_PMC_t;


typedef union {
    L4_Word_t	raw[L4_CTRLXFER_PMCREGS_SIZE+1];
    struct {
	L4_CtrlXferItem_t  item;
	L4_IA32_PMC_t   regs;
    };
} L4_IA32_PMCCtrlXferItem_t;

L4_INLINE void L4_IA32_PMCCtrlXferItemInit(L4_IA32_PMCCtrlXferItem_t *c)
{
    L4_Word_t i;
    L4_CtrlXferItemInit(&c->item, L4_CTRLXFER_PMCREGS_ID);
    for (i=0; i < L4_CTRLXFER_PMCREGS_SIZE; i++)
	c->regs.reg[i] = 0;
    
}

L4_INLINE void L4_IA32_PMCCtrlXferItemSet(L4_IA32_PMCCtrlXferItem_t *c, 
					L4_Word_t reg, L4_Word_t val)
{
    c->regs.reg[reg] = val;
    c->item.mask |= (1<<reg);    
}

L4_INLINE void L4_MsgAppendIA32_PMCCtrlXferItem (L4_Msg_t * msg, L4_IA32_PMCCtrlXferItem_t *c)
{
    L4_MsgAppendCtrlXferItem(msg, &c->item);
}

L4_INLINE L4_Word_t L4_MsgStoreIA32_PMCCtrlXferItem (L4_Msg_t *msg, L4_Word_t mr, L4_IA32_PMCCtrlXferItem_t *c)
{
    return L4_MsgStoreCtrlXferItem(msg, mr, &c->item);
}

/*
 * IA32 Control Regs
 */

typedef union
{
    struct
    {
	L4_Word_t cr0;
	L4_Word_t cr0_shadow;
	L4_Word_t cr0_mask;
	L4_Word_t cr2; 
	L4_Word_t cr3; 
	L4_Word_t cr4;
	L4_Word_t cr4_shadow;
	L4_Word_t cr4_mask;
    };
    L4_Word_t reg[L4_CTRLXFER_CREGS_SIZE];
} L4_IA32_CRegs_t;


/*
 * IA32 Control Regs CtrlXfer Item
 */

typedef union {
    L4_Word_t	raw[L4_CTRLXFER_CREGS_SIZE+1];
    struct {
	L4_CtrlXferItem_t  item;
	L4_IA32_CRegs_t	   regs;
    };
} L4_CRegsCtrlXferItem_t;

L4_INLINE void L4_CRegsCtrlXferItemInit(L4_CRegsCtrlXferItem_t *c)
{
    L4_Word_t i;
    L4_CtrlXferItemInit(&c->item, L4_CTRLXFER_CREGS_ID);
    for (i=0; i < L4_CTRLXFER_CREGS_SIZE; i++)
	c->regs.reg[i] = 0;
    
}

L4_INLINE void L4_CRegsCtrlXferItemSet(L4_CRegsCtrlXferItem_t *c, 
					L4_Word_t reg, L4_Word_t val)
{
    c->regs.reg[reg] = val;
    c->item.mask |= (1<<reg);    
}



L4_INLINE void L4_MsgAppendCRegsCtrlXferItem (L4_Msg_t * msg, L4_CRegsCtrlXferItem_t *c)
{
    L4_MsgAppendCtrlXferItem(msg, &c->item);
}

L4_INLINE L4_Word_t L4_MsgStoreCRegsCtrlXferItem (L4_Msg_t *msg, L4_Word_t mr, L4_CRegsCtrlXferItem_t *c)
{
    return L4_MsgStoreCtrlXferItem(msg, mr, &c->item);
}

/*
 * IA32 Debug Regs
 */

typedef union
{
    struct
    {
	L4_Word_t dr0;
	L4_Word_t dr1;
	L4_Word_t dr2;
	L4_Word_t dr3;
	L4_Word_t dr6;
	L4_Word_t dr7;
    };
    L4_Word_t reg[L4_CTRLXFER_DREGS_SIZE];
} L4_IA32_DRegs_t;


/*
 * IA32 Debug Regs CtrlXfer Item
 */

typedef union {
    L4_Word_t	raw[L4_CTRLXFER_DREGS_SIZE+1];
    struct {
	L4_CtrlXferItem_t  item;
	L4_IA32_DRegs_t	   regs;
    };
} L4_DRegsCtrlXferItem_t;

L4_INLINE void L4_DRegsCtrlXferItemInit(L4_DRegsCtrlXferItem_t *c)
{
    L4_Word_t i;
    L4_CtrlXferItemInit(&c->item, L4_CTRLXFER_DREGS_ID);
    for (i=0; i < L4_CTRLXFER_DREGS_SIZE; i++)
	c->regs.reg[i] = 0;
    
}

L4_INLINE void L4_DRegsCtrlXferItemSet(L4_DRegsCtrlXferItem_t *c, 
					L4_Word_t reg, L4_Word_t val)
{
    c->regs.reg[reg] = val;
    c->item.mask |= (1<<reg);    
}



L4_INLINE void L4_MsgAppendDRegsCtrlXferItem (L4_Msg_t * msg, L4_DRegsCtrlXferItem_t *c)
{
    L4_MsgAppendCtrlXferItem(msg, &c->item);
}

L4_INLINE L4_Word_t L4_MsgStoreDRegsCtrlXferItem (L4_Msg_t *msg, L4_Word_t mr, L4_DRegsCtrlXferItem_t *c)
{
    return L4_MsgStoreCtrlXferItem(msg, mr, &c->item);
}


/*
 * IA32 Segment Register CtrlXfer Item
 */

typedef union
{
    struct
    {
	L4_Word_t segreg;
	L4_Word_t base;
	L4_Word_t limit;
	L4_Word_t attr;
    };
    L4_Word_t reg[L4_CTRLXFER_SEGREG_SIZE];
} L4_IA32_SegmentRegs_t;

typedef union {
    L4_Word_t	raw[L4_CTRLXFER_SEGREG_SIZE+1];
    struct {
	L4_CtrlXferItem_t	item;
	L4_IA32_SegmentRegs_t   regs;
    };
} L4_SegCtrlXferItem_t;

L4_INLINE void L4_SegCtrlXferItemInit(L4_SegCtrlXferItem_t *c, L4_Word_t id)
{
    L4_Word_t i;
    L4_CtrlXferItemInit(&c->item, id);
    for (i=0; i < L4_CTRLXFER_SEGREG_SIZE; i++)
	c->regs.reg[i] = 0;
    
    
}

L4_INLINE void L4_SegCtrlXferItemSet(L4_SegCtrlXferItem_t *c, 
				     L4_Word_t reg, L4_Word_t val)
{
    c->regs.reg[reg] = val;
    c->item.mask |= (1<<reg);    
}

L4_INLINE void L4_MsgAppendSegCtrlXferItem (L4_Msg_t * msg, L4_SegCtrlXferItem_t *c)
{
    L4_MsgAppendCtrlXferItem(msg, &c->item);
}

L4_INLINE L4_Word_t L4_MsgStoreSegCtrlXferItem (L4_Msg_t *msg, L4_Word_t mr, L4_SegCtrlXferItem_t *c)
{
    return L4_MsgStoreCtrlXferItem(msg, mr, &c->item);
}

/* 
 * IA32 DTR CtrlXfer Item
 */

typedef union
{
    struct
    {
	L4_Word_t base;
	L4_Word_t limit;
    };
    L4_Word_t reg[L4_CTRLXFER_DTRREG_SIZE];
} L4_IA32_DTRRegs_t;

typedef union {
    L4_Word_t	raw[L4_CTRLXFER_DTRREG_SIZE+1];
    struct {
	L4_CtrlXferItem_t	item;
	L4_IA32_DTRRegs_t	regs;
    };
} L4_DTRCtrlXferItem_t;

L4_INLINE void L4_DTRCtrlXferItemInit(L4_DTRCtrlXferItem_t *c, L4_Word_t id)
{
    L4_Word_t i;
    L4_CtrlXferItemInit(&c->item, id);
    for (i=0; i < L4_CTRLXFER_DTRREG_SIZE; i++)
	c->regs.reg[i] = 0;
    
    
}

L4_INLINE void L4_DTRCtrlXferItemSet(L4_DTRCtrlXferItem_t *c, 
				     L4_Word_t reg, L4_Word_t val)
{
    c->regs.reg[reg] = val;
    c->item.mask |= (1<<reg);    
}

L4_INLINE void L4_MsgAppendDTRCtrlXferItem (L4_Msg_t * msg, L4_DTRCtrlXferItem_t *c)
{
    L4_MsgAppendCtrlXferItem(msg, &c->item);
}

L4_INLINE L4_Word_t L4_MsgStoreDTRCtrlXferItem (L4_Msg_t *msg, L4_Word_t mr, L4_DTRCtrlXferItem_t *c)
{
    return L4_MsgStoreCtrlXferItem(msg, mr, &c->item);
}

/*
 * IA32 NonRegExc CtrlXfer Item
 */

typedef union
{
    struct
    {
	L4_Word_t as;
	L4_Word_t ias;
	L4_Word_t pending_dbg_exec;
	L4_Word_t entry_info;
	L4_Word_t entry_eec;
	L4_Word_t entry_ilen;
	L4_Word_t exit_info;
	L4_Word_t exit_eec;
	L4_Word_t idt_info;
	L4_Word_t idt_eec;
    };
    L4_Word_t reg[L4_CTRLXFER_NONREGEXC_SIZE];
} L4_IA32_NonRegExcs_t;


typedef union {
    L4_Word_t	raw[L4_CTRLXFER_NONREGEXC_SIZE+1];
    struct {
	L4_CtrlXferItem_t  item;
	L4_IA32_NonRegExcs_t  regs;
    };
} L4_NonRegExcCtrlXferItem_t;


L4_INLINE void L4_NonRegExcCtrlXferItemInit(L4_NonRegExcCtrlXferItem_t *c)
{
    L4_Word_t i;
    L4_CtrlXferItemInit(&c->item, L4_CTRLXFER_NONREGEXC_ID);
    for (i=0; i < L4_CTRLXFER_NONREGEXC_SIZE; i++)
	c->regs.reg[i] = 0;

}

L4_INLINE void L4_NonRegExcCtrlXferItemSet(L4_NonRegExcCtrlXferItem_t *c, L4_Word_t reg, L4_Word_t val)
{
    c->regs.reg[reg] = val;
    c->item.mask |= (1<<reg);    
}

L4_INLINE void L4_MsgAppendNonRegExcCtrlXferItem(L4_Msg_t *msg, L4_NonRegExcCtrlXferItem_t *c)
{
    L4_MsgAppendCtrlXferItem(msg, &c->item);
}

L4_INLINE L4_Word_t L4_MsgStoreNonRegExcCtrlXferItem(L4_Msg_t *msg, L4_Word_t mr, L4_NonRegExcCtrlXferItem_t *c)
{
    return L4_MsgStoreCtrlXferItem(msg, mr, &c->item);
}

/*
 * IA32 Execution Control CtrlXfer Item
 */

typedef union
{
    struct
    {
	L4_Word_t pin;
	L4_Word_t cpu;
	L4_Word_t exc_bitmap;
    };
    L4_Word_t reg[L4_CTRLXFER_EXECCTRL_SIZE];
} L4_IA32_ExecRegs_t;


typedef union {
    L4_Word_t	raw[L4_CTRLXFER_EXECCTRL_SIZE+1];
    struct {
	L4_CtrlXferItem_t  item;
	L4_IA32_ExecRegs_t regs;
    };
} L4_ExecCtrlXferItem_t;


L4_INLINE void L4_ExecCtrlXferItemInit(L4_ExecCtrlXferItem_t *c)
{
    L4_Word_t i;
    L4_CtrlXferItemInit(&c->item, L4_CTRLXFER_EXECCTRL_ID);
    for (i=0; i < L4_CTRLXFER_EXECCTRL_SIZE; i++)
	c->regs.reg[i] = 0;
}

L4_INLINE void L4_ExecCtrlXferItemSet(L4_ExecCtrlXferItem_t *c, 
				     L4_Word_t reg, L4_Word_t val)
{
    c->regs.reg[reg] = val;
    c->item.mask |= (1<<reg);    
}


L4_INLINE void L4_MsgAppendExeccCtrlXferItem(L4_Msg_t *msg, L4_ExecCtrlXferItem_t *c)
{
    L4_MsgAppendCtrlXferItem(msg, &c->item);
}

L4_INLINE L4_Word_t L4_MsgStoreExeccCtrlXferItem(L4_Msg_t *msg, L4_Word_t mr, L4_ExecCtrlXferItem_t *c)
{
    return L4_MsgStoreCtrlXferItem(msg, mr, &c->item);
}


/*
 * IA32 Other Regs
 */

typedef union
{
    struct
    {
    	L4_Word_t sysenter_cs; 
	L4_Word_t sysenter_eip; 
	L4_Word_t sysenter_esp; 
	L4_Word_t dbg_ctl_low; 
	L4_Word_t dbg_ctl_high;  
	L4_Word_t tsc_ofs_low; 
	L4_Word_t tsc_ofs_high; 
	L4_Word_t vapic_address; 
	L4_Word_t tpr_threshold; 
    };
    L4_Word_t reg[L4_CTRLXFER_OTHERREGS_SIZE];
} L4_IA32_OtherRegs_t;


/*
 * IA32 Other Regs CtrlXfer Item
 */

typedef union {
    L4_Word_t	raw[L4_CTRLXFER_OTHERREGS_SIZE+1];
    struct {
	L4_CtrlXferItem_t  item;
	L4_IA32_OtherRegs_t regs;
    };
} L4_OtherRegsCtrlXferItem_t;

L4_INLINE void L4_OtherRegsCtrlXferItemInit(L4_OtherRegsCtrlXferItem_t *c)
{
    L4_Word_t i;
    L4_CtrlXferItemInit(&c->item, L4_CTRLXFER_OTHERREGS_ID);
    for (i=0; i < L4_CTRLXFER_OTHERREGS_SIZE; i++)
	c->regs.reg[i] = 0;
    
}

L4_INLINE void L4_OtherRegsCtrlXferItemSet(L4_OtherRegsCtrlXferItem_t *c, 
					   L4_Word_t reg, L4_Word_t val)
{
    c->regs.reg[reg] = val;
    c->item.mask |= (1<<reg);    
}


L4_INLINE void L4_MsgAppendOtherRegsCtrlXferItem (L4_Msg_t * msg, L4_OtherRegsCtrlXferItem_t *c)
{
    L4_MsgAppendCtrlXferItem(msg, &c->item);
}

L4_INLINE L4_Word_t L4_MsgStoreOtherRegsCtrlXferItem (L4_Msg_t *msg, L4_Word_t mr, L4_OtherRegsCtrlXferItem_t *c)
{
    return L4_MsgStoreCtrlXferItem(msg, mr, &c->item);
}

L4_INLINE void L4_Init (L4_CtrlXferItem_t *c, L4_Word_t fault_id, L4_Word_t fault_mask)
{
    L4_FaultConfCtrlXferItemInit (c, fault_id, fault_mask);
}

L4_INLINE void L4_Init (L4_RegCtrlXferItem_t *c, L4_Word_t id)
{
    L4_RegCtrlXferItemInit(c, id);
}

L4_INLINE void L4_Init (L4_CtrlXferItem_t *c, L4_Word_t id)
{
    L4_CtrlXferItemInit(c, id);
}

L4_INLINE void L4_Init (L4_TStateCtrlXferItem_t *c)
{
    L4_TStateCtrlXferItemInit(c);
}

L4_INLINE void L4_Init (L4_GPRegsCtrlXferItem_t *c)
{
    L4_GPRegsCtrlXferItemInit(c);
}

L4_INLINE void L4_Init (L4_IA32_PMCCtrlXferItem_t *c)
{
    L4_IA32_PMCCtrlXferItemInit(c);
}

L4_INLINE void L4_Init (L4_CRegsCtrlXferItem_t *c)
{
    L4_CRegsCtrlXferItemInit(c);
}

L4_INLINE void L4_Init (L4_DRegsCtrlXferItem_t *c)
{
    L4_DRegsCtrlXferItemInit(c);
}

L4_INLINE void L4_Init (L4_SegCtrlXferItem_t *c, L4_Word_t id)
{
    L4_SegCtrlXferItemInit(c, id);
}

L4_INLINE void L4_Init (L4_DTRCtrlXferItem_t *c, L4_Word_t id)
{
    L4_DTRCtrlXferItemInit(c, id);
}

L4_INLINE void L4_Init (L4_NonRegExcCtrlXferItem_t *c)
{
    L4_NonRegExcCtrlXferItemInit(c);
}

L4_INLINE void L4_Init (L4_ExecCtrlXferItem_t *c)
{
    L4_ExecCtrlXferItemInit(c);
}

L4_INLINE void L4_Init (L4_OtherRegsCtrlXferItem_t *c)
{
    L4_OtherRegsCtrlXferItemInit(c);
}

L4_INLINE void L4_Set (L4_RegCtrlXferItem_t *c, L4_Word_t reg, L4_Word_t val)
{
    L4_RegCtrlXferItemSet(c, reg, val);
}

L4_INLINE void L4_Set (L4_TStateCtrlXferItem_t *c, L4_Word_t reg, L4_Word_t val)
{
    L4_TStateCtrlXferItemSet(c, reg, val);
}

L4_INLINE void L4_Set (L4_GPRegsCtrlXferItem_t *c, L4_Word_t reg, L4_Word_t val)
{
    L4_GPRegsCtrlXferItemSet(c, reg, val);
}

L4_INLINE void L4_Set (L4_IA32_PMCCtrlXferItem_t *c, L4_Word_t reg, L4_Word_t val)
{
    L4_IA32_PMCCtrlXferItemSet(c, reg, val);
}

L4_INLINE void L4_Set (L4_CRegsCtrlXferItem_t *c, L4_Word_t reg, L4_Word_t val)
{
    L4_CRegsCtrlXferItemSet(c, reg, val);
}

L4_INLINE void L4_Set (L4_DRegsCtrlXferItem_t *c, L4_Word_t reg, L4_Word_t val)
{
    L4_DRegsCtrlXferItemSet(c, reg, val);
}

L4_INLINE void L4_Set (L4_SegCtrlXferItem_t *c, L4_Word_t reg, L4_Word_t val)
{
    L4_SegCtrlXferItemSet(c, reg, val);
}

L4_INLINE void L4_Set (L4_DTRCtrlXferItem_t *c, L4_Word_t reg, L4_Word_t val)
{
    L4_DTRCtrlXferItemSet(c, reg, val);
}

L4_INLINE void L4_Set(L4_NonRegExcCtrlXferItem_t *c,  L4_Word_t reg, L4_Word_t val)
{
    L4_NonRegExcCtrlXferItemSet(c, reg, val);
}

L4_INLINE void L4_Set(L4_ExecCtrlXferItem_t *c, L4_Word_t reg, L4_Word_t val)
{
    L4_ExecCtrlXferItemSet(c, reg, val);
}

L4_INLINE void L4_Set(L4_OtherRegsCtrlXferItem_t *c, L4_Word_t reg, L4_Word_t val)
{
    L4_OtherRegsCtrlXferItemSet(c, reg, val);
}

L4_INLINE void L4_Append (L4_Msg_t * msg, L4_CtrlXferItem_t *c)
{
    L4_MsgAppendCtrlXferItem(msg, c);
}

L4_INLINE void L4_Append (L4_Msg_t * msg, L4_RegCtrlXferItem_t *c)
{
    L4_MsgAppendRegCtrlXferItem(msg, c);
}

L4_INLINE void L4_Append (L4_Msg_t * msg, L4_TStateCtrlXferItem_t *c)
{
    L4_MsgAppendTStateCtrlXferItem(msg, c);
}

L4_INLINE void L4_Append (L4_Msg_t * msg, L4_GPRegsCtrlXferItem_t *c)
{
    L4_MsgAppendGPRegsCtrlXferItem(msg, c);
}

L4_INLINE void L4_Append (L4_Msg_t * msg, L4_IA32_PMCCtrlXferItem_t *c)
{
    L4_MsgAppendIA32_PMCCtrlXferItem(msg, c);
}

L4_INLINE void L4_Append (L4_Msg_t * msg, L4_CRegsCtrlXferItem_t *c)
{
    L4_MsgAppendCRegsCtrlXferItem(msg, c);
}

L4_INLINE void L4_Append (L4_Msg_t * msg, L4_DRegsCtrlXferItem_t *c)
{
    L4_MsgAppendDRegsCtrlXferItem(msg, c);
}

L4_INLINE void L4_Append (L4_Msg_t * msg, L4_SegCtrlXferItem_t *c)
{
    L4_MsgAppendSegCtrlXferItem(msg, c);
}

L4_INLINE void L4_Append (L4_Msg_t * msg, L4_DTRCtrlXferItem_t *c)
{
    L4_MsgAppendDTRCtrlXferItem(msg, c);
}

L4_INLINE void L4_Append (L4_Msg_t * msg, L4_NonRegExcCtrlXferItem_t *c)
{
    L4_MsgAppendNonRegExcCtrlXferItem(msg, c);
}

L4_INLINE void L4_Append (L4_Msg_t * msg, L4_ExecCtrlXferItem_t *c)
{
    L4_MsgAppendExeccCtrlXferItem(msg, c);
}

L4_INLINE void L4_Append (L4_Msg_t * msg, L4_OtherRegsCtrlXferItem_t *c)
{
    L4_MsgAppendOtherRegsCtrlXferItem(msg, c);
}

L4_INLINE L4_Word_t L4_Store (L4_Msg_t *msg, L4_Word_t mr, L4_CtrlXferItem_t *c)
{
    return L4_MsgStoreCtrlXferItem(msg, mr, c);
}

L4_INLINE L4_Word_t L4_Store (L4_Msg_t *msg, L4_Word_t mr, L4_RegCtrlXferItem_t *c)
{
    return L4_MsgStoreRegCtrlXferItem(msg, mr, c);
}

L4_INLINE L4_Word_t L4_Store (L4_Msg_t *msg, L4_Word_t mr, L4_TStateCtrlXferItem_t *c)
{
    return L4_MsgStoreTStateCtrlXferItem(msg, mr, c);
}

L4_INLINE L4_Word_t L4_Store (L4_Msg_t *msg, L4_Word_t mr, L4_IA32_PMCCtrlXferItem_t *c)
{
    return L4_MsgStoreIA32_PMCCtrlXferItem(msg, mr, c);
}

L4_INLINE L4_Word_t L4_Store (L4_Msg_t *msg, L4_Word_t mr, L4_GPRegsCtrlXferItem_t *c)
{
    return L4_MsgStoreGPRegsCtrlXferItem(msg, mr, c);
}

L4_INLINE L4_Word_t L4_Store (L4_Msg_t *msg, L4_Word_t mr, L4_CRegsCtrlXferItem_t *c)
{
    return L4_MsgStoreCRegsCtrlXferItem(msg, mr, c);
}

L4_INLINE L4_Word_t L4_Store (L4_Msg_t *msg, L4_Word_t mr, L4_DRegsCtrlXferItem_t *c)
{
    return L4_MsgStoreDRegsCtrlXferItem(msg, mr, c);
}

L4_INLINE L4_Word_t L4_Store (L4_Msg_t *msg, L4_Word_t mr, L4_SegCtrlXferItem_t *c)
{
    return L4_MsgStoreSegCtrlXferItem(msg, mr, c);
}

L4_INLINE L4_Word_t L4_Store (L4_Msg_t *msg, L4_Word_t mr, L4_DTRCtrlXferItem_t *c)
{
    return L4_MsgStoreDTRCtrlXferItem(msg, mr, c);
}

L4_INLINE L4_Word_t L4_Store (L4_Msg_t *msg, L4_Word_t mr, L4_NonRegExcCtrlXferItem_t *c)
{
    return L4_MsgStoreNonRegExcCtrlXferItem(msg, mr, c);
}

L4_INLINE L4_Word_t L4_Store (L4_Msg_t *msg, L4_Word_t mr, L4_ExecCtrlXferItem_t *c)
{
    return L4_MsgStoreExeccCtrlXferItem(msg, mr, c);
}

L4_INLINE L4_Word_t L4_Store (L4_Msg_t *msg, L4_Word_t mr, L4_OtherRegsCtrlXferItem_t *c)
{
    return L4_MsgStoreOtherRegsCtrlXferItem(msg, mr, c);
}

#endif /* define(__cplusplus) */

L4_INLINE L4_Word_t L4_ConfCtrlXferItems(L4_ThreadId_t dest)
{    
    L4_Word_t dummy, old_control;
    L4_ThreadId_t dummy_tid;
    L4_ExchangeRegisters (dest, L4_EXREGS_CTRLXFER_CONF_FLAG, 0, 0 , 0, 0, L4_nilthread,
			  &old_control, &dummy, &dummy, &dummy, &dummy, &dummy_tid);
    return old_control;
}

L4_INLINE L4_Word_t L4_ReadCtrlXferItems(L4_ThreadId_t dest)
{
    L4_Word_t dummy, old_control;
    L4_ThreadId_t dummy_tid;
    L4_ExchangeRegisters (dest, L4_EXREGS_CTRLXFER_READ_FLAG, 0, 0 , 0, 0, L4_nilthread,
			  &old_control, &dummy, &dummy, &dummy, &dummy, &dummy_tid);
    return old_control;
}

L4_INLINE L4_Word_t L4_WriteCtrlXferItems(L4_ThreadId_t dest)
{
    L4_Word_t dummy, old_control;
    L4_ThreadId_t dummy_tid;
    L4_ExchangeRegisters (dest, L4_EXREGS_CTRLXFER_WRITE_FLAG, 0, 0 , 0, 0, L4_nilthread,
			  &old_control, &dummy, &dummy, &dummy, &dummy, &dummy_tid);
    return old_control;
}

/**********************************************************************
 *                       EVT Logging 
 **********************************************************************/
#define L4_LOG_SELECTOR_SIZE		(4096)
/*
 * Must be a power f 2
 */
#define L4_LOG_LD_MAX_RESOURCES                 2
#define L4_LOG_MAX_RESOURCES                    (1 <<  L4_LOG_LD_MAX_RESOURCES)
#define L4_LOG_RESOURCE_PMC			0  /* performance counters at context switch */
#define L4_LOG_RESOURCE_PMIPC                   1  /* preemption IPC event */

#define L4_LOG_MAX_LOGIDS			32
#define L4_LOG_NULL_LOGID			(0xFFFFFFFF)
#define L4_LOG_IDLE_LOGID			(0)
#define L4_LOG_ROOTSERVER_LOGID                 (1)

#define L4_LOG_EVENT_BIT			 5
#define L4_LOG_CURRENT_BIT			 6
#define L4_LOG_CORRESPONDENT_BIT		 7
#define L4_LOG_TIMESTAMP_BIT                     8
#define L4_LOG_PADDING_BIT			 10
#define L4_LOG_COUNTER_BIT			 12
#define L4_LOG_ADD_BIT                           13
#define L4_LOG_ALL_FLAGS			 0xFFE0	       


#define L4_LOG_TIMESTAMP_OFF                     0x0
#define L4_LOG_TIMESTAMP_VIRTUAL		0x1
#define L4_LOG_TIMESTAMP_RDTSC                  0x2
#define L4_LOG_TIMESTAMP_TICK                   0x3

#define L4_LOG_TIMESTAMP_ENABLED	(0x3 << LOG_TIMESTAMP_BIT)
#define L4_LOG_TIMESTAMP_NOT_VIRTUAL	(0x2 << LOG_TIMESTAMP_BIT)
#define L4_LOG_TIMESTAMP_RDTSC_OR_TICK	(0x1 << LOG_TIMESTAMP_BIT)

#define L4_LOG_CPU_TSC_PRECISION	8

#define L4_LOG_SIZE_MASK_MASK		(0x1F) 
#define L4_LOG_MAX_LOG_SIZE_MASK        (32768 -1)
#define L4_LOG_PADDING_MASK             (0x3 << LOG_PADDING_BIT)

#define L4_LOG_SIZE_N_PADDING_MASK	(LOG_SIZE_MASK_MASK | LOG_PADDING_MASK)

typedef L4_Word16_t                     L4_LogSel_t;

typedef union{

    struct{
	L4_Word_t size_mask             :5;
	L4_Word_t event         	:1;	// 0 = off,  1 = log
	L4_Word_t cur                   :1;	// 0 = off,  1 = log
	L4_Word_t correspondent 	:1;	// 0 = off,  1 = on
	L4_Word_t timestamp		:2;	// 0 = off,  1 = virtual, 2 = tsc, 3 = tick
	L4_Word_t padding		:2;	// 0 = +0, 1= +4, 2=+8, 3=+12 bytes
	L4_Word_t counter		:1;	// 0 = off,  1 = on
	L4_Word_t overwrite_add         :1;	// 0 = overwrite,  1 = add
	L4_Word_t reserved		:2;

	L4_Word16_t current_offset;
	
    } __attribute__((packed)) X;
    
    L4_Word32_t raw;
    
   
    
} __attribute__((packed)) L4_LogCtrl_t;


#define L4_LOG_GET_IDX(offset, c)	\
    (L4_Word_t* ) (c + (offset / sizeof(L4_LogFile_Control_t)))

#define L4_LOG_MASK(idx, mask)       ((L4_Word_t) (idx) & (mask))

#define L4_LOG_INC_LOG(idx, ctrl)      \
    idx = (L4_Word_t *) (L4_LOG_MASK((idx + 1), ((1UL << ctrl->X.size_mask) -1)) |\
                         L4_LOG_MASK(idx, (~((1UL << ctrl->X.size_mask) -1))))

#define L4_LOG_DEC_LOG(idx, ctrl)      \
	idx = (L4_Word_t *) (L4_LOG_MASK((idx - 1), ((1UL << ctrl->X.size_mask) -1)) | \
                         L4_LOG_MASK(idx, (~((1UL << ctrl->X.size_mask) -1))))

#define L4_LOG_NEXT_LOG(idx, ctrl)	\
    (L4_Word_t *) (L4_LOG_MASK((idx + 1), ((1UL << ctrl->X.size_mask) -1)) |\
			 L4_LOG_MASK(idx, (~((1UL << ctrl->X.size_mask) -1))))

#define L4_LOG_PREV_LOG(idx, ctrl)						\
    (L4_Word_t *) (L4_LOG_MASK((idx - 1), ((1UL << ctrl->X.size_mask) -1)) |\
			 L4_LOG_MASK(idx, (~((1UL << ctrl->X.size_mask) -1))))
#define L4_LOG_FIRST_LOG(ctrl) \
    (L4_Word_t *) (L4_LOG_MASK(L4_LOG_GET_IDX(ctrl->X.current_offset, ctrl), (~((1UL << ctrl->X.size_mask) -1))))

#define L4_LOG_ENTRIES(ctrl)            (1UL << (ctrl->X.size_mask - 2))
#define L4_LOG_SIZE(ctrl)               (1UL << ctrl->X.size_mask)



L4_INLINE void* L4_LogRegion (void * KernelInterface)
{
    L4_KernelInterfacePage_t * __kip;
    __kip = (L4_KernelInterfacePage_t *) KernelInterface;
    
    return (void *) __kip->__paddingA0[0];
}

L4_INLINE L4_Word_t L4_LogSelectorPage (void * KernelInterface)
{
    L4_KernelInterfacePage_t * __kip;
    __kip = (L4_KernelInterfacePage_t *) KernelInterface;
    
    return __kip->__paddingA0[1];
}

L4_INLINE L4_LogCtrl_t *L4_LogFile (L4_LogCtrl_t *base, L4_LogSel_t *selbase,
                                    L4_Word_t logid, L4_Word_t resource)
{
    L4_LogSel_t sel = selbase[(logid * L4_LOG_MAX_RESOURCES) + resource] / sizeof(L4_LogCtrl_t);
    return &base[sel];
}

L4_INLINE L4_Word_t L4_Set_Logid (L4_ThreadId_t tid, L4_Word_t logid)
{
    L4_Word_t dummy;
    return L4_Schedule(tid, ~0UL, ~0UL, (logid << 9 | 0x1FF), ~0UL, &dummy);
}

/**********************************************************************
 *                       EVT Logging - PMIPC weights 
 **********************************************************************/
#define L4_X86_PMC_PENTIUM_D
#undef L4_X86_PMC_PENTIUM_4

#if defined(L4_X86_PMC_PENTIUM_4)
#define L4_X86_PMC_TSC_WEIGHT		    (617)	    
#define L4_X86_PMC_UC_WEIGHT		    (712)
#define L4_X86_PMC_MQW_WEIGHT		    (475)
#define L4_X86_PMC_RB_WEIGHT		     (56)
#define L4_X86_PMC_MB_WEIGHT		  (34046)
#define L4_X86_PMC_MR_WEIGHT		    (173)		  
#define L4_X86_PMC_MLR_WEIGHT		   (2996)
#define L4_X86_PMC_LDM_WEIGHT		   (1355)

#elif defined(L4_X86_PMC_PENTIUM_D)

/* 
 * P. Moltmann's Study thesis
 */
#if 0
#define L4_X86_PMC_TSC_WEIGHT		   (1270)
#define L4_X86_PMC_UC_WEIGHT		   (1660)
#define L4_X86_PMC_MQW_WEIGHT		   (0)
#define L4_X86_PMC_RB_WEIGHT		   (1007) 
#define L4_X86_PMC_MB_WEIGHT		   (0)
#define L4_X86_PMC_MR_WEIGHT		   (598)    	 
#define L4_X86_PMC_MLR_WEIGHT		   (0)
#define L4_X86_PMC_LDM_WEIGHT		   (19200)
#endif

 /* 
  * P. Moltmann, but recalibrated TSC and UC (J.Stoess):
  *  - ran only hypervisor, without VMs
  *  - made the throttler thread either execute "hlt" or real cycles
  *  - obtained the performance counter events, they differed in TSC/UC mostly
  *  - obtained real CPU power via labview, both 
  *  - results (10 sec runtime): 
  *    HLT   : 29999 tsc +   190 uc  ( + 3 rb + 13 mr ) = 43309
  *    NOHLT : 29981 tsc + 29981 uc  ( + 4 rb + 13 mr ) = 85934
  */
#if 0
#define L4_X86_PMC_TSC_WEIGHT		   (1435) 
#define L4_X86_PMC_UC_WEIGHT		   (1432) 
#define L4_X86_PMC_MQW_WEIGHT		   (0)
#define L4_X86_PMC_RB_WEIGHT		   (1007) 
#define L4_X86_PMC_MB_WEIGHT		   (0)
#define L4_X86_PMC_MR_WEIGHT		   (598)    	 
#define L4_X86_PMC_MLR_WEIGHT		   (0)
#define L4_X86_PMC_LDM_WEIGHT		   (19200)
#endif

/* 
 * J. Stoess earmdq 
 *  
 * time_stamp_counter           TSC
 * global_power_events          UC
 * ld_miss_1L_retired_cnt       LDM
 * mem_retired_cnt              MR 
 * mem_retired_tag              --- ZERO ---
 * mispred_branch_retired       MB
 * mob_load_replay              MLR
 * retired_branch_type          RB
 * uop_queue_writes             MQW
 * x87_fp_uop                   --- UNTRACKED ---
 */

/* Least Squares Method */
#if 1
#define L4_X86_PMC_TSC_WEIGHT		   (1418)  
#define L4_X86_PMC_UC_WEIGHT		   (1285)  
#define L4_X86_PMC_LDM_WEIGHT		   (881)   
#define L4_X86_PMC_MR_WEIGHT		   (649)   
#define L4_X86_PMC_MB_WEIGHT		   (23421) 
#define L4_X86_PMC_MLR_WEIGHT		   (4320)      	
#define L4_X86_PMC_RB_WEIGHT		   (840)   
#define L4_X86_PMC_MQW_WEIGHT		   (75)    
#endif

/* LP/Simplex Method */
#if 0
#define L4_X86_PMC_TSC_WEIGHT		   (1419)
#define L4_X86_PMC_UC_WEIGHT		   (1029)
#define L4_X86_PMC_LDM_WEIGHT		   (1665)
#define L4_X86_PMC_MR_WEIGHT		   (193)
#define L4_X86_PMC_MB_WEIGHT		   (48978)
#define L4_X86_PMC_MLR_WEIGHT		   (5306)
#define L4_X86_PMC_RB_WEIGHT		   (378)
#define L4_X86_PMC_MQW_WEIGHT		   (244)
#endif

#else
#error please define EARM PMC weights
#endif


#endif /* !__L4__IA32__ARCHVM_H__ */