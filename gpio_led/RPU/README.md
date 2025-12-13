# RPU (Real-time Processing Unit) Firmware

This directory contains the FreeRTOS-based firmware running on the RPU (ARM Cortex-R5). The RPU firmware receives commands from the APU and controls the LEDs in real-time.

## Overview

The RPU firmware implements a real-time LED control system using FreeRTOS with the following components:

- **Tx Task**: Generates LED blink patterns based on current mode
- **Rx Task**: Writes LED values to AXI GPIO hardware
- **Timer Callback**: Manages automatic mode rotation (when not overridden by APU)
- **IPI Handler**: Receives commands from APU via Inter-Processor Interrupts

## Directory Structure

```
RPU/
├── gpio_app/              # Main application project
│   ├── src/
│   │   ├── main.c         # FreeRTOS application source
│   │   ├── CMakeLists.txt # Build configuration
│   │   └── lscript.ld     # Linker script
│   └── _ide/              # IDE configuration files
└── platform/              # Vitis platform definition
    ├── hw/                # Hardware platform files
    │   ├── gpio_led_wrapper.xsa  # Hardware platform
    │   ├── gpio_led_wrapper.bit  # PL bitstream
    │   └── sdt/           # System device tree files
    └── psu_cortexr5_0/    # RPU BSP (Board Support Package)
        └── freertos_psu_cortexr5_0/
            └── bsp/       # FreeRTOS BSP files
```

## Application Architecture

### FreeRTOS Tasks

#### Tx Task (`prvTxTask`)
- Generates LED patterns based on `current_blink_mode`
- Sends LED values to a FreeRTOS queue
- Adjusts timing based on mode:
  - **SLOW**: 1000ms delay
  - **FAST**: 200ms delay
  - **RANDOM**: 200ms delay with random values

#### Rx Task (`prvRxTask`)
- Receives LED values from the queue
- Writes directly to AXI GPIO hardware at `0x80000000`
- Higher priority than Tx task for responsive LED updates

### Timer Callback (`vTimerCallback`)
- Executes every 10 seconds
- Rotates modes: SLOW → FAST → RANDOM → SLOW
- Respects APU override (doesn't rotate when `apu_override_active` is set)
- Checks legacy shared memory for mode commands

### IPI Interrupt Handler (`IPI_Handler`)
- Handles Inter-Processor Interrupts from APU
- Reads command from shared memory at `0xFF990000`
- Updates `current_blink_mode` and sets `apu_override_active`
- Writes acknowledgment back to shared memory
- Handles cache coherency for shared memory access

## LED Blink Modes

| Mode | Description | Timing |
|------|-------------|--------|
| 0 (SLOW) | Alternating pattern | 1000ms toggle |
| 1 (FAST) | Alternating pattern | 200ms toggle |
| 2 (RANDOM) | Random LED values | 200ms update |
| 3+ | Release control | Timer resumes rotation |

## Memory Configuration

The firmware configures the MPU (Memory Protection Unit) for:

1. **AXI GPIO Access** (`0x80000000`)
   - Strongly-ordered, shared memory
   - Required for PL peripheral access

2. **Shared Memory (IPI)** (`0xFF990000`)
   - Normal, shared, non-cacheable
   - Used for APU-RPU communication

3. **Legacy Shared Memory** (`0x40000000`)
   - Normal, shared, non-cacheable
   - Simple mode control interface

4. **IPI Registers** (`0xFF310000`)
   - Strongly-ordered, shared memory
   - For IPI interrupt handling

## Building the Firmware

### Using Vitis IDE

1. Open Vitis workspace
2. Import the platform from `platform/hw/gpio_led_wrapper.xsa`
3. Create new application project targeting `psu_cortexr5_0`
4. Select FreeRTOS as the OS
5. Add `gpio_app/src/main.c` to the project
6. Build the project

### Using Command Line

```bash
cd gpio_app
# Use Vitis command-line tools or CMake
# See src/CMakeLists.txt for build configuration
```

**Output:** `gpio_app.elf` - Executable firmware for RPU

## Loading the Firmware

### Using fw_loader (Recommended)
```bash
sudo ./fw_loader gpio_app.elf
```

### Manual Loading
```bash
# Copy firmware to /lib/firmware/
sudo cp gpio_app.elf /lib/firmware/

# Load firmware
echo gpio_app.elf > /sys/class/remoteproc/remoteproc0/firmware

# Start RPU
echo start > /sys/class/remoteproc/remoteproc0/state
```

## Communication Protocol

### IPI Communication Flow

1. **APU Side**:
   - Writes mode to `SHARED_MEM_ADDR + SHM_CMD_OFFSET`
   - Triggers IPI interrupt
   - Polls `SHARED_MEM_ADDR + SHM_ACK_OFFSET` for acknowledgment

2. **RPU Side**:
   - Receives IPI interrupt
   - Reads command from shared memory
   - Updates LED mode
   - Writes acknowledgment: `SHM_ACK_MAGIC | (mode & 0xFF)`

### Shared Memory Layout

```
Offset 0x00: Command/Mode (APU writes, RPU reads)
Offset 0x04: Acknowledgment (RPU writes, APU reads)
```

### Acknowledgment Format
- Magic value: `0xDEADBEEF`
- Format: `SHM_ACK_MAGIC | (mode & 0xFF)`
- Example: Mode 1 → `0xDEADBEEF | 0x01` = `0xDEADBEF0`

## FreeRTOS Configuration

The firmware uses:
- Static allocation for tasks and queues
- Minimal stack sizes (configurable)
- Idle priority for both tasks
- Software timers for mode rotation
- Interrupt-driven IPI handling

### Serial Output
The firmware uses `xil_printf` for debug output via UART. Connect to the RPU UART to see:
- Task startup messages
- Mode change notifications
- IPI interrupt events
- Error messages


## Integration

The RPU firmware integrates with:
- **PL Design**: Accesses AXI GPIO at `0x80000000`
- **APU Applications**: Receives commands via IPI/shared memory
- **APU Kernel Module**: Works with `/sys/kernel/rpu_ipi/` interface

## See Also

- [Main Project README](../README.md) - Overall project documentation
- [APU README](../APU/README.md) - APU applications that communicate with this firmware
- [PL README](../PL/README.md) - PL design that provides the hardware interface
