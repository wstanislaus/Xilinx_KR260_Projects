# Simple Makefile for cross-compiling the APU applications

# Define the cross-compiler (adjust based on your Yocto SDK or environment)
# If running on the target or with CC already set, this will be used.
CXX ?= aarch64-linux-gnu-g++

TARGET1 = apu_app
SRC1 = main.cpp

TARGET2 = fw_loader
SRC2 = fw_loader.cpp

TARGET3 = ipi_app
SRC3 = ipi_app.cpp

all: $(TARGET1) $(TARGET2) $(TARGET3)

$(TARGET1): $(SRC1)
	$(CXX) -o $@ $< -Wall -Wextra

$(TARGET2): $(SRC2)
	$(CXX) -o $@ $< -Wall -Wextra -pthread

$(TARGET3): $(SRC3)
	$(CXX) -o $@ $< -Wall -Wextra

clean:
	rm -f $(TARGET1) $(TARGET2) $(TARGET3)
