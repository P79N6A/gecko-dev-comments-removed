









#ifndef jsdate_h
#define jsdate_h

#include "jstypes.h"

#include "js/RootingAPI.h"
#include "js/TypeDecls.h"

namespace js {









extern JS_FRIEND_API(JSObject *)
NewDateObjectMsec(JSContext *cx, double msec_time);








extern JS_FRIEND_API(JSObject *)
NewDateObject(JSContext *cx, int year, int mon, int mday,
              int hour, int min, int sec);


bool
DateConstructor(JSContext *cx, unsigned argc, JS::Value *vp);


bool
date_now(JSContext *cx, unsigned argc, JS::Value *vp);

bool
date_valueOf(JSContext *cx, unsigned argc, JS::Value *vp);

} 

#endif 
