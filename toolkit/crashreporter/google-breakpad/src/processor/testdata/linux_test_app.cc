








































#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include <string>

#include "client/linux/handler/exception_handler.h"
#include "third_party/lss/linux_syscall_support.h"

namespace {


static bool callback(const char *dump_path, const char *id,
                     void *context,
                     bool succeeded) {
  if (succeeded) {
    printf("dump guid is %s\n", id);
  } else {
    printf("dump failed\n");
  }
  fflush(stdout);

  return succeeded;
}

static void CrashFunction() {
  int *i = reinterpret_cast<int*>(0x45);
  *i = 5;  
}

}  

int main(int argc, char **argv) {
  google_breakpad::ExceptionHandler eh(".", NULL, callback, NULL, true);
  if (!eh.WriteMinidump()) {
    printf("Failed to generate on-demand minidump\n");
  }
  CrashFunction();
  printf("did not crash?\n");
  return 0;
}
