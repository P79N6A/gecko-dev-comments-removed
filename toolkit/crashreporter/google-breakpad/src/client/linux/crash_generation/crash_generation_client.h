




























#ifndef CLIENT_LINUX_CRASH_GENERATION_CRASH_GENERATION_CLIENT_H_
#define CLIENT_LINUX_CRASH_GENERATION_CRASH_GENERATION_CLIENT_H_

#include <stddef.h>

namespace google_breakpad {

class CrashGenerationClient {
public:
  ~CrashGenerationClient()
  {
  }

  
  
  
  
  bool RequestDump(const void* blob, size_t blob_size);

  
  
  
  
  static CrashGenerationClient* TryCreate(int server_fd);

private:
  CrashGenerationClient(int server_fd) : server_fd_(server_fd)
  {
  }

  int server_fd_;

  
  CrashGenerationClient(const CrashGenerationClient&);
  CrashGenerationClient& operator=(const CrashGenerationClient&);
};

} 

#endif 
