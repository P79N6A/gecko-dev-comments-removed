




#ifndef js_Date_h___
#define js_Date_h___

#include "jstypes.h"

namespace JS {



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
