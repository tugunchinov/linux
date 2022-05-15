#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/spinlock.h>

MODULE_LICENSE("GPL");

#define DEVICE_NAME "fifo"

static int major_num = 0;
static struct class* device_class = NULL;
static DEFINE_SPINLOCK(lock);

struct proc_node {
  pid_t pid;

  char* data;
  size_t data_off;
  size_t data_len;
  size_t data_capacity;
  
  int released;
  
  struct list_head node; 
};

static struct proc_node queue = {
  .pid = -1,
  .node = LIST_HEAD_INIT(queue.node),
};

static int
fifo_open(struct inode* inode, struct file* file) {
  struct proc_node* new_node  = kmalloc(sizeof(struct proc_node), GFP_KERNEL);
  new_node->pid = current->pid;
  new_node->data = NULL;
  new_node->data_off = 0;
  new_node->data_len = 0;
  new_node->data_capacity = 0;
  new_node->released = 0;
  
  spin_lock(&lock);

  list_add_tail(&new_node->node, &queue.node);

  spin_unlock(&lock);
  
  return 0;
}

static int
fifo_release(struct inode* inode, struct file* file) {
  struct proc_node* node;

  spin_lock(&lock);
  
  list_for_each_entry(node, &queue.node, node) {
    if (node->pid == current->pid) {
      node->released = 1;

      if (node->data_len == 0) {
        list_del(&node->node);

        spin_unlock(&lock);
        
        if (node->data != NULL) {
          kfree(node->data);
        }
        kfree(node);
      } else {
        spin_unlock(&lock);
      }
      
      return 0;
    }
  }

  spin_unlock(&lock);

  printk(KERN_ERR "Not opened\n");
  return -EPERM;
}

static ssize_t
fifo_read(struct file* file, char __user* buf, size_t len, loff_t* off) {
  spin_lock(&lock);

  struct proc_node* cur_node = list_entry(queue.node.next, struct proc_node, node);
  size_t bytes_read = 0;
  
  while (bytes_read != len && cur_node != &queue) {
    size_t node_data_len = cur_node->data_len - cur_node->data_off;
    size_t read_len = min(len, node_data_len);
    
    if (copy_to_user(buf + bytes_read,
                     cur_node->data + cur_node->data_off,
                     read_len) != 0) {
      spin_unlock(&lock);
      
      printk(KERN_ERR "Unable to copy to user\n");
      return -EFAULT;
    }

    cur_node->data_off += read_len;
    bytes_read += read_len;

    struct proc_node* next = list_entry(cur_node->node.next, struct proc_node, node);

    if (cur_node->released) {
      list_del(&cur_node->node);
      kfree(cur_node->data);
      kfree(cur_node);
    }

    cur_node = next;
  }

  spin_unlock(&lock);
  
  return bytes_read;
}

static ssize_t
fifo_write(struct file* file, const char __user* buf, size_t len, loff_t* off) {
  struct proc_node* node;

  spin_lock(&lock);

  list_for_each_entry(node, &queue.node, node) {
    if (node->pid == current->pid) {
      if (node->data_capacity < node->data_len + len) {
        node->data_capacity = 2 * (node->data_capacity + len);

        char* new_data = kmalloc(node->data_capacity, GFP_KERNEL);
        memcpy(new_data, node->data, node->data_len);
        kfree(node->data);
        node->data = new_data;
      }

      if (copy_from_user(node->data + node->data_len, buf, len) != 0) {
        spin_unlock(&lock);
        
        printk(KERN_ERR "Unable to copy from user\n");
        return -EFAULT;
      }

      node->data_len += len;

      spin_unlock(&lock);

      return len;
    }
  }

  spin_unlock(&lock);

  printk(KERN_ERR "Not opened\n");
  return -EPERM;
}


static struct file_operations file_ops = {
  .owner   = THIS_MODULE,
  .read    = fifo_read,
  .write   = fifo_write,
  .open    = fifo_open,
  .release = fifo_release,
};

static int __init
fifo_init(void) {
  major_num = register_chrdev(0, DEVICE_NAME, &file_ops);

  if (major_num < 0) {
    printk(KERN_ERR "Could not register " DEVICE_NAME ": %d\n", major_num);
    return major_num;
  } else {
    printk(KERN_INFO DEVICE_NAME " module loaded with device major number %d\n",
           major_num);

    device_class = class_create(THIS_MODULE, DEVICE_NAME);
    device_create(device_class, NULL, MKDEV(major_num, 0), NULL, DEVICE_NAME);
  }
  
  return 0;
}

static void __exit
fifo_exit(void) {
  unregister_chrdev(major_num, DEVICE_NAME);
  device_destroy(device_class, MKDEV(major_num, 0));
}

module_init(fifo_init);
module_exit(fifo_exit);
