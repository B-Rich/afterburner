/*********************************************************************
 *
 * Copyright (C) 2005,  University of Karlsruhe
 *
 * File path:     afterburn-wedge/device/i8259a.cc
 * Description:   Front-end emulation for the legacy XT-PIC 8259.
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

#include <device/portio.h>
#include INC_ARCH(intlogic.h)
#include INC_ARCH(bitops.h)
#include <console.h>
#include <debug.h>
#include INC_WEDGE(backend.h)

#ifdef CONFIG_WEDGE_KAXEN
#include INC_WEDGE(xen_hypervisor.h)
#endif
#include <burn_counters.h>

DECLARE_BURN_COUNTER(8259_mask_interrupts);
DECLARE_BURN_COUNTER(8259_master_write);
DECLARE_BURN_COUNTER(8259_master_read);
DECLARE_BURN_COUNTER(8259_slave_write);
DECLARE_BURN_COUNTER(8259_slave_read);
DECLARE_BURN_COUNTER(8259_slow_iface);

bool i8259a_t::pending_vector( word_t & vector, word_t & irq, const word_t irq_base)
{
    //    if( irq_in_service )
    //	return false;
    
    // We try to find and then claim a pending IRQ.  Other processors
    // may compete with us.  We try until we claim an IRQ, or until
    // none remain.
    for (;;) {
	
	word_t masked_irr = irq_request & ~irq_mask;
	
	if( !masked_irr )
	{
	    get_cpu().clear_irq_vector(irq_base);
	    return false;  // No pending, unmasked interrupts.
	}
	
	// Find the highest prio IRQ, which is the lowest numbered IRQ
	// in standard configuration.
	word_t pic_irq = lsb( masked_irr );
	
	irq_request &= ~(1 << pic_irq);
	irq = pic_irq + irq_base;

	dprintf(irq_dbg_level(irq)+1, "i8259: found pending unmasked irq %d\n", irq);
	    
	if( !icw4.is_auto_eoi() )
	    bit_set_atomic( pic_irq, irq_in_service );
	    
	vector = pic_irq + icw2.get_idt_offset();

	if ((irq_request & ~irq_mask) == 0)
	    get_cpu().clear_irq_vector(irq_base);

	return true;
	//}
    }
}

void i8259a_t::reraise_vector(word_t vector, word_t irq_base)
{
    if (vector < icw2.get_idt_offset() || 
	vector >= icw2.get_idt_offset() + 8U)
	return;
    
    word_t pic_irq =  vector - icw2.get_idt_offset();
    word_t irq = pic_irq + irq_base;
	
    ASSERT( pic_irq < 8 );
    
    bit_set_atomic( pic_irq, irq_request );
    
    dprintf(irq_dbg_level(irq)+1, "i8259: reraise vector %d irq %d pic irq %d\n", 
	    vector, irq, pic_irq);
    
    if ((irq_mask & (1 << pic_irq)) == 0)
	get_cpu().set_irq_vector(irq);
}

void i8259a_t::raise_irq( word_t irq, const word_t irq_base)
{
    word_t pic_irq = irq - irq_base;
    
    ASSERT( pic_irq < 8 );
    
    bit_set_atomic( pic_irq, irq_request );
    
    get_intlogic().set_hwirq_mask(irq);

    dprintf(irq_dbg_level(irq)+1, "i8259: raise irq %d pic irq %d\n", irq, pic_irq);
    
    if ((irq_mask & (1 << pic_irq)) == 0)
	get_cpu().set_irq_vector(irq);
}



static const word_t master_irq_offset = 0;
static const word_t slave_irq_offset = 8;

INLINE word_t
port_byte_result( word_t eax, u8_t result )
{
    return (eax & ~word_t(0xff)) | result;
}

/**************************************************************************
 * port reads
 **************************************************************************/

u8_t i8259a_t::port_a_read( void )
{
    if (ocw_read.is_poll_mode())
    {
	word_t irq;
	if (eoi(irq))
	    return irq;
	else return 0x7;
    }
    else if( ocw_read.is_read_isr() )
	return irq_in_service;
    else
	return irq_request;
}

u8_t i8259a_t::port_b_read( void )
{
    u8_t result = 0xff;
    if( EXPECT_FALSE(this->mode != i8259a_t::ocw_mode) )
	printf( "i8259a unimplemented port read (port 0x21/0xa1)\n");
    else
	result = this->irq_mask;
    return result;
}

extern "C" __attribute__((regparm(1)))
word_t device_8259_0x20_in( word_t eax )
{
    INC_BURN_COUNTER(8259_master_read);
    return port_byte_result( eax, get_intlogic().master.port_a_read() );
}

extern "C" __attribute__((regparm(1)))
word_t device_8259_0x21_in( word_t eax )
{
    INC_BURN_COUNTER(8259_master_read);
    return port_byte_result( eax, get_intlogic().master.port_b_read() );
}

extern "C" __attribute__((regparm(1)))
word_t device_8259_0xa0_in( word_t eax )
{
    INC_BURN_COUNTER(8259_slave_read);
    return port_byte_result( eax, get_intlogic().slave.port_a_read() );
}

extern "C" __attribute__((regparm(1)))
word_t device_8259_0xa1_in( word_t eax )
{
    INC_BURN_COUNTER(8259_slave_read);
    return port_byte_result( eax, get_intlogic().slave.port_b_read() );
}

/**************************************************************************
 * port writes
 **************************************************************************/

void i8259a_t::port_a_write( u8_t value )
{
    i8259a_ocw_t ocw = {x: {raw: value}};
    intlogic_t &intlogic = get_intlogic();
    bool unmask_irq = false;
    word_t irq;
    if( ocw.is_icw1() ) 
    {
	dprintf(debug_irq+1, "i8259a icw1 write\n");
	this->icw1.set_raw( value );
	if( !this->icw1.icw4_enabled() )
	    this->icw4.set_raw( 0 );
	this->reset_mode();
	this->irq_mask = 0xff;
    }
    else if( ocw.is_ocw3() ) {
	if( ocw.is_read_request() || ocw.is_poll_mode())
	    ocw_read = ocw;
	else
	    printf( "Unimplemented i8259a ocw3 write.\n");
    }
    else if( ocw.is_specific_eoi() ) 
    {
	//unmask_irq = seoi( ocw.get_level());
	unmask_irq = eoi(irq);
	dprintf(irq_dbg_level(irq), "i8259a specific eoi %d (%c)\n", 
		irq, (unmask_irq ? 'u' : 'X'));
	

    }
    else if( ocw.is_non_specific_eoi() ) 
    {
	unmask_irq = eoi(irq);
	dprintf(irq_dbg_level(irq), "i8259a non-specific eoi %d (%c)\n", 
		irq, (unmask_irq ? 'u' : 'X'));
	
    }
    else
	printf( "Unimplemented i8259a ocw2 write.\n");

    if (unmask_irq &&
	!intlogic.is_hwirq_squashed(irq) &&
	intlogic.test_and_clear_hwirq_mask(irq))
	backend_unmask_device_interrupt(irq);

}

void i8259a_t::port_b_write( u8_t value, u8_t irq_base )
{
    if( EXPECT_TRUE(this->mode == i8259a_t::ocw_mode) )
    {
	// OCW1
	intlogic_t &intlogic = get_intlogic();
	word_t old_irq_mask = this->irq_mask;
	this->irq_mask = value;
	if( (irq_request & old_irq_mask /* = pending but masked */) & ~irq_mask)
	    get_cpu().set_irq_vector( irq_base );

	INC_BURN_COUNTER(8259_mask_interrupts);

	word_t old_hwirq_mask = ((old_irq_mask << irq_base) | ~( 0xff << irq_base));  
	word_t hwirq_mask = ((this->irq_mask << irq_base) | ~( 0xff << irq_base)); 

	dprintf(debug_irq+1, "i8259a port b write irq_mask %x old %x base %x latch %x squash %x mask %x\n",
		hwirq_mask, old_irq_mask, irq_base, intlogic.get_hwirq_latch() ,
		intlogic.get_hwirq_squash(), intlogic.get_hwirq_mask());
	
	// Those unmasked & those not latched & those not squashed
	word_t newly_enabled = ~hwirq_mask & ~intlogic.get_hwirq_latch() 
	    & ~intlogic.get_hwirq_squash();

	while( newly_enabled ) 
	{
	    word_t new_irq = lsb( newly_enabled );
	    bit_clear( new_irq, newly_enabled );
	    
	    intlogic.set_hwirq_latch(new_irq);

#if defined(CONFIG_L4KA_HVM)
	    printf("i8259a enable irq %d, latch %x\n", new_irq, intlogic.get_hwirq_latch());
#endif
	    
	    dprintf(irq_dbg_level(new_irq), "i8259a enable irq %d, latch %x\n", new_irq, intlogic.get_hwirq_latch());
	    if( !backend_enable_device_interrupt(new_irq, get_vcpu()) )
		printf( "Unable to enable passthru device interrupt %d\n", new_irq);
		    
	}
	
	// Those masked before & those unmasked now & those masked in hw
	// & those not squashed
	word_t device_unmasked = old_hwirq_mask & ~hwirq_mask
	    & intlogic.get_hwirq_mask() & ~intlogic.get_hwirq_squash();
	
	while( device_unmasked ) 
	{
	    word_t unmasked_irq = lsb( device_unmasked );
	    bit_clear( unmasked_irq, device_unmasked );
	    intlogic.clear_hwirq_mask(unmasked_irq);

	    dprintf(irq_dbg_level(unmasked_irq), "i8259a unmask irq %d, mask %x\n", 
		    unmasked_irq, intlogic.get_hwirq_mask());
	    backend_unmask_device_interrupt( unmasked_irq );
	}
	
    }

    else if( this->mode == i8259a_t::icw2_mode ) {
	dprintf(debug_irq+1, "i8259a icw2 write\n");
	this->icw2.set_raw( value );
	this->next_mode();
    }
    else if( this->mode == i8259a_t::icw3_mode ) {
	dprintf(debug_irq+1, "i8259a icw3 write\n");
	this->icw3.set_raw( value );
	this->next_mode();
    }
    else if( this->mode == i8259a_t::icw4_mode ) {
	dprintf(debug_irq+1, "i8259a icw4 write\n");
	this->icw4.set_raw( value );
	this->next_mode();
    }
    else {
	printf( "Error: i8259a state machine corrupted.\n");
	this->reset_mode();
    }
}


extern "C" __attribute__((regparm(1)))
word_t device_8259_0x20_out( word_t eax )
{
    INC_BURN_COUNTER(8259_master_write);
    get_intlogic().master.port_a_write( eax );
    return eax;
}

extern "C" __attribute__((regparm(1)))
word_t device_8259_0x21_out( word_t eax )
{
    INC_BURN_COUNTER(8259_master_write);
    get_intlogic().master.port_b_write( eax, master_irq_offset );
    return eax;
}

extern "C" __attribute__((regparm(1)))
word_t device_8259_0xa0_out( word_t eax )
{
    INC_BURN_COUNTER(8259_slave_write);
    get_intlogic().slave.port_a_write( eax );
    return eax;
}

extern "C" __attribute__((regparm(1)))
word_t device_8259_0xa1_out( word_t eax )
{
    INC_BURN_COUNTER(8259_slave_write);
    get_intlogic().slave.port_b_write( eax, slave_irq_offset );
    return eax;
}

/**************************************************************************
 * old interface
 **************************************************************************/

void i8259a_portio( u16_t port, u32_t & value, u32_t bit_width, bool read )
{
    INC_BURN_COUNTER(8259_slow_iface);

    if ( port == 0x20 && read )
	value = device_8259_0x20_in(0);
    else if ( port == 0x21 && read )
	value = device_8259_0x21_in(0);
    else if ( port == 0xa0 && read )
	value = device_8259_0xa0_in(0);
    else if ( port == 0xa1 && read )
	value = device_8259_0xa1_in(0);

    else if ( port == 0x20 )
	device_8259_0x20_out( value );
    else if ( port == 0x21 )
	device_8259_0x21_out( value );
    else if ( port == 0xa0 )
	device_8259_0xa0_out( value );
    else if ( port == 0xa1 )
	device_8259_0xa1_out( value );
    
    else if (port == 0x4d0 || port == 0x4d1)
	printf( "Ignored 8259 edge/level ctrl ECLR%c %c port val %x\n", 
		(port == 0x4d0 ? '0' : '1'), (read ? 'r' : 'w'), value);
    
    else 
    {
	printf( "Unknown port for the 8259 device: %x\n", port);
	value = (u32_t) ~0;
    }
}
