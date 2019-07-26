





#ifndef Object_h___
#define Object_h___

#include "jsobj.h"

namespace js {

extern const JSFunctionSpec object_methods[];
extern const JSFunctionSpec object_static_methods[];


extern JSBool
obj_construct(JSContext *cx, unsigned argc, js::Value *vp);

} 

#endif
