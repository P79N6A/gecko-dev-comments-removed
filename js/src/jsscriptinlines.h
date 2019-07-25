







































#ifndef jsscriptinlines_h___
#define jsscriptinlines_h___

#include "jsautooplen.h"
#include "jscntxt.h"
#include "jsfun.h"
#include "jsopcode.h"
#include "jsscript.h"
#include "jsscope.h"

#include "vm/CallObject.h"
#include "vm/GlobalObject.h"
#include "vm/RegExpObject.h"

#include "jsscopeinlines.h"

namespace js {

inline
Bindings::Bindings(JSContext *cx)
    : lastBinding(NULL), nargs(0), nvars(0), nupvars(0)
{}

inline void
Bindings::transfer(JSContext *cx, Bindings *bindings)
{
    JS_ASSERT(!lastBinding);
    JS_ASSERT(!bindings->lastBinding || !bindings->lastBinding->inDictionary());

    *this = *bindings;
#ifdef DEBUG
    bindings->lastBinding = NULL;
#endif
}

inline void
Bindings::clone(JSContext *cx, Bindings *bindings)
{
    JS_ASSERT(!lastBinding);
    JS_ASSERT(!bindings->lastBinding || !bindings->lastBinding->inDictionary());

    *this = *bindings;
}

Shape *
Bindings::lastShape() const
{
    JS_ASSERT(lastBinding);
    JS_ASSERT(!lastBinding->inDictionary());
    return lastBinding;
}

bool
Bindings::ensureShape(JSContext *cx)
{
    if (!lastBinding) {
        
        gc::AllocKind kind = gc::FINALIZE_OBJECT4;
        JS_ASSERT(gc::GetGCKindSlots(kind) == CallObject::RESERVED_SLOTS + 1);

        lastBinding = EmptyShape::getInitialShape(cx, &CallClass, NULL, NULL, kind,
                                                  BaseShape::VAROBJ);
        if (!lastBinding)
            return false;
    }
    return true;
}

bool
Bindings::extensibleParents()
{
    return lastBinding && lastBinding->extensibleParents();
}

extern void
CurrentScriptFileLineOriginSlow(JSContext *cx, const char **file, uintN *linenop, JSPrincipals **origin);

inline void
CurrentScriptFileLineOrigin(JSContext *cx, const char **file, uintN *linenop, JSPrincipals **origin,
                            LineOption opt = NOT_CALLED_FROM_JSOP_EVAL)
{
    if (opt == CALLED_FROM_JSOP_EVAL) {
        JS_ASSERT(JSOp(*cx->regs().pc) == JSOP_EVAL);
        JS_ASSERT(*(cx->regs().pc + JSOP_EVAL_LENGTH) == JSOP_LINENO);
        JSScript *script = cx->fp()->script();
        *file = script->filename;
        *linenop = GET_UINT16(cx->regs().pc + JSOP_EVAL_LENGTH);
        *origin = script->originPrincipals;
        return;
    }

    CurrentScriptFileLineOriginSlow(cx, file, linenop, origin);
}

} 

inline void
JSScript::setFunction(JSFunction *fun)
{
    function_ = fun;
}

inline JSFunction *
JSScript::getFunction(size_t index)
{
    JSObject *funobj = getObject(index);
    JS_ASSERT(funobj->isFunction() && funobj->toFunction()->isInterpreted());
    return funobj->toFunction();
}

inline JSFunction *
JSScript::getCallerFunction()
{
    JS_ASSERT(savedCallerFun);
    return getFunction(0);
}

inline JSObject *
JSScript::getRegExp(size_t index)
{
    JSObjectArray *arr = regexps();
    JS_ASSERT((uint32) index < arr->length);
    JSObject *obj = arr->vector[index];
    JS_ASSERT(obj->isRegExp());
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
    




    JS_ASSERT(types && types->hasScope());
    js::GlobalObject *obj = types->global;
    return obj && !obj->isCleared();
}

inline js::GlobalObject *
JSScript::global() const
{
    JS_ASSERT(hasGlobal());
    return types->global;
}

inline bool
JSScript::hasClearedGlobal() const
{
    JS_ASSERT(types && types->hasScope());
    js::GlobalObject *obj = types->global;
    return obj && obj->isCleared();
}

inline js::types::TypeScriptNesting *
JSScript::nesting() const
{
    JS_ASSERT(function() && types && types->hasScope());
    return types->nesting;
}

inline void
JSScript::clearNesting()
{
    js::types::TypeScriptNesting *nesting = this->nesting();
    if (nesting) {
        js::Foreground::delete_(nesting);
        types->nesting = NULL;
    }
}

inline void
JSScript::writeBarrierPre(JSScript *script)
{
#ifdef JSGC_INCREMENTAL
    if (!script)
        return;

    JSCompartment *comp = script->compartment();
    if (comp->needsBarrier()) {
        JS_ASSERT(!comp->rt->gcRunning);
        MarkScriptUnbarriered(comp->barrierTracer(), script, "write barrier");
    }
#endif
}

inline void
JSScript::writeBarrierPost(JSScript *script, void *addr)
{
}

#endif 
