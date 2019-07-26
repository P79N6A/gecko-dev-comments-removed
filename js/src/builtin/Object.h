






#ifndef Object_h___
#define Object_h___

#include "jsobj.h"

namespace js {

extern JSFunctionSpec object_methods[];
extern JSFunctionSpec object_static_methods[];


extern JSBool
obj_construct(JSContext *cx, unsigned argc, js::Value *vp);

extern JSString *
obj_toStringHelper(JSContext *cx, HandleObject obj);

} 

#endif
