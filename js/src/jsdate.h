









#ifndef jsdate_h___
#define jsdate_h___

#include "mozilla/FloatingPoint.h"

#include <math.h>

#include "jstypes.h"

extern "C" {
class JSObject;
struct JSContext;
}

extern JSObject *
js_InitDateClass(JSContext *cx, js::HandleObject obj);









extern JS_FRIEND_API(JSObject *)
js_NewDateObjectMsec(JSContext* cx, double msec_time);








extern JS_FRIEND_API(JSObject *)
js_NewDateObject(JSContext* cx, int year, int mon, int mday,
                 int hour, int min, int sec);

extern JS_FRIEND_API(int)
js_DateGetYear(JSContext *cx, JSRawObject obj);

extern JS_FRIEND_API(int)
js_DateGetMonth(JSContext *cx, JSRawObject obj);

extern JS_FRIEND_API(int)
js_DateGetDate(JSContext *cx, JSRawObject obj);

extern JS_FRIEND_API(int)
js_DateGetHours(JSContext *cx, JSRawObject obj);

extern JS_FRIEND_API(int)
js_DateGetMinutes(JSContext *cx, JSRawObject obj);

extern JS_FRIEND_API(int)
js_DateGetSeconds(JSRawObject obj);


JSBool
js_Date(JSContext *cx, unsigned argc, js::Value *vp);

#endif 
