#include <linux/rbtree.h>

#include "phonebook.h"

typedef struct {
  struct rb_node node;
  user_data* data;
} user_data_node;

user_data_node*
find_contact(const char* surname);

int
insert_contact(user_data* data);

void
remove_contact(const char* surname);
