





#ifndef builtin_Reflect_h
#define builtin_Reflect_h

#include "jsobj.h"

namespace js {

extern JSObject*
InitReflect(JSContext* cx, js::HandleObject obj);

}

namespace js {

extern bool
Reflect_getPrototypeOf(JSContext* cx, unsigned argc, Value* vp);

extern bool
Reflect_isExtensible(JSContext* cx, unsigned argc, Value* vp);

}

#endif 
