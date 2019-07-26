









#ifndef jsdate_h
#define jsdate_h

#include "jstypes.h"

#include "js/RootingAPI.h"

extern "C" {
class JSObject;
struct JSContext;
}

namespace JS {
class Value;
}

extern JSObject *
js_InitDateClass(JSContext *cx, JS::HandleObject obj);









extern JS_FRIEND_API(JSObject *)
js_NewDateObjectMsec(JSContext* cx, double msec_time);








extern JS_FRIEND_API(JSObject *)
js_NewDateObject(JSContext* cx, int year, int mon, int mday,
                 int hour, int min, int sec);

extern JS_FRIEND_API(int)
js_DateGetYear(JSContext *cx, JSObject *obj);

extern JS_FRIEND_API(int)
js_DateGetMonth(JSContext *cx, JSObject *obj);

extern JS_FRIEND_API(int)
js_DateGetDate(JSContext *cx, JSObject *obj);

extern JS_FRIEND_API(int)
js_DateGetHours(JSContext *cx, JSObject *obj);

extern JS_FRIEND_API(int)
js_DateGetMinutes(JSContext *cx, JSObject *obj);

extern JS_FRIEND_API(int)
js_DateGetSeconds(JSObject *obj);


bool
js_Date(JSContext *cx, unsigned argc, JS::Value *vp);

#endif 
