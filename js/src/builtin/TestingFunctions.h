





#ifndef builtin_TestingFunctions_h
#define builtin_TestingFunctions_h

#include "jsapi.h"

namespace js {

bool
DefineTestingFunctions(JSContext *cx, JSHandleObject obj);

JSBool
testingFunc_inParallelSection(JSContext *cx, unsigned argc, jsval *vp);

} 

#endif 
