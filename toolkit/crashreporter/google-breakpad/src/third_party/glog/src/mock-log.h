

































#ifndef GLOG_SRC_MOCK_LOG_H_
#define GLOG_SRC_MOCK_LOG_H_


#include "utilities.h"

#include <string>

#include <gmock/gmock.h>

#include "glog/logging.h"

_START_GOOGLE_NAMESPACE_
namespace glog_testing {



















class ScopedMockLog : public GOOGLE_NAMESPACE::LogSink {
 public:
  
  
  ScopedMockLog() { AddLogSink(this); }

  
  virtual ~ScopedMockLog() { RemoveLogSink(this); }

  
  
  
  
  
  
  
  
  
  
  
  
  
  MOCK_METHOD3(Log, void(GOOGLE_NAMESPACE::LogSeverity severity,
                         const std::string& file_path,
                         const std::string& message));

 private:
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual void send(GOOGLE_NAMESPACE::LogSeverity severity,
                    const char* full_filename,
                    const char* base_filename, int line, const tm* tm_time,
                    const char* message, size_t message_len) {
    
    
    message_info_.severity = severity;
    message_info_.file_path = full_filename;
    message_info_.message = std::string(message, message_len);
  }

  
  
  
  
  
  
  
  virtual void WaitTillSent() {
    
    
    
    MessageInfo message_info = message_info_;
    Log(message_info.severity, message_info.file_path, message_info.message);
  }

  
  
  struct MessageInfo {
    GOOGLE_NAMESPACE::LogSeverity severity;
    std::string file_path;
    std::string message;
  };
  MessageInfo message_info_;
};

}  
_END_GOOGLE_NAMESPACE_

#endif  
