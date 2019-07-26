





#include <windows.h>
#include "base/logging.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_util.h"

namespace {
int min_log_level = 0;
}

namespace logging
{

LogMessage::LogMessage(const char* file, int line, LogSeverity severity,
                       int ctr) :
  line_(line)
{
}

LogMessage::LogMessage(const char* file, int line, int ctr) : line_(line)
{
}

LogMessage::LogMessage(const char* file, int line, std::string* result) :
  severity_(LOG_FATAL),
  file_(file),
  line_(line)
{
}

LogMessage::LogMessage(const char* file, int line, LogSeverity severity,
                       std::string* result) :
  severity_(severity),
  file_(file),
  line_(line)
{
}

LogMessage::~LogMessage()
{
}

LogMessage::SaveLastError::SaveLastError() :
  last_error_(::GetLastError())
{
}

LogMessage::SaveLastError::~SaveLastError()
{
  ::SetLastError(last_error_);
}

SystemErrorCode GetLastSystemErrorCode()
{
  return ::GetLastError();
}

int GetMinLogLevel()
{
  return min_log_level;
}

Win32ErrorLogMessage::Win32ErrorLogMessage(const char* file, int line,
                                           LogSeverity severity,
                                           SystemErrorCode err,
                                           const char* module) :
  err_(err),
  module_(module),
  log_message_(file, line, severity)
{
}

Win32ErrorLogMessage::Win32ErrorLogMessage(const char* file,
                                           int line,
                                           LogSeverity severity,
                                           SystemErrorCode err) :
  err_(err),
  module_(NULL),
  log_message_(file, line, severity)
{
}

Win32ErrorLogMessage::~Win32ErrorLogMessage()
{
}

int GetVlogLevelHelper(const char* file, size_t N)
{
  return 0;
}

} 
