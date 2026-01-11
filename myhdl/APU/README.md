# APU (Application Processing Unit) Code

This directory contains code running on the APU (ARM Cortex-A53) running Linux. The APU interacts with the custom MyHDL-generated interrupt generator IP in the PL (Programmable Logic) to demonstrate interrupt handling capabilities.

## Directory Structure

```
APU/
└── pynq_interrupt.ipynb     # Jupyter notebook for interrupt generator demo
```

## Overview

The APU code demonstrates how to:
- Load and interact with a custom FPGA IP created using MyHDL
- Configure interrupt generator registers via AXI4-Lite interface
- Handle interrupts from the PL using PYNQ's interrupt framework
- Use both periodic (timer-based) and software-triggered interrupts

## Applications

### `pynq_interrupt.ipynb` - Interrupt Generator Demo

A Jupyter notebook that provides a complete demonstration of the MyHDL-generated interrupt generator IP.

**Features:**

1. **Hardware Setup**
   - Loads the FPGA bitstream overlay (`interrupt_demo.bit`)
   - Configures the PL clock frequency (100MHz)
   - Initializes interrupt instances for both interrupt outputs

2. **Interrupt Configuration**
   - **Interrupt1**: Configured for software-triggered interrupts
     - Period register set to 0 (disables periodic mode)
     - Can be triggered on-demand by writing to trigger register
   - **Interrupt2**: Configured for periodic interrupts
     - Period register set to configurable value (e.g., 100M clock cycles)
     - Automatically fires at regular intervals

3. **Register Access**
   - Direct memory-mapped register access via PYNQ overlay
   - Read/write operations to configure interrupt behavior
   - Status monitoring via Interrupt Status Register (ISR)

4. **Interrupt Handling**
   - Async interrupt handler using PYNQ's `Interrupt` class
   - Automatic interrupt status clearing
   - Demonstrates proper interrupt acknowledgment flow

## Register Map

The interrupt generator uses the following memory-mapped registers (byte addresses relative to base `0xB0000000`):

| Offset | Name | Width | Description |
|--------|------|-------|-------------|
| `0x00` | `period1` | 32-bit | Period value for interrupt1. Set to 0 to disable periodic interrupts |
| `0x04` | `period2` | 32-bit | Period value for interrupt2. Number of clock cycles between interrupts |
| `0x08` | `isr` | 8-bit | Interrupt Status Register. Read to check status, write 1 to clear bits |
| `0x0C` | `ier` | 8-bit | Interrupt Enable Register. Controls which interrupts are enabled |
| `0x10` | `trigger` | 8-bit | Trigger Register (write-only). Write 1 to trigger interrupts |

**Register Bit Fields:**

- **ISR**:
  - Bit 0: Interrupt1 status
  - Bit 1: Interrupt2 status

- **IER**:
  - Bit 0: Enable interrupt1 output
  - Bit 1: Enable interrupt2 output

- **TRIGGER**:
  - Bit 0: Trigger interrupt1
  - Bit 1: Trigger interrupt2

## Usage Examples

### Software-Triggered Interrupts

```python
# Enable interrupt1
intr.write(ier, 1)  # Enable bit 0

# Trigger interrupt1
intr.write(trigger, 1)  # Write 1 to bit 0

# Wait for interrupt
await interrupt_handler(intr_inst1, 1)

# Clear interrupt status
intr.write(isr, 1)  # Write 1 to bit 0 to clear
```

### Periodic Interrupts

```python
# Set period to 100M clock cycles (1 second at 100MHz)
intr.write(period2, 100000000)

# Enable interrupt2
intr.write(ier, 2)  # Enable bit 1 (value 2 = 0b10)

# Wait for periodic interrupts
await interrupt_handler(intr_inst2, 2)
```

### Combined Configuration

```python
# Configure interrupt1 for software triggering
intr.write(period1, 0)  # Disable periodic mode

# Configure interrupt2 for periodic interrupts
intr.write(period2, 100000000)  # 1 second period

# Enable both interrupts
intr.write(ier, 3)  # Enable both (value 3 = 0b11)
```

## Running the Notebook

### Prerequisites

- Xilinx KR260 board with PYNQ Linux image
- Jupyter notebook server running on the board
- FPGA bitstream file (`interrupt_demo.bit`) in `/lib/firmware/`

### Steps

1. Open the notebook in Jupyter:
   ```bash
   # On the KR260 board
   jupyter notebook
   # Navigate to APU/pynq_interrupt.ipynb
   ```

2. Execute cells in order:
   - Cell 1: Import PYNQ modules and reset PL
   - Cell 2: Load overlay and configure clock
   - Cell 3-5: Initialize interrupt generator and interrupt objects
   - Cell 6: Define register offsets
   - Cell 7: Define interrupt handler function
   - Cell 8-10: Configure and test interrupts

3. Observe interrupt behavior:
   - Periodic interrupts fire automatically at configured intervals
   - Software-triggered interrupts fire immediately when triggered
   - Interrupt status is visible in ISR register

## Interrupt Handler Implementation

The notebook includes an async interrupt handler:

```python
async def interrupt_handler(interrupt_object, interrupt_bit):
    print("Handler task started. Waiting for interrupt...")
    
    # Wait for interrupt
    await interrupt_object.wait()
    
    print("Interrupt received!")
    
    # Clear the interrupt (write 1 to the bit to clear it)
    intr.write(isr, interrupt_bit)
```

This handler:
- Waits asynchronously for interrupt events
- Automatically clears interrupt status after handling
- Can be used with both periodic and software-triggered interrupts

## Memory Addresses

- **Interrupt Generator Base**: `0xB0000000` (4KB address space)
  - Accessible via PYNQ overlay's memory-mapped interface
  - All register offsets are relative to this base address

- **AXI Interrupt Controller Base**: `0xB0010000`
  - Managed by PYNQ interrupt framework
  - Used internally for interrupt routing to Linux kernel

## Dependencies

- PYNQ framework (Python productivity for Zynq)
- Python asyncio for async interrupt handling
- Jupyter notebook environment
- FPGA bitstream loaded in PL

## Integration with PL Design

The APU code interacts with the MyHDL-generated IP through:

1. **AXI4-Lite Register Access**
   - Direct memory mapping via PYNQ overlay
   - Read/write operations to configure IP behavior
   - Status monitoring via register reads

2. **Interrupt Handling**
   - PYNQ's `Interrupt` class provides async interrupt waiting
   - Interrupts are routed through AXI Interrupt Controller to Linux
   - Kernel-level interrupt handling with user-space notification

3. **Clock Configuration**
   - PL clock frequency can be configured via PYNQ
   - Affects periodic interrupt timing
   - Default: 100MHz

## Design Notes

- The interrupt generator IP is fully controlled from software
- Interrupts are level-sensitive and must be cleared by software
- Period registers use 32-bit values, allowing very long periods
- Multiple interrupts can be enabled simultaneously
- Software triggering provides immediate interrupt generation
- Periodic interrupts provide predictable timing for real-time applications

## See Also

- [PL README](../PL/README.md) - PL design and MyHDL IP implementation
- [Main Project README](../README.md) - Overall project documentation
- [PYNQ Documentation](https://pynq.readthedocs.io/) - PYNQ framework documentation
