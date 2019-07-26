





#ifndef TestingFunctions_h__
#define TestingFunctions_h__

#include "jsapi.h"

namespace js {

bool
DefineTestingFunctions(JSContext *cx, JSHandleObject obj);

JSBool
testingFunc_inParallelSection(JSContext *cx, unsigned argc, jsval *vp);

} 

#endif 
