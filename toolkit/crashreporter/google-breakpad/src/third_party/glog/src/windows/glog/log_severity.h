
































#ifndef BASE_LOG_SEVERITY_H__
#define BASE_LOG_SEVERITY_H__


#ifndef GOOGLE_GLOG_DLL_DECL
# if defined(_WIN32) && !defined(__CYGWIN__)
#   define GOOGLE_GLOG_DLL_DECL  __declspec(dllimport)
# else
#   define GOOGLE_GLOG_DLL_DECL
# endif
#endif




typedef int LogSeverity;

const int INFO = 0, WARNING = 1, ERROR = 2, FATAL = 3, NUM_SEVERITIES = 4;


#ifdef NDEBUG
#define DFATAL_LEVEL ERROR
#else
#define DFATAL_LEVEL FATAL
#endif

extern GOOGLE_GLOG_DLL_DECL const char* const LogSeverityNames[NUM_SEVERITIES];



















#ifdef NDEBUG
enum { DEBUG_MODE = 0 };
#define IF_DEBUG_MODE(x)
#else
enum { DEBUG_MODE = 1 };
#define IF_DEBUG_MODE(x) x
#endif

#endif  
