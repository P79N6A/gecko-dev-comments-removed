





#ifndef builtin_Object_h
#define builtin_Object_h

#include "jsapi.h"
#include "js/Value.h"

namespace js {

extern const JSFunctionSpec object_methods[];
extern const JSFunctionSpec object_static_methods[];


bool
obj_construct(JSContext *cx, unsigned argc, JS::Value *vp);

#if JS_HAS_TOSOURCE

JSString *
ObjectToSource(JSContext *cx, JS::HandleObject obj);
#endif 

} 

#endif 
