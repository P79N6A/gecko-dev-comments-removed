


































#ifndef _LOGGING_H_
#define _LOGGING_H_

#include <errno.h>
#include <string.h>
#include <time.h>
#include <string>
#if 1
# include <unistd.h>
#endif
#ifdef __DEPRECATED

# undef __DEPRECATED
# include <strstream>
# define __DEPRECATED
#else
# include <strstream>
#endif
#include <vector>


#ifndef GOOGLE_GLOG_DLL_DECL
# if defined(_WIN32) && !defined(__CYGWIN__)
#   define GOOGLE_GLOG_DLL_DECL  __declspec(dllimport)
# else
#   define GOOGLE_GLOG_DLL_DECL
# endif
#endif







#if 1
#include <stdint.h>             
#endif
#if 1
#include <sys/types.h>          
#endif
#if 1
#include <inttypes.h>           
#endif

#if 0
#include <gflags/gflags.h>
#endif

namespace google {

#if 1      
typedef int32_t int32;
typedef uint32_t uint32;
typedef int64_t int64;
typedef uint64_t uint64;
#elif 1   
typedef int32_t int32;
typedef u_int32_t uint32;
typedef int64_t int64;
typedef u_int64_t uint64;
#elif 0    
typedef __int32 int32;
typedef unsigned __int32 uint32;
typedef __int64 int64;
typedef unsigned __int64 uint64;
#else
#error Do not know how to define a 32-bit integer quantity on your system
#endif

}










#ifndef GOOGLE_STRIP_LOG
#define GOOGLE_STRIP_LOG 0
#endif






#ifndef GOOGLE_PREDICT_BRANCH_NOT_TAKEN
#if 1
#define GOOGLE_PREDICT_BRANCH_NOT_TAKEN(x) (__builtin_expect(x, 0))
#else
#define GOOGLE_PREDICT_BRANCH_NOT_TAKEN(x) x
#endif
#endif


























































































































































#ifndef DECLARE_VARIABLE
#define MUST_UNDEF_GFLAGS_DECLARE_MACROS
#define DECLARE_VARIABLE(type, name, tn)                                      \
  namespace FLAG__namespace_do_not_use_directly_use_DECLARE_##tn##_instead {  \
  extern GOOGLE_GLOG_DLL_DECL type FLAGS_##name;                              \
  }                                                                           \
  using FLAG__namespace_do_not_use_directly_use_DECLARE_##tn##_instead::FLAGS_##name


#define DECLARE_bool(name) \
  DECLARE_VARIABLE(bool, name, bool)


#define DECLARE_int32(name) \
  DECLARE_VARIABLE(google::int32, name, int32)



#define DECLARE_string(name)                                          \
  namespace FLAG__namespace_do_not_use_directly_use_DECLARE_string_instead {  \
  extern GOOGLE_GLOG_DLL_DECL std::string FLAGS_##name;                       \
  }                                                                           \
  using FLAG__namespace_do_not_use_directly_use_DECLARE_string_instead::FLAGS_##name
#endif


DECLARE_bool(logtostderr);


DECLARE_bool(alsologtostderr);



DECLARE_int32(stderrthreshold);


DECLARE_bool(log_prefix);



DECLARE_int32(logbuflevel);


DECLARE_int32(logbufsecs);



DECLARE_int32(minloglevel);



DECLARE_string(log_dir);



DECLARE_string(log_link);

DECLARE_int32(v);  


DECLARE_int32(max_log_size);


DECLARE_bool(stop_logging_if_full_disk);

#ifdef MUST_UNDEF_GFLAGS_DECLARE_MACROS
#undef MUST_UNDEF_GFLAGS_DECLARE_MACROS
#undef DECLARE_VARIABLE
#undef DECLARE_bool
#undef DECLARE_int32
#undef DECLARE_string
#endif








#if GOOGLE_STRIP_LOG == 0
#define COMPACT_GOOGLE_LOG_INFO google::LogMessage( \
      __FILE__, __LINE__)
#define LOG_TO_STRING_INFO(message) google::LogMessage( \
      __FILE__, __LINE__, google::INFO, message)
#else
#define COMPACT_GOOGLE_LOG_INFO google::NullStream()
#define LOG_TO_STRING_INFO(message) google::NullStream()
#endif

#if GOOGLE_STRIP_LOG <= 1
#define COMPACT_GOOGLE_LOG_WARNING google::LogMessage( \
      __FILE__, __LINE__, google::WARNING)
#define LOG_TO_STRING_WARNING(message) google::LogMessage( \
      __FILE__, __LINE__, google::WARNING, message)
#else
#define COMPACT_GOOGLE_LOG_WARNING google::NullStream()
#define LOG_TO_STRING_WARNING(message) google::NullStream()
#endif

#if GOOGLE_STRIP_LOG <= 2
#define COMPACT_GOOGLE_LOG_ERROR google::LogMessage( \
      __FILE__, __LINE__, google::ERROR)
#define LOG_TO_STRING_ERROR(message) google::LogMessage( \
      __FILE__, __LINE__, google::ERROR, message)
#else
#define COMPACT_GOOGLE_LOG_ERROR google::NullStream()
#define LOG_TO_STRING_ERROR(message) google::NullStream()
#endif

#if GOOGLE_STRIP_LOG <= 3
#define COMPACT_GOOGLE_LOG_FATAL google::LogMessageFatal( \
      __FILE__, __LINE__)
#define LOG_TO_STRING_FATAL(message) google::LogMessage( \
      __FILE__, __LINE__, google::FATAL, message)
#else
#define COMPACT_GOOGLE_LOG_FATAL google::NullStreamFatal()
#define LOG_TO_STRING_FATAL(message) google::NullStreamFatal()
#endif



#ifdef NDEBUG
#define COMPACT_GOOGLE_LOG_DFATAL COMPACT_GOOGLE_LOG_ERROR
#elif GOOGLE_STRIP_LOG <= 3
#define COMPACT_GOOGLE_LOG_DFATAL LogMessage( \
      __FILE__, __LINE__, google::FATAL)
#else
#define COMPACT_GOOGLE_LOG_DFATAL google::NullStreamFatal()
#endif

#define GOOGLE_LOG_INFO(counter) google::LogMessage(__FILE__, __LINE__, google::INFO, counter, &google::LogMessage::SendToLog)
#define SYSLOG_INFO(counter) \
  google::LogMessage(__FILE__, __LINE__, google::INFO, counter, \
  &google::LogMessage::SendToSyslogAndLog)
#define GOOGLE_LOG_WARNING(counter)  \
  google::LogMessage(__FILE__, __LINE__, google::WARNING, counter, \
  &google::LogMessage::SendToLog)
#define SYSLOG_WARNING(counter)  \
  google::LogMessage(__FILE__, __LINE__, google::WARNING, counter, \
  &google::LogMessage::SendToSyslogAndLog)
#define GOOGLE_LOG_ERROR(counter)  \
  google::LogMessage(__FILE__, __LINE__, google::ERROR, counter, \
  &google::LogMessage::SendToLog)
#define SYSLOG_ERROR(counter)  \
  google::LogMessage(__FILE__, __LINE__, google::ERROR, counter, \
  &google::LogMessage::SendToSyslogAndLog)
#define GOOGLE_LOG_FATAL(counter) \
  google::LogMessage(__FILE__, __LINE__, google::FATAL, counter, \
  &google::LogMessage::SendToLog)
#define SYSLOG_FATAL(counter) \
  google::LogMessage(__FILE__, __LINE__, google::FATAL, counter, \
  &google::LogMessage::SendToSyslogAndLog)
#define GOOGLE_LOG_DFATAL(counter) \
  google::LogMessage(__FILE__, __LINE__, google::DFATAL_LEVEL, counter, \
  &google::LogMessage::SendToLog)
#define SYSLOG_DFATAL(counter) \
  google::LogMessage(__FILE__, __LINE__, google::DFATAL_LEVEL, counter, \
  &google::LogMessage::SendToSyslogAndLog)

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__CYGWIN__) || defined(__CYGWIN32__)

#define LOG_SYSRESULT(result) \
  if (FAILED(result)) { \
    LPTSTR message = NULL; \
    LPTSTR msg = reinterpret_cast<LPTSTR>(&message); \
    DWORD message_length = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | \
                         FORMAT_MESSAGE_FROM_SYSTEM, \
                         0, result, 0, msg, 100, NULL); \
    if (message_length > 0) { \
      google::LogMessage(__FILE__, __LINE__, ERROR, 0, \
          &google::LogMessage::SendToLog).stream() << message; \
      LocalFree(message); \
    } \
  }
#endif









#define LOG(severity) COMPACT_GOOGLE_LOG_ ## severity.stream()
#define SYSLOG(severity) SYSLOG_ ## severity(0).stream()

namespace google {


#include "glog/log_severity.h"
#include "glog/vlog_is_on.h"



GOOGLE_GLOG_DLL_DECL void InitGoogleLogging(const char* argv0);


GOOGLE_GLOG_DLL_DECL void InstallFailureFunction(void (*fail_func)());

class LogSink;  









#define LOG_TO_SINK(sink, severity) \
  google::LogMessage(                                    \
      __FILE__, __LINE__,                                               \
      google::severity,                                  \
      static_cast<google::LogSink*>(sink), true).stream()
#define LOG_TO_SINK_BUT_NOT_TO_LOGFILE(sink, severity)                  \
  google::LogMessage(                                    \
      __FILE__, __LINE__,                                               \
      google::severity,                                  \
      static_cast<google::LogSink*>(sink), false).stream()











#define LOG_TO_STRING(severity, message) \
  LOG_TO_STRING_##severity(static_cast<string*>(message)).stream()









#define LOG_STRING(severity, outvec) \
  LOG_TO_STRING_##severity(static_cast<vector<string>*>(outvec)).stream()

#define LOG_IF(severity, condition) \
  !(condition) ? (void) 0 : google::LogMessageVoidify() & LOG(severity)
#define SYSLOG_IF(severity, condition) \
  !(condition) ? (void) 0 : google::LogMessageVoidify() & SYSLOG(severity)

#define LOG_ASSERT(condition)  \
  LOG_IF(FATAL, !(condition)) << "Assert failed: " #condition
#define SYSLOG_ASSERT(condition) \
  SYSLOG_IF(FATAL, !(condition)) << "Assert failed: " #condition





#define CHECK(condition)  \
      LOG_IF(FATAL, GOOGLE_PREDICT_BRANCH_NOT_TAKEN(!(condition))) \
             << "Check failed: " #condition " "



struct CheckOpString {
  CheckOpString(std::string* str) : str_(str) { }
  
  
  operator bool() const {
    return GOOGLE_PREDICT_BRANCH_NOT_TAKEN(str_ != NULL);
  }
  std::string* str_;
};




template <class T>
inline const T&       GetReferenceableValue(const T&           t) { return t; }
inline char           GetReferenceableValue(char               t) { return t; }
inline unsigned char  GetReferenceableValue(unsigned char      t) { return t; }
inline signed char    GetReferenceableValue(signed char        t) { return t; }
inline short          GetReferenceableValue(short              t) { return t; }
inline unsigned short GetReferenceableValue(unsigned short     t) { return t; }
inline int            GetReferenceableValue(int                t) { return t; }
inline unsigned int   GetReferenceableValue(unsigned int       t) { return t; }
inline long           GetReferenceableValue(long               t) { return t; }
inline unsigned long  GetReferenceableValue(unsigned long      t) { return t; }
inline long long      GetReferenceableValue(long long          t) { return t; }
inline unsigned long long GetReferenceableValue(unsigned long long t) {
  return t;
}


struct DummyClassToDefineOperator {};

}




inline std::ostream& operator<<(
    std::ostream& out, const google::DummyClassToDefineOperator&) {
  return out;
}

namespace google {


template<class t1, class t2>
std::string* MakeCheckOpString(const t1& v1, const t2& v2, const char* names) {
  
  
  
#if 1
  using ::operator<<;
#endif
  std::strstream ss;
  ss << names << " (" << v1 << " vs. " << v2 << ")";
  return new std::string(ss.str(), ss.pcount());
}





#define DEFINE_CHECK_OP_IMPL(name, op) \
  template <class t1, class t2> \
  inline std::string* Check##name##Impl(const t1& v1, const t2& v2, \
                                        const char* names) { \
    if (v1 op v2) return NULL; \
    else return MakeCheckOpString(v1, v2, names); \
  } \
  inline std::string* Check##name##Impl(int v1, int v2, const char* names) { \
    return Check##name##Impl<int, int>(v1, v2, names); \
  }





DEFINE_CHECK_OP_IMPL(_EQ, ==)
DEFINE_CHECK_OP_IMPL(_NE, !=)
DEFINE_CHECK_OP_IMPL(_LE, <=)
DEFINE_CHECK_OP_IMPL(_LT, < )
DEFINE_CHECK_OP_IMPL(_GE, >=)
DEFINE_CHECK_OP_IMPL(_GT, > )
#undef DEFINE_CHECK_OP_IMPL




#if defined(STATIC_ANALYSIS)

#define CHECK_OP_LOG(name, op, val1, val2, log) CHECK((val1) op (val2))
#elif !defined(NDEBUG)









typedef std::string _Check_string;
#define CHECK_OP_LOG(name, op, val1, val2, log) \
  while (google::_Check_string* _result =                \
         google::Check##name##Impl(                      \
             google::GetReferenceableValue(val1),        \
             google::GetReferenceableValue(val2),        \
             #val1 " " #op " " #val2))                                  \
    log(__FILE__, __LINE__,                                             \
        google::CheckOpString(_result)).stream()
#else


#define CHECK_OP_LOG(name, op, val1, val2, log) \
  while (google::CheckOpString _result = \
         google::Check##name##Impl(GetReferenceableValue(val1), \
                           GetReferenceableValue(val2), \
                           #val1 " " #op " " #val2)) \
    log(__FILE__, __LINE__, _result).stream()
#endif

#if GOOGLE_STRIP_LOG <= 3
#define CHECK_OP(name, op, val1, val2) \
  CHECK_OP_LOG(name, op, val1, val2, google::LogMessageFatal)
#else
#define CHECK_OP(name, op, val1, val2) \
  CHECK_OP_LOG(name, op, val1, val2, google::NullStreamFatal)
#endif 



















#define CHECK_EQ(val1, val2) CHECK_OP(_EQ, ==, val1, val2)
#define CHECK_NE(val1, val2) CHECK_OP(_NE, !=, val1, val2)
#define CHECK_LE(val1, val2) CHECK_OP(_LE, <=, val1, val2)
#define CHECK_LT(val1, val2) CHECK_OP(_LT, < , val1, val2)
#define CHECK_GE(val1, val2) CHECK_OP(_GE, >=, val1, val2)
#define CHECK_GT(val1, val2) CHECK_OP(_GT, > , val1, val2)




#define CHECK_NOTNULL(val) \
  google::CheckNotNull(__FILE__, __LINE__, "'" #val "' Must be non NULL", (val))



#define DECLARE_CHECK_STROP_IMPL(func, expected) \
  GOOGLE_GLOG_DLL_DECL std::string* Check##func##expected##Impl( \
      const char* s1, const char* s2, const char* names);
DECLARE_CHECK_STROP_IMPL(strcmp, true)
DECLARE_CHECK_STROP_IMPL(strcmp, false)
DECLARE_CHECK_STROP_IMPL(strcasecmp, true)
DECLARE_CHECK_STROP_IMPL(strcasecmp, false)
#undef DECLARE_CHECK_STROP_IMPL



#define CHECK_STROP(func, op, expected, s1, s2) \
  while (google::CheckOpString _result = \
         google::Check##func##expected##Impl((s1), (s2), \
                                     #s1 " " #op " " #s2)) \
    LOG(FATAL) << *_result.str_









#define CHECK_STREQ(s1, s2) CHECK_STROP(strcmp, ==, true, s1, s2)
#define CHECK_STRNE(s1, s2) CHECK_STROP(strcmp, !=, false, s1, s2)
#define CHECK_STRCASEEQ(s1, s2) CHECK_STROP(strcasecmp, ==, true, s1, s2)
#define CHECK_STRCASENE(s1, s2) CHECK_STROP(strcasecmp, !=, false, s1, s2)

#define CHECK_INDEX(I,A) CHECK(I < (sizeof(A)/sizeof(A[0])))
#define CHECK_BOUND(B,A) CHECK(B <= (sizeof(A)/sizeof(A[0])))

#define CHECK_DOUBLE_EQ(val1, val2)              \
  do {                                           \
    CHECK_LE((val1), (val2)+0.000000000000001L); \
    CHECK_GE((val1), (val2)-0.000000000000001L); \
  } while (0)

#define CHECK_NEAR(val1, val2, margin)           \
  do {                                           \
    CHECK_LE((val1), (val2)+(margin));           \
    CHECK_GE((val1), (val2)-(margin));           \
  } while (0)







#define PLOG(severity) GOOGLE_PLOG(severity, 0).stream()

#define GOOGLE_PLOG(severity, counter)  \
  google::ErrnoLogMessage( \
      __FILE__, __LINE__, google::severity, counter, \
      &google::LogMessage::SendToLog)

#define PLOG_IF(severity, condition) \
  !(condition) ? (void) 0 : google::LogMessageVoidify() & PLOG(severity)




#define PCHECK(condition)  \
      PLOG_IF(FATAL, GOOGLE_PREDICT_BRANCH_NOT_TAKEN(!(condition))) \
              << "Check failed: " #condition " "









#define CHECK_ERR(invocation)                                          \
PLOG_IF(FATAL, GOOGLE_PREDICT_BRANCH_NOT_TAKEN((invocation) == -1))    \
        << #invocation



#define LOG_EVERY_N_VARNAME(base, line) LOG_EVERY_N_VARNAME_CONCAT(base, line)
#define LOG_EVERY_N_VARNAME_CONCAT(base, line) base ## line

#define LOG_OCCURRENCES LOG_EVERY_N_VARNAME(occurrences_, __LINE__)
#define LOG_OCCURRENCES_MOD_N LOG_EVERY_N_VARNAME(occurrences_mod_n_, __LINE__)

#define SOME_KIND_OF_LOG_EVERY_N(severity, n, what_to_do) \
  static int LOG_OCCURRENCES = 0, LOG_OCCURRENCES_MOD_N = 0; \
  ++LOG_OCCURRENCES; \
  if (++LOG_OCCURRENCES_MOD_N > n) LOG_OCCURRENCES_MOD_N -= n; \
  if (LOG_OCCURRENCES_MOD_N == 1) \
    google::LogMessage( \
        __FILE__, __LINE__, google::severity, LOG_OCCURRENCES, \
        &what_to_do).stream()

#define SOME_KIND_OF_LOG_IF_EVERY_N(severity, condition, n, what_to_do) \
  static int LOG_OCCURRENCES = 0, LOG_OCCURRENCES_MOD_N = 0; \
  ++LOG_OCCURRENCES; \
  if (condition && \
      ((LOG_OCCURRENCES_MOD_N=(LOG_OCCURRENCES_MOD_N + 1) % n) == (1 % n))) \
    google::LogMessage( \
        __FILE__, __LINE__, google::severity, LOG_OCCURRENCES, \
                 &what_to_do).stream()

#define SOME_KIND_OF_PLOG_EVERY_N(severity, n, what_to_do) \
  static int LOG_OCCURRENCES = 0, LOG_OCCURRENCES_MOD_N = 0; \
  ++LOG_OCCURRENCES; \
  if (++LOG_OCCURRENCES_MOD_N > n) LOG_OCCURRENCES_MOD_N -= n; \
  if (LOG_OCCURRENCES_MOD_N == 1) \
    google::ErrnoLogMessage( \
        __FILE__, __LINE__, google::severity, LOG_OCCURRENCES, \
        &what_to_do).stream()

#define SOME_KIND_OF_LOG_FIRST_N(severity, n, what_to_do) \
  static int LOG_OCCURRENCES = 0; \
  if (LOG_OCCURRENCES <= n) \
    ++LOG_OCCURRENCES; \
  if (LOG_OCCURRENCES <= n) \
    google::LogMessage( \
        __FILE__, __LINE__, google::severity, LOG_OCCURRENCES, \
        &what_to_do).stream()

namespace glog_internal_namespace_ {
template <bool>
struct CompileAssert {
};
struct CrashReason;
}  

#define GOOGLE_GLOG_COMPILE_ASSERT(expr, msg) \
  typedef google::glog_internal_namespace_::CompileAssert<(bool(expr))> msg[bool(expr) ? 1 : -1]

#define LOG_EVERY_N(severity, n)                                        \
  GOOGLE_GLOG_COMPILE_ASSERT(google::severity <          \
                             google::NUM_SEVERITIES,     \
                             INVALID_REQUESTED_LOG_SEVERITY);           \
  SOME_KIND_OF_LOG_EVERY_N(severity, (n), google::LogMessage::SendToLog)

#define SYSLOG_EVERY_N(severity, n) \
  SOME_KIND_OF_LOG_EVERY_N(severity, (n), google::LogMessage::SendToSyslogAndLog)

#define PLOG_EVERY_N(severity, n) \
  SOME_KIND_OF_PLOG_EVERY_N(severity, (n), google::LogMessage::SendToLog)

#define LOG_FIRST_N(severity, n) \
  SOME_KIND_OF_LOG_FIRST_N(severity, (n), google::LogMessage::SendToLog)

#define LOG_IF_EVERY_N(severity, condition, n) \
  SOME_KIND_OF_LOG_IF_EVERY_N(severity, (condition), (n), google::LogMessage::SendToLog)


enum PRIVATE_Counter {COUNTER};




#ifndef NDEBUG

#define DLOG(severity) LOG(severity)
#define DVLOG(verboselevel) VLOG(verboselevel)
#define DLOG_IF(severity, condition) LOG_IF(severity, condition)
#define DLOG_EVERY_N(severity, n) LOG_EVERY_N(severity, n)
#define DLOG_IF_EVERY_N(severity, condition, n) \
  LOG_IF_EVERY_N(severity, condition, n)
#define DLOG_ASSERT(condition) LOG_ASSERT(condition)


#define DCHECK(condition) CHECK(condition)
#define DCHECK_EQ(val1, val2) CHECK_EQ(val1, val2)
#define DCHECK_NE(val1, val2) CHECK_NE(val1, val2)
#define DCHECK_LE(val1, val2) CHECK_LE(val1, val2)
#define DCHECK_LT(val1, val2) CHECK_LT(val1, val2)
#define DCHECK_GE(val1, val2) CHECK_GE(val1, val2)
#define DCHECK_GT(val1, val2) CHECK_GT(val1, val2)
#define DCHECK_STREQ(str1, str2) CHECK_STREQ(str1, str2)
#define DCHECK_STRCASEEQ(str1, str2) CHECK_STRCASEEQ(str1, str2)
#define DCHECK_STRNE(str1, str2) CHECK_STRNE(str1, str2)
#define DCHECK_STRCASENE(str1, str2) CHECK_STRCASENE(str1, str2)

#else  

#define DLOG(severity) \
  true ? (void) 0 : google::LogMessageVoidify() & LOG(severity)

#define DVLOG(verboselevel) \
  (true || !VLOG_IS_ON(verboselevel)) ?\
    (void) 0 : google::LogMessageVoidify() & LOG(INFO)

#define DLOG_IF(severity, condition) \
  (true || !(condition)) ? (void) 0 : google::LogMessageVoidify() & LOG(severity)

#define DLOG_EVERY_N(severity, n) \
  true ? (void) 0 : google::LogMessageVoidify() & LOG(severity)

#define DLOG_IF_EVERY_N(severity, condition, n) \
  (true || !(condition))? (void) 0 : google::LogMessageVoidify() & LOG(severity)

#define DLOG_ASSERT(condition) \
  true ? (void) 0 : LOG_ASSERT(condition)

#define DCHECK(condition) \
  while (false) \
    CHECK(condition)

#define DCHECK_EQ(val1, val2) \
  while (false) \
    CHECK_EQ(val1, val2)

#define DCHECK_NE(val1, val2) \
  while (false) \
    CHECK_NE(val1, val2)

#define DCHECK_LE(val1, val2) \
  while (false) \
    CHECK_LE(val1, val2)

#define DCHECK_LT(val1, val2) \
  while (false) \
    CHECK_LT(val1, val2)

#define DCHECK_GE(val1, val2) \
  while (false) \
    CHECK_GE(val1, val2)

#define DCHECK_GT(val1, val2) \
  while (false) \
    CHECK_GT(val1, val2)

#define DCHECK_STREQ(str1, str2) \
  while (false) \
    CHECK_STREQ(str1, str2)

#define DCHECK_STRCASEEQ(str1, str2) \
  while (false) \
    CHECK_STRCASEEQ(str1, str2)

#define DCHECK_STRNE(str1, str2) \
  while (false) \
    CHECK_STRNE(str1, str2)

#define DCHECK_STRCASENE(str1, str2) \
  while (false) \
    CHECK_STRCASENE(str1, str2)


#endif  



#define VLOG(verboselevel) LOG_IF(INFO, VLOG_IS_ON(verboselevel))

#define VLOG_IF(verboselevel, condition) \
  LOG_IF(INFO, (condition) && VLOG_IS_ON(verboselevel))

#define VLOG_EVERY_N(verboselevel, n) \
  LOG_IF_EVERY_N(INFO, VLOG_IS_ON(verboselevel), n)

#define VLOG_IF_EVERY_N(verboselevel, condition, n) \
  LOG_IF_EVERY_N(INFO, (condition) && VLOG_IS_ON(verboselevel), n)










class GOOGLE_GLOG_DLL_DECL LogMessage {
public:
  enum {
    
    
    
    
    
    kNoLogPrefix = -1
  };

  
  
  
  
  
  
#ifdef _MSC_VER
# pragma warning(disable: 4275)
#endif
  class GOOGLE_GLOG_DLL_DECL LogStream : public std::ostrstream {
#ifdef _MSC_VER
# pragma warning(default: 4275)
#endif
  public:
    LogStream(char *buf, int len, int ctr)
      : ostrstream(buf, len),
        ctr_(ctr) {
      self_ = this;
    }

    int ctr() const { return ctr_; }
    void set_ctr(int ctr) { ctr_ = ctr; }
    LogStream* self() const { return self_; }

  private:
    int ctr_;  
    LogStream *self_;  
  };

public:
  
  typedef void (LogMessage::*SendMethod)();

  LogMessage(const char* file, int line, LogSeverity severity, int ctr,
             SendMethod send_method);

  
  

  
  
  
  
  
  LogMessage(const char* file, int line);

  
  
  
  
  
  LogMessage(const char* file, int line, LogSeverity severity);

  
  
  
  LogMessage(const char* file, int line, LogSeverity severity, LogSink* sink,
             bool also_send_to_log);

  
  
  
  LogMessage(const char* file, int line, LogSeverity severity,
             std::vector<std::string>* outvec);

  
  
  
  LogMessage(const char* file, int line, LogSeverity severity,
             std::string* message);

  
  LogMessage(const char* file, int line, const CheckOpString& result);

  ~LogMessage();

  
  
  
  void Flush();

  
  
  static const size_t kMaxLogMessageLen;

  
  
  void SendToLog();  
  void SendToSyslogAndLog();  

  
  static void Fail() __attribute__ ((noreturn));

  std::ostream& stream() { return *(data_->stream_); }

  int preserved_errno() const { return data_->preserved_errno_; }

  
  static int64 num_messages(int severity);

private:
  
  void SendToSinkAndLog();  
  void SendToSink();  

  
  void WriteToStringAndLog();

  void SaveOrSendToLog();  

  void Init(const char* file, int line, LogSeverity severity,
            void (LogMessage::*send_method)());

  
  void RecordCrashReason(glog_internal_namespace_::CrashReason* reason);

  
  static int64 num_messages_[NUM_SEVERITIES];  

  
  
  struct GOOGLE_GLOG_DLL_DECL LogMessageData {
    LogMessageData() {};

    int preserved_errno_;      
    char* buf_;
    char* message_text_;  
    LogStream* stream_alloc_;
    LogStream* stream_;
    char severity_;      
    int line_;                 
    void (LogMessage::*send_method_)();  
    union {  
      LogSink* sink_;             
      std::vector<std::string>* outvec_; 
      std::string* message_;             
    };
    time_t timestamp_;            
    struct ::tm tm_time_;         
    size_t num_prefix_chars_;     
    size_t num_chars_to_log_;     
    size_t num_chars_to_syslog_;  
    const char* basename_;        
    const char* fullname_;        
    bool has_been_flushed_;       
    bool first_fatal_;            

    ~LogMessageData();
   private:
    LogMessageData(const LogMessageData&);
    void operator=(const LogMessageData&);
  };

  static LogMessageData fatal_msg_data_exclusive_;
  static LogMessageData fatal_msg_data_shared_;

  LogMessageData* allocated_;
  LogMessageData* data_;

  friend class LogDestination;

  LogMessage(const LogMessage&);
  void operator=(const LogMessage&);
};




class GOOGLE_GLOG_DLL_DECL LogMessageFatal : public LogMessage {
 public:
  LogMessageFatal(const char* file, int line);
  LogMessageFatal(const char* file, int line, const CheckOpString& result);
  ~LogMessageFatal() __attribute__ ((noreturn));
};



inline void LogAtLevel(int const severity, std::string const &msg) {
  LogMessage(__FILE__, __LINE__, severity).stream() << msg;
}





#define LOG_AT_LEVEL(severity) LogMessage(__FILE__, __LINE__, severity).stream()


template <typename T>
T* CheckNotNull(const char *file, int line, const char *names, T* t) {
  if (t == NULL) {
    LogMessageFatal(file, line, new std::string(names));
  }
  return t;
}




GOOGLE_GLOG_DLL_DECL std::ostream& operator<<(std::ostream &os,
                                              const PRIVATE_Counter&);



class GOOGLE_GLOG_DLL_DECL ErrnoLogMessage : public LogMessage {
 public:

  ErrnoLogMessage(const char* file, int line, LogSeverity severity, int ctr,
                  void (LogMessage::*send_method)());

  
  ~ErrnoLogMessage();

 private:
  ErrnoLogMessage(const ErrnoLogMessage&);
  void operator=(const ErrnoLogMessage&);
};






class GOOGLE_GLOG_DLL_DECL LogMessageVoidify {
 public:
  LogMessageVoidify() { }
  
  
  void operator&(std::ostream&) { }
};




GOOGLE_GLOG_DLL_DECL void FlushLogFiles(LogSeverity min_severity);




GOOGLE_GLOG_DLL_DECL void FlushLogFilesUnsafe(LogSeverity min_severity);






GOOGLE_GLOG_DLL_DECL void SetLogDestination(LogSeverity severity,
                                            const char* base_filename);







GOOGLE_GLOG_DLL_DECL void SetLogSymlink(LogSeverity severity,
                                        const char* symlink_basename);






class GOOGLE_GLOG_DLL_DECL LogSink {
 public:
  virtual ~LogSink();

  
  
  
  virtual void send(LogSeverity severity, const char* full_filename,
                    const char* base_filename, int line,
                    const struct ::tm* tm_time,
                    const char* message, size_t message_len) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual void WaitTillSent();

  
  
  static std::string ToString(LogSeverity severity, const char* file, int line,
                              const struct ::tm* tm_time,
                              const char* message, size_t message_len);
};


GOOGLE_GLOG_DLL_DECL void AddLogSink(LogSink *destination);
GOOGLE_GLOG_DLL_DECL void RemoveLogSink(LogSink *destination);







GOOGLE_GLOG_DLL_DECL void SetLogFilenameExtension(
    const char* filename_extension);






GOOGLE_GLOG_DLL_DECL void SetStderrLogging(LogSeverity min_severity);




GOOGLE_GLOG_DLL_DECL void LogToStderr();







GOOGLE_GLOG_DLL_DECL void SetEmailLogging(LogSeverity min_severity,
                                          const char* addresses);



GOOGLE_GLOG_DLL_DECL bool SendEmail(const char *dest,
                                    const char *subject, const char *body);

GOOGLE_GLOG_DLL_DECL const std::vector<std::string>& GetLoggingDirectories();




void TestOnly_ClearLoggingDirectoriesList();




GOOGLE_GLOG_DLL_DECL void GetExistingTempDirectories(
    std::vector<std::string>* list);




GOOGLE_GLOG_DLL_DECL void ReprintFatalMessage();








GOOGLE_GLOG_DLL_DECL void TruncateLogFile(const char *path,
                                          int64 limit, int64 keep);




GOOGLE_GLOG_DLL_DECL void TruncateStdoutStderr();



GOOGLE_GLOG_DLL_DECL const char* GetLogSeverityName(LogSeverity severity);












namespace base {

class GOOGLE_GLOG_DLL_DECL Logger {
 public:
  virtual ~Logger();

  
  
  
  
  
  
  
  
  virtual void Write(bool force_flush,
                     time_t timestamp,
                     const char* message,
                     int message_len) = 0;

  
  virtual void Flush() = 0;

  
  
  
  virtual uint32 LogSize() = 0;
};




extern GOOGLE_GLOG_DLL_DECL Logger* GetLogger(LogSeverity level);




extern GOOGLE_GLOG_DLL_DECL void SetLogger(LogSeverity level, Logger* logger);

}











GOOGLE_GLOG_DLL_DECL int posix_strerror_r(int err, char *buf, size_t len);



class GOOGLE_GLOG_DLL_DECL NullStream : public LogMessage::LogStream {
 public:
  
  
  
  
  NullStream() : LogMessage::LogStream(message_buffer_, 1, 0) { }
  NullStream(const char* , int ,
             const CheckOpString& ) :
      LogMessage::LogStream(message_buffer_, 1, 0) { }
  NullStream &stream() { return *this; }
 private:
  
  
  
  char message_buffer_[2];
};







template<class T>
inline NullStream& operator<<(NullStream &str, const T &value) { return str; }



class GOOGLE_GLOG_DLL_DECL NullStreamFatal : public NullStream {
 public:
  NullStreamFatal() { }
  NullStreamFatal(const char* file, int line, const CheckOpString& result) :
      NullStream(file, line, result) { }
  __attribute__ ((noreturn)) ~NullStreamFatal() { _exit(1); }
};

















GOOGLE_GLOG_DLL_DECL void InstallFailureSignalHandler();





GOOGLE_GLOG_DLL_DECL void InstallFailureWriter(
    void (*writer)(const char* data, int size));

}

#endif 
