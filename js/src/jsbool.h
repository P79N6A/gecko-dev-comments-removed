






































#ifndef jsbool_h___
#define jsbool_h___




#include "jsapi.h"
#include "jsobj.h"

extern js::Class js_BooleanClass;

inline bool
JSObject::isBoolean() const
{
    return getClass() == &js_BooleanClass;
}

extern JSObject *
js_InitBooleanClass(JSContext *cx, JSObject *obj);

extern JSString *
js_BooleanToString(JSContext *cx, JSBool b);

extern JSBool
js_BooleanToCharBuffer(JSContext *cx, JSBool b, JSCharBuffer &cb);

extern JSBool
js_ValueToBoolean(const js::Value &v);

#endif 
