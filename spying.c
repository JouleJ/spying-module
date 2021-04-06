#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <asm/io.h>

MODULE_LICENSE("GPL");

#define PS2_IRQ 1
#define SCANCODE_IO_PORT 0x60
#define RELEASED_MASK 0x80
#define DELAY_MSECS 60000

static struct timer_list timer;
static int count = 0;

static void tasklet_handler(struct tasklet_struct* tasklet) {
    if (tasklet->data & RELEASED_MASK) {
        ++count;
    }
}

static void timer_callback(struct timer_list* timer) {
    printk(KERN_INFO "(spying) Key pressed in this minute: %d", count);
    count = 0;
    mod_timer(timer, jiffies + msecs_to_jiffies(DELAY_MSECS));
}

DECLARE_TASKLET(spying_tasklet, tasklet_handler);

static irqreturn_t irq_handler(int irq, void* dev_id) {
    spying_tasklet.data = inb(SCANCODE_IO_PORT);
    tasklet_schedule(&spying_tasklet);
    return IRQ_HANDLED;
}

static int __init spying_init(void) {
    printk(KERN_INFO "(spying) Entered");
    timer_setup(&timer, timer_callback, 0);
    add_timer(&timer);
    mod_timer(&timer, jiffies + msecs_to_jiffies(DELAY_MSECS));
    return request_irq(PS2_IRQ, irq_handler, IRQF_SHARED, "spying_ps2_irq", (void*)irq_handler);
}

static void __exit spying_exit(void) {
    if(free_irq(PS2_IRQ, (void*)irq_handler)) {
        printk(KERN_ERR "(spying) Failed to free IRQ");
    }
    del_timer_sync(&timer);
    printk(KERN_INFO "(spying) Exited");
}

module_init(spying_init);
module_exit(spying_exit);
