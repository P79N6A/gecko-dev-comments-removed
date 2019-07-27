





#ifndef jsbool_h
#define jsbool_h





#include "NamespaceImports.h"

namespace js {

extern JSObject *
InitBooleanClass(JSContext *cx, js::HandleObject obj);

extern JSString *
BooleanToString(js::ExclusiveContext *cx, bool b);

}

#endif 
