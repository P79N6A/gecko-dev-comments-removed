



#include "base/logging.h"

#if defined(OS_WIN)
#include <io.h>
#include <windows.h>
typedef HANDLE FileHandle;
typedef HANDLE MutexHandle;

#define write(fd, buf, count) _write(fd, buf, static_cast<unsigned int>(count))

#define STDERR_FILENO 2
#elif defined(OS_MACOSX)
#include <mach/mach.h>
#include <mach/mach_time.h>
#include <mach-o/dyld.h>
#elif defined(OS_POSIX)
#if defined(OS_NACL)
#include <sys/time.h> 
#else
#include <sys/syscall.h>
#endif
#include <time.h>
#endif

#if defined(OS_POSIX)
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define MAX_PATH PATH_MAX
typedef FILE* FileHandle;
typedef pthread_mutex_t* MutexHandle;
#endif

#include <algorithm>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <ostream>

#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/debug/alias.h"
#include "base/debug/debugger.h"
#include "base/debug/stack_trace.h"
#include "base/posix/eintr_wrapper.h"
#include "base/strings/string_piece.h"
#include "base/strings/utf_string_conversions.h"
#include "base/synchronization/lock_impl.h"
#include "base/threading/platform_thread.h"
#include "base/vlog.h"
#if defined(OS_POSIX)
#include "base/safe_strerror_posix.h"
#endif

#if defined(OS_ANDROID)
#include <android/log.h>
#endif

namespace logging {

DcheckState g_dcheck_state = DISABLE_DCHECK_FOR_NON_OFFICIAL_RELEASE_BUILDS;

DcheckState get_dcheck_state() {
  return g_dcheck_state;
}

void set_dcheck_state(DcheckState state) {
  g_dcheck_state = state;
}

namespace {

VlogInfo* g_vlog_info = NULL;
VlogInfo* g_vlog_info_prev = NULL;

const char* const log_severity_names[LOG_NUM_SEVERITIES] = {
  "INFO", "WARNING", "ERROR", "ERROR_REPORT", "FATAL" };

int min_log_level = 0;

LoggingDestination logging_destination = LOG_DEFAULT;


const int kAlwaysPrintErrorLevel = LOG_ERROR;




#if defined(OS_WIN)
typedef std::wstring PathString;
#else
typedef std::string PathString;
#endif
PathString* log_file_name = NULL;


FileHandle log_file = NULL;


bool log_process_id = false;
bool log_thread_id = false;
bool log_timestamp = true;
bool log_tickcount = false;


bool show_error_dialogs = false;



LogAssertHandlerFunction log_assert_handler = NULL;


LogReportHandlerFunction log_report_handler = NULL;

LogMessageHandlerFunction log_message_handler = NULL;



int32 CurrentProcessId() {
#if defined(OS_WIN)
  return GetCurrentProcessId();
#elif defined(OS_POSIX)
  return getpid();
#endif
}

uint64 TickCount() {
#if defined(OS_WIN)
  return GetTickCount();
#elif defined(OS_MACOSX)
  return mach_absolute_time();
#elif defined(OS_NACL)
  
  
  return clock();
#elif defined(OS_POSIX)
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);

  uint64 absolute_micro =
    static_cast<int64>(ts.tv_sec) * 1000000 +
    static_cast<int64>(ts.tv_nsec) / 1000;

  return absolute_micro;
#endif
}

void DeleteFilePath(const PathString& log_name) {
#if defined(OS_WIN)
  DeleteFile(log_name.c_str());
#elif defined (OS_NACL)
  
#else
  unlink(log_name.c_str());
#endif
}

PathString GetDefaultLogFile() {
#if defined(OS_WIN)
  
  wchar_t module_name[MAX_PATH];
  GetModuleFileName(NULL, module_name, MAX_PATH);

  PathString log_file = module_name;
  PathString::size_type last_backslash =
      log_file.rfind('\\', log_file.size());
  if (last_backslash != PathString::npos)
    log_file.erase(last_backslash + 1);
  log_file += L"debug.log";
  return log_file;
#elif defined(OS_POSIX)
  
  return PathString("debug.log");
#endif
}







class LoggingLock {
 public:
  LoggingLock() {
    LockLogging();
  }

  ~LoggingLock() {
    UnlockLogging();
  }

  static void Init(LogLockingState lock_log, const PathChar* new_log_file) {
    if (initialized)
      return;
    lock_log_file = lock_log;
    if (lock_log_file == LOCK_LOG_FILE) {
#if defined(OS_WIN)
      if (!log_mutex) {
        std::wstring safe_name;
        if (new_log_file)
          safe_name = new_log_file;
        else
          safe_name = GetDefaultLogFile();
        
        std::replace(safe_name.begin(), safe_name.end(), '\\', '/');
        std::wstring t(L"Global\\");
        t.append(safe_name);
        log_mutex = ::CreateMutex(NULL, FALSE, t.c_str());

        if (log_mutex == NULL) {
#if DEBUG
          
          int error = GetLastError();  
          base::debug::BreakDebugger();
#endif
          
          return;
        }
      }
#endif
    } else {
      log_lock = new base::internal::LockImpl();
    }
    initialized = true;
  }

 private:
  static void LockLogging() {
    if (lock_log_file == LOCK_LOG_FILE) {
#if defined(OS_WIN)
      ::WaitForSingleObject(log_mutex, INFINITE);
      
      
      
      
      
#elif defined(OS_POSIX)
      pthread_mutex_lock(&log_mutex);
#endif
    } else {
      
      log_lock->Lock();
    }
  }

  static void UnlockLogging() {
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

  
  
  
  static base::internal::LockImpl* log_lock;

  
  
#if defined(OS_WIN)
  static MutexHandle log_mutex;
#elif defined(OS_POSIX)
  static pthread_mutex_t log_mutex;
#endif

  static bool initialized;
  static LogLockingState lock_log_file;
};


bool LoggingLock::initialized = false;

base::internal::LockImpl* LoggingLock::log_lock = NULL;

LogLockingState LoggingLock::lock_log_file = LOCK_LOG_FILE;

#if defined(OS_WIN)

MutexHandle LoggingLock::log_mutex = NULL;
#elif defined(OS_POSIX)
pthread_mutex_t LoggingLock::log_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif




bool InitializeLogFileHandle() {
  if (log_file)
    return true;

  if (!log_file_name) {
    
    
    log_file_name = new PathString(GetDefaultLogFile());
  }

  if ((logging_destination & LOG_TO_FILE) != 0) {
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

void CloseFile(FileHandle log) {
#if defined(OS_WIN)
  CloseHandle(log);
#else
  fclose(log);
#endif
}

void CloseLogFileUnlocked() {
  if (!log_file)
    return;

  CloseFile(log_file);
  log_file = NULL;
}

}  

LoggingSettings::LoggingSettings()
    : logging_dest(LOG_DEFAULT),
      log_file(NULL),
      lock_log(LOCK_LOG_FILE),
      delete_old(APPEND_TO_OLD_LOG_FILE),
      dcheck_state(DISABLE_DCHECK_FOR_NON_OFFICIAL_RELEASE_BUILDS) {}

bool BaseInitLoggingImpl(const LoggingSettings& settings) {
#if defined(OS_NACL)
  
  CHECK_EQ(settings.logging_dest & ~LOG_TO_SYSTEM_DEBUG_LOG, 0);
#endif
  g_dcheck_state = settings.dcheck_state;
  CommandLine* command_line = CommandLine::ForCurrentProcess();
  
  
  if (command_line->HasSwitch(switches::kV) ||
      command_line->HasSwitch(switches::kVModule)) {
    
    
    
    CHECK(!g_vlog_info_prev);
    g_vlog_info_prev = g_vlog_info;

    g_vlog_info =
        new VlogInfo(command_line->GetSwitchValueASCII(switches::kV),
                     command_line->GetSwitchValueASCII(switches::kVModule),
                     &min_log_level);
  }

  logging_destination = settings.logging_dest;

  
  if ((logging_destination & LOG_TO_FILE) == 0)
    return true;

  LoggingLock::Init(settings.lock_log, settings.log_file);
  LoggingLock logging_lock;

  
  
  CloseLogFileUnlocked();

  if (!log_file_name)
    log_file_name = new PathString();
  *log_file_name = settings.log_file;
  if (settings.delete_old == DELETE_OLD_LOG_FILE)
    DeleteFilePath(*log_file_name);

  return InitializeLogFileHandle();
}

void SetMinLogLevel(int level) {
  min_log_level = std::min(LOG_ERROR_REPORT, level);
}

int GetMinLogLevel() {
  return min_log_level;
}

int GetVlogVerbosity() {
  return std::max(-1, LOG_INFO - GetMinLogLevel());
}

int GetVlogLevelHelper(const char* file, size_t N) {
  DCHECK_GT(N, 0U);
  
  
  VlogInfo* vlog_info = g_vlog_info;
  return vlog_info ?
      vlog_info->GetVlogLevel(base::StringPiece(file, N - 1)) :
      GetVlogVerbosity();
}

void SetLogItems(bool enable_process_id, bool enable_thread_id,
                 bool enable_timestamp, bool enable_tickcount) {
  log_process_id = enable_process_id;
  log_thread_id = enable_thread_id;
  log_timestamp = enable_timestamp;
  log_tickcount = enable_tickcount;
}

void SetShowErrorDialogs(bool enable_dialogs) {
  show_error_dialogs = enable_dialogs;
}

void SetLogAssertHandler(LogAssertHandlerFunction handler) {
  log_assert_handler = handler;
}

void SetLogReportHandler(LogReportHandlerFunction handler) {
  log_report_handler = handler;
}

void SetLogMessageHandler(LogMessageHandlerFunction handler) {
  log_message_handler = handler;
}

LogMessageHandlerFunction GetLogMessageHandler() {
  return log_message_handler;
}


#if !defined(COMPILER_MSVC)

template std::string* MakeCheckOpString<int, int>(
    const int&, const int&, const char* names);
template std::string* MakeCheckOpString<unsigned long, unsigned long>(
    const unsigned long&, const unsigned long&, const char* names);
template std::string* MakeCheckOpString<unsigned long, unsigned int>(
    const unsigned long&, const unsigned int&, const char* names);
template std::string* MakeCheckOpString<unsigned int, unsigned long>(
    const unsigned int&, const unsigned long&, const char* names);
template std::string* MakeCheckOpString<std::string, std::string>(
    const std::string&, const std::string&, const char* name);
#endif






void DisplayDebugMessageInDialog(const std::string& str) {
  if (str.empty())
    return;

  if (!show_error_dialogs)
    return;

#if defined(OS_WIN)
  
  
  
  
  
  
  wchar_t prog_name[MAX_PATH];
  GetModuleFileNameW(NULL, prog_name, MAX_PATH);
  wchar_t* backslash = wcsrchr(prog_name, '\\');
  if (backslash)
    backslash[1] = 0;
  wcscat_s(prog_name, MAX_PATH, L"debug_message.exe");

  std::wstring cmdline = UTF8ToWide(str);
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
    : severity_(severity), file_(file), line_(line) {
  Init(file, line);
}

LogMessage::LogMessage(const char* file, int line)
    : severity_(LOG_INFO), file_(file), line_(line) {
  Init(file, line);
}

LogMessage::LogMessage(const char* file, int line, LogSeverity severity)
    : severity_(severity), file_(file), line_(line) {
  Init(file, line);
}

LogMessage::LogMessage(const char* file, int line, std::string* result)
    : severity_(LOG_FATAL), file_(file), line_(line) {
  Init(file, line);
  stream_ << "Check failed: " << *result;
  delete result;
}

LogMessage::LogMessage(const char* file, int line, LogSeverity severity,
                       std::string* result)
    : severity_(severity), file_(file), line_(line) {
  Init(file, line);
  stream_ << "Check failed: " << *result;
  delete result;
}

LogMessage::~LogMessage() {
#if !defined(NDEBUG) && !defined(OS_NACL)
  if (severity_ == LOG_FATAL) {
    
    base::debug::StackTrace trace;
    stream_ << std::endl;  
    trace.OutputToStream(&stream_);
  }
#endif
  stream_ << std::endl;
  std::string str_newline(stream_.str());

  
  if (log_message_handler &&
      log_message_handler(severity_, file_, line_,
                          message_start_, str_newline)) {
    
    return;
  }

  if ((logging_destination & LOG_TO_SYSTEM_DEBUG_LOG) != 0) {
#if defined(OS_WIN)
    OutputDebugStringA(str_newline.c_str());
#elif defined(OS_ANDROID)
    android_LogPriority priority =
        (severity_ < 0) ? ANDROID_LOG_VERBOSE : ANDROID_LOG_UNKNOWN;
    switch (severity_) {
      case LOG_INFO:
        priority = ANDROID_LOG_INFO;
        break;
      case LOG_WARNING:
        priority = ANDROID_LOG_WARN;
        break;
      case LOG_ERROR:
      case LOG_ERROR_REPORT:
        priority = ANDROID_LOG_ERROR;
        break;
      case LOG_FATAL:
        priority = ANDROID_LOG_FATAL;
        break;
    }
    __android_log_write(priority, "chromium", str_newline.c_str());
#endif
    fprintf(stderr, "%s", str_newline.c_str());
    fflush(stderr);
  } else if (severity_ >= kAlwaysPrintErrorLevel) {
    
    
    
    fprintf(stderr, "%s", str_newline.c_str());
    fflush(stderr);
  }

  
  if ((logging_destination & LOG_TO_FILE) != 0) {
    
    
    
    
    
    
    
    LoggingLock::Init(LOCK_LOG_FILE, NULL);
    LoggingLock logging_lock;
    if (InitializeLogFileHandle()) {
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
      fflush(log_file);
#endif
    }
  }

  if (severity_ == LOG_FATAL) {
    
    
    char str_stack[1024];
    str_newline.copy(str_stack, arraysize(str_stack));
    base::debug::Alias(str_stack);

    
    if (base::debug::BeingDebugged()) {
      base::debug::BreakDebugger();
    } else {
      if (log_assert_handler) {
        
        log_assert_handler(std::string(stream_.str()));
      } else {
        
        
        
        
        
#ifndef NDEBUG
        DisplayDebugMessageInDialog(stream_.str());
#endif
        
        base::debug::BreakDebugger();
      }
    }
  } else if (severity_ == LOG_ERROR_REPORT) {
    
    if (log_report_handler) {
      log_report_handler(std::string(stream_.str()));
    } else {
      DisplayDebugMessageInDialog(stream_.str());
    }
  }
}


void LogMessage::Init(const char* file, int line) {
  base::StringPiece filename(file);
  size_t last_slash_pos = filename.find_last_of("\\/");
  if (last_slash_pos != base::StringPiece::npos)
    filename.remove_prefix(last_slash_pos + 1);

  

  stream_ <<  '[';
  if (log_process_id)
    stream_ << CurrentProcessId() << ':';
  if (log_thread_id)
    stream_ << base::PlatformThread::CurrentId() << ':';
  if (log_timestamp) {
    time_t t = time(NULL);
    struct tm local_time = {0};
#if _MSC_VER >= 1400
    localtime_s(&local_time, &t);
#else
    localtime_r(&t, &local_time);
#endif
    struct tm* tm_time = &local_time;
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
  if (severity_ >= 0)
    stream_ << log_severity_names[severity_];
  else
    stream_ << "VERBOSE" << -severity_;

  stream_ << ":" << filename << "(" << line << ")] ";

  message_start_ = stream_.tellp();
}

#if defined(OS_WIN)



typedef DWORD SystemErrorCode;
#endif

SystemErrorCode GetLastSystemErrorCode() {
#if defined(OS_WIN)
  return ::GetLastError();
#elif defined(OS_POSIX)
  return errno;
#else
#error Not implemented
#endif
}

#if defined(OS_WIN)
Win32ErrorLogMessage::Win32ErrorLogMessage(const char* file,
                                           int line,
                                           LogSeverity severity,
                                           SystemErrorCode err,
                                           const char* module)
    : err_(err),
      module_(module),
      log_message_(file, line, severity) {
}

Win32ErrorLogMessage::Win32ErrorLogMessage(const char* file,
                                           int line,
                                           LogSeverity severity,
                                           SystemErrorCode err)
    : err_(err),
      module_(NULL),
      log_message_(file, line, severity) {
}

Win32ErrorLogMessage::~Win32ErrorLogMessage() {
  const int error_message_buffer_size = 256;
  char msgbuf[error_message_buffer_size];
  DWORD flags = FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
  HMODULE hmod;
  if (module_) {
    hmod = GetModuleHandleA(module_);
    if (hmod) {
      flags |= FORMAT_MESSAGE_FROM_HMODULE;
    } else {
      
      
      
      DPLOG(WARNING) << "Couldn't open module " << module_
          << " for error message query";
    }
  } else {
    hmod = NULL;
  }
  DWORD len = FormatMessageA(flags,
                             hmod,
                             err_,
                             0,
                             msgbuf,
                             sizeof(msgbuf) / sizeof(msgbuf[0]),
                             NULL);
  if (len) {
    while ((len > 0) &&
           isspace(static_cast<unsigned char>(msgbuf[len - 1]))) {
      msgbuf[--len] = 0;
    }
    stream() << ": " << msgbuf;
  } else {
    stream() << ": Error " << GetLastError() << " while retrieving error "
        << err_;
  }
  
  
  DWORD last_error = err_;
  base::debug::Alias(&last_error);
}
#elif defined(OS_POSIX)
ErrnoLogMessage::ErrnoLogMessage(const char* file,
                                 int line,
                                 LogSeverity severity,
                                 SystemErrorCode err)
    : err_(err),
      log_message_(file, line, severity) {
}

ErrnoLogMessage::~ErrnoLogMessage() {
  stream() << ": " << safe_strerror(err_);
}
#endif  

void CloseLogFile() {
  LoggingLock logging_lock;
  CloseLogFileUnlocked();
}

void RawLog(int level, const char* message) {
  if (level >= min_log_level) {
    size_t bytes_written = 0;
    const size_t message_len = strlen(message);
    int rv;
    while (bytes_written < message_len) {
      rv = HANDLE_EINTR(
          write(STDERR_FILENO, message + bytes_written,
                message_len - bytes_written));
      if (rv < 0) {
        
        break;
      }
      bytes_written += rv;
    }

    if (message_len > 0 && message[message_len - 1] != '\n') {
      do {
        rv = HANDLE_EINTR(write(STDERR_FILENO, "\n", 1));
        if (rv < 0) {
          
          break;
        }
      } while (rv != 1);
    }
  }

  if (level == LOG_FATAL)
    base::debug::BreakDebugger();
}


#undef write

#if defined(OS_WIN)
std::wstring GetLogFileFullPath() {
  if (log_file_name)
    return *log_file_name;
  return std::wstring();
}
#endif

}  

std::ostream& operator<<(std::ostream& out, const wchar_t* wstr) {
  return out << WideToUTF8(std::wstring(wstr));
}
