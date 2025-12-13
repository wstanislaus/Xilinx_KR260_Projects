# GPIO LED Demo Project

A comprehensive demonstration of heterogeneous processing on the Xilinx KR260, showcasing APU-RPU-PL communication for real-time LED control.

## Overview

This project demonstrates a complete system where:
- **APU** (Linux) sends LED blink mode commands to the RPU
- **RPU** (FreeRTOS) receives commands via IPI interrupts and controls LEDs in real-time
- **PL** (FPGA) provides the AXI GPIO hardware interface to physical LEDs

The system supports multiple control interfaces:
- Linux kernel module with sysfs interface
- User-space C++ applications
- Python/PYNQ notebooks
- Legacy shared memory communication

## Project Structure

```
gpio_led/
├── APU/          # Application Processing Unit (Linux) code
│   ├── apu_app/  # User-space applications
│   ├── kernel_module/  # Linux kernel module for IPI communication
│   └── python/   # PYNQ notebook for LED control
├── PL/           # Programmable Logic (FPGA) design
│   └── gpio_led/ # Vivado project with AXI GPIO block design
└── RPU/          # Real-time Processing Unit (FreeRTOS) firmware
    └── gpio_app/ # FreeRTOS application for LED control
```

## Features

### LED Blink Modes
- **Mode 0 (SLOW)**: 1 second toggle interval
- **Mode 1 (FAST)**: 200ms toggle interval
- **Mode 2 (RANDOM)**: Random LED patterns at 200ms intervals
- **Mode 3+**: Release control to RPU internal state machine

### Communication Methods
1. **IPI + Shared Memory** (Recommended): Uses Inter-Processor Interrupts with shared memory at `0xFF990000`
2. **Legacy Shared Memory**: Direct memory access at `0x40000000` (simpler but less robust)

## Hardware Requirements

- Xilinx KR260 Robotics Starter Kit
- Two LEDs connected to the PL (DS7 and DS8 on the board)

## Building the Project

### 1. Build PL (FPGA Bitstream)
```bash
cd PL
mkdir build && cd build
cmake ..
make
```
This generates `gpio_led.bit` and `gpio_led.hwh` files.

### 2. Build RPU Firmware
```bash
cd RPU/gpio_app
# Use Vitis IDE or command-line tools
# Generates gpio_app.elf
```

### 3. Build APU Applications
```bash
cd APU/apu_app
make
# Generates: apu_app, ipi_app, fw_loader
```

### 4. Build Kernel Module
```bash
cd APU/kernel_module
# Follow instructions in kernel_module/README.md
make
# Generates: rpu_ipi.ko
```

## Deployment

1. **Load PL Bitstream**:
   ```bash
   sudo ./fw_loader gpio_led.bit
   ```

2. **Load RPU Firmware**:
   ```bash
   sudo ./fw_loader gpio_app.elf
   ```

3. **Load Kernel Module** (optional):
   ```bash
   sudo insmod rpu_ipi.ko
   ```

## Usage Examples

### Using Kernel Module (Recommended)
```bash
# Send mode 0 (SLOW)
echo 0 > /sys/kernel/rpu_ipi/write

# Check status
cat /sys/kernel/rpu_ipi/status
```

### Using User-Space Applications
```bash
# IPI-based communication
./ipi_app 1  # FAST mode

# Legacy shared memory
./apu_app 2  # RANDOM mode
```

### Using Python/PYNQ
See `APU/python/led_blink_pynq.ipynb` for Jupyter notebook examples.

## Architecture Details

### Memory Map
- **Shared Memory (IPI)**: `0xFF990000` - Command/acknowledgment interface
- **Legacy Shared Memory**: `0x40000000` - Simple mode control
- **AXI GPIO Base**: `0x80000000` - Hardware LED interface
- **IPI APU Base**: `0xFF300000` - APU IPI trigger registers
- **IPI RPU Base**: `0xFF310000` - RPU IPI interrupt registers

### Communication Flow
1. APU writes mode value to shared memory
2. APU triggers IPI interrupt to RPU
3. RPU receives interrupt, reads command from shared memory
4. RPU updates LED blink mode
5. RPU writes acknowledgment back to shared memory
6. APU polls for acknowledgment (with timeout)

## See Also

- [APU/README.md](APU/README.md) - APU-specific documentation
- [PL/README.md](PL/README.md) - PL design documentation
- [RPU/README.md](RPU/README.md) - RPU firmware documentation
