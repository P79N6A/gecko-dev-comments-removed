





#ifndef jsbool_h
#define jsbool_h




#include "jsapi.h"

extern JSObject *
js_InitBooleanClass(JSContext *cx, js::HandleObject obj);

extern JSString *
js_BooleanToString(JSContext *cx, JSBool b);

namespace js {

inline bool
BooleanGetPrimitiveValue(HandleObject obj, JSContext *cx);

} 

#endif 
