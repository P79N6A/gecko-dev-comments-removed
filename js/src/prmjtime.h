






































#ifndef prmjtime_h___
#define prmjtime_h___




#include <time.h>
#include "jslong.h"
#ifdef MOZILLA_CLIENT
#include "jscompat.h"
#endif

JS_BEGIN_EXTERN_C

typedef struct PRMJTime       PRMJTime;




struct PRMJTime {
    JSInt32 tm_usec;            
    JSInt8 tm_sec;              
    JSInt8 tm_min;              
    JSInt8 tm_hour;             
    JSInt8 tm_mday;             
    JSInt8 tm_mon;              
    JSInt8 tm_wday;             
    JSInt16 tm_year;            
    JSInt16 tm_yday;            
    JSInt8 tm_isdst;            
};


#define PRMJ_USEC_PER_SEC       1000000L
#define PRMJ_USEC_PER_MSEC      1000L


extern JSInt64
PRMJ_Now(void);


#if defined(JS_THREADSAFE) && defined(XP_WIN)
extern void
PRMJ_NowShutdown(void);
#else
#define PRMJ_NowShutdown()
#endif


extern JSInt32
PRMJ_LocalGMTDifference(void);


extern size_t
PRMJ_FormatTime(char *buf, int buflen, const char *fmt, PRMJTime *tm);


extern JSInt64
PRMJ_DSTOffset(JSInt64 local_time);

JS_END_EXTERN_C

#endif 

