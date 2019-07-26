





#ifndef builtin_Object_h
#define builtin_Object_h

#include "jsapi.h"

namespace JS { class Value; }

namespace js {

extern const JSFunctionSpec object_methods[];
extern const JSPropertySpec object_properties[];
extern const JSFunctionSpec object_static_methods[];


bool
obj_construct(JSContext *cx, unsigned argc, JS::Value *vp);

#if JS_HAS_TOSOURCE

JSString *
ObjectToSource(JSContext *cx, JS::HandleObject obj);
#endif 

extern bool
WatchHandler(JSContext *cx, JSObject *obj, jsid id, JS::Value old,
             JS::Value *nvp, void *closure);

} 

#endif 
