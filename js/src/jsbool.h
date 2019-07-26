





#ifndef jsbool_h___
#define jsbool_h___




#include "jsapi.h"
#include "jsobj.h"

extern JSObject *
js_InitBooleanClass(JSContext *cx, js::HandleObject obj);

extern JSString *
js_BooleanToString(JSContext *cx, JSBool b);

namespace js {

inline bool
BooleanGetPrimitiveValue(JSContext *cx, HandleObject obj, Value *vp);

} 

#endif 
