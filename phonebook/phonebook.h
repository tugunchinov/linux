typedef enum {
  PB_INSERT,
  PB_READ,
  PB_REMOVE,
  PB_UNDEFINED,
} pb_action_t;

typedef struct {
  char* name;
  size_t name_len;
  char* surname;
  size_t surname_len;
  unsigned age;
  char* phone;
  size_t phone_len;
  char* email;
  size_t email_len;
} user_data;

typedef struct {
  pb_action_t action;
  user_data* data;
} pb_command_t;
