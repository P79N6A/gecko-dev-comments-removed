







































#ifndef jsscriptinlines_h___
#define jsscriptinlines_h___

#include "jscntxt.h"
#include "jsfun.h"
#include "jsopcode.h"
#include "jsregexp.h"
#include "jsscript.h"
#include "jsscope.h"

namespace js {

inline
Bindings::Bindings(JSContext *cx)
  : lastBinding(cx->runtime->emptyCallShape), nargs(0), nvars(0), nupvars(0)
{
}

inline void
Bindings::transfer(JSContext *cx, Bindings *bindings)
{
    JS_ASSERT(lastBinding == cx->runtime->emptyCallShape);

    *this = *bindings;
#ifdef DEBUG
    bindings->lastBinding = NULL;
#endif

    
    if (lastBinding->inDictionary())
        lastBinding->listp = &this->lastBinding;
}

inline void
Bindings::clone(JSContext *cx, Bindings *bindings)
{
    JS_ASSERT(lastBinding == cx->runtime->emptyCallShape);

    



    JS_ASSERT(!bindings->lastBinding->inDictionary() || bindings->lastBinding->frozen());

    *this = *bindings;
}

const Shape *
Bindings::lastShape() const
{
    JS_ASSERT(lastBinding);
    JS_ASSERT_IF(lastBinding->inDictionary(), lastBinding->frozen());
    return lastBinding;
}

} 

inline JSFunction *
JSScript::getFunction(size_t index)
{
    JSObject *funobj = getObject(index);
    JS_ASSERT(funobj->isFunction());
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
    JS_ASSERT(obj->getClass() == &js_RegExpClass);
    return obj;
}

inline bool
JSScript::isEmpty() const
{
    if (length > 3)
        return false;

    jsbytecode *pc = code;
    if (noScriptRval && JSOp(*pc) == JSOP_FALSE)
        ++pc;
    return JSOp(*pc) == JSOP_STOP;
}

#endif 
