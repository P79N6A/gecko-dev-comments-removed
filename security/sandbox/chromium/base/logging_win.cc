



#include "base/logging_win.h"
#include "base/memory/singleton.h"
#include <initguid.h>  

namespace logging {

using base::win::EtwEventLevel;
using base::win::EtwMofEvent;

DEFINE_GUID(kLogEventId,
    0x7fe69228, 0x633e, 0x4f06, 0x80, 0xc1, 0x52, 0x7f, 0xea, 0x23, 0xe3, 0xa7);

LogEventProvider::LogEventProvider() : old_log_level_(LOG_NONE) {
}

LogEventProvider* LogEventProvider::GetInstance() {
  return Singleton<LogEventProvider,
                   StaticMemorySingletonTraits<LogEventProvider> >::get();
}

bool LogEventProvider::LogMessage(logging::LogSeverity severity,
    const char* file, int line, size_t message_start,
    const std::string& message) {
  EtwEventLevel level = TRACE_LEVEL_NONE;

  
  if (severity >= 0) {
    switch (severity) {
      case LOG_INFO:
        level = TRACE_LEVEL_INFORMATION;
        break;
      case LOG_WARNING:
        level = TRACE_LEVEL_WARNING;
        break;
      case LOG_ERROR:
      case LOG_ERROR_REPORT:
        level = TRACE_LEVEL_ERROR;
        break;
      case LOG_FATAL:
        level = TRACE_LEVEL_FATAL;
        break;
    }
  } else {  
    level = TRACE_LEVEL_INFORMATION - severity;
  }

  
  
  LogEventProvider* provider = LogEventProvider::GetInstance();
  if (provider == NULL || level > provider->enable_level())
    return false;

  
  if (provider->enable_flags() & ENABLE_LOG_MESSAGE_ONLY) {
    EtwMofEvent<1> event(kLogEventId, LOG_MESSAGE, level);
    event.SetField(0, message.length() + 1 - message_start,
        message.c_str() + message_start);

    provider->Log(event.get());
  } else {
    const size_t kMaxBacktraceDepth = 32;
    void* backtrace[kMaxBacktraceDepth];
    DWORD depth = 0;

    
    
    if (provider->enable_flags() & ENABLE_STACK_TRACE_CAPTURE)
      depth = CaptureStackBackTrace(2, kMaxBacktraceDepth, backtrace, NULL);

    EtwMofEvent<5> event(kLogEventId, LOG_MESSAGE_FULL, level);
    if (file == NULL)
      file = "";

    
    event.SetField(0, sizeof(depth), &depth);
    event.SetField(1, sizeof(backtrace[0]) * depth, &backtrace);
    
    event.SetField(2, sizeof(line), &line);
    
    event.SetField(3, strlen(file) + 1, file);
    
    event.SetField(4, message.length() + 1 - message_start,
        message.c_str() + message_start);

    provider->Log(event.get());
  }

  
  if (severity < provider->old_log_level_)
    return true;

  return false;
}

void LogEventProvider::Initialize(const GUID& provider_name) {
  LogEventProvider* provider = LogEventProvider::GetInstance();

  provider->set_provider_name(provider_name);
  provider->Register();

  
  SetLogMessageHandler(LogMessage);
}

void LogEventProvider::Uninitialize() {
  LogEventProvider::GetInstance()->Unregister();
}

void LogEventProvider::OnEventsEnabled() {
  
  old_log_level_ = GetMinLogLevel();

  
  
  EtwEventLevel level = enable_level();
  if (level == TRACE_LEVEL_NONE || level == TRACE_LEVEL_FATAL) {
    SetMinLogLevel(LOG_FATAL);
  } else if (level == TRACE_LEVEL_ERROR) {
    SetMinLogLevel(LOG_ERROR);
  } else if (level == TRACE_LEVEL_WARNING) {
    SetMinLogLevel(LOG_WARNING);
  } else if (level == TRACE_LEVEL_INFORMATION) {
    SetMinLogLevel(LOG_INFO);
  } else if (level >= TRACE_LEVEL_VERBOSE) {
    
    SetMinLogLevel(TRACE_LEVEL_INFORMATION - level);
  }
}

void LogEventProvider::OnEventsDisabled() {
  
  SetMinLogLevel(old_log_level_);
}

}  
