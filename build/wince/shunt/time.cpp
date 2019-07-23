







































#include "mozce_internal.h"
#include "time_conversions.h"

#define strftime __not_supported_on_device_strftime
#define localtime __not_supported_on_device_localtime
#define mktime __not_supported_on_device_mktime
#define gmtime __not_supported_on_device_gmtime
#define time __not_supported_on_device_time
#include <time.h>
#undef strftime
#undef localtime
#undef mktime
#undef gmtime
#undef time

extern "C" {
#if 0
}
#endif


static const int sDaysOfYear[12] = {
    0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
};
static struct tm tmStorage;

#ifdef strftime
#undef strftime
#endif

MOZCE_SHUNT_API size_t strftime(char *, size_t, const char *, const struct tm *)
{
    WINCE_LOG_API_CALL("mozce_strftime called\n");
    return 0;
}


MOZCE_SHUNT_API struct tm* mozce_localtime_r(const time_t* inTimeT,struct tm* outRetval)
{
    WINCE_LOG_API_CALL("tm* mozce_localtime_r called\n");

    struct tm* retval = NULL;

    if(NULL != inTimeT && NULL != outRetval)
    {
        SYSTEMTIME winLocalTime;
        
        time_t_2_LOCALSYSTEMTIME(winLocalTime, *inTimeT);
        
        outRetval->tm_sec = (int)winLocalTime.wSecond;
        outRetval->tm_min = (int)winLocalTime.wMinute;
        outRetval->tm_hour = (int)winLocalTime.wHour;
        outRetval->tm_mday = (int)winLocalTime.wDay;
        outRetval->tm_mon = (int)(winLocalTime.wMonth - 1);
        outRetval->tm_year = (int)(winLocalTime.wYear - 1900);
        outRetval->tm_wday = (int)winLocalTime.wDayOfWeek;
        outRetval->tm_isdst = -1;

        outRetval->tm_yday = (int)winLocalTime.wDay + sDaysOfYear[outRetval->tm_mon];
        if(0 == (winLocalTime.wYear & 3))
        {
            if(2 < winLocalTime.wMonth)
            {
                if(0 == winLocalTime.wYear % 100)
                {
                    if(0 == winLocalTime.wYear % 400)
                    {
                        outRetval->tm_yday++;
                    }
                }
                else
                {
                    outRetval->tm_yday++;
                }
            }
        }

        retval = outRetval;
    }

    return retval;
}


MOZCE_SHUNT_API struct tm* localtime(const time_t* inTimeT)
{
    WINCE_LOG_API_CALL("tm* mozce_localtime called\n");

    return mozce_localtime_r(inTimeT, &tmStorage);
}


MOZCE_SHUNT_API struct tm* mozce_gmtime_r(const time_t* inTimeT, struct tm* outRetval)
{
    WINCE_LOG_API_CALL("tm* mozce_gmtime_r called\n");

    struct tm* retval = NULL;

    if(NULL != inTimeT)
    {
        SYSTEMTIME winGMTime;
        
        time_t_2_SYSTEMTIME(winGMTime, *inTimeT);
        
        outRetval->tm_sec = (int)winGMTime.wSecond;
        outRetval->tm_min = (int)winGMTime.wMinute;
        outRetval->tm_hour = (int)winGMTime.wHour;
        outRetval->tm_mday = (int)winGMTime.wDay;
        outRetval->tm_mon = (int)(winGMTime.wMonth - 1);
        outRetval->tm_year = (int)(winGMTime.wYear - 1900);
        outRetval->tm_wday = (int)winGMTime.wDayOfWeek;
        outRetval->tm_isdst = -1;

        outRetval->tm_yday = (int)winGMTime.wDay + sDaysOfYear[outRetval->tm_mon];
        if(0 == (winGMTime.wYear & 3))
        {
            if(2 < winGMTime.wMonth)
            {
                if(0 == winGMTime.wYear % 100)
                {
                    if(0 == winGMTime.wYear % 400)
                    {
                        outRetval->tm_yday++;
                    }
                }
                else
                {
                    outRetval->tm_yday++;
                }
            }
        }

        retval = outRetval;
    }

    return retval;
}


MOZCE_SHUNT_API struct tm* gmtime(const time_t* inTimeT)
{
    WINCE_LOG_API_CALL("tm* mozce_gmtime called\n");

    return mozce_gmtime_r(inTimeT, &tmStorage);
}


MOZCE_SHUNT_API time_t mktime(struct tm* inTM)
{
    WINCE_LOG_API_CALL("mozce_mktime called\n");

    time_t retval = (time_t)-1;

    if(NULL != inTM)
    {
        SYSTEMTIME winTime;
        struct tm* gmTime = NULL;

        memset(&winTime, 0, sizeof(winTime));

        



        winTime.wSecond = inTM->tm_sec;
        winTime.wMinute = inTM->tm_min;
        winTime.wHour = inTM->tm_hour;
        winTime.wDay = inTM->tm_mday;
        winTime.wMonth = inTM->tm_mon + 1;
        winTime.wYear = inTM->tm_year + 1900;

        


        SYSTEMTIME_2_time_t(retval, winTime);

        


        gmTime = mozce_gmtime_r(&retval, inTM);
    }

    return retval;
}

MOZCE_SHUNT_API time_t time(time_t *)
{
  time_t retval;
  SYSTEMTIME winTime;
  ::GetSystemTime(&winTime);
  SYSTEMTIME_2_time_t(retval, winTime);
  return retval;
}
#if 0
{
#endif
} 

