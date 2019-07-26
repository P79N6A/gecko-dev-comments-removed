









#ifndef jsdate_h___
#define jsdate_h___

#include "mozilla/FloatingPoint.h"

#include <math.h>

#include "jstypes.h"

#include "vm/NumericConversions.h"

extern "C" {
struct JSObject;
struct JSContext;
}

namespace js {


inline double
TimeClip(double time)
{
    
    if (!MOZ_DOUBLE_IS_FINITE(time) || fabs(time) > 8.64e15)
        return js_NaN;

    
    return ToInteger(time + (+0.0));
}

} 

extern JSObject *
js_InitDateClass(JSContext *cx, JSObject *obj);









extern JS_FRIEND_API(JSObject*)
js_NewDateObjectMsec(JSContext* cx, double msec_time);








extern JS_FRIEND_API(JSObject*)
js_NewDateObject(JSContext* cx, int year, int mon, int mday,
                 int hour, int min, int sec);

extern JS_FRIEND_API(int)
js_DateGetYear(JSContext *cx, JSObject* obj);

extern JS_FRIEND_API(int)
js_DateGetMonth(JSContext *cx, JSObject* obj);

extern JS_FRIEND_API(int)
js_DateGetDate(JSContext *cx, JSObject* obj);

extern JS_FRIEND_API(int)
js_DateGetHours(JSContext *cx, JSObject* obj);

extern JS_FRIEND_API(int)
js_DateGetMinutes(JSContext *cx, JSObject* obj);

extern JS_FRIEND_API(int)
js_DateGetSeconds(JSContext *cx, JSObject* obj);

typedef uint32_t JSIntervalTime;

extern JS_FRIEND_API(JSIntervalTime)
js_IntervalNow();


JSBool
js_Date(JSContext *cx, unsigned argc, js::Value *vp);

#endif 
