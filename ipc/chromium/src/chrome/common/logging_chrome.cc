



#include "build/build_config.h"

#if defined(OS_WIN)
#include <windows.h>
#endif

#include <iostream>
#include <fstream>

#include "chrome/common/logging_chrome.h"

#include "base/command_line.h"
#include "base/compiler_specific.h"
#include "base/debug_util.h"
#include "base/file_util.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/string_util.h"
#include "base/sys_info.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/env_vars.h"


static bool dialogs_are_suppressed_ = false;



static bool chrome_logging_initialized_ = false;




MSVC_DISABLE_OPTIMIZE();
static void SilentRuntimeAssertHandler(const std::string& str) {
  DebugUtil::BreakDebugger();
}
static void SilentRuntimeReportHandler(const std::string& str) {
}
MSVC_ENABLE_OPTIMIZE();



static void SuppressDialogs() {
  if (dialogs_are_suppressed_)
    return;

  logging::SetLogAssertHandler(SilentRuntimeAssertHandler);
  logging::SetLogReportHandler(SilentRuntimeReportHandler);

#if defined(OS_WIN)
  UINT new_flags = SEM_FAILCRITICALERRORS |
                   SEM_NOGPFAULTERRORBOX |
                   SEM_NOOPENFILEERRORBOX;

  
  UINT existing_flags = SetErrorMode(new_flags);
  SetErrorMode(existing_flags | new_flags);
#endif

  dialogs_are_suppressed_ = true;
}

namespace logging {

void InitChromeLogging(const CommandLine& command_line,
                       OldFileDeletionState delete_old_log_file) {
  DCHECK(!chrome_logging_initialized_) <<
    "Attempted to initialize logging when it was already initialized.";

  
#ifdef NDEBUG
  bool enable_logging = false;
  const wchar_t *kInvertLoggingSwitch = switches::kEnableLogging;
  const logging::LoggingDestination kDefaultLoggingMode =
      logging::LOG_ONLY_TO_FILE;
#else
  bool enable_logging = true;
  const wchar_t *kInvertLoggingSwitch = switches::kDisableLogging;
  const logging::LoggingDestination kDefaultLoggingMode =
      logging::LOG_TO_BOTH_FILE_AND_SYSTEM_DEBUG_LOG;
#endif

  if (command_line.HasSwitch(kInvertLoggingSwitch))
    enable_logging = !enable_logging;

  logging::LoggingDestination log_mode;
  if (enable_logging) {
    log_mode = kDefaultLoggingMode;
  } else {
    log_mode = logging::LOG_NONE;
  }

#if defined(OS_POSIX)
  std::string log_file_name = WideToUTF8(GetLogFileName());
#elif defined(OS_WIN)
  std::wstring log_file_name = GetLogFileName();
#endif

  logging::InitLogging(log_file_name.c_str(),
                       log_mode,
                       logging::LOCK_LOG_FILE,
                       delete_old_log_file);

  
  logging::SetLogItems(true, true, false, true);

  
  
  
  
  if (base::SysInfo::HasEnvVar(env_vars::kHeadless) ||
      command_line.HasSwitch(switches::kNoErrorDialogs))
    SuppressDialogs();

  std::wstring log_filter_prefix =
      command_line.GetSwitchValue(switches::kLogFilterPrefix);
  logging::SetLogFilterPrefix(WideToUTF8(log_filter_prefix).c_str());

  
  
  std::wstring log_level = command_line.GetSwitchValue(switches::kLoggingLevel);
  int level = 0;
  if (StringToInt(WideToUTF16Hack(log_level), &level)) {
    if ((level >= 0) && (level < LOG_NUM_SEVERITIES))
      logging::SetMinLogLevel(level);
  } else {
    logging::SetMinLogLevel(LOG_WARNING);
  }

  chrome_logging_initialized_ = true;
}



void CleanupChromeLogging() {
  DCHECK(chrome_logging_initialized_) <<
    "Attempted to clean up logging when it wasn't initialized.";

  CloseLogFile();

  chrome_logging_initialized_ = false;
}

std::wstring GetLogFileName() {
  std::wstring filename = base::SysInfo::GetEnvVar(env_vars::kLogFileName);
  if (filename != L"")
    return filename;

  const std::wstring log_filename(L"chrome_debug.log");
  std::wstring log_path;

  if (PathService::Get(chrome::DIR_LOGS, &log_path)) {
    file_util::AppendToPath(&log_path, log_filename);
    return log_path;
  } else {
    
    return log_filename;
  }
}

bool DialogsAreSuppressed() {
  return dialogs_are_suppressed_;
}

size_t GetFatalAssertions(AssertionList* assertions) {
  
  
  if (assertions)
    assertions->clear();
  size_t assertion_count = 0;

  std::ifstream log_file;
#if defined(OS_WIN)
  log_file.open(GetLogFileName().c_str());
#elif defined(OS_POSIX)
  log_file.open(WideToUTF8(GetLogFileName()).c_str());
#endif
  if (!log_file.is_open())
    return 0;

  std::string utf8_line;
  std::wstring wide_line;
  while(!log_file.eof()) {
    getline(log_file, utf8_line);
    if (utf8_line.find(":FATAL:") != std::string::npos) {
      wide_line = UTF8ToWide(utf8_line);
      if (assertions)
        assertions->push_back(wide_line);
      ++assertion_count;
    }
  }
  log_file.close();

  return assertion_count;
}

} 
