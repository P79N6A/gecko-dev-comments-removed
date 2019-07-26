





#ifndef jsbool_h
#define jsbool_h





#include "NamespaceImports.h"

extern JSObject *
js_InitBooleanClass(JSContext *cx, js::HandleObject obj);

extern JSString *
js_BooleanToString(js::ExclusiveContext *cx, bool b);

namespace js {

inline bool
BooleanGetPrimitiveValue(HandleObject obj, JSContext *cx);

} 

#endif 
