# APU (Application Processing Unit) Code

This directory contains all code running on the APU (ARM Cortex-A53) running Linux. The APU acts as the control interface, sending commands to the RPU to control LED blink patterns.

## Directory Structure

```
APU/
├── apu_app/          # User-space C++ applications
│   ├── main.cpp      # Legacy shared memory control application
│   ├── ipi_app.cpp   # IPI-based communication application
│   ├── fw_loader.cpp # Firmware loader for PL and RPU
│   └── Makefile      # Build configuration
├── kernel_module/    # Linux kernel module
│   ├── rpu_ipi.c     # Kernel module source code
│   ├── Makefile      # Kernel module build configuration
│   └── README.md     # Detailed kernel module documentation
└── python/           # Python/PYNQ examples
    └── led_blink_pynq.ipynb  # Jupyter notebook for LED control
```

## Applications

### `apu_app/`

#### `main.cpp` - Legacy Shared Memory Control
A simple application that writes LED blink mode directly to shared memory at `0x40000000`. This is the simplest communication method but doesn't provide acknowledgment.

**Usage:**
```bash
./apu_app <mode>
# Modes: 0=SLOW, 1=FAST, 2=RANDOM, 3+=Release control
```

#### `ipi_app.cpp` - IPI-Based Communication
A more robust application that uses Inter-Processor Interrupts (IPI) and shared memory for reliable APU-to-RPU communication with acknowledgment.

**Features:**
- Writes command to shared memory at `0xFF990000`
- Triggers IPI interrupt to notify RPU
- Polls for acknowledgment with timeout
- Provides status feedback

**Usage:**
```bash
./ipi_app <mode>
# Modes: 0=SLOW, 1=FAST, 2=RANDOM, 3+=Release control
```

#### `fw_loader.cpp` - Firmware Loader
A utility application for loading firmware to both PL (FPGA) and RPU processors.

**Features:**
- Automatically detects `.bit`/`.bin` files for PL
- Automatically detects `.elf` files for RPU
- Converts `.bit` to `.bin` format if needed
- Handles RPU start/stop state management

**Usage:**
```bash
sudo ./fw_loader [gpio_led.bit] [gpio_app.elf]
# Defaults: gpio_led.bit, gpio_app.elf
```

### `kernel_module/`

A Linux kernel module that provides a sysfs interface (`/sys/kernel/rpu_ipi/`) for communicating with the RPU. This is the recommended method for production use as it:
- Handles cache coherency properly
- Provides thread-safe access
- Integrates cleanly with the Linux kernel

**Key Features:**
- `/sys/kernel/rpu_ipi/write`: Write mode value (0, 1, or 2)
- `/sys/kernel/rpu_ipi/status`: Read acknowledgment status

See `kernel_module/README.md` for detailed build and usage instructions.

### `python/`

#### `led_blink_pynq.ipynb`
A Jupyter notebook demonstrating LED control using PYNQ (Python productivity for Zynq). This approach uses the PYNQ overlay system to directly access the AXI GPIO IP from Python.

**Note:** This method bypasses the RPU and directly controls the PL hardware, making it useful for testing the PL design independently.

## Building

### User-Space Applications
```bash
cd apu_app
make
```

### Kernel Module
```bash
cd kernel_module
# Follow instructions in README.md
# Requires cross-compilation toolchain
make
```

## Communication Methods Comparison

| Method | Complexity | Reliability | Use Case |
|--------|-----------|-------------|----------|
| Legacy Shared Memory | Low | Low | Simple testing |
| IPI + Shared Memory | Medium | High | Production applications |
| Kernel Module | High | Highest | System integration |
| PYNQ | Low | Medium | PL testing, rapid prototyping |

## Memory Addresses

- **Shared Memory (IPI)**: `0xFF990000` (4KB)
  - Offset `0x00`: Command/Mode (APU writes, RPU reads)
  - Offset `0x04`: Acknowledgment (RPU writes, APU reads)
- **Legacy Shared Memory**: `0x40000000` (4KB)
  - Direct mode value
- **IPI APU Base**: `0xFF300000`
  - Offset `0x00`: Trigger register
  - Offset `0x04`: Observation register

## Dependencies

- Standard C++ library
- Linux system calls (`/dev/mem`, `mmap`)
- Root privileges for memory mapping
- Kernel headers (for kernel module)

## See Also

- [Kernel Module README](kernel_module/README.md) - Detailed kernel module documentation
- [Main Project README](../README.md) - Overall project documentation
