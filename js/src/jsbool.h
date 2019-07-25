






































#ifndef jsbool_h___
#define jsbool_h___




#include "jsapi.h"
#include "jsobj.h"

extern JSObject *
js_InitBooleanClass(JSContext *cx, JSObject *obj);

extern JSString *
js_BooleanToString(JSContext *cx, JSBool b);

namespace js {

extern bool
BooleanToStringBuffer(JSContext *cx, JSBool b, StringBuffer &sb);

} 

extern JSBool
js_ValueToBoolean(const js::Value &v);

#endif 
