

































#include <stdio.h>

#include "client/windows/handler/exception_handler.h"

namespace {

static bool callback(const wchar_t *dump_path, const wchar_t *id,
                     void *context, EXCEPTION_POINTERS *exinfo,
                     MDRawAssertionInfo *assertion,
                     bool succeeded) {
  if (succeeded) {
    printf("dump guid is %ws\n", id);
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
  google_breakpad::ExceptionHandler eh(
      L".", NULL, callback, NULL,
      google_breakpad::ExceptionHandler::HANDLER_ALL);
  CrashFunction();
  printf("did not crash?\n");
  return 0;
}
