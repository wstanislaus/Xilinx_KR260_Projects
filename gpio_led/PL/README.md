# PL (Programmable Logic) Design

This directory contains the Vivado project for the Programmable Logic (FPGA) portion of the GPIO LED demo. The PL design provides the hardware interface between the processing system and the physical LEDs.

## Overview

The PL design implements an AXI GPIO IP core that:
- Connects to the Zynq UltraScale+ PS via AXI4-Lite interface
- Provides a 2-bit GPIO output for controlling two LEDs
- Is accessible from both APU and RPU processors
- Uses the Smart Memory Controller (SMC) for AXI interconnect

## Directory Structure

```
PL/
├── CMakeLists.txt           # CMake build configuration
├── Makefile                 # Alternative build script
└── gpio_led/                # Vivado project directory
    ├── gpio_led.xpr         # Vivado project file
    ├── gpio_led.xsa         # Exported hardware platform
    ├── gpio_led.srcs/       # Source files
    │   ├── sources_1/       # Block design and IP cores
    │   │   └── bd/
    │   │       └── gpio_led/
    │   │           ├── gpio_led.bd  # Block design
    │   │           └── ip/          # IP core configurations
    │   └── constrs_1/       # Constraints
    │       └── new/
    │           └── gpio_led.xdc    # Pin constraints
    └── gpio_led.gen/        # Generated files
```

## Block Design Components

The block design (`gpio_led.bd`) consists of:

1. **Zynq UltraScale+ PS** (`zynq_ultra_ps_e_0`)
   - Processing System configuration
   - Provides AXI interfaces for PL connectivity
   - Clock and reset generation

2. **AXI GPIO** (`axi_gpio_0`)
   - 2-bit output channel
   - Base address: `0x80000000`
   - Controls LED0 and LED1

3. **AXI Smart Memory Controller** (`axi_smc_0`)
   - AXI interconnect between PS and AXI GPIO
   - Handles address translation and protocol conversion

4. **Processor System Reset** (`rst_ps8_0_99M_0`)
   - Reset controller for PL logic
   - Synchronized to PS clock

## Pin Constraints

The design constrains two GPIO pins to physical LED locations on the KR260:

- **LED0 (DS8)**: Pin E8, LVCMOS18
- **LED1 (DS7)**: Pin F8, LVCMOS18

See `gpio_led.srcs/constrs_1/new/gpio_led.xdc` for detailed pin assignments.

## Building the Design

### Using CMake (Recommended)

```bash
cd PL
mkdir build && cd build
cmake ..
make
```

This will:
1. Run synthesis
2. Run implementation
3. Generate bitstream
4. Export XSA (Xilinx Support Archive) file
5. Extract HWH (Hardware Handoff) file

**Output files:**
- `output/gpio_led.bit` - FPGA bitstream
- `output/gpio_led_wrapper.xsa` - Hardware platform
- `output/gpio_led.hwh` - Hardware description

### Using Makefile

```bash
cd PL
make
```

### Using Vivado GUI

1. Open `gpio_led/gpio_led.xpr` in Vivado
2. Run synthesis, implementation, and bitstream generation
3. Export hardware platform (File → Export → Export Hardware)

## Hardware Platform Export

The generated XSA file contains:
- Bitstream for PL configuration
- Hardware description (HWH) for software tools
- Address map for AXI peripherals
- Clock and reset information

This XSA file is used by:
- Vitis for RPU application development
- PYNQ for Python overlay loading
- System device tree generation

## AXI Address Map

- **AXI GPIO Base**: `0x80000000`
  - Offset `0x00`: GPIO Data Register
  - Offset `0x04`: GPIO Tri-State Register (direction)

## Integration with Software

### RPU Access
The RPU firmware (`RPU/gpio_app/src/main.c`) directly accesses the AXI GPIO at `0x80000000` after configuring the MPU (Memory Protection Unit) for PL access.

### APU Access
The APU can access the AXI GPIO through:
- Direct memory mapping (requires `/dev/mem` access)
- PYNQ overlay system (Python interface)

## Design Notes

- The AXI GPIO is configured as output-only (no input channel)
- The design uses a 99MHz clock from the PS
- Cache coherency is handled by the MPU configuration in software
- The block design follows Xilinx recommended practices for Zynq UltraScale+ designs


## See Also

- [Main Project README](../README.md) - Overall project documentation
- [RPU README](../RPU/README.md) - RPU firmware that uses this design
- [APU README](../APU/README.md) - APU applications that can access this design
