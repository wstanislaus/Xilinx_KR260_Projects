# MyHDL Custom IP Project

This project demonstrates how to create a custom FPGA IP core using the Python-based MyHDL framework and integrate it into a Xilinx Vivado design for the KR260 board. The design implements an interrupt generator IP that can be configured for periodic (timer-based) or software-triggered interrupts.

## Overview

The goal of this project is to add a simple custom FPGA IP using the Python-based MyHDL framework. The project showcases:

- **MyHDL Framework**: Using Python to describe hardware and generate Verilog
- **Custom IP Development**: Creating a reusable interrupt generator IP core
- **Vivado Integration**: Integrating the generated Verilog into a Vivado block design
- **AXI4-Lite Interface**: Connecting custom IP to the Zynq UltraScale+ PS
- **Interrupt Handling**: Demonstrating PL-to-PS interrupt communication
- **PYNQ Integration**: Using Python to interact with the custom IP

## Project Structure

```
myhdl/
├── README.md                    # This file
├── Makefile                     # Build automation for MyHDL conversion
├── PL/                          # Programmable Logic (FPGA) design
│   ├── README.md                # PL design documentation
│   ├── MyHDL/                   # MyHDL source code
│   │   └── src/
│   │       ├── interrupt_gen/           # Interrupt generator core
│   │       ├── interrupt_generator_ip/  # Top-level IP wrapper
│   │       ├── interfaces/              # AXI interface definitions
│   │       └── axi_support/             # AXI support functions
│   └── interrupt_demo/          # Vivado project
│       └── interrupt_demo/
│           ├── interrupt_demo.xpr      # Vivado project file
│           └── interrupt_demo.srcs/    # Source files
├── APU/                         # Application Processing Unit code
│   ├── README.md                # APU documentation
│   └── pynq_interrupt.ipynb    # Jupyter notebook demo
├── build/                       # Generated Verilog files
│   └── interrupt_generator_ip.v
└── venv/                        # Python virtual environment
```

## Features

### Interrupt Generator IP

The custom IP provides:

- **Two Independent Interrupt Outputs**: Separate interrupt1 and interrupt2
- **Periodic Interrupt Mode**: Configurable timer-based interrupts
- **Software-Triggered Mode**: On-demand interrupt generation
- **Register-Based Control**: AXI4-Lite interface for configuration
- **Status Monitoring**: Interrupt status and enable registers

### MyHDL Framework Benefits

- **Python Expressiveness**: Write hardware in familiar Python syntax
- **Modular Design**: Reusable interface definitions and components
- **Simulation Support**: Test hardware logic in Python before synthesis
- **Type Safety**: MyHDL provides hardware-aware type checking
- **Code Generation**: Automatic Verilog generation for Vivado

## Quick Start

### 1. Build MyHDL IP

```bash
cd myhdl
make all
```

This will:
- Create Python virtual environment
- Install MyHDL package
- Convert MyHDL code to Verilog
- Output: `build/interrupt_generator_ip.v`

### 2. Open Vivado Project

```bash
cd PL/interrupt_demo/interrupt_demo
vivado interrupt_demo.xpr
```

The Vivado project already includes the generated Verilog file. You can:
- Run synthesis and implementation
- Generate bitstream
- Export hardware platform (XSA file)

### 3. Run APU Demo

On the KR260 board with PYNQ:

```bash
# Start Jupyter notebook server
jupyter notebook

# Open APU/pynq_interrupt.ipynb
# Execute cells to demonstrate interrupt functionality
```

## Interrupt Generator Registers

| Offset | Name | Width | Description |
|--------|------|-------|-------------|
| `0x00` | `period1` | 32-bit | Period counter for interrupt1 (0 = disabled) |
| `0x04` | `period2` | 32-bit | Period counter for interrupt2 |
| `0x08` | `isr` | 8-bit | Interrupt Status Register |
| `0x0C` | `ier` | 8-bit | Interrupt Enable Register |
| `0x10` | `trigger` | 8-bit | Trigger Register (write-only) |

**Base Address**: `0xB0000000`

## Block Design

The Vivado block design includes:

1. **Zynq UltraScale+ PS**: Processing system
2. **Interrupt Generator IP**: Custom MyHDL-generated IP
3. **AXI Smart Memory Controller**: AXI interconnect
4. **AXI Interrupt Controller**: Interrupt management
5. **Interrupt Concatenator**: Combines interrupt sources
6. **Processor System Reset**: Reset controller

## Build System

### Makefile Targets

- `make venv`: Create Python 3.12 virtual environment
- `make install`: Install MyHDL package
- `make build`: Convert MyHDL to Verilog
- `make clean`: Remove build artifacts and virtual environment
- `make all`: Run venv, install, and build
- `make help`: Show available targets

### Manual Build

```bash
# Create and activate virtual environment
python3.12 -m venv venv
source venv/bin/activate

# Install dependencies
pip install --upgrade pip
pip install myhdl

# Convert to Verilog
PYTHONPATH=. python PL/MyHDL/src/interrupt_generator_ip/interrupt_generator_ip.py
```

## Usage Examples

### Periodic Interrupts

Configure interrupt2 to fire every 1 second (at 100MHz clock):

```python
intr.write(period2, 100000000)  # 100M clock cycles
intr.write(ier, 2)              # Enable interrupt2
```

### Software-Triggered Interrupts

Configure interrupt1 for on-demand triggering:

```python
intr.write(period1, 0)          # Disable periodic mode
intr.write(ier, 1)              # Enable interrupt1
intr.write(trigger, 1)          # Trigger interrupt1
```

## Documentation

- **[PL README](PL/README.md)**: Detailed PL design documentation
  - MyHDL IP architecture
  - Block design components
  - Build instructions
  - AXI address map

- **[APU README](APU/README.md)**: APU application documentation
  - PYNQ notebook usage
  - Register access examples
  - Interrupt handling

## Dependencies

### Build Dependencies

- Python 3.12
- MyHDL package
- Make (for build automation)

### Runtime Dependencies

- Vivado 2025.2 (or compatible)
- Xilinx KR260 board
- PYNQ Linux image (for APU demo)
- Jupyter notebook (for APU demo)

## Design Workflow

1. **Develop in MyHDL**: Write hardware description in Python
2. **Simulate**: Test logic in Python (optional)
3. **Convert to Verilog**: Use MyHDL's conversion tool
4. **Integrate in Vivado**: Add Verilog to block design
5. **Synthesize & Implement**: Generate bitstream
6. **Test on Hardware**: Use PYNQ to interact with IP

## Key Components

### MyHDL Source Files

- `interrupt_gen.py`: Core interrupt generator logic
- `interrupt_generator_ip.py`: Top-level IP wrapper with AXI4-Lite interface
- `axi_lite.py`: AXI4-Lite interface definition
- `axi_local.py`: Simplified local AXI bus interface
- `axi_support.py`: AXI connection and routing logic

### Generated Files

- `interrupt_generator_ip.v`: Verilog output from MyHDL conversion
- `interrupt_demo.xsa`: Exported hardware platform
- `interrupt_demo.bit`: FPGA bitstream

## Interrupt Operation

### Periodic Mode

1. Set period register to desired clock cycle count
2. Enable interrupt in IER register
3. Interrupt fires automatically when counter expires
4. ISR bit is set when interrupt occurs
5. Software clears ISR bit to acknowledge

### Software-Triggered Mode

1. Set period register to 0 (disables periodic mode)
2. Enable interrupt in IER register
3. Write 1 to TRIGGER register bit
4. Interrupt fires immediately
5. ISR bit is set
6. Software clears ISR bit to acknowledge

## See Also

- [MyHDL Official Documentation](http://www.myhdl.org/)
- [PYNQ Documentation](https://pynq.readthedocs.io/)
- [Xilinx Vivado Documentation](https://www.xilinx.com/support/documentation/sw_manuals/xilinx2025_2/ug910-vivado-getting-started.pdf)
- [KR260 Board Documentation](https://www.xilinx.com/products/boards-and-kits/kria.html)

## Notes

- The MyHDL code uses a modular design for reusability
- AXI4-Lite interface provides standard connectivity to PS
- Interrupts are level-sensitive and require software acknowledgment
- Period counters are 32-bit, allowing very long periods
- Clock frequency is configurable (default: 100MHz)
