

















































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

  std::ostream& stream() { return print_stream_; }

 private:
  
  std::ostringstream print_stream_;

  
  LoggingSeverity severity_;
};





#ifndef LOG
#if defined(WEBRTC_LOGGING)








class LogMessageVoidify {
 public:
  LogMessageVoidify() { }
  
  
  void operator&(std::ostream&) { }
};

#define LOG(sev) \
    webrtc::LogMessage(__FILE__, __LINE__, webrtc::sev).stream()



#define LOG_V(sev) \
    webrtc::LogMessage(__FILE__, __LINE__, sev).stream()


#if (defined(__GNUC__) && defined(_DEBUG)) || defined(WANT_PRETTY_LOG_F)
#define LOG_F(sev) LOG(sev) << __PRETTY_FUNCTION__ << ": "
#else
#define LOG_F(sev) LOG(sev) << __FUNCTION__ << ": "
#endif

#else  




#define LOG(sev) \
  while (false)webrtc::LogMessage(NULL, 0, webrtc::sev).stream()
#define LOG_V(sev) \
  while (false) webrtc::LogMessage(NULL, 0, sev).stream()
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

#endif  

}  

#endif  
