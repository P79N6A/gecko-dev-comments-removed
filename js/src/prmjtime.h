






































#ifndef prmjtime_h___
#define prmjtime_h___




#include <time.h>

#ifdef MOZILLA_CLIENT
#include "jscompat.h"
#endif

struct JSContext;













































class DSTOffsetCache {
  public:
    inline DSTOffsetCache();
    JSInt64 getDSTOffsetMilliseconds(int64 localTimeMilliseconds, JSContext *cx);

    inline void purge();

  private:
    JSInt64 computeDSTOffsetMilliseconds(int64 localTimeSeconds);

    JSInt64 offsetMilliseconds;
    JSInt64 rangeStartSeconds, rangeEndSeconds;

    JSInt64 oldOffsetMilliseconds;
    JSInt64 oldRangeStartSeconds, oldRangeEndSeconds;

    static const JSInt64 MAX_UNIX_TIMET = 2145859200; 
    static const JSInt64 MILLISECONDS_PER_SECOND = 1000;
    static const JSInt64 SECONDS_PER_MINUTE = 60;
    static const JSInt64 SECONDS_PER_HOUR = 60 * SECONDS_PER_MINUTE;
    static const JSInt64 SECONDS_PER_DAY = 24 * SECONDS_PER_HOUR;

    static const JSInt64 RANGE_EXPANSION_AMOUNT = 30 * SECONDS_PER_DAY;

  private:
    void sanityCheck();
};

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
    JSInt32 tm_year;            
    JSInt16 tm_yday;            
    JSInt8 tm_isdst;            
};


#define PRMJ_USEC_PER_SEC       1000000L
#define PRMJ_USEC_PER_MSEC      1000L


extern JSInt64
PRMJ_Now(void);


#if defined(JS_THREADSAFE) && (defined(HAVE_GETSYSTEMTIMEASFILETIME) || defined(HAVE_SYSTEMTIMETOFILETIME))
extern void
PRMJ_NowShutdown(void);
#else
#define PRMJ_NowShutdown()
#endif


extern JSInt32
PRMJ_LocalGMTDifference(void);


extern size_t
PRMJ_FormatTime(char *buf, int buflen, const char *fmt, PRMJTime *tm);

JS_END_EXTERN_C

#endif 

