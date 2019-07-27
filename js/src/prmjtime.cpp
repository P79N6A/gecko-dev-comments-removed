







#include "prmjtime.h"

#include "mozilla/DebugOnly.h"
#include "mozilla/MathAlgorithms.h"

#ifdef SOLARIS
#define _REENTRANT 1
#endif
#include <string.h>
#include <time.h>

#include "jstypes.h"
#include "jsutil.h"

#ifdef XP_WIN
#include <windef.h>
#include <winbase.h>
#include <mmsystem.h> 

#if _MSC_VER >= 1400
#define NS_HAVE_INVALID_PARAMETER_HANDLER 1
#endif
#ifdef NS_HAVE_INVALID_PARAMETER_HANDLER
#include <crtdbg.h>   
#include <stdlib.h>   
#endif

#include "prinit.h"

#endif

#ifdef XP_UNIX

#ifdef _SVID_GETTOD   
extern int gettimeofday(struct timeval *tv);
#endif

#include <sys/time.h>

#endif 

using mozilla::DebugOnly;

#if defined(XP_UNIX)
int64_t
PRMJ_Now()
{
    struct timeval tv;

#ifdef _SVID_GETTOD   
    gettimeofday(&tv);
#else
    gettimeofday(&tv, 0);
#endif 

    return int64_t(tv.tv_sec) * PRMJ_USEC_PER_SEC + int64_t(tv.tv_usec);
}

#else


static double
FileTimeToUnixMicroseconds(const FILETIME &ft)
{
    
    int64_t t = (int64_t(ft.dwHighDateTime) << 32) | int64_t(ft.dwLowDateTime);

    
    
    static const int64_t TimeToEpochIn100ns = 0x19DB1DED53E8000;
    t -= TimeToEpochIn100ns;

    
    return double(t) * 0.1;
}

struct CalibrationData {
    double freq;         
    double offset;       
    double timer_offset; 

    bool calibrated;

    CRITICAL_SECTION data_lock;
};

static CalibrationData calibration = { 0 };

static void
NowCalibrate()
{
    MOZ_ASSERT(calibration.freq > 0);

    
    
    timeBeginPeriod(1);
    FILETIME ft, ftStart;
    GetSystemTimeAsFileTime(&ftStart);
    do {
        GetSystemTimeAsFileTime(&ft);
    } while (memcmp(&ftStart, &ft, sizeof(ft)) == 0);
    timeEndPeriod(1);

    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);

    calibration.offset = FileTimeToUnixMicroseconds(ft);
    calibration.timer_offset = double(now.QuadPart);
    calibration.calibrated = true;
}

static const unsigned DataLockSpinCount = 4096;

static void (WINAPI *pGetSystemTimePreciseAsFileTime)(LPFILETIME) = nullptr;

void
PRMJ_NowInit()
{
    memset(&calibration, 0, sizeof(calibration));

    
    
    
    
    LARGE_INTEGER liFreq;
    DebugOnly<BOOL> res = QueryPerformanceFrequency(&liFreq);
    MOZ_ASSERT(res);
    calibration.freq = double(liFreq.QuadPart);
    MOZ_ASSERT(calibration.freq > 0.0);

    InitializeCriticalSectionAndSpinCount(&calibration.data_lock, DataLockSpinCount);

    
    if (HMODULE h = GetModuleHandle("kernel32.dll")) {
        pGetSystemTimePreciseAsFileTime =
            (void (WINAPI *)(LPFILETIME))GetProcAddress(h, "GetSystemTimePreciseAsFileTime");
    }
}

void
PRMJ_NowShutdown()
{
    DeleteCriticalSection(&calibration.data_lock);
}

#define MUTEX_LOCK(m) EnterCriticalSection(m)
#define MUTEX_UNLOCK(m) LeaveCriticalSection(m)
#define MUTEX_SETSPINCOUNT(m, c) SetCriticalSectionSpinCount((m),(c))


int64_t
PRMJ_Now()
{
    if (pGetSystemTimePreciseAsFileTime) {
        
        FILETIME ft;
        pGetSystemTimePreciseAsFileTime(&ft);
        return int64_t(FileTimeToUnixMicroseconds(ft));
    }

    bool calibrated = false;
    bool needsCalibration = !calibration.calibrated;
    double cachedOffset = 0.0;
    while (true) {
        if (needsCalibration) {
            MUTEX_LOCK(&calibration.data_lock);

            
            if (calibration.offset == cachedOffset) {
                
                
                MUTEX_SETSPINCOUNT(&calibration.data_lock, 0);

                NowCalibrate();

                calibrated = true;

                
                MUTEX_SETSPINCOUNT(&calibration.data_lock, DataLockSpinCount);
            }

            MUTEX_UNLOCK(&calibration.data_lock);
        }

        
        FILETIME ft;
        GetSystemTimeAsFileTime(&ft);
        double lowresTime = FileTimeToUnixMicroseconds(ft);

        
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        double highresTimerValue = double(now.QuadPart);

        MUTEX_LOCK(&calibration.data_lock);
        double highresTime = calibration.offset +
            PRMJ_USEC_PER_SEC * (highresTimerValue - calibration.timer_offset) / calibration.freq;
        cachedOffset = calibration.offset;
        MUTEX_UNLOCK(&calibration.data_lock);

        
        
        
        
        
        static const double KernelTickInMicroseconds = 15625.25;

        
        double diff = lowresTime - highresTime;

        
        
        
        
        
        if (mozilla::Abs(diff) <= 2 * KernelTickInMicroseconds) {
            
            return int64_t(highresTime);
        }

        if (calibrated) {
            
            
            
            
            
            
            
            
            
            return int64_t(lowresTime);
        }

        
        
        
        
        
        
        
        
        
        needsCalibration = true;
    }
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
#if defined(XP_UNIX) || defined(XP_WIN)
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
