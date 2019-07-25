








#ifndef SkTime_DEFINED
#define SkTime_DEFINED

#include "SkTypes.h"




class SkTime {
public:
    struct DateTime {
        uint16_t fYear;          
        uint8_t  fMonth;         
        uint8_t  fDayOfWeek;     
        uint8_t  fDay;           
        uint8_t  fHour;          
        uint8_t  fMinute;        
        uint8_t  fSecond;        
    };
    static void GetDateTime(DateTime*);

    static SkMSec GetMSecs();
};

#if defined(SK_DEBUG) && defined(SK_BUILD_FOR_WIN32)
    extern SkMSec gForceTickCount;
#endif

#define SK_TIME_FACTOR      1



class SkAutoTime {
public:
    
    
    SkAutoTime(const char* label = NULL, SkMSec minToDump = 0) : fLabel(label)
    {
        fNow = SkTime::GetMSecs();
        fMinToDump = minToDump;
    }
    ~SkAutoTime()
    {
        SkMSec dur = SkTime::GetMSecs() - fNow;
        if (dur >= fMinToDump) {
            SkDebugf("%s %d\n", fLabel ? fLabel : "", dur);
        }
    }
private:
    const char* fLabel;
    SkMSec      fNow;
    SkMSec      fMinToDump;
};

#endif

