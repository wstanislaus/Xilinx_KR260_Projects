# Makefile wrapper for CMake build system
# This Makefile sets up the build directory and provides convenient targets

# Build directory name (can be overridden: make BUILD_DIR=custom_build)
BUILD_DIR ?= build

# CMake command
CMAKE := cmake

# Default target
.PHONY: all
all: configure
	@$(MAKE) -C $(BUILD_DIR) build_all

# Configure CMake (creates build directory and runs cmake)
.PHONY: configure
configure:
	@if [ ! -d "$(BUILD_DIR)" ]; then \
		echo "Creating build directory: $(BUILD_DIR)"; \
		mkdir -p $(BUILD_DIR); \
	fi
	@if [ ! -f "$(BUILD_DIR)/CMakeCache.txt" ]; then \
		echo "Configuring CMake..."; \
		cd $(BUILD_DIR) && $(CMAKE) ..; \
	else \
		echo "CMake already configured. Run 'make reconfigure' to reconfigure."; \
	fi

# Reconfigure CMake (removes cache and reconfigures)
.PHONY: reconfigure
reconfigure:
	@echo "Reconfiguring CMake..."
	@rm -rf $(BUILD_DIR)/CMakeCache.txt $(BUILD_DIR)/CMakeFiles
	@$(MAKE) configure

# Build targets - delegate to CMake
.PHONY: synth impl bitstream xsa build_all clean_all
synth: configure
	@$(MAKE) -C $(BUILD_DIR) synth

impl: configure
	@$(MAKE) -C $(BUILD_DIR) impl

bitstream: configure
	@$(MAKE) -C $(BUILD_DIR) bitstream

xsa: configure
	@$(MAKE) -C $(BUILD_DIR) xsa

build_all: configure
	@$(MAKE) -C $(BUILD_DIR) build_all

clean_all: configure
	@$(MAKE) -C $(BUILD_DIR) clean_all
	@rm -rf $(BUILD_DIR)

# Clean build directory
.PHONY: clean
clean:
	@echo "Removing build directory: $(BUILD_DIR)"
	@rm -rf $(BUILD_DIR)

# Clean everything (build directory + generated files)
.PHONY: distclean
distclean: configure
	@echo "Cleaning build directory and all generated files..."
	@$(MAKE) -C $(BUILD_DIR) clean_all
	@rm -rf $(BUILD_DIR)

# Help target
.PHONY: help
help:
	@echo "Available targets:"
	@echo "  make              - Configure and run complete build (build_all)"
	@echo "  make configure    - Create build directory and configure CMake"
	@echo "  make reconfigure  - Remove CMake cache and reconfigure"
	@echo ""
	@echo "Build targets (automatically configure if needed):"
	@echo "  make synth        - Run synthesis only"
	@echo "  make impl         - Run implementation (depends on synth)"
	@echo "  make bitstream    - Generate bitstream (depends on impl)"
	@echo "  make xsa          - Export XSA file (depends on bitstream)"
	@echo "  make build_all    - Complete build (synthesis -> implementation -> bitstream -> XSA -> HWH)"
	@echo ""
	@echo "Clean targets:"
	@echo "  make clean_all    - Remove all generated Vivado files (keeps build directory)"
	@echo "  make clean        - Remove build directory"
	@echo "  make distclean    - Remove build directory and all generated files"
	@echo ""
	@echo "Options:"
	@echo "  BUILD_DIR=<dir>   - Use custom build directory (default: build)"
	@echo ""
	@echo "Examples:"
	@echo "  make                    # Complete build"
	@echo "  make build_all           # Complete build (explicit)"
	@echo "  make synth               # Synthesis only"
	@echo "  make clean_all           # Clean generated files"
	@echo "  make BUILD_DIR=out build_all  # Use 'out' as build directory"

