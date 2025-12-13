# Xilinx KR260 Projects

This repository contains projects for the Xilinx KR260 Robotics Starter Kit, demonstrating various hardware-software co-design patterns and inter-processor communication on the Zynq UltraScale+ MPSoC.

## Overview

The KR260 is based on the Zynq UltraScale+ MPSoC, which features:
- **APU (Application Processing Unit)**: Quard-core ARM Cortex-A53 running Linux
- **RPU (Real-time Processing Unit)**: Dual-core ARM Cortex-R5 running bare-metal or FreeRTOS
- **PL (Programmable Logic)**: FPGA fabric for custom hardware acceleration

This repository demonstrates how these three processing domains can work together to create sophisticated embedded systems.

## Hardware Requirements

- Xilinx KR260 Robotics Starter Kit

## Software Requirements

- [Yocto Linux running on the target](https://github.com/wstanislaus/Xilinx_KR260_Yocto/tree/main#)
- Ubuntu 24.04 LTS development machine and it acts as a TFTP and NFS server
- Yocto SDK toolchain installed in the Ubuntu 24.04 development machine
- [Xilinx Vivado and Vitis 2025.2](https://www.xilinx.com/support/download/index.html/content/xilinx/en/downloadNav/vivado-design-tools/2025-2.html)

## Project Structure

### `gpio_led/`
A comprehensive GPIO LED control demo that showcases:
- APU-to-RPU communication via IPI (Inter-Processor Interrupt) and shared memory
- RPU-based real-time LED control using FreeRTOS
- PL-based AXI GPIO IP for hardware LED interface
- Multiple control interfaces (kernel module, user-space applications, Python/PYNQ)

This project serves as an excellent starting point for understanding heterogeneous processing on the KR260 platform.

## Build Utilities

### `build_utils/`
Contains build scripts and templates for automating Vivado project builds using CMake and TCL scripts.

## Getting Started

1. **Prerequisites**:
   - Xilinx Vivado (for PL development)
   - Xilinx Vitis (for RPU/APU application development)
   - Yocto SDK or cross-compilation toolchain (for kernel modules)

2. **Explore the Projects**:
   - Start with `gpio_led/` for a complete example
   - Each project contains its own README with specific build and usage instructions

## License

See the [LICENSE](LICENSE) file for details.
