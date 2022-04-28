#include <linux/slab.h>

#include "pb_tree.h"

static struct rb_root phonebook = RB_ROOT;

user_data_node*
find_contact(const char* surname) {
  struct rb_node* node = phonebook.rb_node;
  
  while (node) {
    user_data_node* this = container_of(node, user_data_node, node);

    int result = strcmp(surname, this->data->surname);
    if (result < 0) {
      node = node->rb_left;
    } else if (result > 0) {
      node = node->rb_right;
    } else {
      return this;
    }
  }

  return NULL;
}

int
insert_contact(user_data* data) {
  user_data_node* node = kmalloc(sizeof(user_data_node), GFP_KERNEL);
  if (node == NULL) {
    printk(KERN_ALERT "Unable to allocate new node.");
    return -EFAULT;
  }
  
  node->data = data;

  struct rb_node** new = &(phonebook.rb_node);
  struct rb_node* parent = NULL;
  
  while (*new) {
    user_data_node* this = container_of(*new, user_data_node, node);
    parent = *new;

    int result = strcmp(data->surname, this->data->surname);
    
    if (result < 0) {
      new = &((*new)->rb_left);
    } else if (result > 0) {
      new = &((*new)->rb_right);
    } else {
      printk(KERN_ALERT "Such contact already exists");
      return -EPERM;
    }
  }

  rb_link_node(&node->node, parent, new);
  rb_insert_color(&node->node, &phonebook);
  
  return 0;
}

void
remove_contact(const char* surname) {
  user_data_node* data = find_contact(surname);

  if (data) {
    rb_erase(&data->node, &phonebook);
    kfree(data->data);
    kfree(data);
  }
}
