#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>

#include "phonebook.h"

extern int
phonebook_open(struct inode*, struct file*);

extern int
phonebook_release(struct inode*, struct file*);

extern ssize_t
phonebook_read(struct file*, char*, size_t, loff_t*);

extern ssize_t
phonebook_write(struct file*, const char*, size_t, loff_t*);


static int
copy_str_to_kernel_space(const char* us_data, char** ks_data, size_t len) {
  if (len == 0) {
    *ks_data = NULL;
    return 0;
  }

  *ks_data = kmalloc(len + 1, GFP_KERNEL); 
  
  if (*ks_data == NULL) {
    return -EFAULT;
  }
  
  if (copy_from_user(*ks_data, us_data, len) != 0) {
    return -EFAULT;
  }
 
  (*ks_data)[len] = '\0';

  return 0;
}

static int
copy_contact_to_kernel_space(const user_data* us_data, user_data* ks_data) {
  if (copy_from_user(ks_data, us_data, sizeof(user_data)) != 0) {
    return -EFAULT;
  }
  
  char* kernel_buf;

  if (copy_str_to_kernel_space(us_data->name, &kernel_buf,
                               us_data->name_len) != 0) {
    return -EFAULT;
  }
  ks_data->name = kernel_buf;
  
  if (copy_str_to_kernel_space(us_data->surname, &kernel_buf,
                               us_data->surname_len) != 0) {
    return -EFAULT;
  }
  ks_data->surname = kernel_buf;

  if (copy_str_to_kernel_space(us_data->phone, &kernel_buf,
                               us_data->phone_len) != 0) {
    return -EFAULT;
  }
  ks_data->phone = kernel_buf;

  if (copy_str_to_kernel_space(us_data->email, &kernel_buf,
                               us_data->email_len) != 0) {
    return -EFAULT;
  }
  ks_data->email = kernel_buf;
  
  return 0;
}

SYSCALL_DEFINE3(get_user, const char __user*, surname, unsigned, len,
                user_data __user*, output) {
  char* surname_kernel;
  if (copy_str_to_kernel_space(surname, &surname_kernel, len) != 0) {
    printk(KERN_ALERT "Unable to copy data to kernel space");
    return -1;
  }

  user_data input_kernel = {
    .surname = surname_kernel,
    .surname_len = len,
  };
  
  pb_command_t command = {
    .action = PB_READ,
    .data = &input_kernel, 
  };

  phonebook_open(NULL, NULL);
  if (phonebook_write(NULL,
                      (const char*)&command,
                      sizeof(pb_command_t),
                      NULL) < 0) {
    phonebook_release(NULL, NULL);
    return -1;
  }

  user_data output_kernel;

  if (phonebook_read(NULL,
                     (char*)&output_kernel,
                     sizeof(user_data),
                     NULL) < 0) {
    phonebook_release(NULL, NULL);
    return -1;
  }
  
  if (copy_to_user(output->name, output_kernel.name,
                   output_kernel.name_len) != 0) {
    phonebook_release(NULL, NULL);
    printk(KERN_ALERT "Unable to copy data to user space");
    return -1;
  }
  output->name_len = output_kernel.name_len;
  output->name[output_kernel.name_len] = '\0';
  
  if (copy_to_user(output->surname, output_kernel.surname,
                   output_kernel.surname_len) != 0) {
    phonebook_release(NULL, NULL);
    printk(KERN_ALERT "Unable to copy data to user space");
    return -1;
  }
  output->surname_len = output_kernel.surname_len;
  output->surname[output_kernel.surname_len] = '\0';
  
  output->age = output_kernel.age;
  
  if (copy_to_user(output->phone, output_kernel.phone,
                   output_kernel.phone_len) != 0) {
    phonebook_release(NULL, NULL);
    printk(KERN_ALERT "Unable to copy data to user space");
    return -1;
  }
  output->phone_len = output_kernel.phone_len;
  output->phone[output_kernel.phone_len] = '\0';
  
  if (copy_to_user(output->email, output_kernel.email,
                   output_kernel.email_len) != 0) {
    printk(KERN_ALERT "Unable to copy data to user space");
    phonebook_release(NULL, NULL);
    return -1;
  }
  output->email_len = output_kernel.email_len;
  output->email[output_kernel.email_len] = '\0';
  
  phonebook_release(NULL, NULL);

  return 0;
}

SYSCALL_DEFINE1(add_user, const user_data __user*, input) {
  user_data* input_kernel = kmalloc(sizeof(user_data), GFP_KERNEL);

  if (copy_contact_to_kernel_space(input, input_kernel) != 0) {
    printk(KERN_ALERT "Unable to copy data to kernel space");
    return -1;
  }
  
  pb_command_t command = {
    .action = PB_INSERT,
    .data = input_kernel,                      
  };
  
  phonebook_open(NULL, NULL);
  if (phonebook_write(NULL,
                      (const char*)&command,
                      sizeof(pb_command_t),
                      NULL) < 0) {
    phonebook_release(NULL, NULL);
    return -1;
  }
  phonebook_release(NULL, NULL);

  return 0;
}

SYSCALL_DEFINE2(del_user, const char __user*, surname, unsigned, len) {
  char* surname_kernel;
  if (copy_str_to_kernel_space(surname, &surname_kernel, len) != 0) {
    printk(KERN_ALERT "Unable to copy data to kernel space");
    return -1;
  }

  user_data input_kernel = {
    .surname = surname_kernel,
    .surname_len = len,
  };
  
  pb_command_t command = {
    .action = PB_REMOVE,
    .data = &input_kernel, 
  };

  phonebook_open(NULL, NULL);
  if (phonebook_write(NULL,
                      (const char*)&command,
                      sizeof(pb_command_t),
                      NULL) < 0) {
    phonebook_release(NULL, NULL);
    return -1;
  }
  phonebook_release(NULL, NULL);
  
  return 0;
}
