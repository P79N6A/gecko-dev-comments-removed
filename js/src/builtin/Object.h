





#ifndef builtin_Object_h
#define builtin_Object_h

#include "jsobj.h"

namespace js {

extern const JSFunctionSpec object_methods[];
extern const JSFunctionSpec object_static_methods[];


bool
obj_construct(JSContext *cx, unsigned argc, js::Value *vp);

#if JS_HAS_TOSOURCE

JSString *
ObjectToSource(JSContext *cx, HandleObject obj);
#endif 

} 

#endif 
