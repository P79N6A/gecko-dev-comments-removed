









































#include "jsstddef.h"
#ifdef SOLARIS
#define _REENTRANT 1
#endif
#include <string.h>
#include <time.h>
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

    
    memset((char *)&ltime,0,sizeof(ltime));
    ltime.tm_mday = 2;
    ltime.tm_year = 70;
#ifdef SUNOS4
    ltime.tm_zone = 0;
    ltime.tm_gmtoff = 0;
    return timelocal(&ltime) - (24 * 3600);
#else
    return (JSInt32)mktime(&ltime) - (24L * 3600L);
#endif
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
#ifndef JS_HAVE_LONG_LONG
    JSLL_SHL(g1970GMTMicroSeconds,g1970GMTMicroSeconds,16);
    JSLL_SHL(g1970GMTMicroSeconds,g1970GMTMicroSeconds,16);
#else
    JSLL_SHL(g1970GMTMicroSeconds,g1970GMTMicroSeconds,32);
#endif
    JSLL_ADD(g1970GMTMicroSeconds,g1970GMTMicroSeconds,low);

    JSLL_I2L(exttime,base_time);
    JSLL_ADD(exttime,exttime,g1970GMTMicroSeconds);
    JSLL_SUB(exttime,exttime,tmp);
    return exttime;
}

#ifdef XP_WIN
typedef struct CalibrationData
{
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

static const JSInt64 win2un = JSLL_INIT(0x19DB1DE, 0xD53E8000);

static CalibrationData calibration = { 0 };

#define FILETIME2INT64(ft) (((JSInt64)ft.dwHighDateTime) << 32LL | (JSInt64)ft.dwLowDateTime)

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
        GetSystemTimeAsFileTime(&ftStart);
        do {
            GetSystemTimeAsFileTime(&ft);
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
static PRStatus PR_CALLBACK
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
































































JSInt64
PRMJ_Now(void)
{
#ifdef XP_OS2
    JSInt64 s, us, ms2us, s2us;
    struct timeb b;
#endif
#ifdef XP_WIN
    static int nCalls = 0;
    long double lowresTime, highresTimerValue;
    FILETIME ft;
    LARGE_INTEGER now;
    JSBool calibrated = JS_FALSE;
    JSBool needsCalibration = JS_FALSE;
    JSInt64 returnedTime;
    long double cachedOffset = 0.0;
#endif
#if defined(XP_UNIX) || defined(XP_BEOS)
    struct timeval tv;
    JSInt64 s, us, s2us;
#endif 

#ifdef XP_OS2
    ftime(&b);
    JSLL_UI2L(ms2us, PRMJ_USEC_PER_MSEC);
    JSLL_UI2L(s2us, PRMJ_USEC_PER_SEC);
    JSLL_UI2L(s, b.time);
    JSLL_UI2L(us, b.millitm);
    JSLL_MUL(us, us, ms2us);
    JSLL_MUL(s, s, s2us);
    JSLL_ADD(s, s, us);
    return s;
#endif
#ifdef XP_WIN

    



    int thiscall = JS_ATOMIC_INCREMENT(&nCalls);
    
    if (thiscall <= 10) {
        GetSystemTimeAsFileTime(&ft);
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


        
        GetSystemTimeAsFileTime(&ft);
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

            

            calibration.last = max(calibration.last,(JSInt64)highresTime);
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
#endif

#if defined(XP_UNIX) || defined(XP_BEOS)
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
#endif 
}


JSInt64
PRMJ_DSTOffset(JSInt64 local_time)
{
    JSInt64 us2s;
    time_t local;
    JSInt32 diff;
    JSInt64  maxtimet;
    struct tm tm;
    PRMJTime prtm;
#ifndef HAVE_LOCALTIME_R
    struct tm *ptm;
#endif


    JSLL_UI2L(us2s, PRMJ_USEC_PER_SEC);
    JSLL_DIV(local_time, local_time, us2s);

    
    JSLL_UI2L(maxtimet,PRMJ_MAX_UNIX_TIMET);

    if(JSLL_CMP(local_time,>,maxtimet)){
        JSLL_UI2L(local_time,PRMJ_MAX_UNIX_TIMET);
    } else if(!JSLL_GE_ZERO(local_time)){
        
        JSLL_UI2L(local_time,PRMJ_DAY_SECONDS);
    }
    JSLL_L2UI(local,local_time);
    PRMJ_basetime(local_time,&prtm);
#ifndef HAVE_LOCALTIME_R
    ptm = localtime(&local);
    if(!ptm){
        return JSLL_ZERO;
    }
    tm = *ptm;
#else
    localtime_r(&local,&tm); 
#endif

    diff = ((tm.tm_hour - prtm.tm_hour) * PRMJ_HOUR_SECONDS) +
	((tm.tm_min - prtm.tm_min) * 60);

    if(diff < 0){
	diff += PRMJ_DAY_SECONDS;
    }

    JSLL_UI2L(local_time,diff);

    JSLL_MUL(local_time,local_time,us2s);

    return(local_time);
}


size_t
PRMJ_FormatTime(char *buf, int buflen, const char *fmt, PRMJTime *prtm)
{
#if defined(XP_UNIX) || defined(XP_WIN) || defined(XP_OS2) || defined(XP_BEOS)
    struct tm a;

    












    memset(&a, 0, sizeof(struct tm));

    a.tm_sec = prtm->tm_sec;
    a.tm_min = prtm->tm_min;
    a.tm_hour = prtm->tm_hour;
    a.tm_mday = prtm->tm_mday;
    a.tm_mon = prtm->tm_mon;
    a.tm_wday = prtm->tm_wday;
    a.tm_year = prtm->tm_year - 1900;
    a.tm_yday = prtm->tm_yday;
    a.tm_isdst = prtm->tm_isdst;

    






#if defined(SUNOS4)
    if (mktime(&a) == -1) {
        





        return 0;
    }
#endif

    return strftime(buf, buflen, fmt, &a);
#endif
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
