/*
 * RPU IPI Kernel Module
 *
 * Provides a sysfs interface for APU-to-RPU communication via shared memory
 * and IPI (Inter-Processor Interrupt). The module exposes:
 * - /sys/kernel/rpu_ipi/write: Write mode value (0, 1, or 2) to send to RPU
 * - /sys/kernel/rpu_ipi/status: Read acknowledgment status (format: "mode,ACK" or "mode,NOACK")
 *
 * The module uses non-cached memory mappings to ensure cache coherency between
 * APU and RPU processors. Messages are sent via shared memory at 0xFF990000
 * and IPI interrupts are triggered via IPI registers at 0xFF300000.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sysfs.h>
#include <linux/kobject.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/pgtable.h>

#define MODULE_NAME "rpu_ipi"
#define MODULE_VERSION_STR "1.0"

/* Memory addresses */
#define SHARED_MEM_ADDR    0xFF990000UL
#define SHARED_MEM_SIZE    0x1000
#define IPI_APU_BASE       0xFF300000UL
#define IPI_SIZE           0x1000

/* Shared Memory Layout */
#define SHM_CMD_OFFSET     0x00  /* Command/Mode (APU writes, RPU reads) */
#define SHM_ACK_OFFSET     0x04  /* Acknowledgment (RPU writes, APU reads) */
#define SHM_ACK_MAGIC      0xDEADBEEF

/* IPI Register Offsets */
#define IPI_TRIG_OFFSET    0x00  /* Trigger register (write-only on source side) */

/* IPI Masks */
#define MASK_CH1_RPU0      0x100  /* Bit 8 - IPI1 to RPU0 */

/* Timeout for acknowledgment (milliseconds) */
#define ACK_TIMEOUT_MS     1500

/* Module state */
static struct kobject *rpu_ipi_kobj;
static void __iomem *shared_mem_base;
static void __iomem *ipi_base;
static int last_sent_mode = -1;
static bool last_ack_received = false;
static DEFINE_MUTEX(rpu_ipi_mutex);

/*
 * Send message to RPU via shared memory and IPI
 *
 * Following OpenAMP/libmetal pattern:
 * 1. Clear previous acknowledgment
 * 2. Write message to shared memory
 * 3. Memory barrier
 * 4. Trigger IPI interrupt
 * 5. Poll for acknowledgment with timeout
 *
 * Returns 0 on success, negative on error
 */
static int send_message_to_rpu(int mode)
{
    unsigned long timeout;
    u32 ack_val;
    
    if (mode < 0 || mode > 2) {
        pr_err("%s: Invalid mode %d (must be 0-2)\n", MODULE_NAME, mode);
        return -EINVAL;
    }
    
    mutex_lock(&rpu_ipi_mutex);
    
    /* Clear previous acknowledgment */
    *(volatile u32 __force *)(shared_mem_base + SHM_ACK_OFFSET) = 0;
    wmb();
    
    /* Small delay to ensure RPU sees the cleared acknowledgment */
    udelay(10);
    
    /* Write message to shared memory */
    *(volatile u32 __force *)(shared_mem_base + SHM_CMD_OFFSET) = (u32)mode;
    wmb();
    
    /* Trigger IPI to RPU0 */
    iowrite32(MASK_CH1_RPU0, ipi_base + IPI_TRIG_OFFSET);
    wmb(); /* Ensure IPI trigger completes */
    
    /* Small delay to allow IPI to propagate to RPU */
    udelay(10);
    
    /* Poll for acknowledgment */
    timeout = jiffies + msecs_to_jiffies(ACK_TIMEOUT_MS);
    
    /* Initial delay to allow RPU to process interrupt */
    usleep_range(100, 200);
    
    while (time_before(jiffies, timeout)) {
        rmb();
        ack_val = *(volatile u32 __force *)(shared_mem_base + SHM_ACK_OFFSET);
        
        /* Check if acknowledgment contains magic value */
        /* RPU writes: SHM_ACK_MAGIC | (cmd_val & 0xFF) */
        /* Accept any value with magic in upper 24 bits as valid acknowledgment */
        if ((ack_val & 0xFFFFFF00) == (SHM_ACK_MAGIC & 0xFFFFFF00)) {
            last_sent_mode = mode;
            last_ack_received = true;
            mutex_unlock(&rpu_ipi_mutex);
            return 0;
        }
        
        usleep_range(50, 100);
    }
    
    /* Timeout */
    last_sent_mode = mode;
    last_ack_received = false;
    pr_warn("%s: Timeout waiting for RPU acknowledgment (mode %d, ACK=0x%X)\n",
           MODULE_NAME, mode, ack_val);
    mutex_unlock(&rpu_ipi_mutex);
    return -ETIMEDOUT;
}

/*
 * Sysfs write handler - accepts mode value (0, 1, or 2)
 */
static ssize_t write_store(struct kobject *kobj, struct kobj_attribute *attr,
                          const char *buf, size_t count)
{
    int mode;
    int ret;
    
    ret = kstrtoint(buf, 10, &mode);
    if (ret) {
        pr_err("%s: Invalid input, expected integer\n", MODULE_NAME);
        return ret;
    }
    if (mode < 0 || mode > 3) {
        pr_err("%s: Invalid mode %d (must be 0-3)\n", MODULE_NAME, mode);
        return -EINVAL;
    }
    
    ret = send_message_to_rpu(mode);
    if (ret) {
        return ret;
    }
    
    return count;
}

/*
 * Sysfs read handler - returns "mode,ACK" or "mode,NOACK"
 */
static ssize_t status_show(struct kobject *kobj, struct kobj_attribute *attr,
                           char *buf)
{
    int len;
    
    mutex_lock(&rpu_ipi_mutex);
    
    if (last_sent_mode == -1) {
        len = sprintf(buf, "NONE,NONE\n");
    } else {
        len = sprintf(buf, "%d,%s\n", last_sent_mode,
                     last_ack_received ? "ACK" : "NOACK");
    }
    
    mutex_unlock(&rpu_ipi_mutex);
    
    return len;
}

/* Sysfs attribute declarations */
static struct kobj_attribute write_attr = __ATTR(write, 0220, NULL, write_store);
static struct kobj_attribute status_attr = __ATTR(status, 0444, status_show, NULL);

static struct attribute *rpu_ipi_attrs[] = {
    &write_attr.attr,
    &status_attr.attr,
    NULL,
};

static struct attribute_group rpu_ipi_attr_group = {
    .attrs = rpu_ipi_attrs,
};

/*
 * Module initialization
 */
static int __init rpu_ipi_init(void)
{
    int ret;
    
    pr_info("%s: Initializing RPU IPI module v%s\n", MODULE_NAME, MODULE_VERSION_STR);
    
    /* Map shared memory region with non-cached protection for cache coherency */
    shared_mem_base = ioremap_prot(SHARED_MEM_ADDR, SHARED_MEM_SIZE,
                                   pgprot_val(pgprot_noncached(PAGE_KERNEL)));
    if (!shared_mem_base) {
        /* Fallback to regular ioremap */
        pr_info("%s: Failed to map shared memory at 0x%lX with non-cached protection, falling back to regular ioremap\n", MODULE_NAME, SHARED_MEM_ADDR);
        shared_mem_base = ioremap(SHARED_MEM_ADDR, SHARED_MEM_SIZE);
        if (!shared_mem_base) {
            pr_err("%s: Failed to map shared memory at 0x%lX\n", MODULE_NAME, SHARED_MEM_ADDR);
            return -ENOMEM;
        }
    }
    
    /* Map IPI register region */
    ipi_base = ioremap(IPI_APU_BASE, IPI_SIZE);
    if (!ipi_base) {
        pr_err("%s: Failed to map IPI registers at 0x%lX\n", MODULE_NAME, IPI_APU_BASE);
        iounmap(shared_mem_base);
        return -ENOMEM;
    }
    
    /* Create sysfs interface */
    rpu_ipi_kobj = kobject_create_and_add("rpu_ipi", kernel_kobj);
    if (!rpu_ipi_kobj) {
        pr_err("%s: Failed to create sysfs kobject\n", MODULE_NAME);
        iounmap(ipi_base);
        iounmap(shared_mem_base);
        return -ENOMEM;
    }
    
    ret = sysfs_create_group(rpu_ipi_kobj, &rpu_ipi_attr_group);
    if (ret) {
        pr_err("%s: Failed to create sysfs attributes\n", MODULE_NAME);
        kobject_put(rpu_ipi_kobj);
        iounmap(ipi_base);
        iounmap(shared_mem_base);
        return ret;
    }
    
    pr_info("%s: Module loaded successfully\n", MODULE_NAME);
    
    return 0;
}

/*
 * Module cleanup
 */
static void __exit rpu_ipi_exit(void)
{
    pr_info("%s: Unloading module\n", MODULE_NAME);
    
    sysfs_remove_group(rpu_ipi_kobj, &rpu_ipi_attr_group);
    kobject_put(rpu_ipi_kobj);
    
    if (ipi_base)
        iounmap(ipi_base);
    if (shared_mem_base)
        iounmap(shared_mem_base);
    
    pr_info("%s: Module unloaded\n", MODULE_NAME);
}

module_init(rpu_ipi_init);
module_exit(rpu_ipi_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("William Stanislaus");
MODULE_DESCRIPTION("RPU IPI Communication Module");
MODULE_VERSION(MODULE_VERSION_STR);
