





#ifndef builtin_TestingFunctions_h
#define builtin_TestingFunctions_h

#include "NamespaceImports.h"

namespace js {

bool
DefineTestingFunctions(JSContext* cx, HandleObject obj, bool fuzzingSafe);

bool
testingFunc_bailout(JSContext* cx, unsigned argc, Value* vp);

bool
testingFunc_assertFloat32(JSContext* cx, unsigned argc, Value* vp);

bool
testingFunc_assertRecoveredOnBailout(JSContext* cx, unsigned argc, Value* vp);

bool
testingFunc_inJit(JSContext* cx, unsigned argc, Value* vp);

bool
testingFunc_inIon(JSContext* cx, unsigned argc, Value* vp);

} 

#endif 
