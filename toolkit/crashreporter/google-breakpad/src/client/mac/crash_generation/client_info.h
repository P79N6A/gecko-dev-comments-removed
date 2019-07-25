




























#ifndef CLIENT_MAC_CRASH_GENERATION_CLIENT_INFO_H_
#define CLIENT_MAC_CRASH_GENERATION_CLIENT_INFO_H_

namespace google_breakpad {

class ClientInfo {
 public:
  explicit ClientInfo(pid_t pid) : pid_(pid) {}

  pid_t pid() const { return pid_; }

 private:
  pid_t pid_;
};

}  

#endif  
