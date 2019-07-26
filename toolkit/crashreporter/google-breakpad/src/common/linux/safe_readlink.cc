































#include <stddef.h>

#include "third_party/lss/linux_syscall_support.h"

namespace google_breakpad {

bool SafeReadLink(const char* path, char* buffer, size_t buffer_size) {
  
  
  
  
  
  ssize_t result_size = sys_readlink(path, buffer, buffer_size);
  if (result_size >= 0 && static_cast<size_t>(result_size) < buffer_size) {
    buffer[result_size] = '\0';
    return true;
  }
  return false;
}

}  
