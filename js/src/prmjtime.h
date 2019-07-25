






































#ifndef prmjtime_h___
#define prmjtime_h___

#include <time.h>

struct JSContext;













































class DSTOffsetCache {
  public:
    inline DSTOffsetCache();
    int64_t getDSTOffsetMilliseconds(int64_t localTimeMilliseconds, JSContext *cx);

    inline void purge();

  private:
    int64_t computeDSTOffsetMilliseconds(int64_t localTimeSeconds);

    int64_t offsetMilliseconds;
    int64_t rangeStartSeconds, rangeEndSeconds;

    int64_t oldOffsetMilliseconds;
    int64_t oldRangeStartSeconds, oldRangeEndSeconds;

    static const int64_t MAX_UNIX_TIMET = 2145859200; 
    static const int64_t MILLISECONDS_PER_SECOND = 1000;
    static const int64_t SECONDS_PER_MINUTE = 60;
    static const int64_t SECONDS_PER_HOUR = 60 * SECONDS_PER_MINUTE;
    static const int64_t SECONDS_PER_DAY = 24 * SECONDS_PER_HOUR;

    static const int64_t RANGE_EXPANSION_AMOUNT = 30 * SECONDS_PER_DAY;

  private:
    void sanityCheck();
};

JS_BEGIN_EXTERN_C

typedef struct PRMJTime       PRMJTime;




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


#if defined(JS_THREADSAFE) && (defined(HAVE_GETSYSTEMTIMEASFILETIME) || defined(HAVE_SYSTEMTIMETOFILETIME))
extern void
PRMJ_NowShutdown(void);
#else
#define PRMJ_NowShutdown()
#endif


extern int32_t
PRMJ_LocalGMTDifference(void);


extern size_t
PRMJ_FormatTime(char *buf, int buflen, const char *fmt, PRMJTime *tm);

JS_END_EXTERN_C

#endif 

