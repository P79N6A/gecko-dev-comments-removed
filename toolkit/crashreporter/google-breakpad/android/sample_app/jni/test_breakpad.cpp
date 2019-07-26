




























#include <stdio.h>

#include "client/linux/handler/exception_handler.h"
#include "client/linux/handler/minidump_descriptor.h"

namespace {

bool DumpCallback(const google_breakpad::MinidumpDescriptor& descriptor,
                  void* context,
                  bool succeeded) {
  printf("Dump path: %s\n", descriptor.path());
  return succeeded;
}

void Crash() {
  volatile int* a = reinterpret_cast<volatile int*>(NULL);
  *a = 1;
}

}  

int main(int argc, char* argv[]) {
  google_breakpad::MinidumpDescriptor descriptor(".");
  google_breakpad::ExceptionHandler eh(descriptor, NULL, DumpCallback,
                                       NULL, true, -1);
  Crash();
  return 0;
}
