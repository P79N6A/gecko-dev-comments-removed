


































#include <cstdio>

#include "client/windows/handler/exception_handler.h"

void callback(const std::wstring &id, void *context, bool succeeded) {
  if (succeeded) {
    printf("dump guid is %ws\n", id.c_str());
  } else {
    printf("dump failed\n");
  }
  exit(1);
}

void CrashFunction() {
  int *i = reinterpret_cast<int*>(0x45);
  *i = 5;  
}

int main(int argc, char **argv) {
  google_airbag::ExceptionHandler eh(L".", callback, NULL, true);
  CrashFunction();
  printf("did not crash?\n");
  return 0;
}
