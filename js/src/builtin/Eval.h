






#include "vm/Stack.h"

#ifndef Eval_h__
#define Eval_h__

namespace js {






extern JSBool
IndirectEval(JSContext *cx, unsigned argc, Value *vp);




extern bool
DirectEval(JSContext *cx, const CallArgs &args);


extern bool
DirectEvalFromIon(JSContext *cx,
                  HandleObject scopeObj, HandleScript callerScript,
                  HandleValue thisValue, HandleString str,
                  MutableHandleValue vp);



extern bool
IsBuiltinEvalForScope(JSObject *scopeChain, const Value &v);


extern bool
IsAnyBuiltinEval(JSFunction *fun);



extern JSPrincipals *
PrincipalsForCompiledCode(const CallReceiver &call, JSContext *cx);

}  
#endif  
