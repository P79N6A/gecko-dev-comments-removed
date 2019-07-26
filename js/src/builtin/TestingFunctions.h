





#ifndef builtin_TestingFunctions_h
#define builtin_TestingFunctions_h

#include "NamespaceImports.h"

namespace js {

bool
DefineTestingFunctions(JSContext *cx, HandleObject obj);

bool
testingFunc_inParallelSection(JSContext *cx, unsigned argc, Value *vp);

bool
testingFunc_bailout(JSContext *cx, unsigned argc, Value *vp);

} 

#endif 
