






































#ifndef jsfuninlines_h___
#define jsfuninlines_h___

#include "jsfun.h"
#include "jsscript.h"

inline bool
JSFunction::inStrictMode() const
{
    return script()->strictModeCode;
}

namespace js {

static inline bool
IsSafeForLazyThisCoercion(JSContext *cx, JSObject *callee)
{
    











    if (callee->isProxy()) {
        callee = callee->unwrap();
        if (!callee->isFunction())
            return true; 

        JSFunction *fun = callee->getFunctionPrivate();
        if (fun->isInterpreted() && fun->inStrictMode())
            return true;
    }
    return callee->getGlobal() == cx->fp()->scopeChain().getGlobal();
}

}

#endif 
