




























#ifndef GOOGLE_BREAKPAD_CLIENT_MAC_CRASH_GENERATION_CRASH_GENERATION_CLIENT_H_
#define GOOGLE_BREAKPAD_CLIENT_MAC_CRASH_GENERATION_CRASH_GENERATION_CLIENT_H_

#include "common/mac/MachIPC.h"

namespace google_breakpad {

class CrashGenerationClient {
 public:
  explicit CrashGenerationClient(const char* mach_port_name)
    : sender_(mach_port_name) {
  }

  
  
  
  bool RequestDumpForException(int exception_type,
			       int exception_code,
			       int exception_subcode,
			       mach_port_t crashing_thread);

  bool RequestDump() {
    return RequestDumpForException(0, 0, 0, MACH_PORT_NULL);
  }

 private:
  MachPortSender sender_;

  
  CrashGenerationClient(const CrashGenerationClient&);
  CrashGenerationClient& operator=(const CrashGenerationClient&);
};

}  

#endif  
