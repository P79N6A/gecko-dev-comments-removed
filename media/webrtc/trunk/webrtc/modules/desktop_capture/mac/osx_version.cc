









#include <sys/utsname.h>

#include "webrtc/system_wrappers/interface/logging.h"

namespace webrtc {

namespace {

int GetDarwinVersion() {
  struct utsname uname_info;
  if (uname(&uname_info) != 0) {
    LOG(LS_ERROR) << "uname failed";
    return 0;
  }

  if (strcmp(uname_info.sysname, "Darwin") != 0)
    return 0;

  char* dot;
  int result = strtol(uname_info.release, &dot, 10);
  if (*dot != '.') {
    LOG(LS_ERROR) << "Failed to parse version";
    return 0;
  }

  return result;
}

}  

bool IsOSLionOrLater() {
  static int darwin_version = GetDarwinVersion();

  
  if (darwin_version < 6) {
    LOG_F(LS_ERROR) << "Invalid Darwin version: " << darwin_version;
    abort();
  }

  
  return darwin_version >= 11;
}

}  
