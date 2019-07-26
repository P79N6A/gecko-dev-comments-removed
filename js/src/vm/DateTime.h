





#ifndef DateTime_h___
#define DateTime_h___

#include "mozilla/FloatingPoint.h"
#include "mozilla/MathAlgorithms.h"
#include "mozilla/StandardInteger.h"

#include <math.h>

#include "NumericConversions.h"

namespace js {


const double HoursPerDay = 24;
const double MinutesPerHour = 60;
const double SecondsPerMinute = 60;
const double msPerSecond = 1000;
const double msPerMinute = msPerSecond * SecondsPerMinute;
const double msPerHour = msPerMinute * MinutesPerHour;


const double msPerDay = msPerHour * HoursPerDay;







const unsigned SecondsPerHour = 60 * 60;
const unsigned SecondsPerDay = SecondsPerHour * 24;


inline double
TimeClip(double time)
{
    
    if (!MOZ_DOUBLE_IS_FINITE(time) || mozilla::Abs(time) > 8.64e15)
        return js_NaN;

    
    return ToInteger(time + (+0.0));
}















































class DateTimeInfo
{
  public:
    DateTimeInfo();

    





    int64_t getDSTOffsetMilliseconds(int64_t utcMilliseconds);

    void updateTimeZoneAdjustment();

    
    double localTZA() { return localTZA_; }

  private:
    









    double localTZA_;

    





    int64_t computeDSTOffsetMilliseconds(int64_t utcSeconds);

    int64_t offsetMilliseconds;
    int64_t rangeStartSeconds, rangeEndSeconds; 

    int64_t oldOffsetMilliseconds;
    int64_t oldRangeStartSeconds, oldRangeEndSeconds; 

    



    int32_t utcToLocalStandardOffsetSeconds;

    static const int64_t MaxUnixTimeT = 2145859200; 

    static const int64_t RangeExpansionAmount = 30 * SecondsPerDay;

    void sanityCheck();
};

}  

#endif 
