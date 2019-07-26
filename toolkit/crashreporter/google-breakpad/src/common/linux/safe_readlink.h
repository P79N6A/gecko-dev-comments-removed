































#ifndef COMMON_LINUX_SAFE_READLINK_H_
#define COMMON_LINUX_SAFE_READLINK_H_

#include <stddef.h>

namespace google_breakpad {















bool SafeReadLink(const char* path, char* buffer, size_t buffer_size);



template <size_t N>
bool SafeReadLink(const char* path, char (&buffer)[N]) {
  return SafeReadLink(path, buffer, sizeof(buffer));
}

}  

#endif  
