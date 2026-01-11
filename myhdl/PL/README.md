# PL (Programmable Logic) Design

This directory contains the Vivado project and MyHDL source code for creating a custom FPGA IP core using the Python-based MyHDL framework. The design implements an interrupt generator IP that can be configured for periodic (timer-based) or software-triggered interrupts.

## Overview

The PL design demonstrates how to:
- Create custom FPGA IP using MyHDL (Python-based hardware description language)
- Convert MyHDL code to Verilog for Vivado integration
- Integrate custom IP into a Vivado block design
- Connect custom IP to the Zynq UltraScale+ PS via AXI4-Lite interface
- Handle interrupts from PL to PS using AXI Interrupt Controller

The interrupt generator IP provides:
- Two independent interrupt outputs
- Configurable periodic interrupt generation (timer-based)
- Software-triggered interrupt capability
- AXI4-Lite register interface for configuration and control

## Directory Structure

```
PL/
├── MyHDL/                    # MyHDL source code
│   └── src/
│       ├── interrupt_gen/           # Interrupt generator core logic
│       │   └── interrupt_gen.py
│       ├── interrupt_generator_ip/  # Top-level IP wrapper
│       │   └── interrupt_generator_ip.py
│       ├── interfaces/              # AXI interface definitions
│       │   ├── axi_lite.py          # AXI4-Lite interface
│       │   └── axi_local.py         # Local AXI bus interface
│       └── axi_support/             # AXI support functions
│           └── axi_support.py       # AXI connection logic
└── interrupt_demo/          # Vivado project directory
    └── interrupt_demo/
        ├── interrupt_demo.xpr        # Vivado project file
        ├── interrupt_demo.srcs/      # Source files
        │   ├── sources_1/
        │   │   ├── bd/               # Block design
        │   │   │   └── interrupt_demo/
        │   │   │       └── interrupt_demo.bd
        │   │   └── hdl/              # Verilog files
        │   │       └── interrupt_generator_ip.v  # Generated from MyHDL
        │   └── constrs_1/            # Constraints (if any)
        └── interrupt_demo.gen/       # Generated files
```

## MyHDL IP Core

### Interrupt Generator (`interrupt_gen.py`)

The core interrupt generator block provides:

**Features:**
- Two independent interrupt outputs (`interrupt1_out`, `interrupt2_out`)
- Periodic interrupt generation using configurable period counters
- Software-triggered interrupts via trigger register
- Interrupt status register (ISR) for monitoring interrupt state
- Interrupt enable register (IER) for controlling interrupt outputs
- AXI4-Lite slave interface for register access

**Registers:**
- `PERIOD1` (offset 0x00): Period counter value for interrupt1 (32-bit)
  - Set to 0 to disable periodic interrupts
  - Non-zero value sets the number of clock cycles between interrupts
- `PERIOD2` (offset 0x04): Period counter value for interrupt2 (32-bit)
  - Same behavior as PERIOD1
- `ISR` (offset 0x08): Interrupt Status Register (8-bit)
  - Bit 0: Interrupt1 status (read to check, write 1 to clear)
  - Bit 1: Interrupt2 status (read to check, write 1 to clear)
- `IER` (offset 0x0C): Interrupt Enable Register (8-bit)
  - Bit 0: Enable interrupt1 output
  - Bit 1: Enable interrupt2 output
- `TRIGGER` (offset 0x10): Trigger Register (8-bit, write-only)
  - Bit 0: Trigger interrupt1 (write 1 to trigger)
  - Bit 1: Trigger interrupt2 (write 1 to trigger)

### IP Wrapper (`interrupt_generator_ip.py`)

The top-level IP wrapper:
- Converts AXI4-Lite interface signals to internal AxiLocal format
- Instantiates the interrupt generator core
- Provides standard AXI4-Lite slave interface for Vivado integration
- Generates Verilog output using MyHDL's conversion tool

## Building the MyHDL IP

### Prerequisites

- Python 3.12
- MyHDL package
- Vivado 2025.2 (or compatible version)

### Build Process

The build process converts MyHDL Python code to Verilog:

```bash
cd myhdl
make all
```

This will:
1. Create a Python virtual environment (`venv/`)
2. Install MyHDL package
3. Convert `interrupt_generator_ip.py` to Verilog
4. Output: `build/interrupt_generator_ip.v`

**Manual build steps:**

```bash
# Create virtual environment
python3.12 -m venv venv
source venv/bin/activate

# Install MyHDL
pip install --upgrade pip
pip install myhdl

# Convert to Verilog
PYTHONPATH=. python PL/MyHDL/src/interrupt_generator_ip/interrupt_generator_ip.py
```

The generated Verilog file can then be added to a Vivado project as a custom IP.

## Block Design Components

The block design (`interrupt_demo.bd`) consists of:

1. **Zynq UltraScale+ PS** (`zynq_ultra_ps_e_0`)
   - Processing System configuration
   - Provides AXI interfaces for PL connectivity
   - Clock and reset generation
   - Interrupt input from PL

2. **Interrupt Generator IP** (`interrupt_generator_0`)
   - Custom MyHDL-generated IP core
   - AXI4-Lite slave interface
   - Two interrupt outputs
   - Base address: `0xB0000000`

3. **AXI Smart Memory Controller** (`axi_smc_0`)
   - AXI interconnect between PS and custom IP
   - Handles address translation and protocol conversion

4. **AXI Interrupt Controller** (`axi_intc_0`)
   - Consolidates PL interrupts
   - Connects to PS interrupt input
   - Base address: `0xB0010000`

5. **Interrupt Concatenator** (`ilconcat_0`)
   - Combines multiple interrupt sources into a single bus
   - Connects interrupt generator outputs to AXI Interrupt Controller

6. **Processor System Reset** (`rst_ps8_0_99M_0`)
   - Reset controller for PL logic
   - Synchronized to PS clock

## AXI Address Map

- **Interrupt Generator Base**: `0xB0000000`
  - Offset `0x00`: PERIOD1 register
  - Offset `0x04`: PERIOD2 register
  - Offset `0x08`: ISR (Interrupt Status Register)
  - Offset `0x0C`: IER (Interrupt Enable Register)
  - Offset `0x10`: TRIGGER register

- **AXI Interrupt Controller Base**: `0xB0010000`
  - Standard AXI Interrupt Controller registers
  - Used to manage and acknowledge interrupts from PL

## Building the Vivado Design

### Using Vivado GUI

1. Open `interrupt_demo/interrupt_demo.xpr` in Vivado
2. Ensure the generated Verilog file (`interrupt_generator_ip.v`) is in the project
3. Run synthesis, implementation, and bitstream generation
4. Export hardware platform (File → Export → Export Hardware)

### Adding MyHDL IP to Vivado

1. Generate Verilog from MyHDL (see "Building the MyHDL IP" above)
2. In Vivado, add the Verilog file as a design source
3. Create a custom IP package or use it directly in block design
4. Configure the IP with AXI4-Lite interface
5. Connect to AXI Smart Memory Controller

## Hardware Platform Export

The generated XSA file contains:
- Bitstream for PL configuration
- Hardware description (HWH) for software tools
- Address map for AXI peripherals
- Clock and reset information
- Interrupt routing information

This XSA file is used by:
- PYNQ for Python overlay loading
- System device tree generation
- Software application development

## Interrupt Operation

### Periodic Interrupts

1. Set period register to desired clock cycle count (e.g., `PERIOD2 = 100000000` for 1 second at 100MHz)
2. Enable interrupt in IER register (set bit 1 for interrupt2)
3. Interrupt fires automatically when period counter expires
4. ISR bit is set when interrupt occurs
5. Clear ISR bit by writing 1 to that bit position

### Software-Triggered Interrupts

1. Set period register to 0 to disable periodic mode
2. Enable interrupt in IER register
3. Write 1 to TRIGGER register bit to generate interrupt
4. ISR bit is set immediately
5. Clear ISR bit by writing 1 to that bit position

## Integration with Software

### APU Access

The APU can access the interrupt generator through:
- PYNQ overlay system (Python interface) - See `APU/pynq_interrupt.ipynb`
- Direct memory mapping (requires `/dev/mem` access)
- Linux device drivers

The PYNQ notebook demonstrates:
- Loading the FPGA overlay
- Configuring interrupt periods
- Enabling/disabling interrupts
- Handling interrupts asynchronously
- Software triggering of interrupts

## Design Notes

- The MyHDL code uses a modular design with separate interface definitions
- AXI4-Lite interface is converted to a simplified AxiLocal bus internally
- The design supports daisy-chaining multiple AXI devices (though only one is used here)
- Interrupts are level-sensitive and must be cleared by software
- The period counters are 32-bit, allowing very long periods
- Clock frequency is 100MHz (configurable in PS settings)

## MyHDL Framework Benefits

- **Python-based**: Leverage Python's expressiveness for hardware design
- **Simulation**: Test hardware logic in Python before synthesis
- **Code reuse**: Modular design with reusable interface definitions
- **Type safety**: MyHDL provides type checking for hardware signals
- **Verification**: Easy to write testbenches in Python

## See Also

- [Main Project README](../README.md) - Overall project documentation
- [APU README](../APU/README.md) - APU applications that use this design
- [MyHDL Documentation](http://www.myhdl.org/) - Official MyHDL framework documentation
