/*********************************************************************
 *
 * Copyright (C) 2005,  University of Karlsruhe
 *
 * File path:     afterburn-wedge/device/pci.cc
 * Description:   Front-end, simple PCI model.
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
 * $Id: pci.cc,v 1.6 2006/09/21 13:40:44 joshua Exp $
 *
 ********************************************************************/

#include <console.h>
#include <debug.h>
#include INC_WEDGE(backend.h)

#include <device/pci.h>
#include <device/portio.h>
#if defined(CONFIG_DEVICE_E1000)
#include <device/e1000.h>
#endif
#if defined(CONFIG_DEVICE_I82371AB)
#include <device/i82371ab.h>
#endif
#include <device/dp83820.h>

#define DIGIT_STRING(a,b,c,d) (((a)<<12) | ((b)<<8) | ((c)<<4) | (d))
#define DIGIT_STRING16(a,b)   (((a) <<4) | (b))

static pci_config_addr_t config_addr;

const static pci_command_t host_bus_command = { x: { fields: {
    io_space_enabled: 0,
    mem_space_enabled: 1,
    bus_master_enabled: 0,
    special_cycles_enabled: 0,
    memory_write_ctrl: 0,
    vga_snoop_ctrl: 0,
    parity_error_response: 0,
    wait_cycle_ctrl: 0,
    system_err_ctrl: 1,
    fast_b2b_ctrl: 0,
    interrupt_disable: 1,
    reserved: 0,
}}};

const static pci_command_t isa_bus_command = { x: { fields: {
    io_space_enabled: 0,
    mem_space_enabled: 1,
    bus_master_enabled: 1,
    special_cycles_enabled: 0,
    memory_write_ctrl: 0,
    vga_snoop_ctrl: 0,
    parity_error_response: 0,
    wait_cycle_ctrl: 0,
    system_err_ctrl: 1,
    fast_b2b_ctrl: 0,
    interrupt_disable: 1,
    reserved: 0,
}}};

const static pci_command_t e1000_command = { x: { fields: {
    io_space_enabled: 1,
    mem_space_enabled: 1,
    bus_master_enabled: 1,
    special_cycles_enabled: 0,
    memory_write_ctrl: 1,
    vga_snoop_ctrl: 0,
    parity_error_response: 0,
    wait_cycle_ctrl: 0,
    system_err_ctrl: 1,
    fast_b2b_ctrl: 0,
    interrupt_disable: 0,
    reserved: 0,
}}};

const static pci_command_t dp83820_command = { x: { fields: {
    io_space_enabled: 1,
    mem_space_enabled: 1,
    bus_master_enabled: 1,
    special_cycles_enabled: 0,
    memory_write_ctrl: 1,
    vga_snoop_ctrl: 0,
    parity_error_response: 0,
    wait_cycle_ctrl: 0,
    system_err_ctrl: 1,
    fast_b2b_ctrl: 0,
    interrupt_disable: 0,
    reserved: 0,
}}};

const static pci_command_t i82371ab_command = { x: { fields: {
    io_space_enabled: 1,
    mem_space_enabled: 1,
    bus_master_enabled: 1,
    special_cycles_enabled: 0,
    memory_write_ctrl: 0,
    vga_snoop_ctrl: 0,
    parity_error_response: 0,
    wait_cycle_ctrl: 0,
    system_err_ctrl: 0,
    fast_b2b_ctrl: 0,
    interrupt_disable: 1,
    reserved: 0,
}}};

const static pci_command_t vga_command = { x: { fields: {
    io_space_enabled: 0,
    mem_space_enabled: 1,
    bus_master_enabled: 0,
    special_cycles_enabled: 0,
    memory_write_ctrl: 0,
    vga_snoop_ctrl: 0,
    parity_error_response: 0,
    wait_cycle_ctrl: 0,
    system_err_ctrl: 0,
    fast_b2b_ctrl: 0,
    interrupt_disable: 1,
    reserved: 0,
}}};



const static pci_status_t host_bus_status = { x: { fields: {
    reserved: 0,
    cap_66: 0,
    user_features : 0,
    fast_b2b: 1,
    data_parity: 0,
    device_select_timing: 0,
    signaled_target_abort: 0,
    received_target_abort: 0,
    received_master_abort: 0,
    signaled_sys_err: 0,
    detected_parity_err: 0,
}}};

const static pci_status_t isa_bus_status = { x: { fields: {
    reserved: 0,
    cap_66: 0,
    user_features : 0,
    fast_b2b: 1,
    data_parity: 0,
    device_select_timing: 0,
    signaled_target_abort: 0,
    received_target_abort: 0,
    received_master_abort: 0,
    signaled_sys_err: 0,
    detected_parity_err: 0,
}}};

const static pci_status_t e1000_status = { x: { fields: {
    reserved: 0,
    cap_66: 1,
    user_features : 0,
    fast_b2b: 1,
    data_parity: 0,
    device_select_timing: 1,
    signaled_target_abort: 0,
    received_target_abort: 0,
    received_master_abort: 0,
    signaled_sys_err: 0,
    detected_parity_err: 0,
}}};

const static pci_status_t dp83820_status = { x: { fields: {
    reserved: 0,
    cap_66: 1,
    user_features : 0,
    fast_b2b: 1,
    data_parity: 0,
    device_select_timing: 1,
    signaled_target_abort: 0,
    received_target_abort: 0,
    received_master_abort: 0,
    signaled_sys_err: 0,
    detected_parity_err: 0,
}}};

const static pci_status_t i82371ab_status = { x: { fields: {
    reserved: 0,
    cap_66: 0,
    user_features : 0,
    fast_b2b: 1,
    data_parity: 0,
    device_select_timing: 1,
    signaled_target_abort: 0,
    received_target_abort: 0,
    received_master_abort: 0,
    signaled_sys_err: 0,
    detected_parity_err: 0,
}}};

const static pci_status_t vga_status = { x: { fields: {
    reserved: 0,
    cap_66: 1,
    user_features : 0,
    fast_b2b: 1,
    data_parity: 0,
    device_select_timing: 1,
    signaled_target_abort: 0,
    received_target_abort: 0,
    received_master_abort: 0,
    signaled_sys_err: 0,
    detected_parity_err: 0,
}}};



const static pci_bist_t host_bus_bist = { x: { raw: 0 } };
const static pci_bist_t e1000_bist = { x: { fields: {
    completion_status: 0,
    reserved: 0,
    start_ctrl: 0,
    supported: 1,
}}};
const static pci_bist_t dp83820_bist = { x: { fields: {
    completion_status: 0,
    reserved: 0,
    start_ctrl: 0,
    supported: 1,
}}};
const static pci_bist_t i82371ab_bist = { x: { fields: {
    completion_status: 0,
    reserved: 0,
    start_ctrl: 0,
    supported: 1,
}}};
const static pci_bist_t vga_bist = { x: { raw: 0 } };


pci_header_t pci_host_bus_header = { x: { fields: { 
    vendor_id: 0x8086,
    device_id: 0x1237,
    command: host_bus_command,
    status: host_bus_status,
    revision_id: 2,
    programming_interface: 0,
    sub_class_code: pci_header_t::host,
    base_class_code: pci_header_t::pci_bridge,
    cache_line_size: 10,
    latency_timer: 32,
    header_type: 0,
    bist: host_bus_bist,
    base_addr_registers: {
	{x:{raw:0}},
	{x:{raw:0}},
	{x:{raw:0}},
	{x:{raw:0}},
	{x:{raw:0}},
	{x:{raw:0}},
    },
    cardbus_cis_pointer: 0,
    subsys_vendor_id: ~0,
    subsys_id: 0,
    rom_base_addr: 0,
    rom_size: 0,
    reserved2: 0,
    interrupt_line: 0xff,
    interrupt_pin: 0,
    min_gnt: 0,
    max_lat: 0,
}},
base_addr_requests: {
    {x:{raw:0}},
    {x:{raw:0}},
    {x:{raw:0}},
    {x:{raw:0}},
    {x:{raw:0}},
    {x:{raw:0}},
},
};

pci_header_t pci_isa_bridge_header = { x: { fields: { 
    vendor_id: 0x8086,
    device_id: 0x7000,
    command: isa_bus_command,
    status: isa_bus_status,
    revision_id: 1,
    programming_interface: 0,
    sub_class_code: pci_header_t::isa,
    base_class_code: pci_header_t::pci_bridge,
    cache_line_size: 10,
    latency_timer: 32,
    header_type: 0x80,
    bist: host_bus_bist,
    base_addr_registers: {
	{x:{raw:0}},
	{x:{raw:0}},
	{x:{raw:0}},
	{x:{raw:0}},
	{x:{raw:0}},
	{x:{raw:0}},
    },
    cardbus_cis_pointer: 0,
    subsys_vendor_id: ~0,
    subsys_id: 0,
    rom_base_addr: 0,
    rom_size: 0,
    reserved2: 0,
    interrupt_line: 0xff,
    interrupt_pin: 0,
    min_gnt: 0,
    max_lat: 0,
}},
base_addr_requests: {
    {x:{raw:0}},
    {x:{raw:0}},
    {x:{raw:0}},
    {x:{raw:0}},
    {x:{raw:0}},
    {x:{raw:0}},
},
};



pci_header_t pci_e1000_header_dev0 = { x: { fields: {
    vendor_id: 0x8086, 
    device_id: 0x100e,
    command: e1000_command,
    status: e1000_status,
    revision_id: 2,
    programming_interface: 0,
    sub_class_code: pci_header_t::ethernet,
    base_class_code: pci_header_t::network,
    cache_line_size: 10,
    latency_timer: 32,
    header_type: 0,
    bist: e1000_bist,
    base_addr_registers: {
	{x:{memory:{mem_request:0, type:0, prefetchable:1, address:0xf000000}}},
	{x:{io:{io_request:1, reserved:0, address:9000}}},
	{x:{io:{io_request:1, reserved:0, address:9300}}},
	{x:{raw:0}},
	{x:{raw:0}},
	{x:{raw:0}},
    },
    cardbus_cis_pointer: 0,
    subsys_vendor_id: ~0,
    subsys_id: 0,
    rom_base_addr: 0,  // TODO
    rom_size: 0,
    reserved2: 0,
#if defined(CONFIG_DEVICE_APIC)
    // see acpi-dsdt.dsl from contrib
    interrupt_line: 11,
#else
    interrupt_line: 5,
#endif
    interrupt_pin: 1,
    min_gnt: 0,
    max_lat: 0,
}},
				       
base_addr_requests: {
			// Request a 64KB region.
    {x:{memory:{mem_request:0, type:0, prefetchable:1, address:0xffff000}}},
    			// Request a 16-byte port region for the e1000.
    {x:{io:{io_request:1, reserved:0, address:0xfffffffc}}},
    			// Request a 16-byte port region for the e1000 bypass.
    {x:{io:{io_request:1, reserved:0, address:0xfffffffc}}},
    {x:{raw:0}},
    {x:{raw:0}},
    {x:{raw:0}},
},
};

pci_header_t pci_dp83820_header_dev0 = { x: { fields: {
    vendor_id: 0x100b, 
    device_id: 0x0022,
    command: dp83820_command,
    status: dp83820_status,
    revision_id: 2,
    programming_interface: 0,
    sub_class_code: pci_header_t::ethernet,
    base_class_code: pci_header_t::network,
    cache_line_size: 10,
    latency_timer: 32,
    header_type: 0,
    bist: dp83820_bist,
    base_addr_registers: {
	{x:{raw:0}},
	{x:{memory:{mem_request:0, type:0, prefetchable:1, address:0xf000000}}},
	{x:{raw:0}},
	{x:{raw:0}},
	{x:{raw:0}},
	{x:{raw:0}},
    },
    cardbus_cis_pointer: 0,
    subsys_vendor_id: DIGIT_STRING16('l','4'),
    subsys_id: DIGIT_STRING16('k','a'),
    rom_base_addr: 0,  // TODO
    rom_size: 0,
    reserved2: 0,
#if defined(CONFIG_DEVICE_APIC)
    interrupt_line: 16,
#else
    interrupt_line: 5,
#endif
    interrupt_pin: 1,
    min_gnt: 0,
    max_lat: 0,
}},
base_addr_requests: {
    {x:{raw:0}}, // Request a 4KB region.
    {x:{memory:{mem_request:0, type:0, prefetchable:1, address:0xfffff00}}},
    {x:{raw:0}},
    {x:{raw:0}},
    {x:{raw:0}},
    {x:{raw:0}},
},
};

pci_header_t pci_i82371ab_header_dev0 = { x: { fields: {
    vendor_id: 0x8086, 
    device_id: 0x7111,
    command: i82371ab_command,
    status: i82371ab_status,
    revision_id: 1,
    programming_interface: 0x80,
    sub_class_code: pci_header_t::ide,
    base_class_code: pci_header_t::mass_storage,
    cache_line_size: 0,
    latency_timer: 0,
    header_type: 0,
    bist: i82371ab_bist,
    base_addr_registers: {
	{x:{raw:0}},
	{x:{raw:0}},
	{x:{raw:0}},
	{x:{raw:0}},
	{x:{io:{io_request:1, reserved:0, address:0x3000}}},
	{x:{raw:0}},
    },
    cardbus_cis_pointer: 0,
    subsys_vendor_id: DIGIT_STRING16('l','4'),
    subsys_id: DIGIT_STRING16('k','a'),
    rom_base_addr: 0,
    rom_size: 0,
    reserved2: 0,
    interrupt_line: 5,
    interrupt_pin: 1,
    min_gnt: 0,
    max_lat: 0,
}},
base_addr_requests: {
    {x:{raw:0}},
    {x:{raw:0}},
    {x:{raw:0}},
    {x:{raw:0}},
    {x:{io:{io_request:1, reserved:0, address:0xfffffffc}}},
    {x:{raw:0}},
},
};

pci_header_t pci_vga_header = { x: { fields: {
    vendor_id: 0x800e, 
    device_id: 0xbeef,
    command: vga_command,
    status: vga_status,
    revision_id: 0,
    programming_interface: 0x80,
    sub_class_code: pci_header_t::display,
    base_class_code: pci_header_t::vga,
    cache_line_size: 0,
    latency_timer: 0,
    header_type: 0,
    bist: vga_bist,
    base_addr_registers: {
	{x:{memory:{mem_request:0, type:0, prefetchable:1, address:0xe000000}}},
	{x:{raw:0}},
	{x:{raw:0}},
	{x:{raw:0}},
	{x:{raw:0}},
	{x:{raw:0}},
    },
    cardbus_cis_pointer: 0,
    subsys_vendor_id: DIGIT_STRING16('l','4'),
    subsys_id: DIGIT_STRING16('k','a'),
    rom_base_addr: 0,
    rom_size: 0,
    reserved2: 0,
    interrupt_line: 0xff,
    interrupt_pin: 1,
    min_gnt: 0,
    max_lat: 0,
}},
base_addr_requests: {
    {x:{memory:{mem_request:0, type:0, prefetchable:1, address:0xff80000}}},
    {x:{raw:0}},
    {x:{raw:0}},
    {x:{raw:0}},
    {x:{raw:0}},
    {x:{raw:0}},
},
};

static pci_header_t *config_header = NULL;
static pci_device_t *config_device = NULL;
static const char *host_bus_name = "HOST";
static const char *isa_bus_name = "ISA";
static const char *vga_device_name = "VGA";
const char *name = NULL;

void pci_config_address_read( u32_t & value, u32_t bit_width )
{
    value = config_addr.x.raw;
}

void pci_config_address_write( u32_t value, u32_t bit_width )
{
    config_header = NULL;
    name = NULL;
    config_addr.x.raw = value;
    
    dprintf(debug_pci, "pci config address write bus %d dev %d func %d\n",
        config_addr.x.fields.bus, config_addr.x.fields.reg,
        config_addr.x.fields.func);

    if( config_addr.x.fields.bus != 0 )
	return;
    
    if( config_addr.x.fields.dev == 0 ) 
    {
	config_header = &pci_host_bus_header;
	name = host_bus_name;
    }
    else if( config_addr.x.fields.dev == 1) 
    {
	if( config_addr.x.fields.func == 0 )
	{
	    config_header = &pci_isa_bridge_header;
	    name = isa_bus_name;
	    
	}
#if defined(CONFIG_DEVICE_I82371AB)
	if( config_addr.x.fields.func == 1 )
	{	    
	    config_header = i82371ab_t::get_device(0)->get_pci_header();
	    config_device = i82371ab_t::get_device(0)->get_pci_device();
	    name = i82371ab_t::get_name();
	}
#endif


    }
    else if( config_addr.x.fields.dev == 2) 
    {
	    config_header = &pci_vga_header;
	    name = vga_device_name;
	    config_device = NULL;
    }
#if defined(CONFIG_DEVICE_E1000)
    else if( config_addr.x.fields.dev == 3 ) 
    {
	config_header = e1000_t::get_device(0)->get_pci_header();
	name = e1000_t::get_name();
	config_device = NULL;
    }
#endif
#if defined(CONFIG_DEVICE_DP83820)
    else if( config_addr.x.fields.dev == 4 ) 
    {
	config_header = dp83820_t::get_device(0)->get_pci_header();
	name = dp83820_t::get_name();
	config_device = NULL;
    }
#endif
}


void pci_config_data_read( u32_t & value, u32_t bit_width, u32_t offset )
{
#if defined(CONFIG_DEVICE_PCI_FORWARD)
    if( (config_addr.x.fields.bus == 0) && 
	    (config_addr.x.fields.dev == 3) &&
	    (config_addr.x.fields.func == 0) )
    {
	backend_pci_config_data_read( config_addr, value, bit_width, offset );
	return;
    }
#endif

    if( !config_addr.is_enabled())
	return;
    
    if( config_addr.x.fields.reg < 16 && config_header)
	value = config_header->read( config_addr.x.fields.reg, offset, bit_width );
    else if (config_device)
	value = config_device->read( config_addr.x.fields.reg-16, offset, bit_width );
    
    dprintf(debug_pci, "pci data read %sm reg %x ofs %x bit width %d val %x\n",
	    name, config_addr.x.fields.reg, offset, bit_width, value);
    
}

void pci_config_data_write( u32_t value, u32_t bit_width, u32_t offset )
{
#if defined(CONFIG_DEVICE_PCI_FORWARD)
    if( (config_addr.x.fields.bus == 0) && 
	    (config_addr.x.fields.dev == 3) &&
	    (config_addr.x.fields.func == 0) )
    {
	backend_pci_config_data_write( config_addr, value, bit_width, offset );
	return;
    }
#endif

    if( !config_addr.is_enabled() || !config_header )
	return;
    
    dprintf(debug_pci, "pci data write %s reg %x ofs %x bit width %d val %x\n",
		name, config_addr.x.fields.reg, offset, bit_width, value);

    if( pci_header_t::is_base_addr_reg(config_addr.x.fields.reg) 
	    && (value == ~(u32_t)0) )
    {
	config_header->set_base_addr_request( config_addr.x.fields.reg );
	return;
    }
    if( config_addr.x.fields.reg < 16 && config_header)
	config_header->write( config_addr.x.fields.reg, offset, bit_width, value );
    else if (config_device)
	config_device->write( config_addr.x.fields.reg-16, offset, bit_width, value );
}


void pci_portio(u16_t port, u32_t & value, u32_t bit_width, bool read)
{
    
    switch( port ) 
    {
    case 0xcf8: 
	if (read)
	    pci_config_address_read( value, bit_width ); 
	else
	    pci_config_address_write( value, bit_width ); 
	break;
    case 0xcfc: 
	if (read)
	    pci_config_data_read( value, bit_width, 0 ); 
	else
	    pci_config_data_write( value, bit_width, 0 ); 
	break;
    case 0xcfd: 
	if (read)
	    pci_config_data_read( value, bit_width, 8 ); 
	else
	    pci_config_data_write( value, bit_width, 8 ); 
	break;
    case 0xcfe: 
	if (read)
	    pci_config_data_read( value, bit_width, 16 ); 
	else
	    pci_config_data_write( value, bit_width, 16 ); 
	break;
    case 0xcff: 
	if (read)
	    pci_config_data_read( value, bit_width, 24 ); 
	else
	    pci_config_data_write( value, bit_width, 24 ); 
	break;
    default:
	dprintf(debug_portio_unhandled, "unhandled pci portio %c port %x val %d width %d\n",
		(read ? 'r' : 'w'), port, value, bit_width);

    }
}
