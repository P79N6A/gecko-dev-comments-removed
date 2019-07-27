









#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#if WEBRTC_WIN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif  

#if defined(WEBRTC_MAC) && !defined(WEBRTC_IOS)
#include <CoreServices/CoreServices.h>
#endif  

#include <algorithm>
#include "webrtc/base/common.h"
#include "webrtc/base/logging.h"





namespace rtc {

void Break() {
#if WEBRTC_WIN
  ::DebugBreak();
#else  
  
  
  
  raise(SIGTRAP);
#endif
  
  
}

static AssertLogger custom_assert_logger_ = NULL;

void SetCustomAssertLogger(AssertLogger logger) {
  custom_assert_logger_ = logger;
}

void LogAssert(const char* function, const char* file, int line,
               const char* expression) {
  if (custom_assert_logger_) {
    custom_assert_logger_(function, file, line, expression);
  } else {
    LOG(LS_ERROR) << file << "(" << line << ")" << ": ASSERT FAILED: "
                  << expression << " @ " << function;
  }
}

bool IsOdd(int n) {
  return (n & 0x1);
}

bool IsEven(int n) {
  return !IsOdd(n);
}

} 
