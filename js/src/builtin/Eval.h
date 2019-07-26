





#ifndef builtin_Eval_h
#define builtin_Eval_h

#include "jsbytecode.h"
#include "NamespaceImports.h"

namespace js {






extern bool
IndirectEval(JSContext *cx, unsigned argc, Value *vp);




extern bool
DirectEval(JSContext *cx, const CallArgs &args);


extern bool
DirectEvalFromIon(JSContext *cx,
                  HandleObject scopeObj, HandleScript callerScript,
                  HandleValue thisValue, HandleString str,
                  jsbytecode * pc, MutableHandleValue vp);



extern bool
IsBuiltinEvalForScope(JSObject *scopeChain, const Value &v);


extern bool
IsAnyBuiltinEval(JSFunction *fun);



extern JSPrincipals *
PrincipalsForCompiledCode(const CallReceiver &call, JSContext *cx);

}  
#endif 
