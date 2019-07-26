

















































#ifndef WEBRTC_SYSTEM_WRAPPERS_INTERFACE_LOGGING_H_
#define WEBRTC_SYSTEM_WRAPPERS_INTERFACE_LOGGING_H_

#include <sstream>

namespace webrtc {













enum LoggingSeverity {
  LS_SENSITIVE, LS_VERBOSE, LS_INFO, LS_WARNING, LS_ERROR
};

class LogMessage {
 public:
  LogMessage(const char* file, int line, LoggingSeverity sev);
  ~LogMessage();

  static bool Loggable(LoggingSeverity sev);
  std::ostream& stream() { return print_stream_; }

 private:
  
  std::ostringstream print_stream_;

  
  LoggingSeverity severity_;
};





#ifndef LOG







class LogMessageVoidify {
 public:
  LogMessageVoidify() { }
  
  
  void operator&(std::ostream&) { }
};

#if defined(WEBRTC_RESTRICT_LOGGING)

#define RESTRICT_LOGGING_PRECONDITION(sev)  \
  sev < webrtc::LS_INFO ? (void) 0 :
#else
#define RESTRICT_LOGGING_PRECONDITION(sev)
#endif

#define LOG_SEVERITY_PRECONDITION(sev) \
  RESTRICT_LOGGING_PRECONDITION(sev) !(webrtc::LogMessage::Loggable(sev)) \
    ? (void) 0 \
    : webrtc::LogMessageVoidify() &

#define LOG(sev) \
  LOG_SEVERITY_PRECONDITION(webrtc::sev) \
    webrtc::LogMessage(__FILE__, __LINE__, webrtc::sev).stream()



#define LOG_V(sev) \
  LOG_SEVERITY_PRECONDITION(sev) \
    webrtc::LogMessage(__FILE__, __LINE__, sev).stream()


#if (defined(__GNUC__) && defined(_DEBUG)) || defined(WANT_PRETTY_LOG_F)
#define LOG_F(sev) LOG(sev) << __PRETTY_FUNCTION__ << ": "
#else
#define LOG_F(sev) LOG(sev) << __FUNCTION__ << ": "
#endif

#define LOG_API0() LOG_F(LS_VERBOSE)
#define LOG_API1(v1) LOG_API0() << #v1 << "=" << v1
#define LOG_API2(v1, v2) LOG_API1(v1) \
    << ", " << #v2 << "=" << v2
#define LOG_API3(v1, v2, v3) LOG_API2(v1, v2) \
    << ", " << #v3 << "=" << v3

#define LOG_FERR0(sev, func) LOG(sev) << #func << " failed"
#define LOG_FERR1(sev, func, v1) LOG_FERR0(sev, func) \
    << ": " << #v1 << "=" << v1
#define LOG_FERR2(sev, func, v1, v2) LOG_FERR1(sev, func, v1) \
    << ", " << #v2 << "=" << v2
#define LOG_FERR3(sev, func, v1, v2, v3) LOG_FERR2(sev, func, v1, v2) \
    << ", " << #v3 << "=" << v3
#define LOG_FERR4(sev, func, v1, v2, v3, v4) LOG_FERR3(sev, func, v1, v2, v3) \
    << ", " << #v4 << "=" << v4

#endif  

}  

#endif  
