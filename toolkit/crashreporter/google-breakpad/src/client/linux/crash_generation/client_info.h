




























#ifndef CLIENT_LINUX_CRASH_GENERATION_CLIENT_INFO_H_
#define CLIENT_LINUX_CRASH_GENERATION_CLIENT_INFO_H_

namespace google_breakpad {

class CrashGenerationServer;

class ClientInfo {
 public:
  ClientInfo(pid_t pid, CrashGenerationServer* crash_server)
    : crash_server_(crash_server),
      pid_(pid) {}

  CrashGenerationServer* crash_server() const { return crash_server_; }
  pid_t pid() const { return pid_; }

 private:
  CrashGenerationServer* crash_server_;
  pid_t pid_;
};

}

#endif 
