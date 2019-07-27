





#ifndef jsscriptinlines_h
#define jsscriptinlines_h

#include "jsscript.h"

#include "asmjs/AsmJSLink.h"
#include "jit/BaselineJIT.h"
#include "jit/IonAnalysis.h"
#include "vm/ScopeObject.h"

#include "jscompartmentinlines.h"

#include "vm/Shape-inl.h"

namespace js {

inline
Bindings::Bindings()
    : callObjShape_(nullptr), bindingArrayAndFlag_(TEMPORARY_STORAGE_BIT),
      numArgs_(0), numBlockScoped_(0), numVars_(0)
{}

inline
AliasedFormalIter::AliasedFormalIter(JSScript *script)
  : begin_(script->bindingArray()),
    p_(begin_),
    end_(begin_ + (script->funHasAnyAliasedFormal() ? script->numArgs() : 0)),
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

inline JSFunction *
LazyScript::functionDelazifying(JSContext *cx) const
{
    if (function_ && !function_->getOrCreateScript(cx))
        return nullptr;
    return function_;
}

} 

inline JSFunction *
JSScript::functionDelazifying() const
{
    if (function_ && function_->isInterpretedLazy()) {
        function_->setUnlazifiedScript(const_cast<JSScript *>(this));
        
        
        if (lazyScript && !lazyScript->maybeScript())
            lazyScript->initScript(const_cast<JSScript *>(this));
    }
    return function_;
}

inline void
JSScript::setFunction(JSFunction *fun)
{
    JS_ASSERT(fun->isTenured());
    function_ = fun;
}

inline void
JSScript::ensureNonLazyCanonicalFunction(JSContext *cx)
{
    
    if (function_ && function_->isInterpretedLazy())
        functionDelazifying();
}

inline JSFunction *
JSScript::getFunction(size_t index)
{
    JSFunction *fun = &getObject(index)->as<JSFunction>();
    JS_ASSERT_IF(fun->isNative(), IsAsmJSModuleNative(fun->native()));
    return fun;
}

inline JSFunction *
JSScript::getCallerFunction()
{
    JS_ASSERT(savedCallerFun());
    return getFunction(0);
}

inline JSFunction *
JSScript::functionOrCallerFunction()
{
    if (functionNonDelazifying())
        return functionNonDelazifying();
    if (savedCallerFun())
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

inline js::RegExpObject *
JSScript::getRegExp(jsbytecode *pc)
{
    JS_ASSERT(containsPC(pc) && containsPC(pc + sizeof(uint32_t)));
    return getRegExp(GET_UINT32_INDEX(pc));
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
JSScript::donorFunction() const
{
    if (!isCallsiteClone())
        return nullptr;
    return &enclosingScopeOrOriginalFunction_->as<JSFunction>();
}

inline void
JSScript::setIsCallsiteClone(JSObject *fun)
{
    JS_ASSERT(shouldCloneAtCallsite());
    shouldCloneAtCallsite_ = false;
    isCallsiteClone_ = true;
    JS_ASSERT(isCallsiteClone());
    JS_ASSERT(fun->is<JSFunction>());
    enclosingScopeOrOriginalFunction_ = fun;
}

inline void
JSScript::setBaselineScript(JSContext *maybecx, js::jit::BaselineScript *baselineScript)
{
    if (hasBaselineScript())
        js::jit::BaselineScript::writeBarrierPre(zone(), baseline);
    MOZ_ASSERT(!hasIonScript());
    baseline = baselineScript;
    updateBaselineOrIonRaw(maybecx);
}

inline bool
JSScript::ensureHasAnalyzedArgsUsage(JSContext *cx)
{
    if (analyzedArgsUsage())
        return true;
    return js::jit::AnalyzeArgumentsUsage(cx, this);
}

#endif 
