.PHONY: venv install clean build help

PYTHON := python3.12
VENV_DIR := venv
VENV_PYTHON := $(VENV_DIR)/bin/python
VENV_PIP := $(VENV_DIR)/bin/pip
SRC_DIR := PL/MyHDL/src
INTERRUPT_GEN_IP := $(SRC_DIR)/interrupt_generator_ip/interrupt_generator_ip.py
OUTPUT_DIR := build
VERILOG_OUTPUT := $(OUTPUT_DIR)/interrupt_generator_ip.v

help:
	@echo "Available targets:"
	@echo "  venv     - Create Python 3.12 virtual environment"
	@echo "  install  - Install myhdl package"
	@echo "  build    - Build Verilog file from interrupt_generator_ip.py"
	@echo "  clean    - Remove virtual environment and build directory"
	@echo "  all      - Run venv, install, and build"

venv:
	@echo "Creating Python 3.12 virtual environment..."
	@if [ ! -d "$(VENV_DIR)" ]; then \
		$(PYTHON) -m venv $(VENV_DIR); \
		echo "Virtual environment created successfully."; \
	else \
		echo "Virtual environment already exists."; \
	fi

install: venv
	@echo "Installing myhdl..."
	@$(VENV_PIP) install --upgrade pip
	@$(VENV_PIP) install myhdl
	@echo "myhdl installed successfully."

build: install
	@echo "Building Verilog file..."
	@mkdir -p $(OUTPUT_DIR)
	@PYTHONPATH="$(CURDIR):$$PYTHONPATH" $(VENV_PYTHON) $(INTERRUPT_GEN_IP)
	@if [ -f "interrupt_generator_ip.v" ]; then \
		mv interrupt_generator_ip.v $(VERILOG_OUTPUT); \
		echo "Verilog file created: $(VERILOG_OUTPUT)"; \
	else \
		echo "Error: Verilog file was not generated."; \
		exit 1; \
	fi

clean:
	@echo "Cleaning up..."
	@rm -rf $(VENV_DIR)
	@rm -rf $(OUTPUT_DIR)
	@rm -f interrupt_generator_ip.v
	@find . -type d -name __pycache__ -exec rm -r {} + 2>/dev/null || true
	@find . -type f -name "*.pyc" -delete 2>/dev/null || true
	@echo "Cleanup complete."

all: venv install build

