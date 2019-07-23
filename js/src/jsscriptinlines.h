







































#ifndef jsscriptinlines_h___
#define jsscriptinlines_h___

#include "jsfun.h"
#include "jsopcode.h"
#include "jsregexp.h"
#include "jsscript.h"

inline JSFunction *
JSScript::getFunction(size_t index)
{
    JSObject *funobj = getObject(index);
    JS_ASSERT(HAS_FUNCTION_CLASS(funobj));
    JS_ASSERT(funobj == (JSObject *) funobj->getPrivate());
    JSFunction *fun = (JSFunction *) funobj;
    JS_ASSERT(FUN_INTERPRETED(fun));
    return fun;
}

inline JSObject *
JSScript::getRegExp(size_t index)
{
    JSObjectArray *arr = regexps();
    JS_ASSERT((uint32) index < arr->length);
    JSObject *obj = arr->vector[index];
    JS_ASSERT(STOBJ_GET_CLASS(obj) == &js_RegExpClass);
    return obj;
}

inline bool
JSScript::isEmpty() const
{
    if (this == emptyScript())
        return true;

    if (length <= 3) {
        jsbytecode *pc = code;

        if (JSOp(*pc) == JSOP_TRACE)
            ++pc;
        if (noScriptRval && JSOp(*pc) == JSOP_FALSE)
            ++pc;
        if (JSOp(*pc) == JSOP_STOP)
            return true;
    }
    return false;
}

#endif 
