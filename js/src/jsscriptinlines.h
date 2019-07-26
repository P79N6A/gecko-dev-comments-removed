






#ifndef jsscriptinlines_h___
#define jsscriptinlines_h___

#include "jsautooplen.h"
#include "jscntxt.h"
#include "jsfun.h"
#include "jsopcode.h"
#include "jsscript.h"

#include "vm/GlobalObject.h"
#include "vm/RegExpObject.h"
#include "vm/Shape.h"

#include "vm/Shape-inl.h"

namespace js {

inline
Bindings::Bindings()
    : callObjShape_(NULL), bindingArrayAndFlag_(TEMPORARY_STORAGE_BIT), numArgs_(0), numVars_(0)
{}

inline
AliasedFormalIter::AliasedFormalIter(js::UnrootedScript script)
  : begin_(script->bindings.bindingArray()),
    p_(begin_),
    end_(begin_ + (script->funHasAnyAliasedFormal ? script->bindings.numArgs() : 0)),
    slot_(CallObject::RESERVED_SLOTS)
{
    settle();
}

extern void
CurrentScriptFileLineOriginSlow(JSContext *cx, const char **file, unsigned *linenop, JSPrincipals **origin);

inline void
CurrentScriptFileLineOrigin(JSContext *cx, const char **file, unsigned *linenop, JSPrincipals **origin,
                            LineOption opt = NOT_CALLED_FROM_JSOP_EVAL)
{
    if (opt == CALLED_FROM_JSOP_EVAL) {
        AutoAssertNoGC nogc;
        JS_ASSERT(JSOp(*cx->regs().pc) == JSOP_EVAL);
        JS_ASSERT(*(cx->regs().pc + JSOP_EVAL_LENGTH) == JSOP_LINENO);
        UnrootedScript script = cx->fp()->script();
        *file = script->filename;
        *linenop = GET_UINT16(cx->regs().pc + JSOP_EVAL_LENGTH);
        *origin = script->originPrincipals;
        return;
    }

    CurrentScriptFileLineOriginSlow(cx, file, linenop, origin);
}

inline void
ScriptCounts::destroy(FreeOp *fop)
{
    fop->free_(pcCountsVector);
    fop->delete_(ionCounts);
}

inline void
MarkScriptFilename(JSRuntime *rt, const char *filename)
{
    




    if (rt->gcIsFull)
        ScriptFilenameEntry::fromFilename(filename)->marked = true;
}

inline void
MarkScriptBytecode(JSRuntime *rt, const jsbytecode *bytecode)
{
    




    if (rt->gcIsFull)
        SharedScriptData::fromBytecode(bytecode)->marked = true;
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

inline js::RegExpObject *
JSScript::getRegExp(size_t index)
{
    js::ObjectArray *arr = regexps();
    JS_ASSERT(uint32_t(index) < arr->length);
    JSObject *obj = arr->vector[index];
    JS_ASSERT(obj->isRegExp());
    return (js::RegExpObject *) obj;
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

inline js::GlobalObject &
JSScript::global() const
{
    



    return *compartment()->maybeGlobal();
}

#ifdef JS_METHODJIT
inline bool
JSScript::ensureHasMJITInfo(JSContext *cx)
{
    if (mJITInfo)
        return true;
    mJITInfo = cx->new_<JITScriptSet>();
    return mJITInfo != NULL;
}

inline void
JSScript::destroyMJITInfo(js::FreeOp *fop)
{
    fop->delete_(mJITInfo);
    mJITInfo = NULL;
}
#endif 

inline void
JSScript::writeBarrierPre(js::UnrootedScript script)
{
#ifdef JSGC_INCREMENTAL
    if (!script)
        return;

    JS::Zone *zone = script->zone();
    if (zone->needsBarrier()) {
        JS_ASSERT(!zone->rt->isHeapBusy());
        js::UnrootedScript tmp = script;
        MarkScriptUnbarriered(zone->barrierTracer(), &tmp, "write barrier");
        JS_ASSERT(tmp == script);
    }
#endif
}

inline void
JSScript::writeBarrierPost(js::UnrootedScript script, void *addr)
{
}

inline JSPrincipals *
JSScript::principals()
{
    return compartment()->principals;
}

#endif 
