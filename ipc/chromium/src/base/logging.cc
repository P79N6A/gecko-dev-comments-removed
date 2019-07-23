



#include "base/logging.h"

#ifdef CHROMIUM_MOZILLA_BUILD

#include "prmem.h"
#include "prprf.h"
#include "base/string_util.h"
#include "nsXPCOM.h"

namespace mozilla {

Logger::~Logger()
{
  PRLogModuleLevel prlevel = PR_LOG_DEBUG;
  int xpcomlevel = -1;

  switch (mSeverity) {
  case LOG_INFO:
    prlevel = PR_LOG_DEBUG;
    xpcomlevel = -1;
    break;

  case LOG_WARNING:
    prlevel = PR_LOG_WARNING;
    xpcomlevel = NS_DEBUG_WARNING;
    break;

  case LOG_ERROR:
    prlevel = PR_LOG_ERROR;
    xpcomlevel = NS_DEBUG_WARNING;
    break;

  case LOG_ERROR_REPORT:
    prlevel = PR_LOG_ERROR;
    xpcomlevel = NS_DEBUG_ASSERTION;
    break;

  case LOG_FATAL:
    prlevel = PR_LOG_ERROR;
    xpcomlevel = NS_DEBUG_ABORT;
    break;
  }

  PR_LOG(GetLog(), prlevel, ("%s:%i: %s", mFile, mLine, mMsg ? mMsg : "<no message>"));
  if (xpcomlevel != -1)
    NS_DebugBreak(xpcomlevel, mMsg, NULL, mFile, mLine);

  PR_Free(mMsg);
}

void
Logger::printf(const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  mMsg = PR_vsprintf_append(mMsg, fmt, args);
  va_end(args);
}

PRLogModuleInfo* Logger::gChromiumPRLog;

PRLogModuleInfo* Logger::GetLog()
{
  if (!gChromiumPRLog)
    gChromiumPRLog = PR_NewLogModule("chromium");
  return gChromiumPRLog;
}

} 

mozilla::Logger&
operator<<(mozilla::Logger& log, const char* s)
{
  log.printf("%s", s);
  return log;
}

mozilla::Logger&
operator<<(mozilla::Logger& log, const std::string& s)
{
  log.printf("%s", s.c_str());
  return log;
}

mozilla::Logger&
operator<<(mozilla::Logger& log, int i)
{
  log.printf("%i", i);
  return log;
}

mozilla::Logger&
operator<<(mozilla::Logger& log, const std::wstring& s)
{
  log.printf("%s", WideToASCII(s).c_str());
  return log;
}

mozilla::Logger&
operator<<(mozilla::Logger& log, void* p)
{
  log.printf("%p", p);
  return log;
}

#else

#if defined(OS_WIN)
#include <windows.h>
typedef HANDLE FileHandle;
typedef HANDLE MutexHandle;
#elif defined(OS_MACOSX)
#include <CoreFoundation/CoreFoundation.h>
#include <mach/mach.h>
#include <mach/mach_time.h>
#include <mach-o/dyld.h>
#elif defined(OS_LINUX)
#include <sys/syscall.h>
#include <time.h>
#endif

#if defined(OS_POSIX)
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#define MAX_PATH PATH_MAX
typedef FILE* FileHandle;
typedef pthread_mutex_t* MutexHandle;
#endif

#include <ctime>
#include <iomanip>
#include <cstring>
#include <algorithm>

#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/debug_util.h"
#include "base/lock_impl.h"
#include "base/string_piece.h"
#include "base/string_util.h"
#include "base/sys_string_conversions.h"

namespace logging {

bool g_enable_dcheck = false;

const char* const log_severity_names[LOG_NUM_SEVERITIES] = {
  "INFO", "WARNING", "ERROR", "ERROR_REPORT", "FATAL" };

int min_log_level = 0;
LogLockingState lock_log_file = LOCK_LOG_FILE;





#if defined(OS_WIN)
LoggingDestination logging_destination = LOG_ONLY_TO_FILE;
#elif defined(OS_POSIX)
LoggingDestination logging_destination = LOG_ONLY_TO_SYSTEM_DEBUG_LOG;
#endif

const int kMaxFilteredLogLevel = LOG_WARNING;
std::string* log_filter_prefix;


const int kAlwaysPrintErrorLevel = LOG_ERROR;




#if defined(OS_WIN)
typedef wchar_t PathChar;
typedef std::wstring PathString;
#else
typedef char PathChar;
typedef std::string PathString;
#endif
PathString* log_file_name = NULL;


FileHandle log_file = NULL;


bool log_process_id = false;
bool log_thread_id = false;
bool log_timestamp = true;
bool log_tickcount = false;



LogAssertHandlerFunction log_assert_handler = NULL;


LogReportHandlerFunction log_report_handler = NULL;




static LockImpl* log_lock = NULL;



#if defined(OS_WIN)
MutexHandle log_mutex = NULL;
#elif defined(OS_POSIX)
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif



int32 CurrentProcessId() {
#if defined(OS_WIN)
  return GetCurrentProcessId();
#elif defined(OS_POSIX)
  return getpid();
#endif
}

int32 CurrentThreadId() {
#if defined(OS_WIN)
  return GetCurrentThreadId();
#elif defined(OS_MACOSX)
  return mach_thread_self();
#elif defined(OS_LINUX)
  return syscall(__NR_gettid);
#endif
}

uint64 TickCount() {
#if defined(OS_WIN)
  return GetTickCount();
#elif defined(OS_MACOSX)
  return mach_absolute_time();
#elif defined(OS_LINUX)
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);

  uint64 absolute_micro =
    static_cast<int64>(ts.tv_sec) * 1000000 +
    static_cast<int64>(ts.tv_nsec) / 1000;

  return absolute_micro;
#endif
}

void CloseFile(FileHandle log) {
#if defined(OS_WIN)
  CloseHandle(log);
#else
  fclose(log);
#endif
}

void DeleteFilePath(const PathString& log_name) {
#if defined(OS_WIN)
  DeleteFile(log_name.c_str());
#else
  unlink(log_name.c_str());
#endif
}




bool InitializeLogFileHandle() {
  if (log_file)
    return true;

  if (!log_file_name) {
    
    
#if defined(OS_WIN)
    
    wchar_t module_name[MAX_PATH];
    GetModuleFileName(NULL, module_name, MAX_PATH);
    log_file_name = new std::wstring(module_name);
    std::wstring::size_type last_backslash =
        log_file_name->rfind('\\', log_file_name->size());
    if (last_backslash != std::wstring::npos)
      log_file_name->erase(last_backslash + 1);
    *log_file_name += L"debug.log";
#elif defined(OS_POSIX)
    
    log_file_name = new std::string("debug.log");
#endif
  }

  if (logging_destination == LOG_ONLY_TO_FILE ||
      logging_destination == LOG_TO_BOTH_FILE_AND_SYSTEM_DEBUG_LOG) {
#if defined(OS_WIN)
    log_file = CreateFile(log_file_name->c_str(), GENERIC_WRITE,
                          FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                          OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (log_file == INVALID_HANDLE_VALUE || log_file == NULL) {
      
      log_file = CreateFile(L".\\debug.log", GENERIC_WRITE,
                            FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                            OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
      if (log_file == INVALID_HANDLE_VALUE || log_file == NULL) {
        log_file = NULL;
        return false;
      }
    }
    SetFilePointer(log_file, 0, 0, FILE_END);
#elif defined(OS_POSIX)
    log_file = fopen(log_file_name->c_str(), "a");
    if (log_file == NULL)
      return false;
#endif
  }

  return true;
}

void InitLogMutex() {
#if defined(OS_WIN)
  if (!log_mutex) {
    
    std::wstring safe_name(*log_file_name);
    std::replace(safe_name.begin(), safe_name.end(), '\\', '/');
    std::wstring t(L"Global\\");
    t.append(safe_name);
    log_mutex = ::CreateMutex(NULL, FALSE, t.c_str());
  }
#elif defined(OS_POSIX)
  
#endif
}

void InitLogging(const PathChar* new_log_file, LoggingDestination logging_dest,
                 LogLockingState lock_log, OldFileDeletionState delete_old) {
  g_enable_dcheck =
      CommandLine::ForCurrentProcess()->HasSwitch(switches::kEnableDCHECK);

  if (log_file) {
    
    
    CloseFile(log_file);
    log_file = NULL;
  }

  lock_log_file = lock_log;
  logging_destination = logging_dest;

  
  if (logging_destination == LOG_NONE ||
      logging_destination == LOG_ONLY_TO_SYSTEM_DEBUG_LOG)
    return;

  if (!log_file_name)
    log_file_name = new PathString();
  *log_file_name = new_log_file;
  if (delete_old == DELETE_OLD_LOG_FILE)
    DeleteFilePath(*log_file_name);

  if (lock_log_file == LOCK_LOG_FILE) {
    InitLogMutex();
  } else if (!log_lock) {
    log_lock = new LockImpl();
  }

  InitializeLogFileHandle();
}

void SetMinLogLevel(int level) {
  min_log_level = level;
}

int GetMinLogLevel() {
  return min_log_level;
}

void SetLogFilterPrefix(const char* filter)  {
  if (log_filter_prefix) {
    delete log_filter_prefix;
    log_filter_prefix = NULL;
  }

  if (filter)
    log_filter_prefix = new std::string(filter);
}

void SetLogItems(bool enable_process_id, bool enable_thread_id,
                 bool enable_timestamp, bool enable_tickcount) {
  log_process_id = enable_process_id;
  log_thread_id = enable_thread_id;
  log_timestamp = enable_timestamp;
  log_tickcount = enable_tickcount;
}

void SetLogAssertHandler(LogAssertHandlerFunction handler) {
  log_assert_handler = handler;
}

void SetLogReportHandler(LogReportHandlerFunction handler) {
  log_report_handler = handler;
}








void DisplayDebugMessage(const std::string& str) {
  if (str.empty())
    return;

#if defined(OS_WIN)
  
  wchar_t prog_name[MAX_PATH];
  GetModuleFileNameW(NULL, prog_name, MAX_PATH);
  wchar_t* backslash = wcsrchr(prog_name, '\\');
  if (backslash)
    backslash[1] = 0;
  wcscat_s(prog_name, MAX_PATH, L"debug_message.exe");

  std::wstring cmdline = base::SysUTF8ToWide(str);
  if (cmdline.empty())
    return;

  STARTUPINFO startup_info;
  memset(&startup_info, 0, sizeof(startup_info));
  startup_info.cb = sizeof(startup_info);

  PROCESS_INFORMATION process_info;
  if (CreateProcessW(prog_name, &cmdline[0], NULL, NULL, false, 0, NULL,
                     NULL, &startup_info, &process_info)) {
    WaitForSingleObject(process_info.hProcess, INFINITE);
    CloseHandle(process_info.hThread);
    CloseHandle(process_info.hProcess);
  } else {
    
    MessageBoxW(NULL, &cmdline[0], L"Fatal error",
                MB_OK | MB_ICONHAND | MB_TOPMOST);
  }
#else
  fprintf(stderr, "%s\n", str.c_str());
#endif
}

#if defined(OS_WIN)
LogMessage::SaveLastError::SaveLastError() : last_error_(::GetLastError()) {
}

LogMessage::SaveLastError::~SaveLastError() {
  ::SetLastError(last_error_);
}
#endif  

LogMessage::LogMessage(const char* file, int line, LogSeverity severity,
                       int ctr)
    : severity_(severity) {
  Init(file, line);
}

LogMessage::LogMessage(const char* file, int line, const CheckOpString& result)
    : severity_(LOG_FATAL) {
  Init(file, line);
  stream_ << "Check failed: " << (*result.str_);
}

LogMessage::LogMessage(const char* file, int line, LogSeverity severity,
                       const CheckOpString& result)
    : severity_(severity) {
  Init(file, line);
  stream_ << "Check failed: " << (*result.str_);
}

LogMessage::LogMessage(const char* file, int line)
     : severity_(LOG_INFO) {
  Init(file, line);
}

LogMessage::LogMessage(const char* file, int line, LogSeverity severity)
    : severity_(severity) {
  Init(file, line);
}


void LogMessage::Init(const char* file, int line) {
  
  const char* last_slash = strrchr(file, '\\');
  if (last_slash)
    file = last_slash + 1;

  

  stream_ <<  '[';
  if (log_process_id)
    stream_ << CurrentProcessId() << ':';
  if (log_thread_id)
    stream_ << CurrentThreadId() << ':';
  if (log_timestamp) {
     time_t t = time(NULL);
#if _MSC_VER >= 1400
    struct tm local_time = {0};
    localtime_s(&local_time, &t);
    struct tm* tm_time = &local_time;
#else
    struct tm* tm_time = localtime(&t);
#endif
    stream_ << std::setfill('0')
            << std::setw(2) << 1 + tm_time->tm_mon
            << std::setw(2) << tm_time->tm_mday
            << '/'
            << std::setw(2) << tm_time->tm_hour
            << std::setw(2) << tm_time->tm_min
            << std::setw(2) << tm_time->tm_sec
            << ':';
  }
  if (log_tickcount)
    stream_ << TickCount() << ':';
  stream_ << log_severity_names[severity_] << ":" << file <<
             "(" << line << ")] ";

  message_start_ = stream_.tellp();
}

LogMessage::~LogMessage() {
  
  
  if (severity_ < min_log_level)
    return;

  std::string str_newline(stream_.str());
#if defined(OS_WIN)
  str_newline.append("\r\n");
#else
  str_newline.append("\n");
#endif

  if (log_filter_prefix && severity_ <= kMaxFilteredLogLevel &&
      str_newline.compare(message_start_, log_filter_prefix->size(),
                          log_filter_prefix->data()) != 0) {
    return;
  }

  if (logging_destination == LOG_ONLY_TO_SYSTEM_DEBUG_LOG ||
      logging_destination == LOG_TO_BOTH_FILE_AND_SYSTEM_DEBUG_LOG) {
#if defined(OS_WIN)
    OutputDebugStringA(str_newline.c_str());
    if (severity_ >= kAlwaysPrintErrorLevel)
#endif
    
    
    
    
    
    
    
    
    
    fprintf(stderr, "%s", str_newline.c_str());
  } else if (severity_ >= kAlwaysPrintErrorLevel) {
    
    
    
    fprintf(stderr, "%s", str_newline.c_str());
  }

  
  if (logging_destination != LOG_NONE &&
      logging_destination != LOG_ONLY_TO_SYSTEM_DEBUG_LOG &&
      InitializeLogFileHandle()) {
    
    
    if (lock_log_file == LOCK_LOG_FILE) {
      
      
      InitLogMutex();

#if defined(OS_WIN)
      DWORD r = ::WaitForSingleObject(log_mutex, INFINITE);
      DCHECK(r != WAIT_ABANDONED);
#elif defined(OS_POSIX)
      pthread_mutex_lock(&log_mutex);
#endif
    } else {
      
      if (!log_lock) {
        
        
        
        
        
        log_lock = new LockImpl();
      }
      log_lock->Lock();
    }

#if defined(OS_WIN)
    SetFilePointer(log_file, 0, 0, SEEK_END);
    DWORD num_written;
    WriteFile(log_file,
              static_cast<const void*>(str_newline.c_str()),
              static_cast<DWORD>(str_newline.length()),
              &num_written,
              NULL);
#else
    fprintf(log_file, "%s", str_newline.c_str());
#endif

    if (lock_log_file == LOCK_LOG_FILE) {
#if defined(OS_WIN)
      ReleaseMutex(log_mutex);
#elif defined(OS_POSIX)
      pthread_mutex_unlock(&log_mutex);
#endif
    } else {
      log_lock->Unlock();
    }
  }

  if (severity_ == LOG_FATAL) {
    
    if (DebugUtil::BeingDebugged()) {
      DebugUtil::BreakDebugger();
    } else {
#ifndef NDEBUG
      
      StackTrace trace;
      stream_ << "\n";  
      trace.OutputToStream(&stream_);
#endif

      if (log_assert_handler) {
        
        log_assert_handler(std::string(stream_.str()));
      } else {
        
        
        
        
        
#ifndef NDEBUG
        DisplayDebugMessage(stream_.str());
#endif
        
        DebugUtil::BreakDebugger();
      }
    }
  } else if (severity_ == LOG_ERROR_REPORT) {
    
    if (log_report_handler) {
      log_report_handler(std::string(stream_.str()));
    } else {
      DisplayDebugMessage(stream_.str());
    }
  }
}

void CloseLogFile() {
  if (!log_file)
    return;

  CloseFile(log_file);
  log_file = NULL;
}

}  

std::ostream& operator<<(std::ostream& out, const wchar_t* wstr) {
  return out << base::SysWideToUTF8(std::wstring(wstr));
}

#endif 
