









































#ifdef SOLARIS
#define _REENTRANT 1
#endif
#include <string.h>
#include <time.h>

#include "jsstdint.h"
#include "jstypes.h"
#include "jsutil.h"

#include "jsprf.h"
#include "jslock.h"
#include "prmjtime.h"

#define PRMJ_DO_MILLISECONDS 1

#ifdef XP_OS2
#include <sys/timeb.h>
#endif
#ifdef XP_WIN
#include <windef.h>
#include <winbase.h>
#include <math.h>     
#include <mmsystem.h> 

#if _MSC_VER >= 1400
#define NS_HAVE_INVALID_PARAMETER_HANDLER 1
#endif
#ifdef NS_HAVE_INVALID_PARAMETER_HANDLER
#include <stdlib.h>   
#include <crtdbg.h>   
#endif

#ifdef JS_THREADSAFE
#include <prinit.h>
#endif

#endif

#ifdef XP_UNIX

#ifdef _SVID_GETTOD   
extern int gettimeofday(struct timeval *tv);
#endif

#include <sys/time.h>

#endif 

#define PRMJ_YEAR_DAYS 365L
#define PRMJ_FOUR_YEARS_DAYS (4 * PRMJ_YEAR_DAYS + 1)
#define PRMJ_CENTURY_DAYS (25 * PRMJ_FOUR_YEARS_DAYS - 1)
#define PRMJ_FOUR_CENTURIES_DAYS (4 * PRMJ_CENTURY_DAYS + 1)
#define PRMJ_HOUR_SECONDS  3600L
#define PRMJ_DAY_SECONDS  (24L * PRMJ_HOUR_SECONDS)
#define PRMJ_YEAR_SECONDS (PRMJ_DAY_SECONDS * PRMJ_YEAR_DAYS)
#define PRMJ_MAX_UNIX_TIMET 2145859200L /*time_t value equiv. to 12/31/2037 */


static inline bool
ComputeLocalTime(time_t local, struct tm *ptm)
{
#ifdef HAVE_LOCALTIME_R
    return localtime_r(&local, ptm);
#else
    struct tm *otm = localtime(&local);
    if (!otm)
        return false;
    *ptm = *otm;
    return true;
#endif
}




JSInt32
PRMJ_LocalGMTDifference()
{
#if defined(XP_WIN)
    




    _tzset();
#endif

    




    int day = 0;
    struct tm tm;

    if (!ComputeLocalTime(0, &tm))
        return 0;
    if (tm.tm_isdst > 0) {
        day = 180;
        if (!ComputeLocalTime(PRMJ_DAY_SECONDS * day, &tm))
            return 0;
    }

    int time = (tm.tm_hour * 3600) + (tm.tm_min * 60) + tm.tm_sec;
    time = PRMJ_DAY_SECONDS - time;

    if (tm.tm_yday == day)
        time -= PRMJ_DAY_SECONDS;

    return time;
}


#define G1970GMTMICROHI        0x00dcdcad /* micro secs to 1970 hi */
#define G1970GMTMICROLOW       0x8b3fa000 /* micro secs to 1970 low */

#define G2037GMTMICROHI        0x00e45fab /* micro secs to 2037 high */
#define G2037GMTMICROLOW       0x7a238000 /* micro secs to 2037 low */

#ifdef HAVE_SYSTEMTIMETOFILETIME

static const JSInt64 win2un = 0x19DB1DED53E8000;

#define FILETIME2INT64(ft) (((JSInt64)ft.dwHighDateTime) << 32LL | (JSInt64)ft.dwLowDateTime)

#endif

#if defined(HAVE_GETSYSTEMTIMEASFILETIME) || defined(HAVE_SYSTEMTIMETOFILETIME)

#if defined(HAVE_GETSYSTEMTIMEASFILETIME)
inline void
LowResTime(LPFILETIME lpft)
{
    GetSystemTimeAsFileTime(lpft);
}
#elif defined(HAVE_SYSTEMTIMETOFILETIME)
inline void
LowResTime(LPFILETIME lpft)
{
    GetCurrentFT(lpft);
}
#else
#error "No implementation of PRMJ_Now was selected."
#endif

typedef struct CalibrationData {
    long double freq;         
    long double offset;       
    long double timer_offset; 

    
    JSInt64 last;

    JSBool calibrated;

#ifdef JS_THREADSAFE
    CRITICAL_SECTION data_lock;
    CRITICAL_SECTION calibration_lock;
#endif
} CalibrationData;

static CalibrationData calibration = { 0 };

static void
NowCalibrate()
{
    FILETIME ft, ftStart;
    LARGE_INTEGER liFreq, now;

    if (calibration.freq == 0.0) {
        if(!QueryPerformanceFrequency(&liFreq)) {
            
            calibration.freq = -1.0;
        } else {
            calibration.freq = (long double) liFreq.QuadPart;
        }
    }
    if (calibration.freq > 0.0) {
        JSInt64 calibrationDelta = 0;

        

        timeBeginPeriod(1);
        LowResTime(&ftStart);
        do {
            LowResTime(&ft);
        } while (memcmp(&ftStart,&ft, sizeof(ft)) == 0);
        timeEndPeriod(1);

        




        QueryPerformanceCounter(&now);

        calibration.offset = (long double) FILETIME2INT64(ft);
        calibration.timer_offset = (long double) now.QuadPart;

        


        calibration.offset -= win2un;
        calibration.offset *= 0.1;
        calibration.last = 0;

        calibration.calibrated = JS_TRUE;
    }
}

#define CALIBRATIONLOCK_SPINCOUNT 0
#define DATALOCK_SPINCOUNT 4096
#define LASTLOCK_SPINCOUNT 4096

#ifdef JS_THREADSAFE
static PRStatus
NowInit(void)
{
    memset(&calibration, 0, sizeof(calibration));
    NowCalibrate();
    InitializeCriticalSectionAndSpinCount(&calibration.calibration_lock, CALIBRATIONLOCK_SPINCOUNT);
    InitializeCriticalSectionAndSpinCount(&calibration.data_lock, DATALOCK_SPINCOUNT);
    return PR_SUCCESS;
}

void
PRMJ_NowShutdown()
{
    DeleteCriticalSection(&calibration.calibration_lock);
    DeleteCriticalSection(&calibration.data_lock);
}

#define MUTEX_LOCK(m) EnterCriticalSection(m)
#define MUTEX_TRYLOCK(m) TryEnterCriticalSection(m)
#define MUTEX_UNLOCK(m) LeaveCriticalSection(m)
#define MUTEX_SETSPINCOUNT(m, c) SetCriticalSectionSpinCount((m),(c))

static PRCallOnceType calibrationOnce = { 0 };

#else

#define MUTEX_LOCK(m)
#define MUTEX_TRYLOCK(m) 1
#define MUTEX_UNLOCK(m)
#define MUTEX_SETSPINCOUNT(m, c)

#endif

#endif 


#if defined(XP_OS2)
JSInt64
PRMJ_Now(void)
{
    JSInt64 s, us, ms2us, s2us;
    struct timeb b;

    ftime(&b);
    ms2us = (JSInt64) PRMJ_USEC_PER_MSEC;
    s2us = (JSInt64) PRMJ_USEC_PER_SEC;
    s = (JSInt64) b.time;
    us = (JSInt64) b.millitm;
    us *= ms2us;
    s *= s2us;
    s += us;
    return s;
}

#elif defined(XP_UNIX)
JSInt64
PRMJ_Now(void)
{
    struct timeval tv;
    JSInt64 s, us, s2us;

#ifdef _SVID_GETTOD   
    gettimeofday(&tv);
#else
    gettimeofday(&tv, 0);
#endif 
    s2us = (JSInt64) PRMJ_USEC_PER_SEC;
    s = (JSInt64) tv.tv_sec;
    us = (JSInt64) tv.tv_usec;
    s *= s2us;
    s += us;
    return s;
}

#else


































































int CALIBRATION_DELAY_COUNT = 10;

JSInt64
PRMJ_Now(void)
{
    static int nCalls = 0;
    long double lowresTime, highresTimerValue;
    FILETIME ft;
    LARGE_INTEGER now;
    JSBool calibrated = JS_FALSE;
    JSBool needsCalibration = JS_FALSE;
    JSInt64 returnedTime;
    long double cachedOffset = 0.0;

    



    int thiscall = JS_ATOMIC_INCREMENT(&nCalls);
    if (thiscall <= CALIBRATION_DELAY_COUNT) {
        LowResTime(&ft);
        return (FILETIME2INT64(ft)-win2un)/10L;
    }

    
#ifdef JS_THREADSAFE
    PR_CallOnce(&calibrationOnce, NowInit);
#endif
    do {
        if (!calibration.calibrated || needsCalibration) {
            MUTEX_LOCK(&calibration.calibration_lock);
            MUTEX_LOCK(&calibration.data_lock);

            
            if(calibration.offset == cachedOffset) {
                

                MUTEX_SETSPINCOUNT(&calibration.data_lock, 0);

                NowCalibrate();

                calibrated = JS_TRUE;

                
                MUTEX_SETSPINCOUNT(&calibration.data_lock, DATALOCK_SPINCOUNT);
            }
            MUTEX_UNLOCK(&calibration.data_lock);
            MUTEX_UNLOCK(&calibration.calibration_lock);
        }


        
        LowResTime(&ft);
        lowresTime = 0.1*(long double)(FILETIME2INT64(ft) - win2un);

        if (calibration.freq > 0.0) {
            long double highresTime, diff;

            DWORD timeAdjustment, timeIncrement;
            BOOL timeAdjustmentDisabled;

            
            long double skewThreshold = 15625.25;
            
            QueryPerformanceCounter(&now);
            highresTimerValue = (long double)now.QuadPart;

            MUTEX_LOCK(&calibration.data_lock);
            highresTime = calibration.offset + PRMJ_USEC_PER_SEC*
                 (highresTimerValue-calibration.timer_offset)/calibration.freq;
            cachedOffset = calibration.offset;

            

            calibration.last = JS_MAX(calibration.last,(JSInt64)highresTime);
            returnedTime = calibration.last;
            MUTEX_UNLOCK(&calibration.data_lock);

            
            if (GetSystemTimeAdjustment(&timeAdjustment,
                                        &timeIncrement,
                                        &timeAdjustmentDisabled)) {
                if (timeAdjustmentDisabled) {
                    
                    skewThreshold = timeAdjustment/10.0;
                } else {
                    
                    skewThreshold = timeIncrement/10.0;
                }
            }

            
            diff = lowresTime - highresTime;

            




            if (fabs(diff) > 2*skewThreshold) {
                

                if (calibrated) {
                    








                    returnedTime = (JSInt64)lowresTime;
                    needsCalibration = JS_FALSE;
                } else {
                    








                    needsCalibration = JS_TRUE;
                }
            } else {
                
                returnedTime = (JSInt64)highresTime;
                needsCalibration = JS_FALSE;
            }
        } else {
            
            returnedTime = (JSInt64)lowresTime;
        }
    } while (needsCalibration);

    return returnedTime;
}
#endif

#ifdef NS_HAVE_INVALID_PARAMETER_HANDLER
static void
PRMJ_InvalidParameterHandler(const wchar_t *expression,
                             const wchar_t *function,
                             const wchar_t *file,
                             unsigned int   line,
                             uintptr_t      pReserved)
{
    
}
#endif


size_t
PRMJ_FormatTime(char *buf, int buflen, const char *fmt, PRMJTime *prtm)
{
    size_t result = 0;
#if defined(XP_UNIX) || defined(XP_WIN) || defined(XP_OS2)
    struct tm a;
    int fake_tm_year = 0;
#ifdef NS_HAVE_INVALID_PARAMETER_HANDLER
    _invalid_parameter_handler oldHandler;
    int oldReportMode;
#endif

    memset(&a, 0, sizeof(struct tm));

    a.tm_sec = prtm->tm_sec;
    a.tm_min = prtm->tm_min;
    a.tm_hour = prtm->tm_hour;
    a.tm_mday = prtm->tm_mday;
    a.tm_mon = prtm->tm_mon;
    a.tm_wday = prtm->tm_wday;

    




#if defined(HAVE_LOCALTIME_R) && defined(HAVE_TM_ZONE_TM_GMTOFF)
    {
        





        struct tm td;
        memset(&td, 0, sizeof(td));
        td.tm_sec = prtm->tm_sec;
        td.tm_min = prtm->tm_min;
        td.tm_hour = prtm->tm_hour;
        td.tm_mday = prtm->tm_mday;
        td.tm_mon = prtm->tm_mon;
        td.tm_wday = prtm->tm_wday;
        td.tm_year = prtm->tm_year - 1900;
        td.tm_yday = prtm->tm_yday;
        td.tm_isdst = prtm->tm_isdst;
        time_t t = mktime(&td);
        localtime_r(&t, &td);

        a.tm_gmtoff = td.tm_gmtoff;
        a.tm_zone = td.tm_zone;
    }
#endif

    









#define FAKE_YEAR_BASE 9900
    if (prtm->tm_year < 1900 || prtm->tm_year > 9999) {
        fake_tm_year = FAKE_YEAR_BASE + prtm->tm_year % 100;
        a.tm_year = fake_tm_year - 1900;
    }
    else {
        a.tm_year = prtm->tm_year - 1900;
    }
    a.tm_yday = prtm->tm_yday;
    a.tm_isdst = prtm->tm_isdst;

    






#ifdef NS_HAVE_INVALID_PARAMETER_HANDLER
    oldHandler = _set_invalid_parameter_handler(PRMJ_InvalidParameterHandler);
    oldReportMode = _CrtSetReportMode(_CRT_ASSERT, 0);
#endif

    result = strftime(buf, buflen, fmt, &a);

#ifdef NS_HAVE_INVALID_PARAMETER_HANDLER
    _set_invalid_parameter_handler(oldHandler);
    _CrtSetReportMode(_CRT_ASSERT, oldReportMode);
#endif

    if (fake_tm_year && result) {
        char real_year[16];
        char fake_year[16];
        size_t real_year_len;
        size_t fake_year_len;
        char* p;

        sprintf(real_year, "%d", prtm->tm_year);
        real_year_len = strlen(real_year);
        sprintf(fake_year, "%d", fake_tm_year);
        fake_year_len = strlen(fake_year);

        
        for (p = buf; (p = strstr(p, fake_year)); p += real_year_len) {
            size_t new_result = result + real_year_len - fake_year_len;
            if ((int)new_result >= buflen) {
                return 0;
            }
            memmove(p + real_year_len, p + fake_year_len, strlen(p + fake_year_len));
            memcpy(p, real_year, real_year_len);
            result = new_result;
            *(buf + result) = '\0';
        }
    }
#endif
    return result;
}

JSInt64
DSTOffsetCache::computeDSTOffsetMilliseconds(int64 localTimeSeconds)
{
    JS_ASSERT(localTimeSeconds >= 0);
    JS_ASSERT(localTimeSeconds <= MAX_UNIX_TIMET);

#if defined(XP_WIN)
    




    _tzset();
#endif

    struct tm tm;
    if (!ComputeLocalTime(static_cast<time_t>(localTimeSeconds), &tm))
        return 0;

    JSInt32 base = PRMJ_LocalGMTDifference();

    int32 dayoff = int32((localTimeSeconds - base) % (SECONDS_PER_HOUR * 24));
    int32 tmoff = tm.tm_sec + (tm.tm_min * SECONDS_PER_MINUTE) +
        (tm.tm_hour * SECONDS_PER_HOUR);

    JSInt32 diff = tmoff - dayoff;

    if (diff < 0)
        diff += SECONDS_PER_DAY;

    return diff * MILLISECONDS_PER_SECOND;
}

JSInt64
DSTOffsetCache::getDSTOffsetMilliseconds(JSInt64 localTimeMilliseconds, JSContext *cx)
{
    sanityCheck();
    noteOffsetCalculation();

    JSInt64 localTimeSeconds = localTimeMilliseconds / MILLISECONDS_PER_SECOND;

    if (localTimeSeconds > MAX_UNIX_TIMET) {
        localTimeSeconds = MAX_UNIX_TIMET;
    } else if (localTimeSeconds < 0) {
        
        localTimeSeconds = SECONDS_PER_DAY;
    }

    





    if (rangeStartSeconds <= localTimeSeconds &&
        localTimeSeconds <= rangeEndSeconds) {
        noteCacheHit();
        return offsetMilliseconds;
    }

    if (oldRangeStartSeconds <= localTimeSeconds &&
        localTimeSeconds <= oldRangeEndSeconds) {
        noteCacheHit();
        return oldOffsetMilliseconds;
    }

    oldOffsetMilliseconds = offsetMilliseconds;
    oldRangeStartSeconds = rangeStartSeconds;
    oldRangeEndSeconds = rangeEndSeconds;

    if (rangeStartSeconds <= localTimeSeconds) {
        JSInt64 newEndSeconds = JS_MIN(rangeEndSeconds + RANGE_EXPANSION_AMOUNT, MAX_UNIX_TIMET);
        if (newEndSeconds >= localTimeSeconds) {
            JSInt64 endOffsetMilliseconds = computeDSTOffsetMilliseconds(newEndSeconds);
            if (endOffsetMilliseconds == offsetMilliseconds) {
                noteCacheMissIncrease();
                rangeEndSeconds = newEndSeconds;
                return offsetMilliseconds;
            }

            offsetMilliseconds = computeDSTOffsetMilliseconds(localTimeSeconds);
            if (offsetMilliseconds == endOffsetMilliseconds) {
                noteCacheMissIncreasingOffsetChangeUpper();
                rangeStartSeconds = localTimeSeconds;
                rangeEndSeconds = newEndSeconds;
            } else {
                noteCacheMissIncreasingOffsetChangeExpand();
                rangeEndSeconds = localTimeSeconds;
            }
            return offsetMilliseconds;
        }

        noteCacheMissLargeIncrease();
        offsetMilliseconds = computeDSTOffsetMilliseconds(localTimeSeconds);
        rangeStartSeconds = rangeEndSeconds = localTimeSeconds;
        return offsetMilliseconds;
    }

    JSInt64 newStartSeconds = JS_MAX(rangeStartSeconds - RANGE_EXPANSION_AMOUNT, 0);
    if (newStartSeconds <= localTimeSeconds) {
        JSInt64 startOffsetMilliseconds = computeDSTOffsetMilliseconds(newStartSeconds);
        if (startOffsetMilliseconds == offsetMilliseconds) {
            noteCacheMissDecrease();
            rangeStartSeconds = newStartSeconds;
            return offsetMilliseconds;
        }

        offsetMilliseconds = computeDSTOffsetMilliseconds(localTimeSeconds);
        if (offsetMilliseconds == startOffsetMilliseconds) {
            noteCacheMissDecreasingOffsetChangeLower();
            rangeStartSeconds = newStartSeconds;
            rangeEndSeconds = localTimeSeconds;
        } else {
            noteCacheMissDecreasingOffsetChangeExpand();
            rangeStartSeconds = localTimeSeconds;
        }
        return offsetMilliseconds;
    }

    noteCacheMissLargeDecrease();
    rangeStartSeconds = rangeEndSeconds = localTimeSeconds;
    offsetMilliseconds = computeDSTOffsetMilliseconds(localTimeSeconds);
    return offsetMilliseconds;
}

void
DSTOffsetCache::sanityCheck()
{
    JS_ASSERT(rangeStartSeconds <= rangeEndSeconds);
    JS_ASSERT_IF(rangeStartSeconds == INT64_MIN, rangeEndSeconds == INT64_MIN);
    JS_ASSERT_IF(rangeEndSeconds == INT64_MIN, rangeStartSeconds == INT64_MIN);
    JS_ASSERT_IF(rangeStartSeconds != INT64_MIN,
                 rangeStartSeconds >= 0 && rangeEndSeconds >= 0);
    JS_ASSERT_IF(rangeStartSeconds != INT64_MIN,
                 rangeStartSeconds <= MAX_UNIX_TIMET && rangeEndSeconds <= MAX_UNIX_TIMET);

#ifdef JS_METER_DST_OFFSET_CACHING
    JS_ASSERT(totalCalculations ==
              hit +
              missIncreasing + missDecreasing +
              missIncreasingOffsetChangeExpand + missIncreasingOffsetChangeUpper +
              missDecreasingOffsetChangeExpand + missDecreasingOffsetChangeLower +
              missLargeIncrease + missLargeDecrease);
#endif
}

#ifdef JS_METER_DST_OFFSET_CACHING
void
DSTOffsetCache::dumpStats()
{
    if (!getenv("JS_METER_DST_OFFSET_CACHING"))
        return;
    FILE *fp = fopen("/tmp/dst-offset-cache.stats", "a");
    if (!fp)
        return;
    typedef unsigned long UL;
    fprintf(fp,
            "hit:\n"
            "  in range: %lu\n"
            "misses:\n"
            "  increase range end:                 %lu\n"
            "  decrease range start:               %lu\n"
            "  increase, offset change, expand:    %lu\n"
            "  increase, offset change, new range: %lu\n"
            "  decrease, offset change, expand:    %lu\n"
            "  decrease, offset change, new range: %lu\n"
            "  large increase:                     %lu\n"
            "  large decrease:                     %lu\n"
            "total: %lu\n\n",
            UL(hit),
            UL(missIncreasing), UL(missDecreasing),
            UL(missIncreasingOffsetChangeExpand), UL(missIncreasingOffsetChangeUpper),
            UL(missDecreasingOffsetChangeExpand), UL(missDecreasingOffsetChangeLower),
            UL(missLargeIncrease), UL(missLargeDecrease),
            UL(totalCalculations));
    fclose(fp);
}
#endif
