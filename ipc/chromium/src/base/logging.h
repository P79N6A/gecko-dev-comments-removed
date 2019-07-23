



#ifndef BASE_LOGGING_H_
#define BASE_LOGGING_H_

#include <string>
#include <cstring>

#include "base/basictypes.h"

#ifdef CHROMIUM_MOZILLA_BUILD

#include "prlog.h"




#ifdef PR_LOGGING

#define ERROR 0

typedef int LogSeverity;
const LogSeverity LOG_INFO = PR_LOG_DEBUG;
const LogSeverity LOG_WARNING = PR_LOG_WARNING;
const LogSeverity LOG_ERROR = PR_LOG_ERROR;
const LogSeverity LOG_ERROR_REPORT = PR_LOG_ERROR;
const LogSeverity LOG_FATAL = PR_LOG_ERROR;
const LogSeverity LOG_0 = LOG_ERROR;

struct ChromiumLogger
{
public:
  ChromiumLogger(int severity)
    : mSeverity(static_cast<PRLogModuleLevel>(severity))
    , mMsg(NULL)
  { }
  ~ChromiumLogger();

  
  void printf(const char* fmt, ...) const;

private:
  static PRLogModuleInfo* gChromiumPRLog;

  PRLogModuleLevel mSeverity;
  mutable char* mMsg;

  DISALLOW_EVIL_CONSTRUCTORS(ChromiumLogger);
};

struct Voidifier
{
  Voidifier() { }
  
  void operator&(const ChromiumLogger&) { }
};

const ChromiumLogger& operator<<(const ChromiumLogger& log, const char* s);
const ChromiumLogger& operator<<(const ChromiumLogger& log, const std::string& s);
const ChromiumLogger& operator<<(const ChromiumLogger& log, int i);
const ChromiumLogger& operator<<(const ChromiumLogger& log, const std::wstring& s);
const ChromiumLogger& operator<<(const ChromiumLogger& log, void* p);

#define LOG(info) ChromiumLogger(LOG_ ## info) << __FILE__ ":" << __LINE__
#define LOG_IF(info, condition) \
  !(condition) ? (void) 0 : Voidifier() & ChromiumLogger(LOG_ ## info) << __FILE__ ":" << __LINE__
#define CHECK(condition) \
  !(condition) ? (void) 0 : Voidifier() & LOG(FATAL)
#define DLOG(info) LOG(info)
#define DLOG_IF(info) LOG_IF(info)
#define DCHECK(condition) CHECK(condition)

#else

struct EmptyLog
{
};

template<class T>
const EmptyLog& operator <<(const EmptyLog& log, const T&) { return log; }

#define LOG(info) EmptyLog()
#define LOG_IF(info, condition) EmptyLog()
#define CHECK(condition) while ((condition) && false) EmptyLog()
#define DLOG(info) EmptyLog()
#define DLOG_IF(info, condition) EmptyLog()
#define DCHECK(condition) while (false && (condition)) EmptyLog()

#endif 

#define LOG_ASSERT(cond) CHECK(ERROR)
#define DLOG_ASSERT(cond) DCHECK(ERROR)

#define NOTREACHED() LOG(ERROR)
#define NOTIMPLEMENTED() LOG(ERROR)

#define DCHECK_EQ(v1, v2) DCHECK((v1) == (v2))
#define DCHECK_NE(v1, v2) DCHECK((v1) != (v2))
#define DCHECK_LE(v1, v2) DCHECK((v1) <= (v2))
#define DCHECK_LT(v1, v2) DCHECK((v1) < (v2))
#define DCHECK_GE(v1, v2) DCHECK((v1) >= (v2))
#define DCHECK_GT(v1, v2) DCHECK((v1) > (v2))

#ifdef assert
#undef assert
#endif
#define assert DLOG_ASSERT

#else

#include <sstream>















































































namespace logging {




enum LoggingDestination { LOG_NONE,
                          LOG_ONLY_TO_FILE,
                          LOG_ONLY_TO_SYSTEM_DEBUG_LOG,
                          LOG_TO_BOTH_FILE_AND_SYSTEM_DEBUG_LOG };









enum LogLockingState { LOCK_LOG_FILE, DONT_LOCK_LOG_FILE };



enum OldFileDeletionState { DELETE_OLD_LOG_FILE, APPEND_TO_OLD_LOG_FILE };











#if defined(OS_WIN)
void InitLogging(const wchar_t* log_file, LoggingDestination logging_dest,
                 LogLockingState lock_log, OldFileDeletionState delete_old);
#elif defined(OS_POSIX)

void InitLogging(const char* log_file, LoggingDestination logging_dest,
                 LogLockingState lock_log, OldFileDeletionState delete_old);
#endif





void SetMinLogLevel(int level);


int GetMinLogLevel();





void SetLogFilterPrefix(const char* filter);





void SetLogItems(bool enable_process_id, bool enable_thread_id,
                 bool enable_timestamp, bool enable_tickcount);





typedef void (*LogAssertHandlerFunction)(const std::string& str);
void SetLogAssertHandler(LogAssertHandlerFunction handler);




typedef void (*LogReportHandlerFunction)(const std::string& str);
void SetLogReportHandler(LogReportHandlerFunction handler);

typedef int LogSeverity;
const LogSeverity LOG_INFO = 0;
const LogSeverity LOG_WARNING = 1;
const LogSeverity LOG_ERROR = 2;
const LogSeverity LOG_ERROR_REPORT = 3;
const LogSeverity LOG_FATAL = 4;
const LogSeverity LOG_NUM_SEVERITIES = 5;


#ifdef NDEBUG
const LogSeverity LOG_DFATAL_LEVEL = LOG_ERROR_REPORT;
#else
const LogSeverity LOG_DFATAL_LEVEL = LOG_FATAL;
#endif




#define COMPACT_GOOGLE_LOG_INFO \
  logging::LogMessage(__FILE__, __LINE__)
#define COMPACT_GOOGLE_LOG_WARNING \
  logging::LogMessage(__FILE__, __LINE__, logging::LOG_WARNING)
#define COMPACT_GOOGLE_LOG_ERROR \
  logging::LogMessage(__FILE__, __LINE__, logging::LOG_ERROR)
#define COMPACT_GOOGLE_LOG_ERROR_REPORT \
  logging::LogMessage(__FILE__, __LINE__, logging::LOG_ERROR_REPORT)
#define COMPACT_GOOGLE_LOG_FATAL \
  logging::LogMessage(__FILE__, __LINE__, logging::LOG_FATAL)
#define COMPACT_GOOGLE_LOG_DFATAL \
  logging::LogMessage(__FILE__, __LINE__, logging::LOG_DFATAL_LEVEL)






#define ERROR 0
#define COMPACT_GOOGLE_LOG_0 \
  logging::LogMessage(__FILE__, __LINE__, logging::LOG_ERROR)










#define LOG(severity) COMPACT_GOOGLE_LOG_ ## severity.stream()
#define SYSLOG(severity) LOG(severity)

#define LOG_IF(severity, condition) \
  !(condition) ? (void) 0 : logging::LogMessageVoidify() & LOG(severity)
#define SYSLOG_IF(severity, condition) LOG_IF(severity, condition)

#define LOG_ASSERT(condition)  \
  LOG_IF(FATAL, !(condition)) << "Assert failed: " #condition ". "
#define SYSLOG_ASSERT(condition) \
  SYSLOG_IF(FATAL, !(condition)) << "Assert failed: " #condition ". "




#define CHECK(condition) \
  LOG_IF(FATAL, !(condition)) << "Check failed: " #condition ". "



struct CheckOpString {
  CheckOpString(std::string* str) : str_(str) { }
  
  
  operator bool() const { return str_ != NULL; }
  std::string* str_;
};




template<class t1, class t2>
std::string* MakeCheckOpString(const t1& v1, const t2& v2, const char* names) {
  std::ostringstream ss;
  ss << names << " (" << v1 << " vs. " << v2 << ")";
  std::string* msg = new std::string(ss.str());
  return msg;
}

extern std::string* MakeCheckOpStringIntInt(int v1, int v2, const char* names);

template<int, int>
std::string* MakeCheckOpString(const int& v1,
                               const int& v2,
                               const char* names) {
  return MakeCheckOpStringIntInt(v1, v2, names);
}










#ifdef OFFICIAL_BUILD



#define DLOG(severity) \
  true ? (void) 0 : logging::LogMessageVoidify() & LOG(severity)

#define DLOG_IF(severity, condition) \
  true ? (void) 0 : logging::LogMessageVoidify() & LOG(severity)

#define DLOG_ASSERT(condition) \
  true ? (void) 0 : LOG_ASSERT(condition)

enum { DEBUG_MODE = 0 };







#define NDEBUG_EAT_STREAM_PARAMETERS \
  logging::LogMessage(__FILE__, __LINE__).stream()

#define DCHECK(condition) \
  while (false && (condition)) NDEBUG_EAT_STREAM_PARAMETERS

#define DCHECK_EQ(val1, val2) \
  while (false && (val1) == (val2)) NDEBUG_EAT_STREAM_PARAMETERS

#define DCHECK_NE(val1, val2) \
  while (false && (val1) == (val2)) NDEBUG_EAT_STREAM_PARAMETERS

#define DCHECK_LE(val1, val2) \
  while (false && (val1) == (val2)) NDEBUG_EAT_STREAM_PARAMETERS

#define DCHECK_LT(val1, val2) \
  while (false && (val1) == (val2)) NDEBUG_EAT_STREAM_PARAMETERS

#define DCHECK_GE(val1, val2) \
  while (false && (val1) == (val2)) NDEBUG_EAT_STREAM_PARAMETERS

#define DCHECK_GT(val1, val2) \
  while (false && (val1) == (val2)) NDEBUG_EAT_STREAM_PARAMETERS

#define DCHECK_STREQ(str1, str2) \
  while (false && (str1) == (str2)) NDEBUG_EAT_STREAM_PARAMETERS

#define DCHECK_STRCASEEQ(str1, str2) \
  while (false && (str1) == (str2)) NDEBUG_EAT_STREAM_PARAMETERS

#define DCHECK_STRNE(str1, str2) \
  while (false && (str1) == (str2)) NDEBUG_EAT_STREAM_PARAMETERS

#define DCHECK_STRCASENE(str1, str2) \
  while (false && (str1) == (str2)) NDEBUG_EAT_STREAM_PARAMETERS

#else
#ifndef NDEBUG


#define DLOG(severity) LOG(severity)
#define DLOG_IF(severity, condition) LOG_IF(severity, condition)
#define DLOG_ASSERT(condition) LOG_ASSERT(condition)


enum { DEBUG_MODE = 1 };
#define DCHECK(condition) \
  LOG_IF(FATAL, !(condition)) << "Check failed: " #condition ". "



#define DCHECK_OP(name, op, val1, val2)  \
  if (logging::CheckOpString _result = \
      logging::Check##name##Impl((val1), (val2), #val1 " " #op " " #val2)) \
    logging::LogMessage(__FILE__, __LINE__, _result).stream()



#define DECLARE_DCHECK_STROP_IMPL(func, expected) \
  std::string* Check##func##expected##Impl(const char* s1, \
                                           const char* s2, \
                                           const char* names);
DECLARE_DCHECK_STROP_IMPL(strcmp, true)
DECLARE_DCHECK_STROP_IMPL(strcmp, false)
DECLARE_DCHECK_STROP_IMPL(_stricmp, true)
DECLARE_DCHECK_STROP_IMPL(_stricmp, false)
#undef DECLARE_DCHECK_STROP_IMPL



#define DCHECK_STROP(func, op, expected, s1, s2) \
  while (CheckOpString _result = \
      logging::Check##func##expected##Impl((s1), (s2), \
                                           #s1 " " #op " " #s2)) \
    LOG(FATAL) << *_result.str_








#define DCHECK_STREQ(s1, s2) DCHECK_STROP(strcmp, ==, true, s1, s2)
#define DCHECK_STRNE(s1, s2) DCHECK_STROP(strcmp, !=, false, s1, s2)
#define DCHECK_STRCASEEQ(s1, s2) DCHECK_STROP(_stricmp, ==, true, s1, s2)
#define DCHECK_STRCASENE(s1, s2) DCHECK_STROP(_stricmp, !=, false, s1, s2)

#define DCHECK_INDEX(I,A) DCHECK(I < (sizeof(A)/sizeof(A[0])))
#define DCHECK_BOUND(B,A) DCHECK(B <= (sizeof(A)/sizeof(A[0])))

#else  


#define DLOG(severity) \
  true ? (void) 0 : logging::LogMessageVoidify() & LOG(severity)

#define DLOG_IF(severity, condition) \
  true ? (void) 0 : logging::LogMessageVoidify() & LOG(severity)

#define DLOG_ASSERT(condition) \
  true ? (void) 0 : LOG_ASSERT(condition)

enum { DEBUG_MODE = 0 };





#define NDEBUG_EAT_STREAM_PARAMETERS \
  logging::LogMessage(__FILE__, __LINE__).stream()


extern bool g_enable_dcheck;
#define DCHECK(condition) \
    !logging::g_enable_dcheck ? void (0) : \
        LOG_IF(ERROR_REPORT, !(condition)) << "Check failed: " #condition ". "



#define DCHECK_OP(name, op, val1, val2)  \
  if (logging::g_enable_dcheck) \
    if (logging::CheckOpString _result = \
        logging::Check##name##Impl((val1), (val2), #val1 " " #op " " #val2)) \
      logging::LogMessage(__FILE__, __LINE__, logging::LOG_ERROR_REPORT, \
                          _result).stream()

#define DCHECK_STREQ(str1, str2) \
  while (false) NDEBUG_EAT_STREAM_PARAMETERS

#define DCHECK_STRCASEEQ(str1, str2) \
  while (false) NDEBUG_EAT_STREAM_PARAMETERS

#define DCHECK_STRNE(str1, str2) \
  while (false) NDEBUG_EAT_STREAM_PARAMETERS

#define DCHECK_STRCASENE(str1, str2) \
  while (false) NDEBUG_EAT_STREAM_PARAMETERS

#endif  





#define DEFINE_DCHECK_OP_IMPL(name, op) \
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
DEFINE_DCHECK_OP_IMPL(EQ, ==)
DEFINE_DCHECK_OP_IMPL(NE, !=)
DEFINE_DCHECK_OP_IMPL(LE, <=)
DEFINE_DCHECK_OP_IMPL(LT, < )
DEFINE_DCHECK_OP_IMPL(GE, >=)
DEFINE_DCHECK_OP_IMPL(GT, > )
#undef DEFINE_DCHECK_OP_IMPL



















#define DCHECK_EQ(val1, val2) DCHECK_OP(EQ, ==, val1, val2)
#define DCHECK_NE(val1, val2) DCHECK_OP(NE, !=, val1, val2)
#define DCHECK_LE(val1, val2) DCHECK_OP(LE, <=, val1, val2)
#define DCHECK_LT(val1, val2) DCHECK_OP(LT, < , val1, val2)
#define DCHECK_GE(val1, val2) DCHECK_OP(GE, >=, val1, val2)
#define DCHECK_GT(val1, val2) DCHECK_OP(GT, > , val1, val2)

#endif  

#define NOTREACHED() DCHECK(false)


#undef assert
#define assert(x) DLOG_ASSERT(x)









class LogMessage {
 public:
  LogMessage(const char* file, int line, LogSeverity severity, int ctr);

  
  
  
  
  
  
  
  
  LogMessage(const char* file, int line);

  
  
  
  
  
  LogMessage(const char* file, int line, LogSeverity severity);

  
  
  LogMessage(const char* file, int line, const CheckOpString& result);

  
  
  LogMessage(const char* file, int line, LogSeverity severity,
             const CheckOpString& result);

  ~LogMessage();

  std::ostream& stream() { return stream_; }

 private:
  void Init(const char* file, int line);

  LogSeverity severity_;
  std::ostringstream stream_;
  size_t message_start_;  
                          
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





void CloseLogFile();

}  







std::ostream& operator<<(std::ostream& out, const wchar_t* wstr);
inline std::ostream& operator<<(std::ostream& out, const std::wstring& wstr) {
  return out << wstr.c_str();
}












#ifndef NOTIMPLEMENTED_POLICY

#define NOTIMPLEMENTED_POLICY 4
#endif

#if defined(COMPILER_GCC)


#define NOTIMPLEMENTED_MSG "Not implemented reached in " << __PRETTY_FUNCTION__
#else
#define NOTIMPLEMENTED_MSG "NOT IMPLEMENTED"
#endif

#if NOTIMPLEMENTED_POLICY == 0
#define NOTIMPLEMENTED() ;
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
  static int count = 0;\
  LOG_IF(ERROR, 0 == count++) << NOTIMPLEMENTED_MSG;\
} while(0)
#endif

#endif 

#endif  
