



#ifndef BASE_LOGGING_WIN_H_
#define BASE_LOGGING_WIN_H_

#include <string>

#include "base/base_export.h"
#include "base/basictypes.h"
#include "base/win/event_trace_provider.h"
#include "base/logging.h"

template <typename Type>
struct StaticMemorySingletonTraits;

namespace logging {


EXTERN_C BASE_EXPORT const GUID kLogEventId;


enum LogEnableMask {
  
  
  ENABLE_STACK_TRACE_CAPTURE = 0x0001,
  
  
  
  ENABLE_LOG_MESSAGE_ONLY = 0x0002,
};



enum LogMessageTypes {
  
  LOG_MESSAGE = 10,
  
  
  LOG_MESSAGE_WITH_STACKTRACE = 11,
  
  
  
  
  
  LOG_MESSAGE_FULL = 12,
};



class BASE_EXPORT LogEventProvider : public base::win::EtwTraceProvider {
 public:
  static LogEventProvider* GetInstance();

  static bool LogMessage(logging::LogSeverity severity, const char* file,
      int line, size_t message_start, const std::string& str);

  static void Initialize(const GUID& provider_name);
  static void Uninitialize();

 protected:
  
  virtual void OnEventsEnabled();
  virtual void OnEventsDisabled();

 private:
  LogEventProvider();

  
  
  logging::LogSeverity old_log_level_;

  friend struct StaticMemorySingletonTraits<LogEventProvider>;
  DISALLOW_COPY_AND_ASSIGN(LogEventProvider);
};

}  

#endif  
