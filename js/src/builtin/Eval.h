





#ifndef builtin_Eval_h
#define builtin_Eval_h

#include "jsbytecode.h"
#include "NamespaceImports.h"

namespace js {






extern bool
IndirectEval(JSContext* cx, unsigned argc, Value* vp);




extern bool
DirectEval(JSContext* cx, const CallArgs& args);


extern bool
DirectEvalStringFromIon(JSContext* cx,
                        HandleObject scopeObj, HandleScript callerScript,
                        HandleValue thisValue, HandleValue newTargetValue,
                        HandleString str, jsbytecode * pc, MutableHandleValue vp);


extern bool
IsAnyBuiltinEval(JSFunction* fun);

} 

#endif 
