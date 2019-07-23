





































#ifndef mozilla_FunctionTimer_h
#define mozilla_FunctionTimer_h

#include <stdarg.h>

#include "mozilla/TimeStamp.h"
#include "nscore.h"

#if defined(NS_FORCE_FUNCTION_TIMER) && !defined(NS_FUNCTION_TIMER)
#define NS_FUNCTION_TIMER
#endif





#ifdef NS_FUNCTION_TIMER




#define NS_TIME_FUNCTION                                                \
    mozilla::FunctionTimer ft__autogen("%s (line %d)", __FUNCTION__, __LINE__)






#define NS_TIME_FUNCTION_MIN(_ms)                                       \
    mozilla::FunctionTimer ft__autogen((_ms), "%s (line %d)", __FUNCTION__, __LINE__)



#define NS_TIME_FUNCTION_MARK_ONLY                                    \
    mozilla::FunctionTimer ft__autogen((-1), "%s (line %d)", __FUNCTION__, __LINE__)



#define NS_TIME_FUNCTION_FMT(...)                                       \
    mozilla::FunctionTimer ft__autogen(__VA_ARGS__)




#define NS_TIME_FUNCTION_MIN_FMT(_ms, ...)                              \
    mozilla::FunctionTimer ft__autogen((_ms), __VA_ARGS__)






#define NS_TIME_FUNCTION_MARK(...)              \
    ft__autogen.Mark(__VA_ARGS__)



#define NS_TIME_FUNCTION_ELAPSED              \
    ft__autogen.Elapsed()



#define NS_TIME_FUNCTION_ELAPSED_SINCE_MARK                             \
    ft__autogen.ElapsedSinceMark

#else

#define NS_TIME_FUNCTION do { } while (0)
#define NS_TIME_FUNCTION_MIN(_ms) do { } while (0)
#define NS_TIME_FUNCTION_MARK_ONLY do { } while (0)
#define NS_TIME_FUNCTION_FMT(...) do { } while (0)
#define NS_TIME_FUNCTION_MIN_FMT(_ms, ...) do { } while (0)
#define NS_TIME_FUNCTION_MARK(...) do { } while (0)
#define NS_TIME_FUNCTION_ELAPSED (0)
#define NS_TIME_FUNCTION_ELAPSED_SINCE_MARK (0)

#endif

namespace mozilla {

class NS_COM FunctionTimerLog
{
public:
    FunctionTimerLog(const char *fname);
    ~FunctionTimerLog();

    void LogString(const char *str);

private:
    void *mFile;
};

class NS_COM FunctionTimer
{
    static FunctionTimerLog* sLog;
    static char *sBuf1, *sBuf2;
    static int sBufSize;

    enum { BUF_LOG_LENGTH = 256 };

public:
    static int InitTimers();

    static int ft_vsnprintf(char *str, int maxlen, const char *fmt, va_list args);
    static int ft_snprintf(char *str, int maxlen, const char *fmt, ...);

public:
    inline void TimeInit() {
        mStart = TimeStamp::Now();
        mLastMark = mStart;
    }

    inline double Elapsed() {
        return (TimeStamp::Now() - mStart).ToSeconds() * 1000.0;
    }

    inline double ElapsedSinceMark() {
        return (TimeStamp::Now() - mLastMark).ToSeconds() * 1000.0;
    }

    FunctionTimer(double minms, const char *s, ...)
        : mMinMs(minms)
    {
        va_list ap;
        va_start(ap, s);

        if (sLog) {
            TimeInit();

            ft_vsnprintf(mString, BUF_LOG_LENGTH, s, ap);
        }

        va_end(ap);
    }

    FunctionTimer(const char *s, ...)
        : mMinMs(0.0)
    {
        va_list ap;
        va_start(ap, s);

        if (sLog) {
            TimeInit();

            ft_vsnprintf(mString, BUF_LOG_LENGTH, s, ap);

            ft_snprintf(sBuf1, sBufSize, "> %s", mString);
            sLog->LogString(sBuf1);
        }

        va_end(ap);
    }

    void Mark(const char *s, ...)
    {
        va_list ap;
        va_start(ap, s);

        ft_vsnprintf(sBuf1, sBufSize, s, ap);

        TimeStamp now(TimeStamp::Now());
        double ms = (now - mStart).ToSeconds() * 1000.0;
        double msl = (now - mLastMark).ToSeconds() * 1000.0;
        mLastMark = now;

        if (msl > mMinMs) {
            ft_snprintf(sBuf2, sBufSize, "%s- %5.2f ms (%5.2f ms total) - %s [%s]", mMinMs < 0.0 ? "" : "*", msl, ms, mString, sBuf1);
            sLog->LogString(sBuf2);
        }

        va_end(ap);
    }

    ~FunctionTimer() {
        TimeStamp now(TimeStamp::Now());
        double ms = (now - mStart).ToSeconds() * 1000.0;
        double msl = (now - mLastMark).ToSeconds() * 1000.0;

        if (mMinMs < 0.0 || (mMinMs >= 0.0 && msl > mMinMs)) {
            ft_snprintf(sBuf1, sBufSize, "%s %5.2f ms (%5.2f ms total) - %s", mMinMs < 0.0 ? "<" : "*", msl, ms, mString);
            sLog->LogString(sBuf1);
        }
    }

    TimeStamp mStart, mLastMark;
    char mString[BUF_LOG_LENGTH+1];
    double mMinMs;
};

} 

#endif 


