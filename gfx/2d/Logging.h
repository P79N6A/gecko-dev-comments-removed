




































#ifndef MOZILLA_GFX_LOGGING_H_
#define MOZILLA_GFX_LOGGING_H_

#include <string>
#include <sstream>

#include "Point.h"

#ifdef WIN32
#include <windows.h>
#endif

#ifdef PR_LOGGING
#include <prlog.h>

extern PRLogModuleInfo *sGFX2DLog;
#endif

namespace mozilla {
namespace gfx {

const int LOG_DEBUG = 1;
const int LOG_WARNING = 2;

#ifdef PR_LOGGING

inline PRLogModuleLevel PRLogLevelForLevel(int aLevel) {
  switch (aLevel) {
  case LOG_DEBUG:
    return PR_LOG_DEBUG;
  case LOG_WARNING:
    return PR_LOG_WARNING;
  }
  return PR_LOG_DEBUG;
}

#endif

extern int sGfxLogLevel;

static void OutputMessage(const std::string &aString, int aLevel) {
#if defined(WIN32) && !defined(PR_LOGGING)
  if (aLevel >= sGfxLogLevel) {
    ::OutputDebugStringA(aString.c_str());
  }
#elif defined(PR_LOGGING)
  if (PR_LOG_TEST(sGFX2DLog, PRLogLevelForLevel(aLevel))) {
    PR_LogPrint(aString.c_str());
  }
#else
  if (aLevel >= sGfxLogLevel) {
    printf(aString.c_str());
  }
#endif
}

class NoLog
{
public:
  NoLog() {}
  ~NoLog() {}

  template<typename T>
  NoLog &operator <<(const T &aLogText) { return *this; }
};

template<int L>
class Log
{
public:
  Log() {}
  ~Log() { mMessage << '\n'; WriteLog(mMessage.str()); }

  Log &operator <<(const std::string &aLogText) { mMessage << aLogText; return *this; }
  Log &operator <<(unsigned int aInt) { mMessage << aInt; return *this; }
  Log &operator <<(const Size &aSize)
    { mMessage << "(" << aSize.width << "x" << aSize.height << ")"; return *this; }
  Log &operator <<(const IntSize &aSize)
    { mMessage << "(" << aSize.width << "x" << aSize.height << ")"; return *this; }

private:

  void WriteLog(const std::string &aString) {
    OutputMessage(aString, L);
  }

  std::stringstream mMessage;
};

typedef Log<LOG_DEBUG> DebugLog;
typedef Log<LOG_WARNING> WarningLog;

#ifdef GFX_LOG_DEBUG
#define gfxDebug DebugLog
#else
#define gfxDebug if (1) ; else NoLog
#endif
#ifdef GFX_LOG_WARNING
#define gfxWarning WarningLog
#else
#define gfxWarning if (1) ; else NoLog
#endif

}
}

#endif 
