#include <asm-generic/io.h>

#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>

MODULE_LICENSE("GPL");

#define LISTVMA "listvma"
#define FINDPAGE "findpage"
#define WRITEVAL "writeval"

static struct proc_dir_entry* entry = NULL;

typedef enum {
  kListVMA,
  kFindPage,
  kWriteVal,
} command_t;

typedef struct {
  command_t command;
  void* addr;
  unsigned long val;
} action_t;

static action_t last_action;

static ssize_t
read(struct file* flip, char __user* buf, size_t len, loff_t* off) {
  if (last_action.command == kListVMA) {
    printk(KERN_INFO "VMA's of %d:\n", current->pid);
    
    struct vm_area_struct* head = current->mm->mmap;
    while (head) {
      printk(KERN_INFO "%p\n", head);
      head = head->vm_next;
    }
  } else if (last_action.command ==  kFindPage) {
    printk(KERN_INFO "va->pa for %d:\n", current->pid);
    printk(KERN_INFO "(va) %p -> (pa) %p", last_action.addr,
           (void*)virt_to_phys(last_action.addr));
  } else if (last_action.command == kWriteVal) {
    unsigned long val;
    if (copy_from_user(&val, last_action.addr, sizeof(unsigned long)) != 0) {
      printk(KERN_ERR "Unable to copy data from user space");
      return -EFAULT;
    }

    printk("Old val of %d: %lu (%p)", current->pid, val, last_action.addr);

    if (copy_to_user(last_action.addr, &last_action.val,
                     sizeof(unsigned long)) != 0) {
      printk(KERN_ERR "Unable to copy data to user space");
      return -EFAULT;
    }

    printk("New val of %d: %lu (%p)", current->pid, last_action.val,
           last_action.addr);

  } else {
    panic("Incorrect command");
  }

  return 0;
}

static ssize_t
write(struct file* flip, const char __user* buf, size_t len, loff_t* off) {
  char* kern_buf = kmalloc(len, GFP_KERNEL);

  if (copy_from_user(kern_buf, buf, len) != 0) {
    printk(KERN_ERR "Unable to copy data to kernel space\n");
    return -EFAULT;
  }

  if (strncmp(kern_buf, LISTVMA, strlen(LISTVMA)) == 0) {
    last_action.command = kListVMA;
  } else if (strncmp(kern_buf, FINDPAGE, strlen(FINDPAGE)) == 0) {
    last_action.command = kFindPage;

    uintptr_t addr;
    if (sscanf(kern_buf, "%*s %lx", &addr) <= 0) {
      kfree(kern_buf);
      printk(KERN_ERR "Wrong format\n");
      return -EFAULT;
    }
    last_action.addr = (void*) addr;
  } else if (strncmp(kern_buf, WRITEVAL, strlen(WRITEVAL)) == 0) {
    last_action.command = kWriteVal;

    uintptr_t addr;
    if (sscanf(kern_buf, "%*s %lx %lu", &addr, &last_action.val) <= 0) {
      kfree(kern_buf);
      printk(KERN_ERR "Wrong format\n");
      return -EFAULT;
    }
    last_action.addr = (void*) addr;
  } else {
    printk(KERN_ERR "Unknown command\n");
    return -EFAULT;
  }
  
  kfree(kern_buf);
  return len;
}

static const struct proc_ops fops = {
  .proc_read = read,
  .proc_write = write,
};

static int __init
communicator_init(void) {
  entry = proc_create("mmaneg", 0660, NULL, &fops);
  if (entry == NULL) {
    printk(KERN_ERR "Unable to create procfs entry\n");
    return -EFAULT;
  }
  
  return 0;
}

static void __exit
communicator_exit(void) {
  proc_remove(entry);
}

module_init(communicator_init);
module_exit(communicator_exit);
