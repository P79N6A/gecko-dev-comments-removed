




























#ifndef CLIENT_LINUX_CRASH_GENERATION_CLIENT_INFO_H_
#define CLIENT_LINUX_CRASH_GENERATION_CLIENT_INFO_H_

namespace google_breakpad {

class CrashGenerationServer;

struct ClientInfo {
  pid_t pid() const { return pid_; }

  CrashGenerationServer* crash_server_;
  pid_t pid_;
  char* crash_context;
  size_t crash_context_size;
};

}

#endif 
