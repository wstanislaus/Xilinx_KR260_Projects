# RPU IPI Kernel Module

This kernel module provides a sysfs interface to communicate with the RPU (Real-time Processing Unit) via shared memory and IPI (Inter-Processor Interrupt) interrupts.

## Features

- **Sysfs Interface**: Exposes two files in `/sys/kernel/rpu_ipi/`
  - `write`: Write a mode value (0, 1, or 2) to send to RPU
  - `status`: Read the last sent message and acknowledgment status

- **Message Format**: Status returns `"mode,ACK"` or `"mode,NOACK"` (e.g., `"1,ACK"`)

- **Thread-Safe**: Uses mutex to protect concurrent access

## Building

### Prerequisites

- **Cross-compilation**: Yocto SDK toolchain installed and kernel source prepared

### Installing the SDK Toolchain

If you haven't installed the SDK toolchain yet:

```bash
# 1. Build the SDK (if not already built)
git clone https://github.com/wstanislaus/Xilinx_KR260_Yocto.git
cd Xilinx_KR260_Yocto
./setup.sh
make build-sdk

# 2. Install the SDK
# The SDK installer will be at:
#   build/tmp-glibc/deploy/sdk/kria-toolchain-kr260-sdk.sh
./build/tmp-glibc/deploy/sdk/kria-toolchain-kr260-sdk.sh -y -d /tools/kr260_yocto_toolchain

```

### Cross-Compilation for KR260 (ARM64)

#### Step 1: Prepare Kernel Source

Before building kernel modules, you need to prepare the kernel source tree:

```bash
# 1. Navigate to the kernel source directory in the SDK
cd /tools/kr260_yocto_toolchain/sysroots/cortexa72-cortexa53-oe-linux/usr/lib/modules/6.12.40-xilinx/build

# 2. Source the SDK environment
source /tools/kr260_yocto_toolchain/environment-setup-cortexa72-cortexa53-oe-linux

# 3. Prepare the kernel source for module building
make ARCH=arm64 modules_prepare
```

**Note:** This step only needs to be done once after installing the SDK, or if the kernel source is updated.

#### Step 2: Build the Module

After preparing the kernel source, build the module:

```bash
# 1. Return to the kernel module directory
cd ~/git/Demo2/kernel_module

# 2. Source the SDK environment (if not already sourced)
source /tools/kr260_yocto_toolchain/environment-setup-cortexa72-cortexa53-oe-linux

# 3. Build the module
# KDIR will automatically use the SDK kernel source path
make

# Copy the rpu_ipi.ko to the target machine
```

## Loading and Unloading

### Load Module

```bash
sudo insmod rpu_ipi.ko
```

### Unload Module

```bash
sudo rmmod rpu_ipi
```

### Check Module Status

```bash
# Check if module is loaded
lsmod | grep rpu_ipi

# View module information
modinfo rpu_ipi.ko

# View kernel messages
dmesg | tail -20
```

## Usage

### Send Message to RPU

```bash
# Send mode 0 (SLOW)
echo 0 > /sys/kernel/rpu_ipi/write

# Send mode 1 (FAST)
echo 1 > /sys/kernel/rpu_ipi/write

# Send mode 2 (RANDOM)
echo 2 > /sys/kernel/rpu_ipi/write
```

### Check Acknowledgment Status

```bash
cat /sys/kernel/rpu_ipi/status
```

Output examples:
- `1,ACK` - Mode 1 was sent and acknowledged by RPU
- `2,NOACK` - Mode 2 was sent but not acknowledged (timeout)
- `NONE,NONE` - No message has been sent yet

## Memory Map

The module accesses the following memory regions:

- **Shared Memory**: `0xFF990000` (4KB)
  - Offset `0x00`: Command/Mode (APU writes, RPU reads)
  - Offset `0x04`: Acknowledgment (RPU writes, APU reads)

- **IPI Registers**: `0xFF300000` (4KB)
  - Offset `0x00`: Trigger register
  - Offset `0x04`: Observation register

## Module Parameters

None. All configuration is hardcoded to match the [DTS overlay](https://github.com/wstanislaus/Xilinx_KR260_Yocto/blob/main/dts/kr260_overlay.dtso) configuration.


## Integration with RPU Firmware

This module is designed to work with the RPU firmware in `RPU/gpio_app/src/main.c`, which:

1. Receives IPI interrupts on Channel 1 (`0xFF310000`)
2. Reads commands from shared memory at `0xFF990000`
3. Writes acknowledgments back to shared memory
4. Processes LED blink mode changes

Ensure the RPU firmware is loaded and running before using this module.

