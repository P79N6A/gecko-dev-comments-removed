






























#ifndef COMMON_SOLARIS_MESSAGE_OUTPUT_H__
#define COMMON_SOLARIS_MESSAGE_OUTPUT_H__

namespace google_breakpad {

const int MESSAGE_MAX = 1000;




#define print_message1(std, message) \
  char buffer[MESSAGE_MAX]; \
  int len = snprintf(buffer, MESSAGE_MAX, message); \
  write(std, buffer, len)

#define print_message2(std, message, para) \
  char buffer[MESSAGE_MAX]; \
  int len = snprintf(buffer, MESSAGE_MAX, message, para); \
  write(std, buffer, len);

}  

#endif  
