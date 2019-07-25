





































#ifndef mozilla_FunctionTimer_h
#define mozilla_FunctionTimer_h

#include <stdarg.h>

#include "mozilla/TimeStamp.h"
#include "nscore.h"
#include "nsAutoPtr.h"

#if defined(NS_FORCE_FUNCTION_TIMER) && !defined(NS_FUNCTION_TIMER)
#define NS_FUNCTION_TIMER
#endif





#ifdef NS_FUNCTION_TIMER

#ifdef __GNUC__
#define MOZ_FUNCTION_NAME __PRETTY_FUNCTION__
#elif defined(_MSC_VER)
#define MOZ_FUNCTION_NAME __FUNCTION__
#else
#warning "Define a suitable MOZ_FUNCTION_NAME for this platform"
#define MOZ_FUNCTION_NAME ""
#endif




#define NS_TIME_FUNCTION                                                \
    mozilla::FunctionTimer ft__autogen("%s (line %d)", MOZ_FUNCTION_NAME, __LINE__)






#define NS_TIME_FUNCTION_MIN(_ms)                                       \
    mozilla::FunctionTimer ft__autogen((_ms), "%s (line %d)", MOZ_FUNCTION_NAME, __LINE__)



#define NS_TIME_FUNCTION_MARK_ONLY                                    \
    mozilla::FunctionTimer ft__autogen((-1), "%s (line %d)", MOZ_FUNCTION_NAME, __LINE__)



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



#define NS_TIME_FUNCTION_LATEST \
    mozilla::FunctionTimer::LatestSinceStartup()

#else

#define NS_TIME_FUNCTION do { } while (0)
#define NS_TIME_FUNCTION_MIN(_ms) do { } while (0)
#define NS_TIME_FUNCTION_MARK_ONLY do { } while (0)
#define NS_TIME_FUNCTION_FMT(...) do { } while (0)
#define NS_TIME_FUNCTION_MIN_FMT(_ms, ...) do { } while (0)
#define NS_TIME_FUNCTION_MARK(...) do { } while (0)
#define NS_TIME_FUNCTION_ELAPSED (0)
#define NS_TIME_FUNCTION_ELAPSED_SINCE_MARK (0)
#define NS_TIME_FUNCTION_LATEST (mozilla::TimeDuration(0))

#endif

namespace mozilla {

class FunctionTimerLog
{
public:
    FunctionTimerLog(const char *fname);
    ~FunctionTimerLog();

    void LogString(const char *str);

    TimeDuration LatestSinceStartup() const;

private:
    void *mFile;
    TimeStamp mLatest;
};

class FunctionTimer
{
    static nsAutoPtr<FunctionTimerLog> sLog;
    static char *sBuf1;
    static char *sBuf2;
    static int sBufSize;
    static unsigned sDepth;

    enum { BUF_LOG_LENGTH = 1024 };

public:
    static int InitTimers();

    static int ft_vsnprintf(char *str, int maxlen, const char *fmt, va_list args);
    static int ft_snprintf(char *str, int maxlen, const char *fmt, ...);

    static void LogMessage(const char *s, ...) {
        va_list ap;
        va_start(ap, s);

        if (sLog) {
            ft_vsnprintf(sBuf1, sBufSize, s, ap);
            sLog->LogString(sBuf1);
        }

        va_end(ap);
    }

private:
    void Init(const char *s, va_list ap) {
        if (mEnabled) {
            TimeInit();

            ft_vsnprintf(mString, BUF_LOG_LENGTH, s, ap);

            ft_snprintf(sBuf1, sBufSize, "> (% 3d)%*s|%s%s", mDepth, mDepth, " ", mHasMinMs ? "?MINMS " : "", mString);
            sLog->LogString(sBuf1);
        }
    }

public:
    inline void TimeInit() {
        if (mEnabled) {
            mStart = TimeStamp::Now();
            mLastMark = mStart;
        }
    }

    inline double Elapsed() {
        if (mEnabled)
            return (TimeStamp::Now() - mStart).ToSeconds() * 1000.0;
        return 0.0;
    }

    inline double ElapsedSinceMark() {
        if (mEnabled)
            return (TimeStamp::Now() - mLastMark).ToSeconds() * 1000.0;
        return 0.0;
    }

    static inline TimeDuration LatestSinceStartup() {
        return sLog ? sLog->LatestSinceStartup() : TimeDuration(0);
    }

    FunctionTimer(double minms, const char *s, ...)
        : mMinMs(minms), mHasMinMs(true),
          mEnabled(sLog && s && *s), mDepth(++sDepth)
    {
        va_list ap;
        va_start(ap, s);

        Init(s, ap);

        va_end(ap);
    }

    FunctionTimer(const char *s, ...)
        : mMinMs(0.0), mHasMinMs(false),
          mEnabled(sLog && s && *s), mDepth(++sDepth)
    {
        va_list ap;
        va_start(ap, s);

        Init(s, ap);

        va_end(ap);
    }

    void Mark(const char *s, ...)
    {
        va_list ap;
        va_start(ap, s);

        if (mEnabled) {
            ft_vsnprintf(sBuf1, sBufSize, s, ap);

            TimeStamp now(TimeStamp::Now());
            double ms = (now - mStart).ToSeconds() * 1000.0;
            double msl = (now - mLastMark).ToSeconds() * 1000.0;
            mLastMark = now;

            ft_snprintf(sBuf2, sBufSize, "* (% 3d)%*s|%s%9.2f ms (%9.2f ms total) - %s [%s]", mDepth, mDepth, " ",
                (!mHasMinMs || mMinMs < 0.0 || msl > mMinMs) ? "<MINMS " : "", msl, ms, mString, sBuf1);
            sLog->LogString(sBuf2);
        }

        va_end(ap);
    }

    ~FunctionTimer() {
        if (mEnabled) {
            TimeStamp now(TimeStamp::Now());
            double ms = (now - mStart).ToSeconds() * 1000.0;
            double msl = (now - mLastMark).ToSeconds() * 1000.0;

            ft_snprintf(sBuf1, sBufSize, "< (% 3d)%*s|%s%9.2f ms (%9.2f ms total) - %s", mDepth, mDepth, " ",
                (!mHasMinMs || (mMinMs >= 0.0 && msl > mMinMs)) ? "" : "<MINMS ", msl, ms, mString);
            sLog->LogString(sBuf1);
        }

        --sDepth;
    }

    TimeStamp mStart, mLastMark;
    const double mMinMs;
    char mString[BUF_LOG_LENGTH+1];
    const bool mHasMinMs;
    const bool mEnabled;
    const unsigned mDepth;
};

} 

#endif 


