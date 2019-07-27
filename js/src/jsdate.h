









#ifndef jsdate_h
#define jsdate_h

#include "jstypes.h"

#include "js/RootingAPI.h"
#include "js/TypeDecls.h"









extern JS_FRIEND_API(JSObject *)
js_NewDateObjectMsec(JSContext* cx, double msec_time);








extern JS_FRIEND_API(JSObject *)
js_NewDateObject(JSContext* cx, int year, int mon, int mday,
                 int hour, int min, int sec);


bool
js_Date(JSContext *cx, unsigned argc, JS::Value *vp);

namespace js {


bool
date_now(JSContext *cx, unsigned argc, JS::Value *vp);

bool
date_valueOf(JSContext *cx, unsigned argc, JS::Value *vp);

} 

#endif 
