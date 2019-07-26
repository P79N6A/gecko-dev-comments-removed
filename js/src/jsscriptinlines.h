





#ifndef jsscriptinlines_h
#define jsscriptinlines_h

#include "jsscript.h"

#include "jit/AsmJSLink.h"
#include "jit/BaselineJIT.h"
#include "vm/ScopeObject.h"

#include "jscompartmentinlines.h"

#include "vm/Shape-inl.h"

namespace js {

inline
Bindings::Bindings()
    : callObjShape_(nullptr), bindingArrayAndFlag_(TEMPORARY_STORAGE_BIT), numArgs_(0), numVars_(0)
{}

inline
AliasedFormalIter::AliasedFormalIter(JSScript *script)
  : begin_(script->bindings.bindingArray()),
    p_(begin_),
    end_(begin_ + (script->funHasAnyAliasedFormal ? script->bindings.numArgs() : 0)),
    slot_(CallObject::RESERVED_SLOTS)
{
    settle();
}

inline void
ScriptCounts::destroy(FreeOp *fop)
{
    fop->free_(pcCountsVector);
    fop->delete_(ionCounts);
}

void
SetFrameArgumentsObject(JSContext *cx, AbstractFramePtr frame,
                        HandleScript script, JSObject *argsobj);

} 

inline void
JSScript::setFunction(JSFunction *fun)
{
    JS_ASSERT(fun->isTenured());
    function_ = fun;
}

inline JSFunction *
JSScript::getFunction(size_t index)
{
    JSFunction *fun = &getObject(index)->as<JSFunction>();
#ifdef DEBUG
    JS_ASSERT_IF(fun->isNative(), IsAsmJSModuleNative(fun->native()));
#endif
    return fun;
}

inline JSFunction *
JSScript::getCallerFunction()
{
    JS_ASSERT(savedCallerFun);
    return getFunction(0);
}

inline JSFunction *
JSScript::functionOrCallerFunction()
{
    if (function())
        return function();
    if (savedCallerFun)
        return getCallerFunction();
    return nullptr;
}

inline js::RegExpObject *
JSScript::getRegExp(size_t index)
{
    js::ObjectArray *arr = regexps();
    JS_ASSERT(uint32_t(index) < arr->length);
    JSObject *obj = arr->vector[index];
    JS_ASSERT(obj->is<js::RegExpObject>());
    return (js::RegExpObject *) obj;
}

inline js::GlobalObject &
JSScript::global() const
{
    



    return *compartment()->maybeGlobal();
}

inline JSPrincipals *
JSScript::principals()
{
    return compartment()->principals;
}

inline JSFunction *
JSScript::originalFunction() const {
    if (!isCallsiteClone)
        return nullptr;
    return &enclosingScopeOrOriginalFunction_->as<JSFunction>();
}

inline void
JSScript::setOriginalFunctionObject(JSObject *fun) {
    JS_ASSERT(isCallsiteClone);
    JS_ASSERT(fun->is<JSFunction>());
    enclosingScopeOrOriginalFunction_ = fun;
}

inline void
JSScript::setBaselineScript(js::jit::BaselineScript *baselineScript) {
#ifdef JS_ION
    if (hasBaselineScript())
        js::jit::BaselineScript::writeBarrierPre(tenuredZone(), baseline);
#endif
    baseline = baselineScript;
    updateBaselineOrIonRaw();
}

#endif 
