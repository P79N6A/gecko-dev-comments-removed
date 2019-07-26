









#include "webrtc/system_wrappers/interface/logging.h"

#include <string.h>

#include <sstream>

#include "webrtc/common_types.h"
#include "webrtc/system_wrappers/interface/trace.h"

namespace webrtc {
namespace {

TraceLevel WebRtcSeverity(LoggingSeverity sev) {
  switch (sev) {
    
    case LS_SENSITIVE:  return kTraceInfo;
    case LS_VERBOSE:    return kTraceInfo;
    case LS_INFO:       return kTraceTerseInfo;
    case LS_WARNING:    return kTraceWarning;
    case LS_ERROR:      return kTraceError;
    default:            return kTraceNone;
  }
}

const char* DescribeFile(const char* file) {
  const char* end1 = ::strrchr(file, '/');
  const char* end2 = ::strrchr(file, '\\');
  if (!end1 && !end2)
    return file;
  else
    return (end1 > end2) ? end1 + 1 : end2 + 1;
}

}  

LogMessage::LogMessage(const char* file, int line, LoggingSeverity sev)
    : severity_(sev) {
  print_stream_ << "(" << DescribeFile(file) << ":" << line << "): ";
}

bool LogMessage::Loggable(LoggingSeverity sev) {
  
  return WebRtcSeverity(sev) & Trace::level_filter() ? true : false;
}

LogMessage::~LogMessage() {
  const std::string& str = print_stream_.str();
  Trace::Add(WebRtcSeverity(severity_), kTraceUndefined, 0, str.c_str());
}

}  
