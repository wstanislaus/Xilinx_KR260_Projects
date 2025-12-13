/*
 * APU Application to control LED Blink Mode on RPU via Shared Memory and IPI.
 *
 * Usage: ./ipi_app <mode>
 * Modes:
 *   0: SLOW
 *   1: FAST
 *   2: RANDOM
 *   3+: Release control (RPU internal state machine)
 *
 * Memory Map:
 *   0xFF990000: Shared Control Word (u32)
 *   0xFF300000: APU IPI Base (Trigger)
 */

#include <iostream>
#include <cstdlib>
#include <cstdint>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#define SHARED_MEM_ADDR 0xFF990000
#define SHARED_MEM_SIZE 0x1000 // 4KB

// Shared Memory Layout (following OpenAMP pattern)
#define SHM_CMD_OFFSET     0x00  // Command/Mode (APU writes, RPU reads)
#define SHM_ACK_OFFSET     0x04  // Acknowledgment (RPU writes, APU reads)
#define SHM_ACK_MAGIC      0xDEADBEEF  // Magic value to indicate acknowledgment
#define SHM_ACK_TIMEOUT_MS 1000  // Timeout for acknowledgment (1 second)

#define IPI_APU_BASE    0xFF300000
#define IPI_TRIG_OFFSET 0x00
#define IPI_OBS_OFFSET  0x04

// Target Masks (ZynqMP IPI bitmasks from device tree)
#define MASK_CH1_RPU0 0x100    // 256 (bit 8) - IPI1 to RPU0


int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <mode>" << std::endl;
        std::cerr << "Modes: 0=SLOW, 1=FAST, 2=RANDOM, 3+=Release control" << std::endl;
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
    void* shared_base = mmap(0, SHARED_MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, SHARED_MEM_ADDR);
    if (shared_base == MAP_FAILED) {
        std::perror("Error mapping shared memory");
        close(mem_fd);
        return 1;
    }

    // Map the IPI APU base region (Source)
    void* ipi_apu_base = mmap(0, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, IPI_APU_BASE);
    if (ipi_apu_base == MAP_FAILED) {
        std::perror("Error mapping IPI APU memory");
        munmap(shared_base, SHARED_MEM_SIZE);
        close(mem_fd);
        return 1;
    }

    // Accessors
    volatile uint32_t* shared_cmd = (volatile uint32_t*)((char*)shared_base + SHM_CMD_OFFSET);
    volatile uint32_t* shared_ack = (volatile uint32_t*)((char*)shared_base + SHM_ACK_OFFSET);
    volatile uint32_t* ipi_trig = (volatile uint32_t*)((char*)ipi_apu_base + IPI_TRIG_OFFSET);
    volatile uint32_t* ipi_obs  = (volatile uint32_t*)((char*)ipi_apu_base + IPI_OBS_OFFSET);

    // Clear previous acknowledgment
    *shared_ack = 0;
    __sync_synchronize();
    
    // Step 1: Write message to shared memory FIRST
    *shared_cmd = (uint32_t)mode;
    std::cout << "Written mode " << mode << " to shared memory at 0x" 
              << std::hex << (SHARED_MEM_ADDR + SHM_CMD_OFFSET) << std::endl;
    
    // Memory barrier to ensure write completes before triggering IPI
    __sync_synchronize();
    
    // Step 2: Trigger IPI to notify RPU
    std::cout << "Triggering IPI to RPU0 (Mask 0x" << std::hex << MASK_CH1_RPU0 << ")..." << std::endl;
    *ipi_trig = MASK_CH1_RPU0;
    
    // Step 3: Poll for acknowledgment from RPU
    std::cout << "Waiting for RPU acknowledgment..." << std::endl;
    uint32_t ack_val = 0;
    int timeout_count = 0;
    const int max_timeout = SHM_ACK_TIMEOUT_MS * 1000; // Convert to microseconds
    
    while (timeout_count < max_timeout) {
        __sync_synchronize();
        ack_val = *shared_ack;
        
        // Check if acknowledgment matches expected value
        // RPU writes: SHM_ACK_MAGIC | (mode & 0xFF)
        if ((ack_val & 0xFFFFFF00) == (SHM_ACK_MAGIC & 0xFFFFFF00)) {
            uint32_t ack_mode = ack_val & 0xFF;
            if (ack_mode == (uint32_t)mode) {
                std::cout << "RPU acknowledged! Mode " << mode << " processed successfully." << std::endl;
                break;
            }
        }
        
        usleep(100); // 100 microseconds
        timeout_count += 100;
    }
    
    if (timeout_count >= max_timeout) {
        std::cerr << "ERROR: Timeout waiting for RPU acknowledgment!" << std::endl;
        std::cerr << "Last ACK value: 0x" << std::hex << ack_val << std::endl;
    }

    // Status Read
    uint32_t obs_val = *ipi_obs;
    std::cout << "--- Status ---" << std::endl;
    std::cout << "Shared Mem CMD: " << std::dec << *shared_cmd << std::endl;
    std::cout << "Shared Mem ACK: 0x" << std::hex << *shared_ack << std::endl;
    std::cout << "APU IPI OBS (0xFF300004): 0x" << std::hex << obs_val;
    if (obs_val & MASK_CH1_RPU0) {
        std::cout << " -> Ch1 (RPU0) PENDING" << std::endl;
    } else {
        std::cout << " -> Ch1 (RPU0) IDLE" << std::endl;
    }

    // Cleanup
    munmap(shared_base, SHARED_MEM_SIZE);
    munmap(ipi_apu_base, 0x1000);
    close(mem_fd);

    return 0;
}
