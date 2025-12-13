# Makefile for RPU IPI Kernel Module
#
# Cross-compilation for KR260 (ARM64):
#   1. Source the Yocto toolchain environment:
#      source /tools/kr260_yocto_toolchain/environment-setup-cortexa72-cortexa53-oe-linux
#   2. Build (KDIR defaults to SDK kernel source):
#      make
#   3. Or override KDIR if needed:
#      make KDIR=/path/to/kernel/source
#
# Native build (x86_64):
#   make                    # Uses host kernel headers
#
# To use on target:
#   echo 1 > /sys/kernel/rpu_ipi/write
#   cat /sys/kernel/rpu_ipi/status

# Default SDK kernel source path (for cross-compilation)
# Use usr/lib/modules (standard SDK location where kernel-devsrc installs)
SDK_KERNEL_PATH ?= /tools/kr260_yocto_toolchain/sysroots/cortexa72-cortexa53-oe-linux/usr/lib/modules/6.12.40-xilinx/build

# Detect if we're cross-compiling by checking for CROSS_COMPILE
# This should be set by the environment-setup script
ifneq ($(CROSS_COMPILE),)
    # Cross-compilation detected
    ARCH ?= arm64
    # Use SDK kernel source as default if KDIR not explicitly set
    KDIR ?= $(SDK_KERNEL_PATH)
    # Use compiler from environment (set by environment-setup script)
    # CC, LD, etc. are already set by the environment
    $(info Cross-compiling for $(ARCH) with $(CC))
    $(info Using KDIR: $(KDIR))
else
    # throw error if not cross-compiling
    $(error "Cross-compilation is required. Please source the Yocto toolchain environment.")
endif

# Module name
# For single-source modules, just specify the object file
# The kernel build system will automatically compile rpu_ipi.c to rpu_ipi.o
obj-m += rpu_ipi.o

# Build target
all:
	@if [ ! -f "$(KDIR)/.config" ]; then \
		echo "WARNING: Kernel source at $(KDIR) may not be properly configured."; \
		echo "This might cause build errors. Ensure kernel-devsrc is installed in SDK."; \
	fi
	@if [ ! -d "$(KDIR)/include/asm" ] && [ ! -L "$(KDIR)/include/asm" ]; then \
		echo "WARNING: Kernel source may need preparation. Attempting modules_prepare..."; \
		$(MAKE) -C $(KDIR) M=$(PWD) ARCH=$(ARCH) modules_prepare || true; \
	fi
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=$(ARCH) modules

# Clean target
clean:
	@if [ -d "$(KDIR)" ]; then \
		$(MAKE) -C $(KDIR) M=$(PWD) ARCH=$(ARCH) clean; \
	fi
	rm -f *.o *.ko *.mod.c *.mod *.symvers *.order

# Help target
help:
	@echo "RPU IPI Kernel Module Makefile"
	@echo ""
	@echo "Cross-compilation for KR260 (ARM64):"
	@echo "  1. Source the Yocto toolchain environment:"
	@echo "     source /tools/kr260_yocto_toolchain/environment-setup-cortexa72-cortexa53-oe-linux"
	@echo "  2. Build (KDIR defaults to SDK kernel source):"
	@echo "     make"
	@echo ""
	@echo "  Default SDK kernel path: $(SDK_KERNEL_PATH)"
	@echo "  Override KDIR if needed: make KDIR=/path/to/kernel/source"
	@echo ""
	@echo "Native build (x86_64):"
	@echo "  make          - Build using host kernel headers"
	@echo ""
	@echo "Targets:"
	@echo "  make          - Build the kernel module"
	@echo "  make clean    - Clean build artifacts"
	@echo ""
	@echo "On target device:"
	@echo "  sudo insmod rpu_ipi.ko     - Load module"
	@echo "  sudo rmmod rpu_ipi         - Unload module"
	@echo "  echo 1 > /sys/kernel/rpu_ipi/write  - Send mode 1 to RPU"
	@echo "  cat /sys/kernel/rpu_ipi/status      - Check acknowledgment"
	@echo ""
	@echo "Current settings:"
	@echo "  KDIR=$(KDIR)"
	@echo "  ARCH=$(ARCH)"
	@echo "  CROSS_COMPILE=$(CROSS_COMPILE)"
	@echo ""
	@echo "Default SDK kernel path:"
	@echo "  $(SDK_KERNEL_PATH)"

.PHONY: all clean help
