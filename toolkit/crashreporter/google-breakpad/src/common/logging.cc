


































#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <string>

#include "common/using_std_string.h"
#include "common/logging.h"
#include "common/pathname_stripper.h"

#if defined(_MSC_VER) && _MSC_VER < 1900
#define snprintf _snprintf
#endif

#ifdef __ANDROID__
# include <android/log.h>
#endif

namespace google_breakpad {

LogStream::LogStream(std::ostream &stream, Severity severity,
                     const char *file, int line)
    : stream_(stream) {
  time_t clock;
  time(&clock);
  struct tm tm_struct;
#ifdef _WIN32
  localtime_s(&tm_struct, &clock);
#else
  localtime_r(&clock, &tm_struct);
#endif
  char time_string[20];
  strftime(time_string, sizeof(time_string), "%Y-%m-%d %H:%M:%S", &tm_struct);

  const char *severity_string = "UNKNOWN_SEVERITY";
  switch (severity) {
    case SEVERITY_INFO:
      severity_string = "INFO";
      break;
    case SEVERITY_ERROR:
      severity_string = "ERROR";
      break;
  }

  str_ << time_string << ": " << PathnameStripper::File(file) << ":" <<
          line << ": " << severity_string << ": ";
}

LogStream::~LogStream() {
#ifdef __ANDROID__
  __android_log_print(ANDROID_LOG_ERROR,
                      "Profiler", "%s", str_.str().c_str());
#else
  stream_ << str_.str();
  stream_ << std::endl;
#endif
}

string HexString(uint32_t number) {
  char buffer[11];
  snprintf(buffer, sizeof(buffer), "0x%x", number);
  return string(buffer);
}

string HexString(uint64_t number) {
  char buffer[19];
  snprintf(buffer, sizeof(buffer), "0x%" PRIx64, number);
  return string(buffer);
}

string HexString(int number) {
  char buffer[19];
  snprintf(buffer, sizeof(buffer), "0x%x", number);
  return string(buffer);
}

int ErrnoString(string *error_string) {
  assert(error_string);

  
  
  
  
  error_string->assign(strerror(errno));
  return errno;
}

}  

bool is_power_of_2(uint64_t x_in)
{
  uint64_t x = x_in;
  x = x | (x >> 1);
  x = x | (x >> 2);
  x = x | (x >> 4);
  x = x | (x >> 8);
  x = x | (x >> 16);
  x = x | (x >> 32);
  x = x - (x >> 1);
  
  return x == x_in;
}
