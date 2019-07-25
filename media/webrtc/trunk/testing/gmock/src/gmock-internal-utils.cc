




































#include "gmock/internal/gmock-internal-utils.h"

#include <ctype.h>
#include <ostream>  
#include <string>
#include "gmock/gmock.h"
#include "gmock/internal/gmock-port.h"
#include "gtest/gtest.h"

namespace testing {
namespace internal {





string ConvertIdentifierNameToWords(const char* id_name) {
  string result;
  char prev_char = '\0';
  for (const char* p = id_name; *p != '\0'; prev_char = *(p++)) {
    
    
    const bool starts_new_word = IsUpper(*p) ||
        (!IsAlpha(prev_char) && IsLower(*p)) ||
        (!IsDigit(prev_char) && IsDigit(*p));

    if (IsAlNum(*p)) {
      if (starts_new_word && result != "")
        result += ' ';
      result += ToLower(*p);
    }
  }
  return result;
}




class GoogleTestFailureReporter : public FailureReporterInterface {
 public:
  virtual void ReportFailure(FailureType type, const char* file, int line,
                             const string& message) {
    AssertHelper(type == FATAL ?
                 TestPartResult::kFatalFailure :
                 TestPartResult::kNonFatalFailure,
                 file,
                 line,
                 message.c_str()) = Message();
    if (type == FATAL) {
      posix::Abort();
    }
  }
};



FailureReporterInterface* GetFailureReporter() {
  
  
  
  
  
  static FailureReporterInterface* const failure_reporter =
      new GoogleTestFailureReporter();
  return failure_reporter;
}


static GTEST_DEFINE_STATIC_MUTEX_(g_log_mutex);



bool LogIsVisible(LogSeverity severity) {
  if (GMOCK_FLAG(verbose) == kInfoVerbosity) {
    
    return true;
  } else if (GMOCK_FLAG(verbose) == kErrorVerbosity) {
    
    return false;
  } else {
    
    
    return severity == WARNING;
  }
}








void Log(LogSeverity severity, const string& message,
         int stack_frames_to_skip) {
  if (!LogIsVisible(severity))
    return;

  
  MutexLock l(&g_log_mutex);

  
  

  if (severity == WARNING) {
    
    std::cout << "\nGMOCK WARNING:";
  }
  
  if (message.empty() || message[0] != '\n') {
    std::cout << "\n";
  }
  std::cout << message;
  if (stack_frames_to_skip >= 0) {
#ifdef NDEBUG
    
    const int actual_to_skip = 0;
#else
    
    
    const int actual_to_skip = stack_frames_to_skip + 1;
#endif  

    
    if (!message.empty() && *message.rbegin() != '\n') {
      std::cout << "\n";
    }
    std::cout << "Stack trace:\n"
         << ::testing::internal::GetCurrentOsStackTraceExceptTop(
             ::testing::UnitTest::GetInstance(), actual_to_skip);
  }
  std::cout << ::std::flush;
}

}  
}  
