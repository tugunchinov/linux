#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/timer.h>

MODULE_LICENSE("GPL");

#define DEV_COOKIE "tracker-e1a14754-0931-48e0-9022-90773e218d95"
#define REPEAT_TIME 60000  // ms

static volatile int button_pressed = 0;
static volatile int button_released = 1;
static volatile unsigned long long typing_count = 0;

static struct timer_list timer;

void
tracker_tasklet_handler(struct tasklet_struct* unused) {
  if (button_pressed) {
    button_pressed = 0;
    button_released = 1;
  } else if (button_released) {
    button_pressed = 1;
    button_released = 0;
    ++typing_count;
  }
}

static DECLARE_TASKLET(tracker_tasklet, tracker_tasklet_handler);

void
timer_callback(struct timer_list* unused) {
  printk(KERN_INFO "Typing count: %llu\n", typing_count);
  mod_timer(&timer, jiffies + msecs_to_jiffies(REPEAT_TIME));
}

static irqreturn_t
type_handler(int irq, void* dev) {
  tasklet_schedule(&tracker_tasklet);
  
  return IRQ_HANDLED;
}

static int __init
tracker_init(void) {
  if (request_irq(1, type_handler, IRQF_SHARED, "keyboard",
                  DEV_COOKIE) < 0) {
    printk(KERN_ALERT "Unable to register interrupt handler");
    return -EIO;
  }

  timer_setup(&timer, timer_callback, 0);
  mod_timer(&timer, jiffies + msecs_to_jiffies(REPEAT_TIME));
  
  return 0;
}


static void __exit
tracker_exit(void) {
  del_timer(&timer);
  free_irq(1, DEV_COOKIE);
}


module_init(tracker_init);
module_exit(tracker_exit);
