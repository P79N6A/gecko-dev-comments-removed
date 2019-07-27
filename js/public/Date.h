




#ifndef js_Date_h
#define js_Date_h

#include "mozilla/FloatingPoint.h"
#include "mozilla/MathAlgorithms.h"

#include "jstypes.h"

#include "js/Conversions.h"
#include "js/Value.h"

namespace JS {

class ClippedTime
{
    double t;

    
    double timeClip(double time) {
        
        const double MaxTimeMagnitude = 8.64e15;
        if (!mozilla::IsFinite(time) || mozilla::Abs(time) > MaxTimeMagnitude)
            return JS::GenericNaN();

        
        return JS::ToInteger(time) + (+0.0);
    }

  public:
    ClippedTime() : t(JS::GenericNaN()) {}
    explicit ClippedTime(double time) : t(timeClip(time)) {}

    static ClippedTime NaN() { return ClippedTime(); }

    double value() const { return t; }
};

inline ClippedTime
TimeClip(double d)
{
    return ClippedTime(d);
}



JS_PUBLIC_API(double)
MakeDate(double year, unsigned month, unsigned day);



JS_PUBLIC_API(double)
YearFromTime(double time);



JS_PUBLIC_API(double)
MonthFromTime(double time);



JS_PUBLIC_API(double)
DayFromTime(double time);

} 

#endif 
