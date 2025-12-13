/*
    Copyright (C) 2017 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
    Copyright (c) 2012 - 2022 Xilinx, Inc. All Rights Reserved.
    Copyright (c) 2025 Advanced Micro Devices, Inc. All Rights Reserved.
	SPDX-License-Identifier: MIT


    http://www.FreeRTOS.org
    http://aws.amazon.com/freertos


    1 tab == 4 spaces!
*/

/*
 * This application demonstrates a FreeRTOS-based LED Blinking system on a Xilinx RPU.
 *
 * It consists of three main components:
 * 1. Tx Task: Generates LED blink patterns based on the current mode and sends them to a queue.
 * 2. Rx Task: Receives the LED status from the queue and writes it to the AXI GPIO hardware.
 * 3. Timer Callback: Periodically changes the blink mode (Slow -> Fast -> Random).
 *
 * The AXI GPIO is accessed directly from the RPU after configuring the MPU to allow access
 * to the PL address space.
 */

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
/* Xilinx includes. */
#include "xil_printf.h"
#include "xparameters.h"

#include <xil_io.h>
#include "xil_mpu.h"
#include "xil_cache.h"
#include "xreg_cortexr5.h"
#include "sleep.h"
#include "xinterrupt_wrap.h"
#include <stdlib.h>

// Base address for the AXI GPIO IP (Check your .hwh file!)
#define AXI_GPIO_BASE_ADDR 0x80000000
#define GPIO_DATA_OFFSET   0x00 // Data Register Offset
#define GPIO_TRI_OFFSET    0x04      // Tri-State Register Offset (Set direction)

// IPI and Shared Memory Configuration
#define IPI_CH1_BASE       0xFF310000 // RPU0 IPI Channel 1
#define IPI_ISR_OFFSET     0x10  // Interrupt Status Register
#define IPI_IMR_OFFSET     0x14  // Interrupt Mask Register
#define IPI_IER_OFFSET     0x18  // Interrupt Enable Register
#define IPI_IDR_OFFSET     0x1C  // Interrupt Disable Register
#define IPI_INT_ID         65  // GIC_SPI 33 -> ID 65 (Standard for IPI1/RPU0)
#define IPI_INTC_PARENT    0xF9000000 // GIC Base Address
#define APU_MASK           0x01

// APU to RPU0 message passing interface
#define SHARED_MEM_ADDR    0xFF990000
#define SHM_CMD_OFFSET     0x00  // Command/Mode (APU writes, RPU reads)
#define SHM_ACK_OFFSET     0x04  // Acknowledgment (RPU writes, APU reads)
#define SHM_ACK_MAGIC      0xDEADBEEF  // Magic value to indicate acknowledgment

// Legacy Shared Memory
#define LEGACY_SHARED_MEM_ADDR 0x40000000

#define TIMER_ID           1
#define DELAY_10_SECONDS    10000UL
/*-----------------------------------------------------------*/

typedef enum {
    BLINK_SLOW,
    BLINK_FAST,
    BLINK_RANDOM
} BlinkMode_t;

volatile BlinkMode_t current_blink_mode = BLINK_SLOW;
volatile int apu_override_active = 0;


/* The Tx and Rx tasks as described at the top of this file. */
static void prvTxTask( void *pvParameters );
static void prvRxTask( void *pvParameters );
static void vTimerCallback( TimerHandle_t pxTimer );
static void IPI_Handler(void *CallbackRef);

/*-----------------------------------------------------------*/

/* The queue used by the Tx and Rx tasks.
 * The Tx task sends a u32 led_status value to the Rx task.
 */
static TaskHandle_t xTxTask;
static TaskHandle_t xRxTask;
static QueueHandle_t xQueue = NULL;
static TimerHandle_t xTimer = NULL;

#if (configSUPPORT_STATIC_ALLOCATION == 1)
#define QUEUE_BUFFER_SIZE		100

uint8_t ucQueueStorageArea[ QUEUE_BUFFER_SIZE ];
StackType_t xStack1[ configMINIMAL_STACK_SIZE ];
StackType_t xStack2[ configMINIMAL_STACK_SIZE ];
StaticTask_t xTxBuffer,xRxBuffer;
StaticTimer_t xTimerBuffer;
static StaticQueue_t xStaticQueue;
#endif

int main( void )
{
	const TickType_t x10seconds = pdMS_TO_TICKS( DELAY_10_SECONDS );

	xil_printf( "LED blink example main\r\n" );

	/* Create the two tasks.  The Tx task is given a lower priority than the
	Rx task, so the Rx task will leave the Blocked state and pre-empt the Tx
	task as soon as the Tx task places an item in the queue. */
	xTaskCreate( 	prvTxTask, 					/* The function that implements the task. */
					( const char * ) "Tx", 		/* Text name for the task, provided to assist debugging only. */
					configMINIMAL_STACK_SIZE, 	/* The stack allocated to the task. */
					NULL, 						/* The task parameter is not used, so set to NULL. */
					tskIDLE_PRIORITY,			/* The task runs at the idle priority. */
					&xTxTask );

	xTaskCreate( prvRxTask,
				 ( const char * ) "GB",
				 configMINIMAL_STACK_SIZE,
				 NULL,
				 tskIDLE_PRIORITY + 1,
				 &xRxTask );

	/* Create the queue used by the tasks.  The Rx task has a higher priority
	than the Tx task, so will preempt the Tx task and remove values from the
	queue as soon as the Tx task writes to the queue - therefore the queue can
	never have more than one item in it. */
	xQueue = xQueueCreate( 	1,						/* There is only one space in the queue. */
							sizeof( u32 ) );	/* Each space in the queue is large enough to hold a uint32_t. */

	/* Check the queue was created. */
	configASSERT( xQueue );

	/* Create a timer that manages the LED Blink Mode state machine.
     * The timer expires every 10 seconds and triggers vTimerCallback to
     * switch between SLOW, FAST, and RANDOM modes.
     * pdTRUE enables auto-reload, so the timer runs indefinitely.
     */
	xTimer = xTimerCreate( (const char *) "Timer",
							x10seconds,
							pdTRUE,
							(void *) TIMER_ID,
							vTimerCallback);
	/* Check the timer was created. */
	configASSERT( xTimer );

	/* start the timer with a block time of 0 ticks. This means as soon
	   as the schedule starts the timer will start running and will expire after
	   10 seconds */
	xTimerStart( xTimer, 0 );

	// --- RPU Peripheral Initialization ---
    // Configure MPU for PL access (AXI GPIO)
    Xil_SetTlbAttributes(AXI_GPIO_BASE_ADDR, STRONG_ORDERD_SHARED | PRIV_RW_USER_RW);

    // Configure MPU for Shared Memory Access (OCM/DDR) at 0xFF990000
    Xil_SetTlbAttributes(SHARED_MEM_ADDR, NORM_SHARED_NCACHE | PRIV_RW_USER_RW);

    // Configure MPU for Legacy Shared Memory Access (DDR) at 0x40000000
    Xil_SetTlbAttributes(LEGACY_SHARED_MEM_ADDR, NORM_SHARED_NCACHE | PRIV_RW_USER_RW);

    // Initialize Legacy Shared Memory
    Xil_Out32(LEGACY_SHARED_MEM_ADDR, 3);
    
    // Configure MPU for IPI Access (Map all relevant channels)
    Xil_SetTlbAttributes(IPI_CH1_BASE, STRONG_ORDERD_SHARED | PRIV_RW_USER_RW);

    xil_printf("MPU Configured. Setting up Interrupts...\r\n");

    // Initialize IPI following OpenAMP/libmetal pattern:
    // 1. Disable IPI interrupt (IDR)
    // 2. Clear old IPI interrupt (ISR)
    // 3. Register handler
    // 4. Enable interrupt (IER)
    
    // Step 1: Disable IPI interrupt from APU (disable before setup)
    Xil_Out32(IPI_CH1_BASE + IPI_IDR_OFFSET, APU_MASK);
    
    // Step 2: Clear any old IPI interrupt (clear all possible sources)
    Xil_Out32(IPI_CH1_BASE + IPI_ISR_OFFSET, 0xFFFFFFFF);
    
    // Small delay to ensure registers settle
    volatile int delay;
    for(delay=0; delay<1000; delay++);
    
    // Step 3: Connect IPI Interrupt using Wrapper
    // Encoded ID for Level Sensitive High (Trigger = 4)
    // ID 65 | (4 << 12) = 0x4041
    u32 IpiIntrId = IPI_INT_ID | (4 << 12);
    
    xil_printf("Connecting IPI Interrupt (ID %d, Encoded: 0x%X)...\r\n", IPI_INT_ID, IpiIntrId);
    
    int Status = XSetupInterruptSystem(NULL, (Xil_ExceptionHandler)IPI_Handler, 
                                     IpiIntrId, IPI_INTC_PARENT, 0x00);
    
    if (Status != XST_SUCCESS) {
        xil_printf("IPI Interrupt Connect Failed (Status: %d)\r\n", Status);
    } else {
        xil_printf("IPI Interrupt Connected successfully (ID %d)\r\n", IPI_INT_ID);
        
        // Step 4: Enable IPI Interrupt from APU in the IPI Controller (IER)
        // Note: IER is write-only, so we can't read it back
        Xil_Out32(IPI_CH1_BASE + IPI_IER_OFFSET, APU_MASK);
        
        // Verify interrupt is enabled by checking IMR (Interrupt Mask Register)
        // IMR bit 0 = 0 means interrupt is enabled (not masked)
        u32 imr_val = Xil_In32(IPI_CH1_BASE + IPI_IMR_OFFSET);
        if ((imr_val & APU_MASK) == 0) {
            xil_printf("IPI Enabled successfully\r\n");
        } else {
            xil_printf("WARNING: IPI may not be enabled\r\n");
        }
        
        // Explicitly enable in GIC
        XEnableIntrId(IpiIntrId, IPI_INTC_PARENT);
        
        // Clear any pending interrupts
        u32 isr_final = Xil_In32(IPI_CH1_BASE + IPI_ISR_OFFSET);
        if (isr_final != 0) {
            Xil_Out32(IPI_CH1_BASE + IPI_ISR_OFFSET, 0xFFFFFFFF);
        }
    }


    // 1. Set the GPIO direction to OUTPUT (clear the tri-state register)
    Xil_Out32(AXI_GPIO_BASE_ADDR + GPIO_TRI_OFFSET, 0x0);

    xil_printf( "GPIO initialized. Starting scheduler.\r\n" );

	/* Start the tasks and timer running. */
	vTaskStartScheduler();

	/* If all is well, the scheduler will now be running, and the following line
	will never be reached. */
	for( ;; );
}


/*-----------------------------------------------------------*/
/* The Tx Task:
 * - Generates LED patterns based on the current `current_blink_mode`.
 */
static void prvTxTask( void *pvParameters )
{
	(void)pvParameters;
	TickType_t xDelay;
    u32 led_val = 0x1;

    xil_printf("Tx Task Started\r\n");

	for( ;; )
	{
        switch (current_blink_mode) {
            case BLINK_SLOW:
                xDelay = pdMS_TO_TICKS(1000);
                led_val = (led_val == 0x1) ? 0x2 : 0x1;
                break;
            case BLINK_FAST:
                xDelay = pdMS_TO_TICKS(200);
                led_val = (led_val == 0x1) ? 0x2 : 0x1;
                break;
            case BLINK_RANDOM:
                xDelay = pdMS_TO_TICKS(200);
                led_val = rand() % 4;
                break;
            default:
                xDelay = pdMS_TO_TICKS(1000);
                break;
        }

		vTaskDelay( xDelay );

		xQueueSend( xQueue, &led_val, 0UL );
	}
}

/*-----------------------------------------------------------*/
/* The Rx Task:
 * - Writes the received LED status value directly to the AXI GPIO hardware.
 */
static void prvRxTask( void *pvParameters )
{
	(void)pvParameters;
    u32 received_led_status;

    xil_printf("Rx Task Started\r\n");

	for( ;; )
	{
		xQueueReceive( 	xQueue,
						&received_led_status,
						portMAX_DELAY );

        Xil_Out32(AXI_GPIO_BASE_ADDR + GPIO_DATA_OFFSET, received_led_status);
	}
}

/*-----------------------------------------------------------*/
/* The Timer Callback:
 * - Manages internal state machine if APU override is not active.
 */
static void vTimerCallback( TimerHandle_t pxTimer )
{
    u32 legacy_val;
	configASSERT( pxTimer );

    // Only rotate modes if APU IPI override is NOT active
    if (!apu_override_active) {
        // Check Legacy Shared Memory
        Xil_DCacheInvalidateRange(LEGACY_SHARED_MEM_ADDR, 32);
        legacy_val = Xil_In32(LEGACY_SHARED_MEM_ADDR);
        if (legacy_val <= 2) {
             if ((BlinkMode_t)legacy_val != current_blink_mode) {
                 current_blink_mode = (BlinkMode_t)legacy_val;
                 xil_printf("Timer: Legacy Shared Mem set mode to %d\r\n", current_blink_mode);
             }
        } else {
            // No legacy override, proceed with rotation
            if (current_blink_mode == BLINK_SLOW) {
                current_blink_mode = BLINK_FAST;
                xil_printf("Timer: Switching to FAST mode\r\n");
            } else if (current_blink_mode == BLINK_FAST) {
                current_blink_mode = BLINK_RANDOM;
                xil_printf("Timer: Switching to RANDOM mode\r\n");
            } else {
                current_blink_mode = BLINK_SLOW;
                xil_printf("Timer: Switching to SLOW mode\r\n");
            }
        }
    }
}

/*-----------------------------------------------------------*/
/* IPI Interrupt Handler - Following OpenAMP/libmetal pattern */
static void IPI_Handler(void *CallbackRef) {
    (void)CallbackRef;
    
    // Read ISR IMMEDIATELY (before any other operations) to check if interrupt is pending
    // Use memory barrier to ensure we read the actual hardware state
    __sync_synchronize();
    u32 isr = Xil_In32(IPI_CH1_BASE + IPI_ISR_OFFSET);
    
    // If ISR is 0, this is a spurious interrupt - clear and return immediately
    // This can happen at startup or from other interrupt sources sharing the same ID
    if (isr == 0) {
        // Don't print for spurious interrupts to avoid console spam
        // Just clear any potential stuck state and return
        Xil_Out32(IPI_CH1_BASE + IPI_ISR_OFFSET, 0xFFFFFFFF);
        return;
    }
    
    xil_printf("*** IPI Handler Called! ISR=0x%X ***\r\n", isr);
    
    // Check if APU (Bit 0) triggered the interrupt
    if (isr & APU_MASK) {
        // Clear the interrupt by writing mask to ISR (following OpenAMP pattern)
        Xil_Out32(IPI_CH1_BASE + IPI_ISR_OFFSET, APU_MASK);
        
        // Invalidate Cache for Shared Mem to ensure fresh read from DDR
        Xil_DCacheInvalidateRange(SHARED_MEM_ADDR, 32);
        
        // Read command/mode from shared memory (offset 0x00)
        u32 cmd_val = Xil_In32(SHARED_MEM_ADDR + SHM_CMD_OFFSET);
        xil_printf("IPI Received! Command Value: %d\r\n", cmd_val);
        
        // Process the message
        if (cmd_val <= 2) {
            // Valid mode: Set blink mode and activate APU override
            current_blink_mode = (BlinkMode_t)cmd_val;
            apu_override_active = 1;
            xil_printf("Mode set to %d (APU Override Active)\r\n", current_blink_mode);
        } else {
            // Invalid mode (>2): Release control, let timer resume
            apu_override_active = 0;
            xil_printf("APU released control. Timer resuming.\r\n");
        }
        
        // Write acknowledgment to shared memory (offset 0x04)
        // Use magic value + mode to confirm we processed it
        Xil_Out32(SHARED_MEM_ADDR + SHM_ACK_OFFSET, SHM_ACK_MAGIC | (cmd_val & 0xFF));
        
        // Flush cache to ensure APU sees the acknowledgment
        Xil_DCacheFlushRange(SHARED_MEM_ADDR + SHM_ACK_OFFSET, 4);
        
        xil_printf("Acknowledgment written (0x%X)\r\n", SHM_ACK_MAGIC | (cmd_val & 0xFF));
    } else {
        // Interrupt not for us - clear it and return
        // Clear all bits in ISR to prevent stuck interrupt
        Xil_Out32(IPI_CH1_BASE + IPI_ISR_OFFSET, 0xFFFFFFFF);
    }
}
