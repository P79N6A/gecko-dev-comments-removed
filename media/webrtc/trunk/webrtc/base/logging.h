












































#ifndef WEBRTC_BASE_LOGGING_H_
#define WEBRTC_BASE_LOGGING_H_

#ifdef HAVE_CONFIG_H
#include "config.h"  
#endif

#include <list>
#include <sstream>
#include <string>
#include <utility>
#include "webrtc/base/basictypes.h"
#include "webrtc/base/criticalsection.h"

namespace rtc {

class StreamInterface;
















struct ConstantLabel { int value; const char * label; };
#define KLABEL(x) { x, #x }
#define TLABEL(x, y) { x, y }
#define LASTLABEL { 0, 0 }

const char * FindLabel(int value, const ConstantLabel entries[]);
std::string ErrorName(int err, const ConstantLabel* err_table);













enum LoggingSeverity { LS_SENSITIVE, LS_VERBOSE, LS_INFO, LS_WARNING, LS_ERROR,
                       INFO = LS_INFO,
                       WARNING = LS_WARNING,
                       LERROR = LS_ERROR };


enum LogErrorContext {
  ERRCTX_NONE,
  ERRCTX_ERRNO,     
  ERRCTX_HRESULT,   
  ERRCTX_OSSTATUS,  

  
  ERRCTX_EN = ERRCTX_ERRNO,     
  ERRCTX_HR = ERRCTX_HRESULT,   
  ERRCTX_OS = ERRCTX_OSSTATUS,  
};

class LogMessage {
 public:
  static const int NO_LOGGING;
  static const uint32 WARN_SLOW_LOGS_DELAY = 50;  

  LogMessage(const char* file, int line, LoggingSeverity sev,
             LogErrorContext err_ctx = ERRCTX_NONE, int err = 0,
             const char* module = NULL);
  ~LogMessage();

  static inline bool Loggable(LoggingSeverity sev) { return (sev >= min_sev_); }
  std::ostream& stream() { return print_stream_; }

  
  
  
  
  
  static uint32 LogStartTime();

  
  
  static uint32 WallClockStartTime();

  
  
  static void LogContext(int min_sev);
  
  static void LogThreads(bool on = true);
  
  static void LogTimestamps(bool on = true);

  
  
  static void LogToDebug(int min_sev);
  static int GetLogToDebug() { return dbg_sev_; }

  
  
  
  
  
  
  
  static void LogToStream(StreamInterface* stream, int min_sev);
  static int GetLogToStream(StreamInterface* stream = NULL);
  static void AddLogToStream(StreamInterface* stream, int min_sev);
  static void RemoveLogToStream(StreamInterface* stream);

  
  
  static int GetMinLogSeverity() { return min_sev_; }

  static void SetDiagnosticMode(bool f) { is_diagnostic_mode_ = f; }
  static bool IsDiagnosticMode() { return is_diagnostic_mode_; }

  
  
  
  static void ConfigureLogging(const char* params, const char* filename);

  
  static int ParseLogSeverity(const std::string& value);

 private:
  typedef std::list<std::pair<StreamInterface*, int> > StreamList;

  
  static void UpdateMinLogSeverity();

  
  static const char* Describe(LoggingSeverity sev);
  static const char* DescribeFile(const char* file);

  
  static void OutputToDebug(const std::string& msg, LoggingSeverity severity_);
  static void OutputToStream(StreamInterface* stream, const std::string& msg);

  
  std::ostringstream print_stream_;

  
  LoggingSeverity severity_;

  
  
  std::string extra_;

  
  
  uint32 warn_slow_logs_delay_;

  
  static CriticalSection crit_;

  
  
  
  
  
  static int min_sev_, dbg_sev_, ctx_sev_;

  
  static StreamList streams_;

  
  static bool thread_, timestamp_;

  
  static bool is_diagnostic_mode_;

  DISALLOW_EVIL_CONSTRUCTORS(LogMessage);
};





class LogMultilineState {
 public:
  size_t unprintable_count_[2];
  LogMultilineState() {
    unprintable_count_[0] = unprintable_count_[1] = 0;
  }
};



void LogMultiline(LoggingSeverity level, const char* label, bool input,
                  const void* data, size_t len, bool hex_mode,
                  LogMultilineState* state);

#ifndef LOG








class LogMessageVoidify {
 public:
  LogMessageVoidify() { }
  
  
  void operator&(std::ostream&) { }
};

#define LOG_SEVERITY_PRECONDITION(sev) \
  !(rtc::LogMessage::Loggable(sev)) \
    ? (void) 0 \
    : rtc::LogMessageVoidify() &

#define LOG(sev) \
  LOG_SEVERITY_PRECONDITION(rtc::sev) \
    rtc::LogMessage(__FILE__, __LINE__, rtc::sev).stream()



#define LOG_V(sev) \
  LOG_SEVERITY_PRECONDITION(sev) \
    rtc::LogMessage(__FILE__, __LINE__, sev).stream()


#if (defined(__GNUC__) && defined(_DEBUG)) || defined(WANT_PRETTY_LOG_F)
#define LOG_F(sev) LOG(sev) << __PRETTY_FUNCTION__ << ": "
#define LOG_T_F(sev) LOG(sev) << this << ": " << __PRETTY_FUNCTION__ << ": "
#else
#define LOG_F(sev) LOG(sev) << __FUNCTION__ << ": "
#define LOG_T_F(sev) LOG(sev) << this << ": " << __FUNCTION__ << ": "
#endif

#define LOG_CHECK_LEVEL(sev) \
  rtc::LogCheckLevel(rtc::sev)
#define LOG_CHECK_LEVEL_V(sev) \
  rtc::LogCheckLevel(sev)
inline bool LogCheckLevel(LoggingSeverity sev) {
  return (LogMessage::GetMinLogSeverity() <= sev);
}

#define LOG_E(sev, ctx, err, ...) \
  LOG_SEVERITY_PRECONDITION(rtc::sev) \
    rtc::LogMessage(__FILE__, __LINE__, rtc::sev, \
                          rtc::ERRCTX_ ## ctx, err , ##__VA_ARGS__) \
        .stream()

#define LOG_T(sev) LOG(sev) << this << ": "

#define LOG_ERRNO_EX(sev, err) \
  LOG_E(sev, ERRNO, err)
#define LOG_ERRNO(sev) \
  LOG_ERRNO_EX(sev, errno)

#if defined(WEBRTC_WIN)
#define LOG_GLE_EX(sev, err) \
  LOG_E(sev, HRESULT, err)
#define LOG_GLE(sev) \
  LOG_GLE_EX(sev, GetLastError())
#define LOG_GLEM(sev, mod) \
  LOG_E(sev, HRESULT, GetLastError(), mod)
#define LOG_ERR_EX(sev, err) \
  LOG_GLE_EX(sev, err)
#define LOG_ERR(sev) \
  LOG_GLE(sev)
#define LAST_SYSTEM_ERROR \
  (::GetLastError())
#elif __native_client__
#define LOG_ERR_EX(sev, err) \
  LOG(sev)
#define LOG_ERR(sev) \
  LOG(sev)
#define LAST_SYSTEM_ERROR \
  (0)
#elif defined(WEBRTC_POSIX)
#define LOG_ERR_EX(sev, err) \
  LOG_ERRNO_EX(sev, err)
#define LOG_ERR(sev) \
  LOG_ERRNO(sev)
#define LAST_SYSTEM_ERROR \
  (errno)
#endif  

#define PLOG(sev, err) \
  LOG_ERR_EX(sev, err)



#endif  

}  

#endif  
