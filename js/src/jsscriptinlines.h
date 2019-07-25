







































#ifndef jsscriptinlines_h___
#define jsscriptinlines_h___

#include "jsautooplen.h"
#include "jscntxt.h"
#include "jsfun.h"
#include "jsopcode.h"
#include "jsregexp.h"
#include "jsscript.h"
#include "jsscope.h"
#include "vm/GlobalObject.h"

#include "jsscopeinlines.h"

namespace js {

inline
Bindings::Bindings(JSContext *cx)
  : lastBinding(NULL), nargs(0), nvars(0), nupvars(0),
    hasExtensibleParents(false)
{
}

inline void
Bindings::transfer(JSContext *cx, Bindings *bindings)
{
    JS_ASSERT(!lastBinding);

    *this = *bindings;
#ifdef DEBUG
    bindings->lastBinding = NULL;
#endif

    
    if (lastBinding && lastBinding->inDictionary())
        lastBinding->listp = &this->lastBinding;
}

inline void
Bindings::clone(JSContext *cx, Bindings *bindings)
{
    JS_ASSERT(!lastBinding);

    



    JS_ASSERT(!bindings->lastBinding ||
              !bindings->lastBinding->inDictionary() ||
              bindings->lastBinding->frozen());

    *this = *bindings;
}

Shape *
Bindings::lastShape() const
{
    JS_ASSERT(lastBinding);
    JS_ASSERT_IF(lastBinding->inDictionary(), lastBinding->frozen());
    return lastBinding;
}

bool
Bindings::ensureShape(JSContext *cx)
{
    if (!lastBinding) {
        lastBinding = EmptyShape::getEmptyCallShape(cx);
        if (!lastBinding)
            return false;
    }
    return true;
}

extern const char *
CurrentScriptFileAndLineSlow(JSContext *cx, uintN *linenop);

inline const char *
CurrentScriptFileAndLine(JSContext *cx, uintN *linenop, LineOption opt)
{
    if (opt == CALLED_FROM_JSOP_EVAL) {
        JS_ASSERT(js_GetOpcode(cx, cx->fp()->script(), cx->regs().pc) == JSOP_EVAL);
        JS_ASSERT(*(cx->regs().pc + JSOP_EVAL_LENGTH) == JSOP_LINENO);
        *linenop = GET_UINT16(cx->regs().pc + JSOP_EVAL_LENGTH);
        return cx->fp()->script()->filename;
    }

    return CurrentScriptFileAndLineSlow(cx, linenop);
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

inline bool
JSScript::hasGlobal() const
{
    




    if (!compileAndGo)
        return false;
    js::GlobalObject *obj = hasFunction ? function()->getGlobal() : where.global;
    return obj && !obj->isCleared();
}

inline js::GlobalObject *
JSScript::global() const
{
    JS_ASSERT(hasGlobal());
    return hasFunction ? function()->getGlobal() : where.global;
}

inline bool
JSScript::hasClearedGlobal() const
{
    if (!compileAndGo)
        return false;
    js::GlobalObject *obj = hasFunction ? function()->getGlobal() : where.global;
    return obj && obj->isCleared();
}

#endif 
