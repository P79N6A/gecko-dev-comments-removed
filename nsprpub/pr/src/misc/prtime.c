











































#include "prinit.h"
#include "prtime.h"
#include "prlock.h"
#include "prprf.h"
#include "prlog.h"

#include <string.h>
#include <ctype.h>
#include <errno.h>  
#include <time.h>
















#define COUNT_LEAPS(Y)   ( ((Y)-1)/4 - ((Y)-1)/100 + ((Y)-1)/400 )
#define COUNT_DAYS(Y)  ( ((Y)-1)*365 + COUNT_LEAPS(Y) )
#define DAYS_BETWEEN_YEARS(A, B)  (COUNT_DAYS(B) - COUNT_DAYS(A))










static const int lastDayOfMonth[2][13] = {
    {-1, 30, 58, 89, 119, 150, 180, 211, 242, 272, 303, 333, 364},
    {-1, 30, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365}
};





static const PRInt8 nDays[2][12] = {
    {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
    {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
};





static void        ComputeGMT(PRTime time, PRExplodedTime *gmt);
static int         IsLeapYear(PRInt16 year);
static void        ApplySecOffset(PRExplodedTime *time, PRInt32 secOffset);












static void
ComputeGMT(PRTime time, PRExplodedTime *gmt)
{
    PRInt32 tmp, rem;
    PRInt32 numDays;
    PRInt64 numDays64, rem64;
    int isLeap;
    PRInt64 sec;
    PRInt64 usec;
    PRInt64 usecPerSec;
    PRInt64 secPerDay;

    




    LL_I2L(usecPerSec, 1000000L);
    LL_DIV(sec, time, usecPerSec);
    LL_MOD(usec, time, usecPerSec);
    LL_L2I(gmt->tm_usec, usec);
    
    if (gmt->tm_usec < 0) {
        PRInt64 one;

        LL_I2L(one, 1L);
        LL_SUB(sec, sec, one);
        gmt->tm_usec += 1000000L;
    }

    LL_I2L(secPerDay, 86400L);
    LL_DIV(numDays64, sec, secPerDay);
    LL_MOD(rem64, sec, secPerDay);
    
    LL_L2I(numDays, numDays64);
    LL_L2I(rem, rem64);
    if (rem < 0) {
        numDays--;
        rem += 86400L;
    }

    

    gmt->tm_wday = (numDays + 4) % 7;
    if (gmt->tm_wday < 0) {
        gmt->tm_wday += 7;
    }

    

    gmt->tm_hour = rem / 3600;
    rem %= 3600;
    gmt->tm_min = rem / 60;
    gmt->tm_sec = rem % 60;

    







    numDays += 719162;       
    tmp = numDays / 146097;  
    rem = numDays % 146097;
    gmt->tm_year = tmp * 400 + 1;

    

    tmp = rem / 36524;    
    rem %= 36524;
    if (tmp == 4) {       
        tmp = 3;
        rem = 36524;
    }
    gmt->tm_year += tmp * 100;

    

    tmp = rem / 1461;     
    rem %= 1461;
    gmt->tm_year += tmp * 4;

    

    tmp = rem / 365;
    rem %= 365;
    if (tmp == 4) {       
        tmp = 3;
        rem = 365;
    }

    gmt->tm_year += tmp;
    gmt->tm_yday = rem;
    isLeap = IsLeapYear(gmt->tm_year);

    

    for (tmp = 1; lastDayOfMonth[isLeap][tmp] < gmt->tm_yday; tmp++) {
    }
    gmt->tm_month = --tmp;
    gmt->tm_mday = gmt->tm_yday - lastDayOfMonth[isLeap][tmp];

    gmt->tm_params.tp_gmt_offset = 0;
    gmt->tm_params.tp_dst_offset = 0;
}













PR_IMPLEMENT(void)
PR_ExplodeTime(
        PRTime usecs,
        PRTimeParamFn params,
        PRExplodedTime *exploded)
{
    ComputeGMT(usecs, exploded);
    exploded->tm_params = params(exploded);
    ApplySecOffset(exploded, exploded->tm_params.tp_gmt_offset
            + exploded->tm_params.tp_dst_offset);
}












PR_IMPLEMENT(PRTime)
PR_ImplodeTime(const PRExplodedTime *exploded)
{
    PRExplodedTime copy;
    PRTime retVal;
    PRInt64 secPerDay, usecPerSec;
    PRInt64 temp;
    PRInt64 numSecs64;
    PRInt32 numDays;
    PRInt32 numSecs;

    
    copy = *exploded;
    PR_NormalizeTime(&copy, PR_GMTParameters);

    numDays = DAYS_BETWEEN_YEARS(1970, copy.tm_year);
    
    numSecs = copy.tm_yday * 86400 + copy.tm_hour * 3600
            + copy.tm_min * 60 + copy.tm_sec;

    LL_I2L(temp, numDays);
    LL_I2L(secPerDay, 86400);
    LL_MUL(temp, temp, secPerDay);
    LL_I2L(numSecs64, numSecs);
    LL_ADD(numSecs64, numSecs64, temp);

    
    LL_I2L(temp,  copy.tm_params.tp_gmt_offset);
    LL_SUB(numSecs64, numSecs64, temp);
    LL_I2L(temp,  copy.tm_params.tp_dst_offset);
    LL_SUB(numSecs64, numSecs64, temp);

    LL_I2L(usecPerSec, 1000000L);
    LL_MUL(temp, numSecs64, usecPerSec);
    LL_I2L(retVal, copy.tm_usec);
    LL_ADD(retVal, retVal, temp);

    return retVal;
}











static int IsLeapYear(PRInt16 year)
{
    if ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0)
        return 1;
    else
        return 0;
}






static void
ApplySecOffset(PRExplodedTime *time, PRInt32 secOffset)
{
    time->tm_sec += secOffset;

    
    if (time->tm_sec < 0 || time->tm_sec >= 60) {
        time->tm_min += time->tm_sec / 60;
        time->tm_sec %= 60;
        if (time->tm_sec < 0) {
            time->tm_sec += 60;
            time->tm_min--;
        }
    }

    if (time->tm_min < 0 || time->tm_min >= 60) {
        time->tm_hour += time->tm_min / 60;
        time->tm_min %= 60;
        if (time->tm_min < 0) {
            time->tm_min += 60;
            time->tm_hour--;
        }
    }

    if (time->tm_hour < 0) {
        
        time->tm_hour += 24;
        time->tm_mday--;
        time->tm_yday--;
        if (time->tm_mday < 1) {
            time->tm_month--;
            if (time->tm_month < 0) {
                time->tm_month = 11;
                time->tm_year--;
                if (IsLeapYear(time->tm_year))
                    time->tm_yday = 365;
                else
                    time->tm_yday = 364;
            }
            time->tm_mday = nDays[IsLeapYear(time->tm_year)][time->tm_month];
        }
        time->tm_wday--;
        if (time->tm_wday < 0)
            time->tm_wday = 6;
    } else if (time->tm_hour > 23) {
        
        time->tm_hour -= 24;
        time->tm_mday++;
        time->tm_yday++;
        if (time->tm_mday >
                nDays[IsLeapYear(time->tm_year)][time->tm_month]) {
            time->tm_mday = 1;
            time->tm_month++;
            if (time->tm_month > 11) {
                time->tm_month = 0;
                time->tm_year++;
                time->tm_yday = 0;
            }
        }
        time->tm_wday++;
        if (time->tm_wday > 6)
            time->tm_wday = 0;
    }
}

PR_IMPLEMENT(void)
PR_NormalizeTime(PRExplodedTime *time, PRTimeParamFn params)
{
    int daysInMonth;
    PRInt32 numDays;

    
    time->tm_sec -= time->tm_params.tp_gmt_offset
            + time->tm_params.tp_dst_offset;
    time->tm_params.tp_gmt_offset = 0;
    time->tm_params.tp_dst_offset = 0;

    

    if (time->tm_usec < 0 || time->tm_usec >= 1000000) {
        time->tm_sec +=  time->tm_usec / 1000000;
        time->tm_usec %= 1000000;
        if (time->tm_usec < 0) {
            time->tm_usec += 1000000;
            time->tm_sec--;
        }
    }

    
    if (time->tm_sec < 0 || time->tm_sec >= 60) {
        time->tm_min += time->tm_sec / 60;
        time->tm_sec %= 60;
        if (time->tm_sec < 0) {
            time->tm_sec += 60;
            time->tm_min--;
        }
    }

    if (time->tm_min < 0 || time->tm_min >= 60) {
        time->tm_hour += time->tm_min / 60;
        time->tm_min %= 60;
        if (time->tm_min < 0) {
            time->tm_min += 60;
            time->tm_hour--;
        }
    }

    if (time->tm_hour < 0 || time->tm_hour >= 24) {
        time->tm_mday += time->tm_hour / 24;
        time->tm_hour %= 24;
        if (time->tm_hour < 0) {
            time->tm_hour += 24;
            time->tm_mday--;
        }
    }

    
    if (time->tm_month < 0 || time->tm_month >= 12) {
        time->tm_year += time->tm_month / 12;
        time->tm_month %= 12;
        if (time->tm_month < 0) {
            time->tm_month += 12;
            time->tm_year--;
        }
    }

    

    if (time->tm_mday < 1) {
        
        do {
            
            time->tm_month--;
            if (time->tm_month < 0) {
                time->tm_month = 11;
                time->tm_year--;
            }
            time->tm_mday += nDays[IsLeapYear(time->tm_year)][time->tm_month];
        } while (time->tm_mday < 1);
    } else {
        daysInMonth = nDays[IsLeapYear(time->tm_year)][time->tm_month];
        while (time->tm_mday > daysInMonth) {
            
            time->tm_mday -= daysInMonth;
            time->tm_month++;
            if (time->tm_month > 11) {
                time->tm_month = 0;
                time->tm_year++;
            }
            daysInMonth = nDays[IsLeapYear(time->tm_year)][time->tm_month];
        }
    }

    
    time->tm_yday = time->tm_mday +
            lastDayOfMonth[IsLeapYear(time->tm_year)][time->tm_month];
	    
    numDays = DAYS_BETWEEN_YEARS(1970, time->tm_year) + time->tm_yday;
    time->tm_wday = (numDays + 4) % 7;
    if (time->tm_wday < 0) {
        time->tm_wday += 7;
    }

    

    time->tm_params = params(time);

    ApplySecOffset(time, time->tm_params.tp_gmt_offset
            + time->tm_params.tp_dst_offset);
}

















#if defined(HAVE_INT_LOCALTIME_R)












#define MT_safe_localtime(timer, result) \
        (localtime_r(timer, result) == -1 ? NULL: result)

#elif defined(HAVE_POINTER_LOCALTIME_R)

#define MT_safe_localtime localtime_r

#else

#define HAVE_LOCALTIME_MONITOR 1  /* We use 'monitor' to serialize our calls
                                   * to localtime(). */
static PRLock *monitor = NULL;

static struct tm *MT_safe_localtime(const time_t *clock, struct tm *result)
{
    struct tm *tmPtr;
    int needLock = PR_Initialized();  



    if (needLock) PR_Lock(monitor);

    














    
    tmPtr = localtime(clock);

#if defined(WIN16) || defined(XP_OS2)
    if ( (PRInt32) *clock < 0 ||
         ( (PRInt32) *clock == 0 && tmPtr->tm_year != 70))
        result = NULL;
    else
        *result = *tmPtr;
#else
    if (tmPtr) {
        *result = *tmPtr;
    } else {
        result = NULL;
    }
#endif 

    if (needLock) PR_Unlock(monitor);

    return result;
}

#endif  

void _PR_InitTime(void)
{
#ifdef HAVE_LOCALTIME_MONITOR
    monitor = PR_NewLock();
#endif
#ifdef WINCE
    _MD_InitTime();
#endif
}

void _PR_CleanupTime(void)
{
#ifdef HAVE_LOCALTIME_MONITOR
    if (monitor) {
        PR_DestroyLock(monitor);
        monitor = NULL;
    }
#endif
#ifdef WINCE
    _MD_CleanupTime();
#endif
}

#if defined(XP_UNIX) || defined(XP_PC) || defined(XP_BEOS)

PR_IMPLEMENT(PRTimeParameters)
PR_LocalTimeParameters(const PRExplodedTime *gmt)
{

    PRTimeParameters retVal;
    struct tm localTime;
    time_t secs;
    PRTime secs64;
    PRInt64 usecPerSec;
    PRInt64 usecPerSec_1;
    PRInt64 maxInt32;
    PRInt64 minInt32;
    PRInt32 dayOffset;
    PRInt32 offset2Jan1970;
    PRInt32 offsetNew;
    int isdst2Jan1970;

    













    secs = 86400L;
    (void) MT_safe_localtime(&secs, &localTime);

    

    offset2Jan1970 = (PRInt32)localTime.tm_sec 
            + 60L * (PRInt32)localTime.tm_min
            + 3600L * (PRInt32)localTime.tm_hour
            + 86400L * (PRInt32)((PRInt32)localTime.tm_mday - 2L);

    isdst2Jan1970 = localTime.tm_isdst;

    










    secs64 = PR_ImplodeTime(gmt);    
    LL_I2L(usecPerSec, PR_USEC_PER_SEC);
    LL_I2L(usecPerSec_1, PR_USEC_PER_SEC - 1);
    
    if (LL_GE_ZERO(secs64)) {
        LL_DIV(secs64, secs64, usecPerSec);
    } else {
        LL_NEG(secs64, secs64);
        LL_ADD(secs64, secs64, usecPerSec_1);
        LL_DIV(secs64, secs64, usecPerSec);
        LL_NEG(secs64, secs64);
    }
    LL_I2L(maxInt32, PR_INT32_MAX);
    LL_I2L(minInt32, PR_INT32_MIN);
    if (LL_CMP(secs64, >, maxInt32) || LL_CMP(secs64, <, minInt32)) {
        
        retVal.tp_gmt_offset = offset2Jan1970;
        retVal.tp_dst_offset = 0;
        return retVal;
    }
    LL_L2I(secs, secs64);

    






    if (MT_safe_localtime(&secs, &localTime) == NULL) {
        retVal.tp_gmt_offset = offset2Jan1970;
        retVal.tp_dst_offset = 0;
        return retVal;
    }

    





    dayOffset = (PRInt32) localTime.tm_wday - gmt->tm_wday;

    




    if (dayOffset == -6) {
        
        dayOffset = 1;
    } else if (dayOffset == 6) {
        
        dayOffset = -1;
    }

    offsetNew = (PRInt32)localTime.tm_sec - gmt->tm_sec
            + 60L * ((PRInt32)localTime.tm_min - gmt->tm_min)
            + 3600L * ((PRInt32)localTime.tm_hour - gmt->tm_hour)
            + 86400L * (PRInt32)dayOffset;

    if (localTime.tm_isdst <= 0) {
        
        retVal.tp_gmt_offset = offsetNew;
        retVal.tp_dst_offset = 0;
    } else {
        
        if (isdst2Jan1970 <=0) {
            




            retVal.tp_gmt_offset = offset2Jan1970;
            retVal.tp_dst_offset = offsetNew - offset2Jan1970;
        } else {
            




            retVal.tp_gmt_offset = offsetNew - 3600;
            retVal.tp_dst_offset = 3600;
        }
    }
    
    return retVal;
}

#endif    

















#define firstSunday(mday, wday) (((mday - wday + 7 - 1) % 7) + 1)










static PRInt32 
NthSunday(PRInt32 mday, PRInt32 wday, PRInt32 N, PRInt32 ndays) 
{
    PRInt32 firstSun = firstSunday(mday, wday);

    if (N < 0) 
        N = (ndays - firstSun) / 7;
    return firstSun + (7 * N);
}

typedef struct DSTParams {
    PRInt8 dst_start_month;       
    PRInt8 dst_start_Nth_Sunday;  
    PRInt8 dst_start_month_ndays; 
    PRInt8 dst_end_month;         
    PRInt8 dst_end_Nth_Sunday;    
    PRInt8 dst_end_month_ndays;   
} DSTParams;

static const DSTParams dstParams[2] = {
    
    { 3, 0, 30, 9, -1, 31 },
    
    { 2, 1, 31, 10, 0, 30 }
};

PR_IMPLEMENT(PRTimeParameters)
PR_USPacificTimeParameters(const PRExplodedTime *gmt)
{
    const DSTParams *dst;
    PRTimeParameters retVal;
    PRExplodedTime st;

    





    retVal.tp_gmt_offset = -8L * 3600L;

    




    st.tm_usec = gmt->tm_usec;
    st.tm_sec = gmt->tm_sec;
    st.tm_min = gmt->tm_min;
    st.tm_hour = gmt->tm_hour;
    st.tm_mday = gmt->tm_mday;
    st.tm_month = gmt->tm_month;
    st.tm_year = gmt->tm_year;
    st.tm_wday = gmt->tm_wday;
    st.tm_yday = gmt->tm_yday;

    
    ApplySecOffset(&st, retVal.tp_gmt_offset);

    if (st.tm_year < 2007) { 
	dst = &dstParams[0];
    } else {                 
	dst = &dstParams[1];
    }

    



    if (st.tm_month < dst->dst_start_month) {
        retVal.tp_dst_offset = 0L;
    } else if (st.tm_month == dst->dst_start_month) {
	int NthSun = NthSunday(st.tm_mday, st.tm_wday, 
			       dst->dst_start_Nth_Sunday, 
			       dst->dst_start_month_ndays);
	if (st.tm_mday < NthSun) {              
	    retVal.tp_dst_offset = 0L;
        } else if (st.tm_mday == NthSun) {      
	    
	    if (st.tm_hour < 2) {
		retVal.tp_dst_offset = 0L;
	    } else {
		retVal.tp_dst_offset = 3600L;
	    }
	} else {                                
	    retVal.tp_dst_offset = 3600L;
        }
    } else if (st.tm_month < dst->dst_end_month) {
        retVal.tp_dst_offset = 3600L;
    } else if (st.tm_month == dst->dst_end_month) {
	int NthSun = NthSunday(st.tm_mday, st.tm_wday, 
			       dst->dst_end_Nth_Sunday, 
			       dst->dst_end_month_ndays);
	if (st.tm_mday < NthSun) {              
	    retVal.tp_dst_offset = 3600L;
        } else if (st.tm_mday == NthSun) {      
	    
	    if (st.tm_hour < 1) {
		retVal.tp_dst_offset = 3600L;
	    } else {
		retVal.tp_dst_offset = 0L;
	    }
	} else {                                
	    retVal.tp_dst_offset = 0L;
        }
    } else {
        retVal.tp_dst_offset = 0L;
    }
    return retVal;
}












PR_IMPLEMENT(PRTimeParameters)
PR_GMTParameters(const PRExplodedTime *gmt)
{
    PRTimeParameters retVal = { 0, 0 };
    return retVal;
}























typedef enum
{
  TT_UNKNOWN,

  TT_SUN, TT_MON, TT_TUE, TT_WED, TT_THU, TT_FRI, TT_SAT,

  TT_JAN, TT_FEB, TT_MAR, TT_APR, TT_MAY, TT_JUN,
  TT_JUL, TT_AUG, TT_SEP, TT_OCT, TT_NOV, TT_DEC,

  TT_PST, TT_PDT, TT_MST, TT_MDT, TT_CST, TT_CDT, TT_EST, TT_EDT,
  TT_AST, TT_NST, TT_GMT, TT_BST, TT_MET, TT_EET, TT_JST
} TIME_TOKEN;
































PR_IMPLEMENT(PRStatus)
PR_ParseTimeStringToExplodedTime(
        const char *string,
        PRBool default_to_gmt,
        PRExplodedTime *result)
{
  TIME_TOKEN dotw = TT_UNKNOWN;
  TIME_TOKEN month = TT_UNKNOWN;
  TIME_TOKEN zone = TT_UNKNOWN;
  int zone_offset = -1;
  int dst_offset = 0;
  int date = -1;
  PRInt32 year = -1;
  int hour = -1;
  int min = -1;
  int sec = -1;

  const char *rest = string;

  int iterations = 0;

  PR_ASSERT(string && result);
  if (!string || !result) return PR_FAILURE;

  while (*rest)
        {

          if (iterations++ > 1000)
                {
                  return PR_FAILURE;
                }

          switch (*rest)
                {
                case 'a': case 'A':
                  if (month == TT_UNKNOWN &&
                          (rest[1] == 'p' || rest[1] == 'P') &&
                          (rest[2] == 'r' || rest[2] == 'R'))
                        month = TT_APR;
                  else if (zone == TT_UNKNOWN &&
                                   (rest[1] == 's' || rest[1] == 'S') &&
                                   (rest[2] == 't' || rest[2] == 'T'))
                        zone = TT_AST;
                  else if (month == TT_UNKNOWN &&
                                   (rest[1] == 'u' || rest[1] == 'U') &&
                                   (rest[2] == 'g' || rest[2] == 'G'))
                        month = TT_AUG;
                  break;
                case 'b': case 'B':
                  if (zone == TT_UNKNOWN &&
                          (rest[1] == 's' || rest[1] == 'S') &&
                          (rest[2] == 't' || rest[2] == 'T'))
                        zone = TT_BST;
                  break;
                case 'c': case 'C':
                  if (zone == TT_UNKNOWN &&
                          (rest[1] == 'd' || rest[1] == 'D') &&
                          (rest[2] == 't' || rest[2] == 'T'))
                        zone = TT_CDT;
                  else if (zone == TT_UNKNOWN &&
                                   (rest[1] == 's' || rest[1] == 'S') &&
                                   (rest[2] == 't' || rest[2] == 'T'))
                        zone = TT_CST;
                  break;
                case 'd': case 'D':
                  if (month == TT_UNKNOWN &&
                          (rest[1] == 'e' || rest[1] == 'E') &&
                          (rest[2] == 'c' || rest[2] == 'C'))
                        month = TT_DEC;
                  break;
                case 'e': case 'E':
                  if (zone == TT_UNKNOWN &&
                          (rest[1] == 'd' || rest[1] == 'D') &&
                          (rest[2] == 't' || rest[2] == 'T'))
                        zone = TT_EDT;
                  else if (zone == TT_UNKNOWN &&
                                   (rest[1] == 'e' || rest[1] == 'E') &&
                                   (rest[2] == 't' || rest[2] == 'T'))
                        zone = TT_EET;
                  else if (zone == TT_UNKNOWN &&
                                   (rest[1] == 's' || rest[1] == 'S') &&
                                   (rest[2] == 't' || rest[2] == 'T'))
                        zone = TT_EST;
                  break;
                case 'f': case 'F':
                  if (month == TT_UNKNOWN &&
                          (rest[1] == 'e' || rest[1] == 'E') &&
                          (rest[2] == 'b' || rest[2] == 'B'))
                        month = TT_FEB;
                  else if (dotw == TT_UNKNOWN &&
                                   (rest[1] == 'r' || rest[1] == 'R') &&
                                   (rest[2] == 'i' || rest[2] == 'I'))
                        dotw = TT_FRI;
                  break;
                case 'g': case 'G':
                  if (zone == TT_UNKNOWN &&
                          (rest[1] == 'm' || rest[1] == 'M') &&
                          (rest[2] == 't' || rest[2] == 'T'))
                        zone = TT_GMT;
                  break;
                case 'j': case 'J':
                  if (month == TT_UNKNOWN &&
                          (rest[1] == 'a' || rest[1] == 'A') &&
                          (rest[2] == 'n' || rest[2] == 'N'))
                        month = TT_JAN;
                  else if (zone == TT_UNKNOWN &&
                                   (rest[1] == 's' || rest[1] == 'S') &&
                                   (rest[2] == 't' || rest[2] == 'T'))
                        zone = TT_JST;
                  else if (month == TT_UNKNOWN &&
                                   (rest[1] == 'u' || rest[1] == 'U') &&
                                   (rest[2] == 'l' || rest[2] == 'L'))
                        month = TT_JUL;
                  else if (month == TT_UNKNOWN &&
                                   (rest[1] == 'u' || rest[1] == 'U') &&
                                   (rest[2] == 'n' || rest[2] == 'N'))
                        month = TT_JUN;
                  break;
                case 'm': case 'M':
                  if (month == TT_UNKNOWN &&
                          (rest[1] == 'a' || rest[1] == 'A') &&
                          (rest[2] == 'r' || rest[2] == 'R'))
                        month = TT_MAR;
                  else if (month == TT_UNKNOWN &&
                                   (rest[1] == 'a' || rest[1] == 'A') &&
                                   (rest[2] == 'y' || rest[2] == 'Y'))
                        month = TT_MAY;
                  else if (zone == TT_UNKNOWN &&
                                   (rest[1] == 'd' || rest[1] == 'D') &&
                                   (rest[2] == 't' || rest[2] == 'T'))
                        zone = TT_MDT;
                  else if (zone == TT_UNKNOWN &&
                                   (rest[1] == 'e' || rest[1] == 'E') &&
                                   (rest[2] == 't' || rest[2] == 'T'))
                        zone = TT_MET;
                  else if (dotw == TT_UNKNOWN &&
                                   (rest[1] == 'o' || rest[1] == 'O') &&
                                   (rest[2] == 'n' || rest[2] == 'N'))
                        dotw = TT_MON;
                  else if (zone == TT_UNKNOWN &&
                                   (rest[1] == 's' || rest[1] == 'S') &&
                                   (rest[2] == 't' || rest[2] == 'T'))
                        zone = TT_MST;
                  break;
                case 'n': case 'N':
                  if (month == TT_UNKNOWN &&
                          (rest[1] == 'o' || rest[1] == 'O') &&
                          (rest[2] == 'v' || rest[2] == 'V'))
                        month = TT_NOV;
                  else if (zone == TT_UNKNOWN &&
                                   (rest[1] == 's' || rest[1] == 'S') &&
                                   (rest[2] == 't' || rest[2] == 'T'))
                        zone = TT_NST;
                  break;
                case 'o': case 'O':
                  if (month == TT_UNKNOWN &&
                          (rest[1] == 'c' || rest[1] == 'C') &&
                          (rest[2] == 't' || rest[2] == 'T'))
                        month = TT_OCT;
                  break;
                case 'p': case 'P':
                  if (zone == TT_UNKNOWN &&
                          (rest[1] == 'd' || rest[1] == 'D') &&
                          (rest[2] == 't' || rest[2] == 'T'))
                        zone = TT_PDT;
                  else if (zone == TT_UNKNOWN &&
                                   (rest[1] == 's' || rest[1] == 'S') &&
                                   (rest[2] == 't' || rest[2] == 'T'))
                        zone = TT_PST;
                  break;
                case 's': case 'S':
                  if (dotw == TT_UNKNOWN &&
                          (rest[1] == 'a' || rest[1] == 'A') &&
                          (rest[2] == 't' || rest[2] == 'T'))
                        dotw = TT_SAT;
                  else if (month == TT_UNKNOWN &&
                                   (rest[1] == 'e' || rest[1] == 'E') &&
                                   (rest[2] == 'p' || rest[2] == 'P'))
                        month = TT_SEP;
                  else if (dotw == TT_UNKNOWN &&
                                   (rest[1] == 'u' || rest[1] == 'U') &&
                                   (rest[2] == 'n' || rest[2] == 'N'))
                        dotw = TT_SUN;
                  break;
                case 't': case 'T':
                  if (dotw == TT_UNKNOWN &&
                          (rest[1] == 'h' || rest[1] == 'H') &&
                          (rest[2] == 'u' || rest[2] == 'U'))
                        dotw = TT_THU;
                  else if (dotw == TT_UNKNOWN &&
                                   (rest[1] == 'u' || rest[1] == 'U') &&
                                   (rest[2] == 'e' || rest[2] == 'E'))
                        dotw = TT_TUE;
                  break;
                case 'u': case 'U':
                  if (zone == TT_UNKNOWN &&
                          (rest[1] == 't' || rest[1] == 'T') &&
                          !(rest[2] >= 'A' && rest[2] <= 'Z') &&
                          !(rest[2] >= 'a' && rest[2] <= 'z'))
                        
                        zone = TT_GMT;
                  break;
                case 'w': case 'W':
                  if (dotw == TT_UNKNOWN &&
                          (rest[1] == 'e' || rest[1] == 'E') &&
                          (rest[2] == 'd' || rest[2] == 'D'))
                        dotw = TT_WED;
                  break;

                case '+': case '-':
                  {
                        const char *end;
                        int sign;
                        if (zone_offset != -1)
                          {
                                
                                rest++;
                                break;
                          }
                        if (zone != TT_UNKNOWN && zone != TT_GMT)
                          {
                                
                                rest++;
                                break;
                          }

                        sign = ((*rest == '+') ? 1 : -1);
                        rest++; 
                        end = rest;
                        while (*end >= '0' && *end <= '9')
                          end++;
                        if (rest == end) 
                          break;

                        if ((end - rest) == 4)
                          
                          zone_offset = (((((rest[0]-'0')*10) + (rest[1]-'0')) * 60) +
                                                         (((rest[2]-'0')*10) + (rest[3]-'0')));
                        else if ((end - rest) == 2)
                          
                          zone_offset = (((rest[0]-'0')*10) + (rest[1]-'0')) * 60;
                        else if ((end - rest) == 1)
                          
                          zone_offset = (rest[0]-'0') * 60;
                        else
                          
                          break;

                        zone_offset *= sign;
                        zone = TT_GMT;
                        break;
                  }

                case '0': case '1': case '2': case '3': case '4':
                case '5': case '6': case '7': case '8': case '9':
                  {
                        int tmp_hour = -1;
                        int tmp_min = -1;
                        int tmp_sec = -1;
                        const char *end = rest + 1;
                        while (*end >= '0' && *end <= '9')
                          end++;

                        

                        if (*end == ':')
                          {
                                if (hour >= 0 && min >= 0) 
                                  break;

                                
                                if ((end - rest) > 2)
                                  
                                  break;
                                else if ((end - rest) == 2)
                                  tmp_hour = ((rest[0]-'0')*10 +
                                                          (rest[1]-'0'));
                                else
                                  tmp_hour = (rest[0]-'0');

                                

                                rest = ++end;
                                while (*end >= '0' && *end <= '9')
                                  end++;

                                if (end == rest)
                                  
                                  break;
                                else if ((end - rest) > 2)
                                  
                                  break;
                                else if ((end - rest) == 2)
                                  tmp_min = ((rest[0]-'0')*10 +
                                                         (rest[1]-'0'));
                                else
                                  tmp_min = (rest[0]-'0');

                                
                                rest = end;
                                if (*rest == ':')
                                  rest++;
                                end = rest;
                                while (*end >= '0' && *end <= '9')
                                  end++;

                                if (end == rest)
                                  
                                  ;
                                else if ((end - rest) > 2)
                                  
                                  break;
                                else if ((end - rest) == 2)
                                  tmp_sec = ((rest[0]-'0')*10 +
                                                         (rest[1]-'0'));
                                else
                                  tmp_sec = (rest[0]-'0');

                                


                                


                                if (tmp_hour <= 12)
                                  {
                                        const char *s = end;
                                        while (*s && (*s == ' ' || *s == '\t'))
                                          s++;
                                        if ((s[0] == 'p' || s[0] == 'P') &&
                                                (s[1] == 'm' || s[1] == 'M'))
                                          
                                          tmp_hour = (tmp_hour == 12 ? 12 : tmp_hour + 12);
                                        else if (tmp_hour == 12 &&
                                                         (s[0] == 'a' || s[0] == 'A') &&
                                                         (s[1] == 'm' || s[1] == 'M'))
                                          
                                          tmp_hour = 0;
                                  }

                                hour = tmp_hour;
                                min = tmp_min;
                                sec = tmp_sec;
                                rest = end;
                                break;
                          }
                        else if ((*end == '/' || *end == '-') &&
                                         end[1] >= '0' && end[1] <= '9')
                          {
                                



                                int n1, n2, n3;
                                const char *s;

                                if (month != TT_UNKNOWN)
                                  
                                  break;

                                s = rest;

                                n1 = (*s++ - '0');                                
                                if (*s >= '0' && *s <= '9')
                                  n1 = n1*10 + (*s++ - '0');

                                if (*s != '/' && *s != '-')                
                                  break;
                                s++;

                                if (*s < '0' || *s > '9')                
                                  break;
                                n2 = (*s++ - '0');
                                if (*s >= '0' && *s <= '9')
                                  n2 = n2*10 + (*s++ - '0');

                                if (*s != '/' && *s != '-')                
                                  break;
                                s++;

                                if (*s < '0' || *s > '9')                
                                  break;
                                n3 = (*s++ - '0');
                                if (*s >= '0' && *s <= '9')
                                  n3 = n3*10 + (*s++ - '0');

                                if (*s >= '0' && *s <= '9')            
                                  {
                                        n3 = n3*10 + (*s++ - '0');
                                        if (*s < '0' || *s > '9')
                                          break;
                                        n3 = n3*10 + (*s++ - '0');
                                        if (*s >= '0' && *s <= '9')
                                          n3 = n3*10 + (*s++ - '0');
                                  }

                                if ((*s >= '0' && *s <= '9') ||        
                                        (*s >= 'A' && *s <= 'Z') ||
                                        (*s >= 'a' && *s <= 'z'))
                                  break;

                                




                                if (n1 > 31 || n1 == 0)  
                                  {
                                        if (n2 > 12) break;
                                        if (n3 > 31) break;
                                        year = n1;
                                        if (year < 70)
                                            year += 2000;
                                        else if (year < 100)
                                            year += 1900;
                                        month = (TIME_TOKEN)(n2 + ((int)TT_JAN) - 1);
                                        date = n3;
                                        rest = s;
                                        break;
                                  }

                                if (n1 > 12 && n2 > 12)  
                                  {
                                        rest = s;
                                        break;
                                  }

                                if (n3 < 70)
                                    n3 += 2000;
                                else if (n3 < 100)
                                    n3 += 1900;

                                if (n1 > 12)  
                                  {
                                        date = n1;
                                        month = (TIME_TOKEN)(n2 + ((int)TT_JAN) - 1);
                                        year = n3;
                                  }
                                else                  
                                  {
                                        

                                        month = (TIME_TOKEN)(n1 + ((int)TT_JAN) - 1);
                                        date = n2;
                                        year = n3;
                                  }
                                rest = s;
                          }
                        else if ((*end >= 'A' && *end <= 'Z') ||
                                         (*end >= 'a' && *end <= 'z'))
                          
                          ;
                        else if ((end - rest) == 5)                
                          year = (year < 0
                                          ? ((rest[0]-'0')*10000L +
                                                 (rest[1]-'0')*1000L +
                                                 (rest[2]-'0')*100L +
                                                 (rest[3]-'0')*10L +
                                                 (rest[4]-'0'))
                                          : year);
                        else if ((end - rest) == 4)                
                          year = (year < 0
                                          ? ((rest[0]-'0')*1000L +
                                                 (rest[1]-'0')*100L +
                                                 (rest[2]-'0')*10L +
                                                 (rest[3]-'0'))
                                          : year);
                        else if ((end - rest) == 2)                
                          {
                                int n = ((rest[0]-'0')*10 +
                                                 (rest[1]-'0'));
                                









                                if (date < 0 && n < 32)
                                  date = n;
                                else if (year < 0)
                                  {
                                        if (n < 70)
                                          year = 2000 + n;
                                        else if (n < 100)
                                          year = 1900 + n;
                                        else
                                          year = n;
                                  }
                                
                          }
                        else if ((end - rest) == 1)                
                          date = (date < 0 ? (rest[0]-'0') : date);
                        

                        break;
                  }
                }

          



          while (*rest &&
                         *rest != ' ' && *rest != '\t' &&
                         *rest != ',' && *rest != ';' &&
                         *rest != '-' && *rest != '+' &&
                         *rest != '/' &&
                         *rest != '(' && *rest != ')' && *rest != '[' && *rest != ']')
                rest++;
          
        SKIP_MORE:
          while (*rest &&
                         (*rest == ' ' || *rest == '\t' ||
                          *rest == ',' || *rest == ';' || *rest == '/' ||
                          *rest == '(' || *rest == ')' || *rest == '[' || *rest == ']'))
                rest++;

          

         
          if (*rest == '-' && ((rest > string && isalpha(rest[-1]) && year < 0)
              || rest[1] < '0' || rest[1] > '9'))
                {
                  rest++;
                  goto SKIP_MORE;
                }

        }

  if (zone != TT_UNKNOWN && zone_offset == -1)
        {
          switch (zone)
                {
                case TT_PST: zone_offset = -8 * 60; break;
                case TT_PDT: zone_offset = -8 * 60; dst_offset = 1 * 60; break;
                case TT_MST: zone_offset = -7 * 60; break;
                case TT_MDT: zone_offset = -7 * 60; dst_offset = 1 * 60; break;
                case TT_CST: zone_offset = -6 * 60; break;
                case TT_CDT: zone_offset = -6 * 60; dst_offset = 1 * 60; break;
                case TT_EST: zone_offset = -5 * 60; break;
                case TT_EDT: zone_offset = -5 * 60; dst_offset = 1 * 60; break;
                case TT_AST: zone_offset = -4 * 60; break;
                case TT_NST: zone_offset = -3 * 60 - 30; break;
                case TT_GMT: zone_offset =  0 * 60; break;
                case TT_BST: zone_offset =  0 * 60; dst_offset = 1 * 60; break;
                case TT_MET: zone_offset =  1 * 60; break;
                case TT_EET: zone_offset =  2 * 60; break;
                case TT_JST: zone_offset =  9 * 60; break;
                default:
                  PR_ASSERT (0);
                  break;
                }
        }

  



  if (month == TT_UNKNOWN || date == -1 || year == -1 || year > PR_INT16_MAX)
      return PR_FAILURE;

  memset(result, 0, sizeof(*result));
  if (sec != -1)
        result->tm_sec = sec;
  if (min != -1)
        result->tm_min = min;
  if (hour != -1)
        result->tm_hour = hour;
  if (date != -1)
        result->tm_mday = date;
  if (month != TT_UNKNOWN)
        result->tm_month = (((int)month) - ((int)TT_JAN));
  if (year != -1)
        result->tm_year = year;
  if (dotw != TT_UNKNOWN)
        result->tm_wday = (((int)dotw) - ((int)TT_SUN));
  



  PR_NormalizeTime(result, PR_GMTParameters);
  

  if (zone == TT_UNKNOWN && default_to_gmt)
        {
          
          zone = TT_GMT;
          zone_offset = 0;
        }

  if (zone_offset == -1)
         {
           

          struct tm localTime;
          time_t secs;

          PR_ASSERT(result->tm_month > -1 &&
                    result->tm_mday > 0 &&
                    result->tm_hour > -1 &&
                    result->tm_min > -1 &&
                    result->tm_sec > -1);

            










          
  
          if(result->tm_year >= 1970)
                {
                  PRInt64 usec_per_sec;

                  localTime.tm_sec = result->tm_sec;
                  localTime.tm_min = result->tm_min;
                  localTime.tm_hour = result->tm_hour;
                  localTime.tm_mday = result->tm_mday;
                  localTime.tm_mon = result->tm_month;
                  localTime.tm_year = result->tm_year - 1900;
                  



                  localTime.tm_isdst = -1;

#if _MSC_VER == 1400  
                  













                  if (result->tm_year >= 3000) {
                      
                      errno = EINVAL;
                      secs = (time_t) -1;
                  } else {
                      secs = mktime(&localTime);
                  }
#else
                  secs = mktime(&localTime);
#endif
                  if (secs != (time_t) -1)
                    {
                      PRTime usecs64;
                      LL_I2L(usecs64, secs);
                      LL_I2L(usec_per_sec, PR_USEC_PER_SEC);
                      LL_MUL(usecs64, usecs64, usec_per_sec);
                      PR_ExplodeTime(usecs64, PR_LocalTimeParameters, result);
                      return PR_SUCCESS;
                    }
                }
                
                


                secs = 86400;
                (void) MT_safe_localtime(&secs, &localTime);
                zone_offset = localTime.tm_min
                              + 60 * localTime.tm_hour
                              + 1440 * (localTime.tm_mday - 2);
        }

  result->tm_params.tp_gmt_offset = zone_offset * 60;
  result->tm_params.tp_dst_offset = dst_offset * 60;

  return PR_SUCCESS;
}

PR_IMPLEMENT(PRStatus)
PR_ParseTimeString(
        const char *string,
        PRBool default_to_gmt,
        PRTime *result)
{
  PRExplodedTime tm;
  PRStatus rv;

  rv = PR_ParseTimeStringToExplodedTime(string,
                                        default_to_gmt,
                                        &tm);
  if (rv != PR_SUCCESS)
        return rv;

  *result = PR_ImplodeTime(&tm);

  return PR_SUCCESS;
}






















PR_IMPLEMENT(PRUint32)
PR_FormatTime(char *buf, int buflen, const char *fmt, const PRExplodedTime *tm)
{
    size_t rv;
    struct tm a;
    struct tm *ap;

    if (tm) {
        ap = &a;
        a.tm_sec = tm->tm_sec;
        a.tm_min = tm->tm_min;
        a.tm_hour = tm->tm_hour;
        a.tm_mday = tm->tm_mday;
        a.tm_mon = tm->tm_month;
        a.tm_wday = tm->tm_wday;
        a.tm_year = tm->tm_year - 1900;
        a.tm_yday = tm->tm_yday;
        a.tm_isdst = tm->tm_params.tp_dst_offset ? 1 : 0;

        




#if defined(SUNOS4) || (__GLIBC__ >= 2) || defined(XP_BEOS) \
        || defined(NETBSD) || defined(OPENBSD) || defined(FREEBSD) \
        || defined(DARWIN) || defined(SYMBIAN)
        a.tm_zone = NULL;
        a.tm_gmtoff = tm->tm_params.tp_gmt_offset +
                      tm->tm_params.tp_dst_offset;
#endif
    } else {
        ap = NULL;
    }

    rv = strftime(buf, buflen, fmt, ap);
    if (!rv && buf && buflen > 0) {
        




        buf[0] = '\0';
    }
    return rv;
}






static const char* abbrevDays[] =
{
   "Sun","Mon","Tue","Wed","Thu","Fri","Sat"
};

static const char* days[] =
{
   "Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"
};

static const char* abbrevMonths[] =
{
   "Jan", "Feb", "Mar", "Apr", "May", "Jun",
   "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static const char* months[] =
{ 
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
};






#define ADDCHAR( buf, bufSize, ch )             \
do                                              \
{                                               \
   if( bufSize < 1 )                            \
   {                                            \
      *(--buf) = '\0';                          \
      return 0;                                 \
   }                                            \
   *buf++ = ch;                                 \
   bufSize--;                                   \
}                                               \
while(0)






#define ADDSTR( buf, bufSize, str )             \
do                                              \
{                                               \
   PRUint32 strSize = strlen( str );              \
   if( strSize > bufSize )                      \
   {                                            \
      if( bufSize==0 )                          \
         *(--buf) = '\0';                       \
      else                                      \
         *buf = '\0';                           \
      return 0;                                 \
   }                                            \
   memcpy(buf, str, strSize);                   \
   buf += strSize;                              \
   bufSize -= strSize;                          \
}                                               \
while(0)


static unsigned int  pr_WeekOfYear(const PRExplodedTime* time,
        unsigned int firstDayOfWeek);










             
PR_IMPLEMENT(PRUint32)
PR_FormatTimeUSEnglish( char* buf, PRUint32 bufSize,
                        const char* format, const PRExplodedTime* time )
{
   char*         bufPtr = buf;
   const char*   fmtPtr;
   char          tmpBuf[ 40 ];        
   const int     tmpBufSize = sizeof( tmpBuf );

   
   for( fmtPtr=format; *fmtPtr != '\0'; fmtPtr++ )
   {
      if( *fmtPtr != '%' )
      {
         ADDCHAR( bufPtr, bufSize, *fmtPtr );
      }
      else
      {
         switch( *(++fmtPtr) )
         {
         case '%':
            
            ADDCHAR( bufPtr, bufSize, '%' );
            break;
            
         case 'a':
            
            ADDSTR( bufPtr, bufSize, abbrevDays[ time->tm_wday ] );
            break;
               
         case 'A':
            
            ADDSTR( bufPtr, bufSize, days[ time->tm_wday ] );
            break;
        
         case 'b':
            
            ADDSTR( bufPtr, bufSize, abbrevMonths[ time->tm_month ] );
            break;
        
         case 'B':
            
            ADDSTR(bufPtr, bufSize,  months[ time->tm_month ] );
            break;
        
         case 'c':
            
            PR_FormatTimeUSEnglish( tmpBuf, tmpBufSize, "%a %b %d %H:%M:%S %Y", time );
            ADDSTR( bufPtr, bufSize, tmpBuf );
            break;
        
         case 'd':
            
            PR_snprintf(tmpBuf,tmpBufSize,"%.2ld",time->tm_mday );
            ADDSTR( bufPtr, bufSize, tmpBuf ); 
            break;

         case 'H':
            
            PR_snprintf(tmpBuf,tmpBufSize,"%.2ld",time->tm_hour );
            ADDSTR( bufPtr, bufSize, tmpBuf ); 
            break;
        
         case 'I':
            
            PR_snprintf(tmpBuf,tmpBufSize,"%.2ld",
                        (time->tm_hour%12) ? time->tm_hour%12 : (PRInt32) 12 );
            ADDSTR( bufPtr, bufSize, tmpBuf ); 
            break;
        
         case 'j':
            
            PR_snprintf(tmpBuf,tmpBufSize,"%.3d",time->tm_yday + 1);
            ADDSTR( bufPtr, bufSize, tmpBuf ); 
            break;
        
         case 'm':
            
            PR_snprintf(tmpBuf,tmpBufSize,"%.2ld",time->tm_month+1);
            ADDSTR( bufPtr, bufSize, tmpBuf ); 
            break;
        
         case 'M':
            
            PR_snprintf(tmpBuf,tmpBufSize,"%.2ld",time->tm_min );
            ADDSTR( bufPtr, bufSize, tmpBuf ); 
            break;
       
         case 'p':
            
            ADDSTR( bufPtr, bufSize, (time->tm_hour<12)?"AM":"PM" ); 
            break;
        
         case 'S':
            
            PR_snprintf(tmpBuf,tmpBufSize,"%.2ld",time->tm_sec );
            ADDSTR( bufPtr, bufSize, tmpBuf ); 
            break;
     
         case 'U':
            
            PR_snprintf(tmpBuf,tmpBufSize,"%.2d", pr_WeekOfYear( time, 0 ) );
            ADDSTR( bufPtr, bufSize, tmpBuf );
            break;
        
         case 'w':
            
            PR_snprintf(tmpBuf,tmpBufSize,"%d",time->tm_wday );
            ADDSTR( bufPtr, bufSize, tmpBuf ); 
            break;
        
         case 'W':
            
            PR_snprintf(tmpBuf,tmpBufSize,"%.2d", pr_WeekOfYear( time, 1 ) );
            ADDSTR( bufPtr, bufSize, tmpBuf );
            break;
        
         case 'x':
            
            PR_FormatTimeUSEnglish( tmpBuf, tmpBufSize, "%m/%d/%y", time );
            ADDSTR( bufPtr, bufSize, tmpBuf );
            break;
        
         case 'X':
            
            PR_FormatTimeUSEnglish( tmpBuf, tmpBufSize, "%H:%M:%S", time );
            ADDSTR( bufPtr, bufSize, tmpBuf );
            break;
        
         case 'y':
            
            PR_snprintf(tmpBuf,tmpBufSize,"%.2d",time->tm_year % 100 );
            ADDSTR( bufPtr, bufSize, tmpBuf ); 
            break;
        
         case 'Y':
            
            PR_snprintf(tmpBuf,tmpBufSize,"%.4d",time->tm_year );
            ADDSTR( bufPtr, bufSize, tmpBuf ); 
            break;
        
         case 'Z':
            



            PR_FormatTime( tmpBuf, tmpBufSize, "%Z", time );
            ADDSTR( bufPtr, bufSize, tmpBuf ); 
            break;

         default:
            
            ADDCHAR( bufPtr, bufSize, '%' );
            ADDCHAR( bufPtr, bufSize, *fmtPtr );
            break;
            
         }
      }
   }

   ADDCHAR( bufPtr, bufSize, '\0' );
   return (PRUint32)(bufPtr - buf - 1);
}













static unsigned int
pr_WeekOfYear(const PRExplodedTime* time, unsigned int firstDayOfWeek)
{
   int dayOfWeek;
   int dayOfYear;

  


  dayOfWeek = time->tm_wday - firstDayOfWeek;
  if (dayOfWeek < 0)
    dayOfWeek += 7;
  
  dayOfYear = time->tm_yday - dayOfWeek;


  if( dayOfYear <= 0 )
  {
     
     return 0;
  }
  else
  {
     





     return (dayOfYear / 7) + ( (dayOfYear % 7) == 0 ? 0 : 1 );
  }
}

