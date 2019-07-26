





#ifndef prmjtime_h___
#define prmjtime_h___

#include <time.h>




struct PRMJTime {
    int32_t tm_usec;            
    int8_t tm_sec;              
    int8_t tm_min;              
    int8_t tm_hour;             
    int8_t tm_mday;             
    int8_t tm_mon;              
    int8_t tm_wday;             
    int32_t tm_year;            
    int16_t tm_yday;            
    int8_t tm_isdst;            
};


#define PRMJ_USEC_PER_SEC       1000000L
#define PRMJ_USEC_PER_MSEC      1000L


extern int64_t
PRMJ_Now(void);


#if defined(JS_THREADSAFE) && defined(XP_WIN)
extern void
PRMJ_NowShutdown(void);
#else
#define PRMJ_NowShutdown()
#endif


extern size_t
PRMJ_FormatTime(char *buf, int buflen, const char *fmt, PRMJTime *tm);

#endif 

