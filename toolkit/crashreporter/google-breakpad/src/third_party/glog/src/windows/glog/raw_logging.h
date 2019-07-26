






































#ifndef BASE_RAW_LOGGING_H_
#define BASE_RAW_LOGGING_H_

#include <time.h>

namespace google {

#include "glog/log_severity.h"
#include "glog/vlog_is_on.h"


#ifndef GOOGLE_GLOG_DLL_DECL
# if defined(_WIN32) && !defined(__CYGWIN__)
#   define GOOGLE_GLOG_DLL_DECL  __declspec(dllimport)
# else
#   define GOOGLE_GLOG_DLL_DECL
# endif
#endif















#define RAW_LOG(severity, ...) \
  do { \
    switch (google::severity) {  \
      case 0: \
        RAW_LOG_INFO(__VA_ARGS__); \
        break; \
      case 1: \
        RAW_LOG_WARNING(__VA_ARGS__); \
        break; \
      case 2: \
        RAW_LOG_ERROR(__VA_ARGS__); \
        break; \
      case 3: \
        RAW_LOG_FATAL(__VA_ARGS__); \
        break; \
      default: \
        break; \
    } \
  } while (0)



#if STRIP_LOG == 0
#define RAW_VLOG(verboselevel, ...) \
  do { \
    if (VLOG_IS_ON(verboselevel)) { \
      RAW_LOG_INFO(__VA_ARGS__); \
    } \
  } while (0)
#else
#define RAW_VLOG(verboselevel, ...) RawLogStub__(0, __VA_ARGS__)
#endif 

#if STRIP_LOG == 0
#define RAW_LOG_INFO(...) google::RawLog__(google::INFO, \
                                   __FILE__, __LINE__, __VA_ARGS__)
#else
#define RAW_LOG_INFO(...) google::RawLogStub__(0, __VA_ARGS__)
#endif 

#if STRIP_LOG <= 1
#define RAW_LOG_WARNING(...) google::RawLog__(google::WARNING,   \
                                      __FILE__, __LINE__, __VA_ARGS__)
#else
#define RAW_LOG_WARNING(...) google::RawLogStub__(0, __VA_ARGS__)
#endif 

#if STRIP_LOG <= 2
#define RAW_LOG_ERROR(...) google::RawLog__(google::ERROR,       \
                                    __FILE__, __LINE__, __VA_ARGS__)
#else
#define RAW_LOG_ERROR(...) google::RawLogStub__(0, __VA_ARGS__)
#endif 

#if STRIP_LOG <= 3
#define RAW_LOG_FATAL(...) google::RawLog__(google::FATAL,       \
                                    __FILE__, __LINE__, __VA_ARGS__)
#else
#define RAW_LOG_FATAL(...) \
  do { \
    google::RawLogStub__(0, __VA_ARGS__);        \
    exit(1); \
  } while (0)
#endif 






#define RAW_CHECK(condition, message)                                   \
  do {                                                                  \
    if (!(condition)) {                                                 \
      RAW_LOG(FATAL, "Check %s failed: %s", #condition, message);       \
    }                                                                   \
  } while (0)


#ifndef NDEBUG

#define RAW_DLOG(severity, ...) RAW_LOG(severity, __VA_ARGS__)
#define RAW_DCHECK(condition, message) RAW_CHECK(condition, message)

#else  

#define RAW_DLOG(severity, ...)                                 \
  while (false)                                                 \
    RAW_LOG(severity, __VA_ARGS__)
#define RAW_DCHECK(condition, message) \
  while (false) \
    RAW_CHECK(condition, message)

#endif  



static inline void RawLogStub__(int ignored, ...) {
}





GOOGLE_GLOG_DLL_DECL void RawLog__(LogSeverity severity,
                                   const char* file,
                                   int line,
                                   const char* format, ...)
   ;




GOOGLE_GLOG_DLL_DECL void RawLog__SetLastTime(const struct tm& t, int usecs);

}

#endif  
