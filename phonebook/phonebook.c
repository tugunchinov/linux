#include <linux/cdev.h>
#include <linux/fs.h>

#include "pb_tree.h"

MODULE_LICENSE("GPL");

#define DEVICE_NAME "phonebook"

int
phonebook_open(struct inode*, struct file*);

int
phonebook_release(struct inode*, struct file*);

ssize_t
phonebook_read(struct file*, char*, size_t, loff_t*);

ssize_t
phonebook_write(struct file*, const char*, size_t, loff_t*);

static struct file_operations file_ops = {
  .owner = THIS_MODULE,
  .read = phonebook_read,
  .write = phonebook_write,
  .open = phonebook_open,
  .release = phonebook_release
};

static struct class* device_class = NULL;

static int major_num = 0;

static int device_open_count = 0;

static user_data* read_data = NULL;

ssize_t
phonebook_read(struct file* flip, char* buf, size_t len, loff_t* off) { 
  if (read_data != NULL) {
    user_data* out_data = (user_data*)buf;

    out_data->name = read_data->name;
    out_data->name_len = read_data->name_len;
    
    out_data->surname = read_data->surname;
    out_data->surname_len = read_data->surname_len;

    out_data->age = read_data->age;
    
    out_data->phone = read_data->phone;
    out_data->phone_len = read_data->phone_len;
    
    out_data->email = read_data->email;
    out_data->email_len = read_data->email_len;
    
  } else {
    printk(KERN_ALERT "Nothing to read");
    return -1;
  }

  return len;
}

ssize_t
phonebook_write(struct file* flip, const char* buf, size_t len, loff_t* off) {
  if (len != sizeof(pb_command_t)) {
    printk(KERN_ALERT "Incorrecct argument");
    return -1;
  }

  user_data* data = ((pb_command_t*)buf)->data;
  
  if (data == NULL) {
    printk(KERN_ALERT "Data is NULL");
    return -1;
  }

  if (data->surname == NULL) {
    printk(KERN_ALERT "Surname can't be NULL");
    return -1;
  }

  pb_action_t action = ((pb_command_t*)buf)->action;
  
  if (action == PB_INSERT) {
    if (insert_contact(data) != 0) {
      printk(KERN_ALERT "Unable to add contact");
      return -1;
    } else {
      printk(KERN_INFO "Succesfully added");
    }
  } else if (action == PB_READ) {
    user_data_node* node = find_contact(data->surname);
    if (node != NULL) {
      read_data = node->data;
    } else {
      printk(KERN_ALERT "Not found: %s\n", data->surname);
      return -1;
    }
  } else if (action == PB_REMOVE) {
    remove_contact(data->surname);
  } else {
    printk(KERN_ALERT "Undefined command");
    return -1;
  }

  return len;
}

int
phonebook_open(struct inode* inode, struct file* file) {
  if (device_open_count > 0) {
    return -EBUSY;
  }

  ++device_open_count;

  return 0;
}

int
phonebook_release(struct inode *inode, struct file *file) {
  --device_open_count;
  return 0;
}

static int __init
phonebook_init(void) {
  major_num = register_chrdev(0, DEVICE_NAME, &file_ops);

  if (major_num < 0) {
    printk(KERN_ALERT "Could not register " DEVICE_NAME ": %d\n", major_num);
    return major_num;
  } else {
    printk(KERN_INFO DEVICE_NAME " module loaded with device major number %d\n",
           major_num);

    device_class = class_create(THIS_MODULE, DEVICE_NAME);
    device_create(device_class, NULL, MKDEV(major_num, 0), NULL, DEVICE_NAME);
    
    return 0;
  }
}

static void __exit
phonebook_exit(void) {
  unregister_chrdev(major_num, DEVICE_NAME);
  printk(KERN_INFO DEVICE_NAME " module is removed. "
         "Don't forget to remove device.\n");
}

module_init(phonebook_init);
module_exit(phonebook_exit);
