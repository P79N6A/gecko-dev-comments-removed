
































































#include "base/third_party/nspr/prtime.h"
#include "build/build_config.h"

#if defined(OS_WIN)
#include <windows.h>
#elif defined(OS_MACOSX)
#include <CoreFoundation/CoreFoundation.h>
#endif
#include <errno.h>  
#include <time.h>


#if defined(OS_WIN)
static void localtime_r(const time_t* secs, struct tm* time) {
  (void) localtime_s(time, secs);
}
#endif











PRTime
PR_ImplodeTime(const PRExplodedTime *exploded)
{
    
    
    static const PRTime kSecondsToMicroseconds = static_cast<PRTime>(1000000);
#if defined(OS_WIN)
   
    SYSTEMTIME st = {0};
    FILETIME ft = {0};
    ULARGE_INTEGER uli = {0};

    st.wYear = exploded->tm_year;
    st.wMonth = exploded->tm_month + 1;
    st.wDayOfWeek = exploded->tm_wday;
    st.wDay = exploded->tm_mday;
    st.wHour = exploded->tm_hour;
    st.wMinute = exploded->tm_min;
    st.wSecond = exploded->tm_sec;
    st.wMilliseconds = exploded->tm_usec/1000;
     
    if (!SystemTimeToFileTime(&st, &ft)) {
      NOTREACHED() << "Unable to convert time";
      return 0;
    }
    
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    
    
    PRTime result =
        static_cast<PRTime>((uli.QuadPart / 10) - 11644473600000000i64);
    
    result -= (exploded->tm_params.tp_gmt_offset +
               exploded->tm_params.tp_dst_offset) * kSecondsToMicroseconds;
    return result;
#elif defined(OS_MACOSX)
    
    CFGregorianDate gregorian_date;
    gregorian_date.year = exploded->tm_year;
    gregorian_date.month = exploded->tm_month + 1;
    gregorian_date.day = exploded->tm_mday;
    gregorian_date.hour = exploded->tm_hour;
    gregorian_date.minute = exploded->tm_min;
    gregorian_date.second = exploded->tm_sec;

    
    
    
    CFAbsoluteTime absolute_time = 
        CFGregorianDateGetAbsoluteTime(gregorian_date, NULL);
    PRTime result = static_cast<PRTime>(absolute_time);
    result -= exploded->tm_params.tp_gmt_offset +
              exploded->tm_params.tp_dst_offset;
    result += kCFAbsoluteTimeIntervalSince1970;  
    result *= kSecondsToMicroseconds;
    result += exploded->tm_usec;
    return result;
#elif defined(OS_LINUX)
    struct tm exp_tm = {0};
    exp_tm.tm_sec  = exploded->tm_sec;
    exp_tm.tm_min  = exploded->tm_min;
    exp_tm.tm_hour = exploded->tm_hour;
    exp_tm.tm_mday = exploded->tm_mday;
    exp_tm.tm_mon  = exploded->tm_month;
    exp_tm.tm_year = exploded->tm_year - 1900;

    
    time_t absolute_time = timegm(&exp_tm);

    
    
    if (absolute_time == -1 &&
        exploded->tm_year != 1969 && exploded->tm_month != 11 &&
        exploded->tm_mday != 31 && exploded->tm_hour != 23 &&
        exploded->tm_min != 59 && exploded->tm_sec != 59) {
      
      
      if (exploded->tm_year >= 1970)
        return static_cast<PRTime>(LONG_MAX) * kSecondsToMicroseconds;
      
      
      return static_cast<PRTime>(LONG_MIN) * kSecondsToMicroseconds;
    }

    PRTime result = static_cast<PRTime>(absolute_time);
    result -= exploded->tm_params.tp_gmt_offset +
              exploded->tm_params.tp_dst_offset;
    result *= kSecondsToMicroseconds;
    result += exploded->tm_usec;
    return result;
#else
#error No PR_ImplodeTime implemented on your platform.
#endif
}
















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

void
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












PRTimeParameters
PR_GMTParameters(const PRExplodedTime *gmt)
{
#if defined(XP_MAC)
#pragma unused (gmt)
#endif

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
































PRStatus
PR_ParseTimeString(
        const char *string,
        PRBool default_to_gmt,
        PRTime *result_imploded)
{
  PRExplodedTime tm;
  PRExplodedTime *result = &tm;
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
                      *result_imploded = usecs64;
                      return PR_SUCCESS;
                    }
                }
                
                


                secs = 86400;
                localtime_r(&secs, &localTime);
                zone_offset = localTime.tm_min
                              + 60 * localTime.tm_hour
                              + 1440 * (localTime.tm_mday - 2);
        }

  result->tm_params.tp_gmt_offset = zone_offset * 60;
  result->tm_params.tp_dst_offset = dst_offset * 60;

  *result_imploded = PR_ImplodeTime(result);
  return PR_SUCCESS;
}
