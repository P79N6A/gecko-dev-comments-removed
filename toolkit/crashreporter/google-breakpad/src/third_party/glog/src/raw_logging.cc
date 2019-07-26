
































#include "utilities.h"

#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>               
#endif
#include <fcntl.h>                 
#include <time.h>
#include "config.h"
#include "glog/logging.h"          
#include "glog/raw_logging.h"
#include "base/commandlineflags.h"

#ifdef HAVE_STACKTRACE
# include "stacktrace.h"
#endif

#if defined(HAVE_SYSCALL_H)
#include <syscall.h>                 
#elif defined(HAVE_SYS_SYSCALL_H)
#include <sys/syscall.h>                 
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#if defined(HAVE_SYSCALL_H) || defined(HAVE_SYS_SYSCALL_H)
# define safe_write(fd, s, len)  syscall(SYS_write, fd, s, len)
#else
  
# define safe_write(fd, s, len)  write(fd, s, len)
#endif

_START_GOOGLE_NAMESPACE_




static struct ::tm last_tm_time_for_raw_log;
static int last_usecs_for_raw_log;

void RawLog__SetLastTime(const struct ::tm& t, int usecs) {
  memcpy(&last_tm_time_for_raw_log, &t, sizeof(last_tm_time_for_raw_log));
  last_usecs_for_raw_log = usecs;
}









static bool DoRawLog(char** buf, int* size, const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  int n = vsnprintf(*buf, *size, format, ap);
  va_end(ap);
  if (n < 0 || n > *size) return false;
  *size -= n;
  *buf += n;
  return true;
}


inline static bool VADoRawLog(char** buf, int* size,
                              const char* format, va_list ap) {
  int n = vsnprintf(*buf, *size, format, ap);
  if (n < 0 || n > *size) return false;
  *size -= n;
  *buf += n;
  return true;
}

static const int kLogBufSize = 3000;
static bool crashed = false;
static CrashReason crash_reason;
static char crash_buf[kLogBufSize + 1] = { 0 };  

void RawLog__(LogSeverity severity, const char* file, int line,
              const char* format, ...) {
  if (!(FLAGS_logtostderr || severity >= FLAGS_stderrthreshold ||
        FLAGS_alsologtostderr || !IsGoogleLoggingInitialized())) {
    return;  
  }
  
  struct ::tm& t = last_tm_time_for_raw_log;
  char buffer[kLogBufSize];
  char* buf = buffer;
  int size = sizeof(buffer);

  
  DoRawLog(&buf, &size, "%c%02d%02d %02d:%02d:%02d.%06d %5u %s:%d] RAW: ",
           LogSeverityNames[severity][0],
           1 + t.tm_mon, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec,
           last_usecs_for_raw_log,
           static_cast<unsigned int>(GetTID()),
           const_basename(const_cast<char *>(file)), line);

  
  const char* msg_start = buf;
  const int msg_size = size;

  va_list ap;
  va_start(ap, format);
  bool no_chop = VADoRawLog(&buf, &size, format, ap);
  va_end(ap);
  if (no_chop) {
    DoRawLog(&buf, &size, "\n");
  } else {
    DoRawLog(&buf, &size, "RAW_LOG ERROR: The Message was too long!\n");
  }
  
  
  
  
  safe_write(STDERR_FILENO, buffer, strlen(buffer));
  if (severity == FATAL)  {
    if (!sync_val_compare_and_swap(&crashed, false, true)) {
      crash_reason.filename = file;
      crash_reason.line_number = line;
      memcpy(crash_buf, msg_start, msg_size);  
      crash_reason.message = crash_buf;
#ifdef HAVE_STACKTRACE
      crash_reason.depth =
          GetStackTrace(crash_reason.stack, ARRAYSIZE(crash_reason.stack), 1);
#else
      crash_reason.depth = 0;
#endif
      SetCrashReason(&crash_reason);
    }
    LogMessage::Fail();  
  }
}

_END_GOOGLE_NAMESPACE_
