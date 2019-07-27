




#ifndef MOZILLA_GFX_LOGGING_H_
#define MOZILLA_GFX_LOGGING_H_

#include <string>
#include <sstream>
#include <stdio.h>

#ifdef MOZ_LOGGING
#include <prlog.h>
#endif

#if defined(MOZ_WIDGET_GONK) || defined(MOZ_WIDGET_ANDROID)
#include "nsDebug.h"
#endif
#include "Point.h"
#include "BaseRect.h"
#include "Matrix.h"
#include "mozilla/TypedEnum.h"

#ifdef WIN32







extern "C" __declspec(dllimport) void __stdcall OutputDebugStringA(const char* lpOutputString);
#endif

#if defined(PR_LOGGING)
extern GFX2D_API PRLogModuleInfo *GetGFX2DLog();
#endif

namespace mozilla {
namespace gfx {




const int LOG_CRITICAL = 1;
const int LOG_WARNING = 2;
const int LOG_DEBUG = 3;
const int LOG_DEBUG_PRLOG = 4;
const int LOG_EVERYTHING = 5; 

#if defined(DEBUG)
const int LOG_DEFAULT = LOG_EVERYTHING;
#else
const int LOG_DEFAULT = LOG_CRITICAL;
#endif

#if defined(PR_LOGGING)

inline PRLogModuleLevel PRLogLevelForLevel(int aLevel) {
  switch (aLevel) {
  case LOG_CRITICAL:
    return PR_LOG_ERROR;
  case LOG_WARNING:
    return PR_LOG_WARNING;
  case LOG_DEBUG:
    return PR_LOG_DEBUG;
  case LOG_DEBUG_PRLOG:
    return PR_LOG_DEBUG;
  case LOG_EVERYTHING:
    return PR_LOG_ALWAYS;
  }
  return PR_LOG_DEBUG;
}

#endif

class PreferenceAccess
{
public:
  virtual ~PreferenceAccess();

  
  
  
  virtual void LivePref(const char* aName, int32_t* aVar, int32_t aDefault);

public:
  static void SetAccess(PreferenceAccess* aAccess);

public:
  
  
  

  
  
  
  
  
  
  
  static int32_t sGfxLogLevel;

private:
  static void RegisterAll() {
    
    
    
    sAccess->LivePref("gfx.logging.level", &sGfxLogLevel, LOG_DEFAULT);
  }
  static PreferenceAccess* sAccess;
};

struct BasicLogger
{
  
  
  
  static bool ShouldOutputMessage(int aLevel) {
    if (PreferenceAccess::sGfxLogLevel >= aLevel) {
#if defined(WIN32) && !defined(PR_LOGGING)
      return true;
#elif defined(MOZ_WIDGET_GONK) || defined(MOZ_WIDGET_ANDROID)
      return true;
#elif defined(PR_LOGGING)
      if (PR_LOG_TEST(GetGFX2DLog(), PRLogLevelForLevel(aLevel))) {
        return true;
      } else if ((PreferenceAccess::sGfxLogLevel >= LOG_DEBUG_PRLOG) ||
                 (aLevel < LOG_DEBUG)) {
        return true;
      }
#else
      return true;
#endif
    }
    return false;
  }

  static void OutputMessage(const std::string &aString,
                            int aLevel,
                            bool aNoNewline) {
    
    
    
    
    
    
    
    
    
    if (PreferenceAccess::sGfxLogLevel >= aLevel) {
#if defined(WIN32) && !defined(PR_LOGGING)
      ::OutputDebugStringA((aNoNewline ? aString : aString+"\n").c_str());
#elif defined(MOZ_WIDGET_GONK) || defined(MOZ_WIDGET_ANDROID)
      printf_stderr("%s%s", aString.c_str(), aNoNewline ? "" : "\n");
#elif defined(PR_LOGGING)
      if (PR_LOG_TEST(GetGFX2DLog(), PRLogLevelForLevel(aLevel))) {
        PR_LogPrint("%s%s", aString.c_str(), aNoNewline ? "" : "\n");
      } else if ((PreferenceAccess::sGfxLogLevel >= LOG_DEBUG_PRLOG) ||
                 (aLevel < LOG_DEBUG)) {
        printf("%s%s", aString.c_str(), aNoNewline ? "" : "\n");
      }
#else
      printf("%s%s", aString.c_str(), aNoNewline ? "" : "\n");
#endif
    }
  }
};

struct CriticalLogger {
  static void OutputMessage(const std::string &aString, int aLevel, bool aNoNewline);
};



class LogForwarder {
public:
  virtual ~LogForwarder() {}
  virtual void Log(const std::string &aString) = 0;
};

class NoLog
{
public:
  NoLog() {}
  ~NoLog() {}

  template<typename T>
  NoLog &operator <<(const T &aLogText) { return *this; }
};

MOZ_BEGIN_ENUM_CLASS(LogOptions, int)
  NoNewline = 0x01,
  AutoPrefix = 0x02
MOZ_END_ENUM_CLASS(LogOptions)

template<typename T>
struct Hexa {
  explicit Hexa(T aVal) : mVal(aVal) {}
  T mVal;
};
template<typename T>
Hexa<T> hexa(T val) { return Hexa<T>(val); }

template<int L, typename Logger = BasicLogger>
class Log
{
public:
  explicit Log(int aOptions = (int)LogOptions::AutoPrefix)
    : mOptions(aOptions)
    , mLogIt(BasicLogger::ShouldOutputMessage(L))
  {
    if (mLogIt && AutoPrefix()) {
      mMessage << "[GFX" << L << "]: ";
    }
  }
  ~Log() {
    Flush();
  }

  void Flush() {
    if (MOZ_LIKELY(!LogIt())) return;

    std::string str = mMessage.str();
    if (!str.empty()) {
      WriteLog(str);
    }
    if (AutoPrefix()) {
      mMessage.str("[GFX");
      mMessage << L << "]: ";
    } else {
      mMessage.str("");
    }
    mMessage.clear();
  }

  Log &operator <<(char aChar) {
    if (MOZ_UNLIKELY(LogIt())) {
      mMessage << aChar;
    }
    return *this;
  }
  Log &operator <<(const std::string &aLogText) { 
    if (MOZ_UNLIKELY(LogIt())) {
      mMessage << aLogText;
    }
    return *this;
  }
  Log &operator <<(const char aStr[]) {
    if (MOZ_UNLIKELY(LogIt())) {
      mMessage << static_cast<const char*>(aStr);
    }
    return *this;
  }
  Log &operator <<(bool aBool) {
    if (MOZ_UNLIKELY(LogIt())) {
      mMessage << (aBool ? "true" : "false");
    }
    return *this;
  }
  Log &operator <<(int aInt) {
    if (MOZ_UNLIKELY(LogIt())) {
      mMessage << aInt;
    }
    return *this;
  }
  Log &operator <<(unsigned int aInt) {
    if (MOZ_UNLIKELY(LogIt())) {
      mMessage << aInt;
    }
    return *this;
  }
  Log &operator <<(long aLong) {
    if (MOZ_UNLIKELY(LogIt())) {
      mMessage << aLong;
    }
    return *this;
  }
  Log &operator <<(unsigned long aLong) {
    if (MOZ_UNLIKELY(LogIt())) {
      mMessage << aLong;
    }
    return *this;
  }
  Log &operator <<(long long aLong) {
    if (MOZ_UNLIKELY(LogIt())) {
      mMessage << aLong;
    }
    return *this;
  }
  Log &operator <<(unsigned long long aLong) {
    if (MOZ_UNLIKELY(LogIt())) {
      mMessage << aLong;
    }
    return *this;
  }
  Log &operator <<(Float aFloat) {
    if (MOZ_UNLIKELY(LogIt())) {
      mMessage << aFloat;
    }
    return *this;
  }
  Log &operator <<(double aDouble) {
    if (MOZ_UNLIKELY(LogIt())) {
      mMessage << aDouble;
    }
    return *this;
  }
  template <typename T, typename Sub, typename Coord>
  Log &operator <<(const BasePoint<T, Sub, Coord>& aPoint) {
    if (MOZ_UNLIKELY(LogIt())) {
      mMessage << "Point" << aPoint;
    }
    return *this;
  }
  template <typename T, typename Sub>
  Log &operator <<(const BaseSize<T, Sub>& aSize) {
    if (MOZ_UNLIKELY(LogIt())) {
      mMessage << "Size(" << aSize.width << "," << aSize.height << ")";
    }
    return *this;
  }
  template <typename T, typename Sub, typename Point, typename SizeT, typename Margin>
  Log &operator <<(const BaseRect<T, Sub, Point, SizeT, Margin>& aRect) {
    if (MOZ_UNLIKELY(LogIt())) {
      mMessage << "Rect" << aRect;
    }
    return *this;
  }
  Log &operator<<(const Matrix& aMatrix) {
    if (MOZ_UNLIKELY(LogIt())) {
      mMessage << "Matrix(" << aMatrix._11 << " " << aMatrix._12 << " ; " << aMatrix._21 << " " << aMatrix._22 << " ; " << aMatrix._31 << " " << aMatrix._32 << ")";
    }
    return *this;
  }
  template<typename T>
  Log &operator<<(Hexa<T> aHex) {
    if (MOZ_UNLIKELY(LogIt())) {
      mMessage << "0x" << std::hex << aHex.mVal << std::dec;
    }
    return *this;
  }

  inline bool LogIt() const { return mLogIt; }
  inline bool NoNewline() const { return mOptions & int(LogOptions::NoNewline); }
  inline bool AutoPrefix() const { return mOptions & int(LogOptions::AutoPrefix); }


private:
  void WriteLog(const std::string &aString) {
    if (MOZ_UNLIKELY(LogIt())) {
      Logger::OutputMessage(aString, L, NoNewline());
    }
  }

  std::stringstream mMessage;
  int mOptions;
  bool mLogIt;
};

typedef Log<LOG_DEBUG> DebugLog;
typedef Log<LOG_WARNING> WarningLog;
typedef Log<LOG_CRITICAL, CriticalLogger> CriticalLog;

#ifdef GFX_LOG_DEBUG
#define gfxDebug mozilla::gfx::DebugLog
#else
#define gfxDebug if (1) ; else mozilla::gfx::NoLog
#endif
#ifdef GFX_LOG_WARNING
#define gfxWarning mozilla::gfx::WarningLog
#else
#define gfxWarning if (1) ; else mozilla::gfx::NoLog
#endif


#define gfxCriticalError mozilla::gfx::CriticalLog



#ifdef __cplusplus
#ifdef DEBUG
inline bool MOZ2D_warn_if_impl(bool aCondition, const char* aExpr,
                               const char* aFile, int32_t aLine)
{
  if (MOZ_UNLIKELY(aCondition)) {
    gfxWarning() << aExpr << " at " << aFile << ":" << aLine;
  }
  return aCondition;
}
#define MOZ2D_WARN_IF(condition) \
  MOZ2D_warn_if_impl(condition, #condition, __FILE__, __LINE__)
#else
#define MOZ2D_WARN_IF(condition) (bool)(condition)
#endif
#endif

const int INDENT_PER_LEVEL = 2;

class TreeLog
{
public:
  explicit TreeLog(const std::string& aPrefix = "")
        : mLog(int(LogOptions::NoNewline)),
          mPrefix(aPrefix),
          mDepth(0),
          mStartOfLine(true),
          mConditionedOnPref(false),
          mPrefFunction(nullptr) {}

  template <typename T>
  TreeLog& operator<<(const T& aObject) {
    if (mConditionedOnPref && !mPrefFunction()) {
      return *this;
    }
    if (mStartOfLine) {
      mLog << '[' << mPrefix << "] " << std::string(mDepth * INDENT_PER_LEVEL, ' ');
      mStartOfLine = false;
    }
    mLog << aObject;
    if (EndsInNewline(aObject)) {
      
      
      mLog.Flush();
      mStartOfLine = true;
    }
    return *this;
  }

  void IncreaseIndent() { ++mDepth; }
  void DecreaseIndent() { --mDepth; }

  void ConditionOnPrefFunction(bool(*aPrefFunction)()) {
    mConditionedOnPref = true;
    mPrefFunction = aPrefFunction;
  }
private:
  Log<LOG_DEBUG> mLog;
  std::string mPrefix;
  uint32_t mDepth;
  bool mStartOfLine;
  bool mConditionedOnPref;
  bool (*mPrefFunction)();

  template <typename T>
  static bool EndsInNewline(const T& aObject) {
    return false;
  }

  static bool EndsInNewline(const std::string& aString) {
    return !aString.empty() && aString[aString.length() - 1] == '\n';
  }

  static bool EndsInNewline(char aChar) {
    return aChar == '\n';
  }

  static bool EndsInNewline(const char* aString) {
    return EndsInNewline(std::string(aString));
  }
};

class TreeAutoIndent
{
public:
  explicit TreeAutoIndent(TreeLog& aTreeLog) : mTreeLog(aTreeLog) {
    mTreeLog.IncreaseIndent();
  }
  ~TreeAutoIndent() {
    mTreeLog.DecreaseIndent();
  }
private:
  TreeLog& mTreeLog;
};

}
}

#endif 
