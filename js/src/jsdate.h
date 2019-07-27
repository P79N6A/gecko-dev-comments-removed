









#ifndef jsdate_h
#define jsdate_h

#include "jstypes.h"

#include "js/Date.h"
#include "js/RootingAPI.h"
#include "js/TypeDecls.h"

#include "vm/DateTime.h"

namespace js {









extern JSObject*
NewDateObjectMsec(JSContext* cx, JS::ClippedTime t);








extern JS_FRIEND_API(JSObject*)
NewDateObject(JSContext* cx, int year, int mon, int mday,
              int hour, int min, int sec);


bool
DateConstructor(JSContext* cx, unsigned argc, JS::Value* vp);


bool
date_now(JSContext* cx, unsigned argc, JS::Value* vp);

bool
date_valueOf(JSContext* cx, unsigned argc, JS::Value* vp);

} 

#endif 
