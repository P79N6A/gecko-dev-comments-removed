













#include <cstdarg>
#include <cstdio>
#include <cstdlib>

#if defined(__GLIBC__) && !defined(__UCLIBC__)
#include <cxxabi.h>
#include <execinfo.h>
#endif

#if defined(WEBRTC_ANDROID)
#define LOG_TAG "rtc"
#include <android/log.h>  
#endif

#include "webrtc/base/checks.h"

#if defined(_MSC_VER)


#pragma warning(disable:4722)
#endif

namespace rtc {

void VPrintError(const char* format, va_list args) {
#if defined(WEBRTC_ANDROID)
  __android_log_vprint(ANDROID_LOG_ERROR, LOG_TAG, format, args);
#else
  vfprintf(stderr, format, args);
#endif
}

void PrintError(const char* format, ...) {
  va_list args;
  va_start(args, format);
  VPrintError(format, args);
  va_end(args);
}




void DumpBacktrace() {
#if defined(__GLIBC__) && !defined(__UCLIBC__)
  void* trace[100];
  int size = backtrace(trace, sizeof(trace) / sizeof(*trace));
  char** symbols = backtrace_symbols(trace, size);
  PrintError("\n==== C stack trace ===============================\n\n");
  if (size == 0) {
    PrintError("(empty)\n");
  } else if (symbols == NULL) {
    PrintError("(no symbols)\n");
  } else {
    for (int i = 1; i < size; ++i) {
      char mangled[201];
      if (sscanf(symbols[i], "%*[^(]%*[(]%200[^)+]", mangled) == 1) {  
        PrintError("%2d: ", i);
        int status;
        size_t length;
        char* demangled = abi::__cxa_demangle(mangled, NULL, &length, &status);
        PrintError("%s\n", demangled != NULL ? demangled : mangled);
        free(demangled);
      } else {
        
        PrintError("%s\n", symbols[i]);
      }
    }
  }
  free(symbols);
#endif
}

FatalMessage::FatalMessage(const char* file, int line) {
  Init(file, line);
}

FatalMessage::FatalMessage(const char* file, int line, std::string* result) {
  Init(file, line);
  stream_ << "Check failed: " << *result << std::endl << "# ";
  delete result;
}

NO_RETURN FatalMessage::~FatalMessage() {
  fflush(stdout);
  fflush(stderr);
  stream_ << std::endl << "#" << std::endl;
  PrintError(stream_.str().c_str());
  DumpBacktrace();
  fflush(stderr);
  abort();
}

void FatalMessage::Init(const char* file, int line) {
  stream_ << std::endl << std::endl << "#" << std::endl << "# Fatal error in "
          << file << ", line " << line << std::endl << "# ";
}


#ifndef WEBRTC_CHROMIUM_BUILD


#if !defined(COMPILER_MSVC)

template std::string* MakeCheckOpString<int, int>(
    const int&, const int&, const char* names);
template std::string* MakeCheckOpString<unsigned long, unsigned long>(
    const unsigned long&, const unsigned long&, const char* names);
template std::string* MakeCheckOpString<unsigned long, unsigned int>(
    const unsigned long&, const unsigned int&, const char* names);
template std::string* MakeCheckOpString<unsigned int, unsigned long>(
    const unsigned int&, const unsigned long&, const char* names);
template std::string* MakeCheckOpString<std::string, std::string>(
    const std::string&, const std::string&, const char* name);
#endif

#endif  

}  
