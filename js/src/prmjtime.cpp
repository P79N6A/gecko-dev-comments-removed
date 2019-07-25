









































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

#if _MSC_VER >= 1400 && !defined(WINCE)
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

#if defined(XP_UNIX) || defined(XP_BEOS)

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


static void PRMJ_basetime(JSInt64 tsecs, PRMJTime *prtm);



JSInt32
PRMJ_LocalGMTDifference()
{
    struct tm ltime;

#if defined(XP_WIN) && !defined(WINCE)
    




    _tzset();
#endif
    
    memset((char *)&ltime,0,sizeof(ltime));
    ltime.tm_mday = 2;
    ltime.tm_year = 70;
    return (JSInt32)mktime(&ltime) - (24L * 3600L);
}


#define G1970GMTMICROHI        0x00dcdcad /* micro secs to 1970 hi */
#define G1970GMTMICROLOW       0x8b3fa000 /* micro secs to 1970 low */

#define G2037GMTMICROHI        0x00e45fab /* micro secs to 2037 high */
#define G2037GMTMICROLOW       0x7a238000 /* micro secs to 2037 low */


static JSInt64
PRMJ_ToExtendedTime(JSInt32 base_time)
{
    JSInt64 exttime;
    JSInt64 g1970GMTMicroSeconds;
    JSInt64 low;
    JSInt32 diff;
    JSInt64  tmp;
    JSInt64  tmp1;

    diff = PRMJ_LocalGMTDifference();
    JSLL_UI2L(tmp, PRMJ_USEC_PER_SEC);
    JSLL_I2L(tmp1,diff);
    JSLL_MUL(tmp,tmp,tmp1);

    JSLL_UI2L(g1970GMTMicroSeconds,G1970GMTMICROHI);
    JSLL_UI2L(low,G1970GMTMICROLOW);
    JSLL_SHL(g1970GMTMicroSeconds,g1970GMTMicroSeconds,16);
    JSLL_SHL(g1970GMTMicroSeconds,g1970GMTMicroSeconds,16);
    JSLL_ADD(g1970GMTMicroSeconds,g1970GMTMicroSeconds,low);

    JSLL_I2L(exttime,base_time);
    JSLL_ADD(exttime,exttime,g1970GMTMicroSeconds);
    JSLL_SUB(exttime,exttime,tmp);
    return exttime;
}

#ifdef HAVE_SYSTEMTIMETOFILETIME

static const JSInt64 win2un = JSLL_INIT(0x19DB1DE, 0xD53E8000);

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
#ifdef WINCE
    JSInt64 granularity;
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

#ifdef WINCE
        calibration.granularity = (FILETIME2INT64(ft) -
                                   FILETIME2INT64(ftStart))/10;
#endif
        




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
#ifdef WINCE
    InitializeCriticalSection(&calibration.calibration_lock);
    InitializeCriticalSection(&calibration.data_lock);
#else
    InitializeCriticalSectionAndSpinCount(&calibration.calibration_lock, CALIBRATIONLOCK_SPINCOUNT);
    InitializeCriticalSectionAndSpinCount(&calibration.data_lock, DATALOCK_SPINCOUNT);
#endif
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
#ifdef WINCE
#define MUTEX_SETSPINCOUNT(m, c)
#else
#define MUTEX_SETSPINCOUNT(m, c) SetCriticalSectionSpinCount((m),(c))
#endif

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
    JSLL_UI2L(ms2us, PRMJ_USEC_PER_MSEC);
    JSLL_UI2L(s2us, PRMJ_USEC_PER_SEC);
    JSLL_UI2L(s, b.time);
    JSLL_UI2L(us, b.millitm);
    JSLL_MUL(us, us, ms2us);
    JSLL_MUL(s, s, s2us);
    JSLL_ADD(s, s, us);
    return s;
}

#elif defined(XP_UNIX) || defined(XP_BEOS)
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
    JSLL_UI2L(s2us, PRMJ_USEC_PER_SEC);
    JSLL_UI2L(s, tv.tv_sec);
    JSLL_UI2L(us, tv.tv_usec);
    JSLL_MUL(s, s, s2us);
    JSLL_ADD(s, s, us);
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

#ifdef WINCE
            
            skewThreshold = calibration.granularity;
#else
            
            if (GetSystemTimeAdjustment(&timeAdjustment,
                                        &timeIncrement,
                                        &timeAdjustmentDisabled)) {
                if (timeAdjustmentDisabled) {
                    
                    skewThreshold = timeAdjustment/10.0;
                } else {
                    
                    skewThreshold = timeIncrement/10.0;
                }
            }
#endif
            
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
#if defined(XP_UNIX) || defined(XP_WIN) || defined(XP_OS2) || defined(XP_BEOS)
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
        time_t bogus = 0;
        localtime_r(&bogus, &td);
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


static int mtab[] = {
    
    31,28,31,30,31,30,
    
    31,31,30,31,30,31
};






static void
PRMJ_basetime(JSInt64 tsecs, PRMJTime *prtm)
{
    
    JSInt32 year    = 0;
    JSInt32 month   = 0;
    JSInt32 yday    = 0;
    JSInt32 mday    = 0;
    JSInt32 wday    = 6; 
    JSInt32 days    = 0;
    JSInt32 seconds = 0;
    JSInt32 minutes = 0;
    JSInt32 hours   = 0;
    JSInt32 isleap  = 0;

    
    JSInt64 result;
    JSInt64	result1;
    JSInt64	result2;

    JSInt64 base;

    

    JSInt32 fourCenturyBlocks;
    JSInt32 centuriesLeft;
    JSInt32 fourYearBlocksLeft;
    JSInt32 yearsLeft;

    

    JSInt64 fourYears;
    JSInt64 century;
    JSInt64 fourCenturies;

    JSLL_UI2L(result, PRMJ_DAY_SECONDS);

    JSLL_I2L(fourYears, PRMJ_FOUR_YEARS_DAYS);
    JSLL_MUL(fourYears, fourYears, result);

    JSLL_I2L(century, PRMJ_CENTURY_DAYS);
    JSLL_MUL(century, century, result);

    JSLL_I2L(fourCenturies, PRMJ_FOUR_CENTURIES_DAYS);
    JSLL_MUL(fourCenturies, fourCenturies, result);

    
    base = PRMJ_ToExtendedTime(0);
    JSLL_UI2L(result,  PRMJ_USEC_PER_SEC);
    JSLL_DIV(base,base,result);
    JSLL_ADD(tsecs,tsecs,base);

    




    
    JSLL_UI2L(result, PRMJ_YEAR_SECONDS);
    if (!JSLL_CMP(tsecs,<,result)) {
        days = PRMJ_YEAR_DAYS;
        year = 1;
        JSLL_SUB(tsecs, tsecs, result);
    }

    
    JSLL_UDIVMOD(&result1, &result2, tsecs, fourCenturies);
    JSLL_L2I(fourCenturyBlocks, result1);
    year += fourCenturyBlocks * 400;
    days += fourCenturyBlocks * PRMJ_FOUR_CENTURIES_DAYS;
    tsecs = result2;

    JSLL_UDIVMOD(&result1, &result2, tsecs, century);
    JSLL_L2I(centuriesLeft, result1);
    year += centuriesLeft * 100;
    days += centuriesLeft * PRMJ_CENTURY_DAYS;
    tsecs = result2;

    JSLL_UDIVMOD(&result1, &result2, tsecs, fourYears);
    JSLL_L2I(fourYearBlocksLeft, result1);
    year += fourYearBlocksLeft * 4;
    days += fourYearBlocksLeft * PRMJ_FOUR_YEARS_DAYS;
    tsecs = result2;

    
    JSLL_UDIVMOD(&result1, &result2, tsecs, result);
    JSLL_L2I(yearsLeft, result1);
    year += yearsLeft;
    days += yearsLeft * PRMJ_YEAR_DAYS;
    tsecs = result2;

    


    isleap =
        (yearsLeft == 3) && (fourYearBlocksLeft != 24 || centuriesLeft == 3);
    JS_ASSERT(isleap ==
              ((year % 4 == 0) && (year % 100 != 0 || year % 400 == 0)));

    JSLL_UI2L(result1,PRMJ_DAY_SECONDS);

    JSLL_DIV(result,tsecs,result1);
    JSLL_L2I(mday,result);

    
    while(((month == 1 && isleap) ?
            (mday >= mtab[month] + 1) :
            (mday >= mtab[month]))){
	 yday += mtab[month];
	 days += mtab[month];

	 mday -= mtab[month];

         
	 if(month == 1 && isleap != 0){
	     yday++;
	     days++;
	     mday--;
	 }
	 month++;
    }

    
    JSLL_MUL(result,result,result1);
    JSLL_SUB(tsecs,tsecs,result);

    mday++; 
    days += mday;
    wday = (days + wday) % 7;

    yday += mday;

    
    JSLL_UI2L(result1,PRMJ_HOUR_SECONDS);
    JSLL_DIV(result,tsecs,result1);
    JSLL_L2I(hours,result);
    JSLL_MUL(result,result,result1);
    JSLL_SUB(tsecs,tsecs,result);

    
    JSLL_UI2L(result1,60);
    JSLL_DIV(result,tsecs,result1);
    JSLL_L2I(minutes,result);
    JSLL_MUL(result,result,result1);
    JSLL_SUB(tsecs,tsecs,result);

    JSLL_L2I(seconds,tsecs);

    prtm->tm_usec  = 0L;
    prtm->tm_sec   = (JSInt8)seconds;
    prtm->tm_min   = (JSInt8)minutes;
    prtm->tm_hour  = (JSInt8)hours;
    prtm->tm_mday  = (JSInt8)mday;
    prtm->tm_mon   = (JSInt8)month;
    prtm->tm_wday  = (JSInt8)wday;
    prtm->tm_year  = (JSInt16)year;
    prtm->tm_yday  = (JSInt16)yday;
}

JSInt64
DSTOffsetCache::computeDSTOffsetMilliseconds(int64 localTimeSeconds)
{
    JS_ASSERT(localTimeSeconds >= 0);
    JS_ASSERT(localTimeSeconds <= MAX_UNIX_TIMET);

#if defined(XP_WIN) && !defined(WINCE)
    




    _tzset();
#endif

    time_t local = static_cast<time_t>(localTimeSeconds);
    PRMJTime prtm;
    struct tm tm;
    PRMJ_basetime(localTimeSeconds, &prtm);
#ifndef HAVE_LOCALTIME_R
    struct tm *ptm = localtime(&local);
    if (!ptm)
        return 0;
    tm = *ptm;
#else
    localtime_r(&local, &tm); 
#endif

    JSInt32 diff = ((tm.tm_hour - prtm.tm_hour) * SECONDS_PER_HOUR) +
                   ((tm.tm_min - prtm.tm_min) * SECONDS_PER_MINUTE);

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

    





    if (rangeStartSeconds <= localTimeSeconds) {
        if (localTimeSeconds <= rangeEndSeconds) {
            noteCacheHit();
            return offsetMilliseconds;
        }

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
