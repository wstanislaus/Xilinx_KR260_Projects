/*
 * APU Application to control LED Blink Mode on RPU via Legacy Shared Memory.
 *
 * Usage: ./apu_app <mode>
 * Modes:
 *   0: SLOW
 *   1: FAST
 *   2: RANDOM
 *   3+: Release control (RPU internal state machine)
 *
 * Memory Map:
 *   0x40000000: Shared Control Word (u32)
 */

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#define SHARED_MEM_ADDR 0x40000000
#define SHARED_MEM_SIZE 0x1000 // 4KB

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <mode>" << std::endl;
        std::cerr << "  0: SLOW" << std::endl;
        std::cerr << "  1: FAST" << std::endl;
        std::cerr << "  2: RANDOM" << std::endl;
        std::cerr << "  3+: Auto (RPU Control)" << std::endl;
        return 1;
    }

    int mode = std::atoi(argv[1]);

    // Open /dev/mem to access physical memory
    int mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (mem_fd == -1) {
        std::perror("Error opening /dev/mem");
        return 1;
    }

    // Map the shared memory region
    void* mapped_base = mmap(0, SHARED_MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, SHARED_MEM_ADDR);
    if (mapped_base == MAP_FAILED) {
        std::perror("Error mapping memory");
        close(mem_fd);
        return 1;
    }

    // Write the mode to the shared memory address
    volatile uint32_t* shared_ctrl = (volatile uint32_t*)mapped_base;
    *shared_ctrl = (uint32_t)mode;

    std::cout << "Written mode " << mode << " to legacy shared memory at 0x" << std::hex << SHARED_MEM_ADDR << std::endl;

    // Cleanup
    if (munmap(mapped_base, SHARED_MEM_SIZE) == -1) {
        std::perror("Error unmapping memory");
    }
    close(mem_fd);

    return 0;
}
