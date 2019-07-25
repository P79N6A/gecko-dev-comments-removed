










































#ifndef jsdate_h___
#define jsdate_h___

#include "mozilla/FloatingPoint.h"

#include "jscntxt.h"

#define HalfTimeDomain  8.64e15

#define TIMECLIP(d) ((MOZ_DOUBLE_IS_FINITE(d) \
                      && !((d < 0 ? -d : d) > HalfTimeDomain)) \
                     ? js_DoubleToInteger(d + (+0.)) : js_NaN)

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
