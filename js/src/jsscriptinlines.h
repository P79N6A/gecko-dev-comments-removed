







































#ifndef jsscriptinlines_h___
#define jsscriptinlines_h___

#include "jsfun.h"
#include "jsregexp.h"

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

#endif 
