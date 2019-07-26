































#include <limits.h>
#include <stdlib.h>

#include "client/linux/minidump_writer/minidump_writer_unittest_utils.h"
#include "common/linux/safe_readlink.h"
#include "common/using_std_string.h"

namespace google_breakpad {

string GetHelperBinary() {
  string helper_path;
  char *bindir = getenv("bindir");
  if (bindir) {
    helper_path = string(bindir) + "/";
  } else {
    
    char self_path[PATH_MAX];
    if (!SafeReadLink("/proc/self/exe", self_path)) {
      return "";
    }
    helper_path = string(self_path);
    size_t pos = helper_path.rfind('/');
    if (pos == string::npos) {
      return "";
    }
    helper_path.erase(pos + 1);
  }

  helper_path += "linux_dumper_unittest_helper";

  return helper_path;
}

}  
