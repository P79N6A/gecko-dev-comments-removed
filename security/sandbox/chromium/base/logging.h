



#ifndef BASE_LOGGING_H_
#define BASE_LOGGING_H_

#include <cassert>
#include <string>
#include <cstring>
#include <sstream>

#include "base/base_export.h"
#include "base/basictypes.h"
#include "base/debug/debugger.h"
#include "build/build_config.h"

























































































































namespace logging {


#if defined(OS_WIN)
typedef wchar_t PathChar;
#else
typedef char PathChar;
#endif



enum LoggingDestination {
  LOG_NONE                = 0,
  LOG_TO_FILE             = 1 << 0,
  LOG_TO_SYSTEM_DEBUG_LOG = 1 << 1,

  LOG_TO_ALL = LOG_TO_FILE | LOG_TO_SYSTEM_DEBUG_LOG,

  
  
  
#if defined(OS_WIN)
  LOG_DEFAULT = LOG_TO_FILE,
#elif defined(OS_POSIX)
  LOG_DEFAULT = LOG_TO_SYSTEM_DEBUG_LOG,
#endif
};








enum LogLockingState { LOCK_LOG_FILE, DONT_LOCK_LOG_FILE };



enum OldFileDeletionState { DELETE_OLD_LOG_FILE, APPEND_TO_OLD_LOG_FILE };

struct BASE_EXPORT LoggingSettings {
  
  
  
  
  
  
  LoggingSettings();

  LoggingDestination logging_dest;

  
  
  const PathChar* log_file;
  LogLockingState lock_log;
  OldFileDeletionState delete_old;
};





#if NDEBUG
#define BaseInitLoggingImpl BaseInitLoggingImpl_built_with_NDEBUG
#else
#define BaseInitLoggingImpl BaseInitLoggingImpl_built_without_NDEBUG
#endif




BASE_EXPORT bool BaseInitLoggingImpl(const LoggingSettings& settings);















inline bool InitLogging(const LoggingSettings& settings) {
  return BaseInitLoggingImpl(settings);
}







BASE_EXPORT void SetMinLogLevel(int level);


BASE_EXPORT int GetMinLogLevel();


BASE_EXPORT int GetVlogVerbosity();





BASE_EXPORT int GetVlogLevelHelper(const char* file_start, size_t N);

template <size_t N>
int GetVlogLevel(const char (&file)[N]) {
  return GetVlogLevelHelper(file, N);
}





BASE_EXPORT void SetLogItems(bool enable_process_id, bool enable_thread_id,
                             bool enable_timestamp, bool enable_tickcount);




BASE_EXPORT void SetShowErrorDialogs(bool enable_dialogs);





typedef void (*LogAssertHandlerFunction)(const std::string& str);
BASE_EXPORT void SetLogAssertHandler(LogAssertHandlerFunction handler);





typedef bool (*LogMessageHandlerFunction)(int severity,
    const char* file, int line, size_t message_start, const std::string& str);
BASE_EXPORT void SetLogMessageHandler(LogMessageHandlerFunction handler);
BASE_EXPORT LogMessageHandlerFunction GetLogMessageHandler();

typedef int LogSeverity;
const LogSeverity LOG_VERBOSE = -1;  


const LogSeverity LOG_INFO = 0;
const LogSeverity LOG_WARNING = 1;
const LogSeverity LOG_ERROR = 2;
const LogSeverity LOG_FATAL = 3;
const LogSeverity LOG_NUM_SEVERITIES = 4;


#ifdef NDEBUG
const LogSeverity LOG_DFATAL = LOG_ERROR;
#else
const LogSeverity LOG_DFATAL = LOG_FATAL;
#endif




#define COMPACT_GOOGLE_LOG_EX_INFO(ClassName, ...) \
  logging::ClassName(__FILE__, __LINE__, logging::LOG_INFO , ##__VA_ARGS__)
#define COMPACT_GOOGLE_LOG_EX_WARNING(ClassName, ...) \
  logging::ClassName(__FILE__, __LINE__, logging::LOG_WARNING , ##__VA_ARGS__)
#define COMPACT_GOOGLE_LOG_EX_ERROR(ClassName, ...) \
  logging::ClassName(__FILE__, __LINE__, logging::LOG_ERROR , ##__VA_ARGS__)
#define COMPACT_GOOGLE_LOG_EX_FATAL(ClassName, ...) \
  logging::ClassName(__FILE__, __LINE__, logging::LOG_FATAL , ##__VA_ARGS__)
#define COMPACT_GOOGLE_LOG_EX_DFATAL(ClassName, ...) \
  logging::ClassName(__FILE__, __LINE__, logging::LOG_DFATAL , ##__VA_ARGS__)

#define COMPACT_GOOGLE_LOG_INFO \
  COMPACT_GOOGLE_LOG_EX_INFO(LogMessage)
#define COMPACT_GOOGLE_LOG_WARNING \
  COMPACT_GOOGLE_LOG_EX_WARNING(LogMessage)
#define COMPACT_GOOGLE_LOG_ERROR \
  COMPACT_GOOGLE_LOG_EX_ERROR(LogMessage)
#define COMPACT_GOOGLE_LOG_FATAL \
  COMPACT_GOOGLE_LOG_EX_FATAL(LogMessage)
#define COMPACT_GOOGLE_LOG_DFATAL \
  COMPACT_GOOGLE_LOG_EX_DFATAL(LogMessage)

#if defined(OS_WIN)





#define ERROR 0
#define COMPACT_GOOGLE_LOG_EX_0(ClassName, ...) \
  COMPACT_GOOGLE_LOG_EX_ERROR(ClassName , ##__VA_ARGS__)
#define COMPACT_GOOGLE_LOG_0 COMPACT_GOOGLE_LOG_ERROR

const LogSeverity LOG_0 = LOG_ERROR;
#endif




#define LOG_IS_ON(severity) \
  ((::logging::LOG_ ## severity) >= ::logging::GetMinLogLevel())





#define VLOG_IS_ON(verboselevel) \
  ((verboselevel) <= ::logging::GetVlogLevel(__FILE__))



#define LAZY_STREAM(stream, condition)                                  \
  !(condition) ? (void) 0 : ::logging::LogMessageVoidify() & (stream)









#define LOG_STREAM(severity) COMPACT_GOOGLE_LOG_ ## severity.stream()

#define LOG(severity) LAZY_STREAM(LOG_STREAM(severity), LOG_IS_ON(severity))
#define LOG_IF(severity, condition) \
  LAZY_STREAM(LOG_STREAM(severity), LOG_IS_ON(severity) && (condition))

#define SYSLOG(severity) LOG(severity)
#define SYSLOG_IF(severity, condition) LOG_IF(severity, condition)


#define VLOG_STREAM(verbose_level) \
  logging::LogMessage(__FILE__, __LINE__, -verbose_level).stream()

#define VLOG(verbose_level) \
  LAZY_STREAM(VLOG_STREAM(verbose_level), VLOG_IS_ON(verbose_level))

#define VLOG_IF(verbose_level, condition) \
  LAZY_STREAM(VLOG_STREAM(verbose_level), \
      VLOG_IS_ON(verbose_level) && (condition))

#if defined (OS_WIN)
#define VPLOG_STREAM(verbose_level) \
  logging::Win32ErrorLogMessage(__FILE__, __LINE__, -verbose_level, \
    ::logging::GetLastSystemErrorCode()).stream()
#elif defined(OS_POSIX)
#define VPLOG_STREAM(verbose_level) \
  logging::ErrnoLogMessage(__FILE__, __LINE__, -verbose_level, \
    ::logging::GetLastSystemErrorCode()).stream()
#endif

#define VPLOG(verbose_level) \
  LAZY_STREAM(VPLOG_STREAM(verbose_level), VLOG_IS_ON(verbose_level))

#define VPLOG_IF(verbose_level, condition) \
  LAZY_STREAM(VPLOG_STREAM(verbose_level), \
    VLOG_IS_ON(verbose_level) && (condition))



#define LOG_ASSERT(condition)  \
  LOG_IF(FATAL, !(condition)) << "Assert failed: " #condition ". "
#define SYSLOG_ASSERT(condition) \
  SYSLOG_IF(FATAL, !(condition)) << "Assert failed: " #condition ". "

#if defined(OS_WIN)
#define PLOG_STREAM(severity) \
  COMPACT_GOOGLE_LOG_EX_ ## severity(Win32ErrorLogMessage, \
      ::logging::GetLastSystemErrorCode()).stream()
#elif defined(OS_POSIX)
#define PLOG_STREAM(severity) \
  COMPACT_GOOGLE_LOG_EX_ ## severity(ErrnoLogMessage, \
      ::logging::GetLastSystemErrorCode()).stream()
#endif

#define PLOG(severity)                                          \
  LAZY_STREAM(PLOG_STREAM(severity), LOG_IS_ON(severity))

#define PLOG_IF(severity, condition) \
  LAZY_STREAM(PLOG_STREAM(severity), LOG_IS_ON(severity) && (condition))


#define EAT_STREAM_PARAMETERS                                           \
  true ? (void) 0 : ::logging::LogMessageVoidify() & LOG_STREAM(FATAL)








#if defined(OFFICIAL_BUILD) && defined(NDEBUG) && !defined(OS_ANDROID)







#define CHECK(condition)                                                \
  !(condition) ? ::base::debug::BreakDebugger() : EAT_STREAM_PARAMETERS

#define PCHECK(condition) CHECK(condition)

#define CHECK_OP(name, op, val1, val2) CHECK((val1) op (val2))

#else

#if defined(_PREFAST_) && defined(OS_WIN)







#define CHECK(condition)                \
  __analysis_assume(!!(condition)),     \
  LAZY_STREAM(LOG_STREAM(FATAL), false) \
  << "Check failed: " #condition ". "

#define PCHECK(condition)                \
  __analysis_assume(!!(condition)),      \
  LAZY_STREAM(PLOG_STREAM(FATAL), false) \
  << "Check failed: " #condition ". "

#else  

#define CHECK(condition)                       \
  LAZY_STREAM(LOG_STREAM(FATAL), !(condition)) \
  << "Check failed: " #condition ". "

#define PCHECK(condition)                       \
  LAZY_STREAM(PLOG_STREAM(FATAL), !(condition)) \
  << "Check failed: " #condition ". "

#endif  






#define CHECK_OP(name, op, val1, val2)                          \
  if (std::string* _result =                                    \
      logging::Check##name##Impl((val1), (val2),                \
                                 #val1 " " #op " " #val2))      \
    logging::LogMessage(__FILE__, __LINE__, _result).stream()

#endif





template<class t1, class t2>
std::string* MakeCheckOpString(const t1& v1, const t2& v2, const char* names) {
  std::ostringstream ss;
  ss << names << " (" << v1 << " vs. " << v2 << ")";
  std::string* msg = new std::string(ss.str());
  return msg;
}



extern template BASE_EXPORT std::string* MakeCheckOpString<int, int>(
    const int&, const int&, const char* names);
extern template BASE_EXPORT
std::string* MakeCheckOpString<unsigned long, unsigned long>(
    const unsigned long&, const unsigned long&, const char* names);
extern template BASE_EXPORT
std::string* MakeCheckOpString<unsigned long, unsigned int>(
    const unsigned long&, const unsigned int&, const char* names);
extern template BASE_EXPORT
std::string* MakeCheckOpString<unsigned int, unsigned long>(
    const unsigned int&, const unsigned long&, const char* names);
extern template BASE_EXPORT
std::string* MakeCheckOpString<std::string, std::string>(
    const std::string&, const std::string&, const char* name);





#define DEFINE_CHECK_OP_IMPL(name, op) \
  template <class t1, class t2> \
  inline std::string* Check##name##Impl(const t1& v1, const t2& v2, \
                                        const char* names) { \
    if (v1 op v2) return NULL; \
    else return MakeCheckOpString(v1, v2, names); \
  } \
  inline std::string* Check##name##Impl(int v1, int v2, const char* names) { \
    if (v1 op v2) return NULL; \
    else return MakeCheckOpString(v1, v2, names); \
  }
DEFINE_CHECK_OP_IMPL(EQ, ==)
DEFINE_CHECK_OP_IMPL(NE, !=)
DEFINE_CHECK_OP_IMPL(LE, <=)
DEFINE_CHECK_OP_IMPL(LT, < )
DEFINE_CHECK_OP_IMPL(GE, >=)
DEFINE_CHECK_OP_IMPL(GT, > )
#undef DEFINE_CHECK_OP_IMPL

#define CHECK_EQ(val1, val2) CHECK_OP(EQ, ==, val1, val2)
#define CHECK_NE(val1, val2) CHECK_OP(NE, !=, val1, val2)
#define CHECK_LE(val1, val2) CHECK_OP(LE, <=, val1, val2)
#define CHECK_LT(val1, val2) CHECK_OP(LT, < , val1, val2)
#define CHECK_GE(val1, val2) CHECK_OP(GE, >=, val1, val2)
#define CHECK_GT(val1, val2) CHECK_OP(GT, > , val1, val2)
#define CHECK_IMPLIES(val1, val2) CHECK(!(val1) || (val2))

#if defined(NDEBUG)
#define ENABLE_DLOG 0
#else
#define ENABLE_DLOG 1
#endif

#if defined(NDEBUG) && !defined(DCHECK_ALWAYS_ON)
#define DCHECK_IS_ON 0
#else
#define DCHECK_IS_ON 1
#endif



#if ENABLE_DLOG

#define DLOG_IS_ON(severity) LOG_IS_ON(severity)
#define DLOG_IF(severity, condition) LOG_IF(severity, condition)
#define DLOG_ASSERT(condition) LOG_ASSERT(condition)
#define DPLOG_IF(severity, condition) PLOG_IF(severity, condition)
#define DVLOG_IF(verboselevel, condition) VLOG_IF(verboselevel, condition)
#define DVPLOG_IF(verboselevel, condition) VPLOG_IF(verboselevel, condition)

#else






#define DLOG_IS_ON(severity) false
#define DLOG_IF(severity, condition) EAT_STREAM_PARAMETERS
#define DLOG_ASSERT(condition) EAT_STREAM_PARAMETERS
#define DPLOG_IF(severity, condition) EAT_STREAM_PARAMETERS
#define DVLOG_IF(verboselevel, condition) EAT_STREAM_PARAMETERS
#define DVPLOG_IF(verboselevel, condition) EAT_STREAM_PARAMETERS

#endif









enum { DEBUG_MODE = ENABLE_DLOG };

#undef ENABLE_DLOG

#define DLOG(severity)                                          \
  LAZY_STREAM(LOG_STREAM(severity), DLOG_IS_ON(severity))

#define DPLOG(severity)                                         \
  LAZY_STREAM(PLOG_STREAM(severity), DLOG_IS_ON(severity))

#define DVLOG(verboselevel) DVLOG_IF(verboselevel, VLOG_IS_ON(verboselevel))

#define DVPLOG(verboselevel) DVPLOG_IF(verboselevel, VLOG_IS_ON(verboselevel))



#if DCHECK_IS_ON

#define COMPACT_GOOGLE_LOG_EX_DCHECK(ClassName, ...) \
  COMPACT_GOOGLE_LOG_EX_FATAL(ClassName , ##__VA_ARGS__)
#define COMPACT_GOOGLE_LOG_DCHECK COMPACT_GOOGLE_LOG_FATAL
const LogSeverity LOG_DCHECK = LOG_FATAL;

#else  


#define COMPACT_GOOGLE_LOG_EX_DCHECK(ClassName, ...) \
  COMPACT_GOOGLE_LOG_EX_INFO(ClassName , ##__VA_ARGS__)
#define COMPACT_GOOGLE_LOG_DCHECK COMPACT_GOOGLE_LOG_INFO
const LogSeverity LOG_DCHECK = LOG_INFO;

#endif  






#if defined(_PREFAST_) && defined(OS_WIN)


#define DCHECK(condition)                                               \
  __analysis_assume(!!(condition)),                                     \
  LAZY_STREAM(LOG_STREAM(DCHECK), false)                                \
  << "Check failed: " #condition ". "

#define DPCHECK(condition)                                              \
  __analysis_assume(!!(condition)),                                     \
  LAZY_STREAM(PLOG_STREAM(DCHECK), false)                               \
  << "Check failed: " #condition ". "

#else  

#define DCHECK(condition)                                               \
  LAZY_STREAM(LOG_STREAM(DCHECK), DCHECK_IS_ON ? !(condition) : false)  \
  << "Check failed: " #condition ". "

#define DPCHECK(condition)                                              \
  LAZY_STREAM(PLOG_STREAM(DCHECK), DCHECK_IS_ON ? !(condition) : false) \
  << "Check failed: " #condition ". "

#endif  



#define DCHECK_OP(name, op, val1, val2)                         \
  if (DCHECK_IS_ON)                                             \
    if (std::string* _result =                                  \
        logging::Check##name##Impl((val1), (val2),              \
                                   #val1 " " #op " " #val2))    \
      logging::LogMessage(                                      \
          __FILE__, __LINE__, ::logging::LOG_DCHECK,            \
          _result).stream()




















#define DCHECK_EQ(val1, val2) DCHECK_OP(EQ, ==, val1, val2)
#define DCHECK_NE(val1, val2) DCHECK_OP(NE, !=, val1, val2)
#define DCHECK_LE(val1, val2) DCHECK_OP(LE, <=, val1, val2)
#define DCHECK_LT(val1, val2) DCHECK_OP(LT, < , val1, val2)
#define DCHECK_GE(val1, val2) DCHECK_OP(GE, >=, val1, val2)
#define DCHECK_GT(val1, val2) DCHECK_OP(GT, > , val1, val2)
#define DCHECK_IMPLIES(val1, val2) DCHECK(!(val1) || (val2))

#if !DCHECK_IS_ON && defined(OS_CHROMEOS)
#define NOTREACHED() LOG(ERROR) << "NOTREACHED() hit in " << \
    __FUNCTION__ << ". "
#else
#define NOTREACHED() DCHECK(false)
#endif


#undef assert
#define assert(x) DLOG_ASSERT(x)









class BASE_EXPORT LogMessage {
 public:
  
  LogMessage(const char* file, int line, LogSeverity severity);

  
  
  LogMessage(const char* file, int line, std::string* result);

  
  LogMessage(const char* file, int line, LogSeverity severity,
             std::string* result);

  ~LogMessage();

  std::ostream& stream() { return stream_; }

 private:
  void Init(const char* file, int line);

  LogSeverity severity_;
  std::ostringstream stream_;
  size_t message_start_;  
                          
  
  const char* file_;
  const int line_;

#if defined(OS_WIN)
  
  
  
  
  
  class SaveLastError {
   public:
    SaveLastError();
    ~SaveLastError();

    unsigned long get_error() const { return last_error_; }

   protected:
    unsigned long last_error_;
  };

  SaveLastError last_error_;
#endif

  DISALLOW_COPY_AND_ASSIGN(LogMessage);
};



inline void LogAtLevel(int const log_level, std::string const &msg) {
  LogMessage(__FILE__, __LINE__, log_level).stream() << msg;
}




class LogMessageVoidify {
 public:
  LogMessageVoidify() { }
  
  
  void operator&(std::ostream&) { }
};

#if defined(OS_WIN)
typedef unsigned long SystemErrorCode;
#elif defined(OS_POSIX)
typedef int SystemErrorCode;
#endif



BASE_EXPORT SystemErrorCode GetLastSystemErrorCode();
BASE_EXPORT std::string SystemErrorCodeToString(SystemErrorCode error_code);

#if defined(OS_WIN)

class BASE_EXPORT Win32ErrorLogMessage {
 public:
  Win32ErrorLogMessage(const char* file,
                       int line,
                       LogSeverity severity,
                       SystemErrorCode err);

  
  ~Win32ErrorLogMessage();

  std::ostream& stream() { return log_message_.stream(); }

 private:
  SystemErrorCode err_;
  LogMessage log_message_;

  DISALLOW_COPY_AND_ASSIGN(Win32ErrorLogMessage);
};
#elif defined(OS_POSIX)

class BASE_EXPORT ErrnoLogMessage {
 public:
  ErrnoLogMessage(const char* file,
                  int line,
                  LogSeverity severity,
                  SystemErrorCode err);

  
  ~ErrnoLogMessage();

  std::ostream& stream() { return log_message_.stream(); }

 private:
  SystemErrorCode err_;
  LogMessage log_message_;

  DISALLOW_COPY_AND_ASSIGN(ErrnoLogMessage);
};
#endif  





BASE_EXPORT void CloseLogFile();


BASE_EXPORT void RawLog(int level, const char* message);

#define RAW_LOG(level, message) logging::RawLog(logging::LOG_ ## level, message)

#define RAW_CHECK(condition)                                                   \
  do {                                                                         \
    if (!(condition))                                                          \
      logging::RawLog(logging::LOG_FATAL, "Check failed: " #condition "\n");   \
  } while (0)

#if defined(OS_WIN)

BASE_EXPORT std::wstring GetLogFileFullPath();
#endif

}  









namespace std {






BASE_EXPORT std::ostream& operator<<(std::ostream& out, const wchar_t* wstr);
inline std::ostream& operator<<(std::ostream& out, const std::wstring& wstr) {
  return out << wstr.c_str();
}
}  












#ifndef NOTIMPLEMENTED_POLICY
#if defined(OS_ANDROID) && defined(OFFICIAL_BUILD)
#define NOTIMPLEMENTED_POLICY 0
#else

#define NOTIMPLEMENTED_POLICY 4
#endif
#endif

#if defined(COMPILER_GCC)


#define NOTIMPLEMENTED_MSG "Not implemented reached in " << __PRETTY_FUNCTION__
#else
#define NOTIMPLEMENTED_MSG "NOT IMPLEMENTED"
#endif

#if NOTIMPLEMENTED_POLICY == 0
#define NOTIMPLEMENTED() EAT_STREAM_PARAMETERS
#elif NOTIMPLEMENTED_POLICY == 1

#define NOTIMPLEMENTED() COMPILE_ASSERT(false, NOT_IMPLEMENTED)
#elif NOTIMPLEMENTED_POLICY == 2
#define NOTIMPLEMENTED() COMPILE_ASSERT(false, NOT_IMPLEMENTED)
#elif NOTIMPLEMENTED_POLICY == 3
#define NOTIMPLEMENTED() NOTREACHED()
#elif NOTIMPLEMENTED_POLICY == 4
#define NOTIMPLEMENTED() LOG(ERROR) << NOTIMPLEMENTED_MSG
#elif NOTIMPLEMENTED_POLICY == 5
#define NOTIMPLEMENTED() do {\
  static bool logged_once = false;\
  LOG_IF(ERROR, !logged_once) << NOTIMPLEMENTED_MSG;\
  logged_once = true;\
} while(0);\
EAT_STREAM_PARAMETERS
#endif

#endif  
