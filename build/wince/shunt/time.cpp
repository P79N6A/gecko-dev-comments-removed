





































 
#include "include/mozce_shunt.h"
#include "time_conversions.h"
#include <stdlib.h>
#include <Windows.h>






#define strftime __not_supported_on_device_strftime
#define localtime __not_supported_on_device_localtime
#define mktime __not_supported_on_device_mktime
#define gmtime __not_supported_on_device_gmtime
#define time __not_supported_on_device_time
#define clock __not_supported_on_device_clock
#include <time.h>
#undef strftime
#undef localtime
#undef mktime
#undef gmtime
#undef time
#undef clock

extern "C" {
  size_t strftime(char *, size_t, const char *, const struct tm *);
  struct tm* localtime(const time_t* inTimeT);
  time_t mktime(struct tm* inTM);
  struct tm* gmtime(const time_t* inTimeT);
  time_t time(time_t *);
  clock_t clock();
}

static const int sDaysOfYear[12] = {
  0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
};
static struct tm tmStorage;

size_t strftime(char *out, size_t maxsize, const char *pat, const struct tm *time)
{
  WCHAR* tmpBuf = (WCHAR*)malloc(sizeof(WCHAR) * maxsize);
  if (!tmpBuf)
    return 0;
  wcsftime(tmpBuf, maxsize, pat, time);
  size_t ret = ::WideCharToMultiByte(CP_ACP, 0, tmpBuf, -1, out, maxsize, 0, 0);
  free(tmpBuf);
  return ret;
}

struct tm* gmtime_r(const time_t* inTimeT, struct tm* outRetval)
{
  struct tm* retval = NULL;
  
  if(NULL != inTimeT) {
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
    if(0 == (winGMTime.wYear & 3)) {
      if(2 < winGMTime.wMonth) {
        if(0 == winGMTime.wYear % 100) {
          if(0 == winGMTime.wYear % 400) {
            outRetval->tm_yday++;
          }
        }else {
          outRetval->tm_yday++;
        }
      }
    }
    retval = outRetval;
  }
  return retval;
}

struct tm* localtime_r(const time_t* inTimeT,struct tm* outRetval)
{
  struct tm* retval = NULL;
  
  if(NULL != inTimeT && NULL != outRetval) {
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
    if(0 == (winLocalTime.wYear & 3)) {
      if(2 < winLocalTime.wMonth) {
        if(0 == winLocalTime.wYear % 100) {
          if(0 == winLocalTime.wYear % 400) {
            outRetval->tm_yday++;
          }
        } else {
          outRetval->tm_yday++;
        }
      }
    }
    retval = outRetval;
  }
  return retval;
}


struct tm* localtime(const time_t* inTimeT)
{
  return localtime_r(inTimeT, &tmStorage);
}

struct tm* gmtime(const time_t* inTimeT)
{
  return gmtime_r(inTimeT, &tmStorage);
}


time_t mktime(struct tm* inTM)
{
  time_t retval = (time_t)-1;
  
  if(NULL != inTM) {
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
    
    


    gmTime = gmtime_r(&retval, inTM);
  }
  return retval;
}

time_t time(time_t *)
{
  time_t retval;
  SYSTEMTIME winTime;
  ::GetSystemTime(&winTime);
  SYSTEMTIME_2_time_t(retval, winTime);
  return retval;
}

clock_t clock() 
{
  return -1;
}

