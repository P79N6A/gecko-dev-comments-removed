






#ifndef Stack_inl_h__
#define Stack_inl_h__

#include "jscntxt.h"
#include "jscompartment.h"

#include "methodjit/MethodJIT.h"
#include "vm/Stack.h"
#ifdef JS_ION
#include "ion/IonFrameIterator-inl.h"
#endif
#include "jsscriptinlines.h"

#include "ArgumentsObject-inl.h"
#include "ScopeObject-inl.h"


namespace js {






static inline bool
IsCacheableNonGlobalScope(JSObject *obj)
{
    bool cacheable = (obj->isCall() || obj->isBlock() || obj->isDeclEnv());

    JS_ASSERT_IF(cacheable, !obj->getOps()->lookupProperty);
    return cacheable;
}

inline HandleObject
StackFrame::scopeChain() const
{
    JS_ASSERT_IF(!(flags_ & HAS_SCOPECHAIN), isFunctionFrame());
    if (!(flags_ & HAS_SCOPECHAIN)) {
        scopeChain_ = callee().environment();
        flags_ |= HAS_SCOPECHAIN;
    }
    return HandleObject::fromMarkedLocation(&scopeChain_);
}

inline GlobalObject &
StackFrame::global() const
{
    return scopeChain()->global();
}

inline JSObject &
StackFrame::varObj()
{
    JSObject *obj = scopeChain();
    while (!obj->isVarObj())
        obj = obj->enclosingScope();
    return *obj;
}

inline JSCompartment *
StackFrame::compartment() const
{
    JS_ASSERT(scopeChain()->compartment() == script()->compartment());
    return scopeChain()->compartment();
}

#ifdef JS_METHODJIT
inline mjit::JITScript *
StackFrame::jit()
{
    AutoAssertNoGC nogc;
    return script()->getJIT(isConstructing(), script()->compartment()->compileBarriers());
}
#endif

inline void
StackFrame::initPrev(JSContext *cx)
{
    JS_ASSERT(flags_ & HAS_PREVPC);
    if (FrameRegs *regs = cx->maybeRegs()) {
        prev_ = regs->fp();
        prevpc_ = regs->pc;
        prevInline_ = regs->inlined();
        JS_ASSERT(uint32_t(prevpc_ - prev_->script()->code) < prev_->script()->length);
    } else {
        prev_ = NULL;
#ifdef DEBUG
        prevpc_ = (jsbytecode *)0xbadc;
        prevInline_ = (InlinedSite *)0xbadc;
#endif
    }
}

inline void
StackFrame::resetGeneratorPrev(JSContext *cx)
{
    flags_ |= HAS_PREVPC;
    initPrev(cx);
}

inline void
StackFrame::initInlineFrame(JSFunction *fun, StackFrame *prevfp, jsbytecode *prevpc)
{
    



    flags_ = StackFrame::FUNCTION;
    exec.fun = fun;
    resetInlinePrev(prevfp, prevpc);

    if (prevfp->hasPushedSPSFrame())
        setPushedSPSFrame();
}

inline void
StackFrame::resetInlinePrev(StackFrame *prevfp, jsbytecode *prevpc)
{
    JS_ASSERT_IF(flags_ & StackFrame::HAS_PREVPC, prevInline_);
    flags_ |= StackFrame::HAS_PREVPC;
    prev_ = prevfp;
    prevpc_ = prevpc;
    prevInline_ = NULL;
}

inline void
StackFrame::initCallFrame(JSContext *cx, JSFunction &callee,
                          UnrootedScript script, uint32_t nactual, StackFrame::Flags flagsArg)
{
    JS_ASSERT((flagsArg & ~(CONSTRUCTING |
                            LOWERED_CALL_APPLY |
                            OVERFLOW_ARGS |
                            UNDERFLOW_ARGS)) == 0);
    JS_ASSERT(callee.nonLazyScript() == script);

    
    flags_ = FUNCTION | HAS_PREVPC | HAS_SCOPECHAIN | HAS_BLOCKCHAIN | flagsArg;
    exec.fun = &callee;
    u.nactual = nactual;
    scopeChain_ = callee.environment();
    ncode_ = NULL;
    initPrev(cx);
    blockChain_= NULL;
    JS_ASSERT(!hasBlockChain());
    JS_ASSERT(!hasHookData());
    JS_ASSERT(annotation() == NULL);

    initVarsToUndefined();
}





inline void
StackFrame::initFixupFrame(StackFrame *prev, StackFrame::Flags flags, void *ncode, unsigned nactual)
{
    JS_ASSERT((flags & ~(CONSTRUCTING |
                         LOWERED_CALL_APPLY |
                         FUNCTION |
                         OVERFLOW_ARGS |
                         UNDERFLOW_ARGS)) == 0);

    flags_ = FUNCTION | flags;
    prev_ = prev;
    ncode_ = ncode;
    u.nactual = nactual;
}

inline bool
StackFrame::jitHeavyweightFunctionPrologue(JSContext *cx)
{
    JS_ASSERT(isNonEvalFunctionFrame());
    JS_ASSERT(fun()->isHeavyweight());

    CallObject *callobj = CallObject::createForFunction(cx, this);
    if (!callobj)
        return false;

    pushOnScopeChain(*callobj);
    flags_ |= HAS_CALL_OBJ;

    return true;
}

inline void
StackFrame::initVarsToUndefined()
{
    AutoAssertNoGC nogc;
    SetValueRangeToUndefined(slots(), script()->nfixed);
}

inline JSObject *
StackFrame::createRestParameter(JSContext *cx)
{
    JS_ASSERT(fun()->hasRest());
    unsigned nformal = fun()->nargs - 1, nactual = numActualArgs();
    unsigned nrest = (nactual > nformal) ? nactual - nformal : 0;
    return NewDenseCopiedArray(cx, nrest, actuals() + nformal);
}

inline Value &
StackFrame::unaliasedVar(unsigned i, MaybeCheckAliasing checkAliasing)
{
    AutoAssertNoGC nogc;
    JS_ASSERT_IF(checkAliasing, !script()->varIsAliased(i));
    JS_ASSERT(i < script()->nfixed);
    return slots()[i];
}

inline Value &
StackFrame::unaliasedLocal(unsigned i, MaybeCheckAliasing checkAliasing)
{
#ifdef DEBUG
    AutoAssertNoGC nogc;
    if (checkAliasing) {
        JS_ASSERT(i < script()->nslots);
        if (i < script()->nfixed) {
            JS_ASSERT(!script()->varIsAliased(i));
        } else {
            unsigned depth = i - script()->nfixed;
            for (StaticBlockObject *b = maybeBlockChain(); b; b = b->enclosingBlock()) {
                if (b->containsVarAtDepth(depth)) {
                    JS_ASSERT(!b->isAliased(depth - b->stackDepth()));
                    break;
                }
            }
        }
    }
#endif
    return slots()[i];
}

inline Value &
StackFrame::unaliasedFormal(unsigned i, MaybeCheckAliasing checkAliasing)
{
    JS_ASSERT(i < numFormalArgs());
    JS_ASSERT_IF(checkAliasing, !script()->argsObjAliasesFormals());
    JS_ASSERT_IF(checkAliasing, !script()->formalIsAliased(i));
    return formals()[i];
}

inline Value &
StackFrame::unaliasedActual(unsigned i, MaybeCheckAliasing checkAliasing)
{
    JS_ASSERT(i < numActualArgs());
    JS_ASSERT_IF(checkAliasing, !script()->argsObjAliasesFormals());
    JS_ASSERT_IF(checkAliasing && i < numFormalArgs(), !script()->formalIsAliased(i));
    return i < numFormalArgs() ? formals()[i] : actuals()[i];
}

template <class Op>
inline void
StackFrame::forEachUnaliasedActual(Op op)
{
    JS_ASSERT(!script()->funHasAnyAliasedFormal);
    JS_ASSERT(!script()->needsArgsObj());

    unsigned nformal = numFormalArgs();
    unsigned nactual = numActualArgs();

    const Value *formalsEnd = (const Value *)this;
    const Value *formals = formalsEnd - nformal;

    if (nactual <= nformal) {
        const Value *actualsEnd = formals + nactual;
        for (const Value *p = formals; p < actualsEnd; ++p)
            op(*p);
    } else {
        for (const Value *p = formals; p < formalsEnd; ++p)
            op(*p);

        const Value *actualsEnd = formals - 2;
        const Value *actuals = actualsEnd - nactual;
        for (const Value *p = actuals + nformal; p < actualsEnd; ++p)
            op(*p);
    }
}

struct CopyTo
{
    Value *dst;
    CopyTo(Value *dst) : dst(dst) {}
    void operator()(const Value &src) { *dst++ = src; }
};

struct CopyToHeap
{
    HeapValue *dst;
    CopyToHeap(HeapValue *dst) : dst(dst) {}
    void operator()(const Value &src) { dst->init(src); ++dst; }
};

inline unsigned
StackFrame::numFormalArgs() const
{
    JS_ASSERT(hasArgs());
    return fun()->nargs;
}

inline unsigned
StackFrame::numActualArgs() const
{
    







    JS_ASSERT(hasArgs());
    if (JS_UNLIKELY(flags_ & (OVERFLOW_ARGS | UNDERFLOW_ARGS)))
        return u.nactual;
    return numFormalArgs();
}

inline ArgumentsObject &
StackFrame::argsObj() const
{
    JS_ASSERT(script()->needsArgsObj());
    JS_ASSERT(flags_ & HAS_ARGS_OBJ);
    return *argsObj_;
}

inline void
StackFrame::initArgsObj(ArgumentsObject &argsobj)
{
    JS_ASSERT(script()->needsArgsObj());
    flags_ |= HAS_ARGS_OBJ;
    argsObj_ = &argsobj;
}

inline ScopeObject &
StackFrame::aliasedVarScope(ScopeCoordinate sc) const
{
    JSObject *scope = &scopeChain()->asScope();
    for (unsigned i = sc.hops; i; i--)
        scope = &scope->asScope().enclosingScope();
    return scope->asScope();
}

inline void
StackFrame::pushOnScopeChain(ScopeObject &scope)
{
    JS_ASSERT(*scopeChain() == scope.enclosingScope() ||
              *scopeChain() == scope.asCall().enclosingScope().asDeclEnv().enclosingScope());
    scopeChain_ = &scope;
    flags_ |= HAS_SCOPECHAIN;
}

inline void
StackFrame::popOffScopeChain()
{
    JS_ASSERT(flags_ & HAS_SCOPECHAIN);
    scopeChain_ = &scopeChain_->asScope().enclosingScope();
}

inline CallObject &
StackFrame::callObj() const
{
    JS_ASSERT(fun()->isHeavyweight());

    JSObject *pobj = scopeChain();
    while (JS_UNLIKELY(!pobj->isCall()))
        pobj = pobj->enclosingScope();
    return pobj->asCall();
}



STATIC_POSTCONDITION(!return || ubound(from) >= nvals)
JS_ALWAYS_INLINE bool
StackSpace::ensureSpace(JSContext *cx, MaybeReportError report, Value *from, ptrdiff_t nvals) const
{
    mozilla::Maybe<AutoAssertNoGC> maybeNoGC;
    if (report)
        AssertCanGC();
    else
        maybeNoGC.construct();

    assertInvariants();
    JS_ASSERT(from >= firstUnused());
#ifdef XP_WIN
    JS_ASSERT(from <= commitEnd_);
#endif
    if (JS_UNLIKELY(conservativeEnd_ - from < nvals))
        return ensureSpaceSlow(cx, report, from, nvals);
    return true;
}

inline Value *
StackSpace::getStackLimit(JSContext *cx, MaybeReportError report)
{
    AssertCanGC();
    FrameRegs &regs = cx->regs();
    unsigned nvals = regs.fp()->script()->nslots + STACK_JIT_EXTRA;
    return ensureSpace(cx, report, regs.sp, nvals)
           ? conservativeEnd_
           : NULL;
}



JS_ALWAYS_INLINE StackFrame *
ContextStack::getCallFrame(JSContext *cx, MaybeReportError report, const CallArgs &args,
                           JSFunction *fun, HandleScript script, StackFrame::Flags *flags) const
{
    mozilla::Maybe<AutoAssertNoGC> maybeNoGC;
    if (report)
        AssertCanGC();
    else
        maybeNoGC.construct();

    JS_ASSERT(fun->nonLazyScript() == script);
    unsigned nformal = fun->nargs;

    Value *firstUnused = args.end();
    JS_ASSERT(firstUnused == space().firstUnused());

    
    unsigned nvals = VALUES_PER_STACK_FRAME + script->nslots + StackSpace::STACK_JIT_EXTRA;

    

    if (args.length() == nformal) {
        if (!space().ensureSpace(cx, report, firstUnused, nvals))
            return NULL;
        return reinterpret_cast<StackFrame *>(firstUnused);
    }

    if (args.length() < nformal) {
        *flags = StackFrame::Flags(*flags | StackFrame::UNDERFLOW_ARGS);
        unsigned nmissing = nformal - args.length();
        if (!space().ensureSpace(cx, report, firstUnused, nmissing + nvals))
            return NULL;
        SetValueRangeToUndefined(firstUnused, nmissing);
        return reinterpret_cast<StackFrame *>(firstUnused + nmissing);
    }

    *flags = StackFrame::Flags(*flags | StackFrame::OVERFLOW_ARGS);
    unsigned ncopy = 2 + nformal;
    if (!space().ensureSpace(cx, report, firstUnused, ncopy + nvals))
        return NULL;
    Value *dst = firstUnused;
    Value *src = args.base();
    PodCopy(dst, src, ncopy);
    return reinterpret_cast<StackFrame *>(firstUnused + ncopy);
}

JS_ALWAYS_INLINE bool
ContextStack::pushInlineFrame(JSContext *cx, FrameRegs &regs, const CallArgs &args,
                              JSFunction &callee, HandleScript script,
                              InitialFrameFlags initial, MaybeReportError report)
{
    mozilla::Maybe<AutoAssertNoGC> maybeNoGC;
    if (report)
        AssertCanGC();
    else
        maybeNoGC.construct();

    JS_ASSERT(onTop());
    JS_ASSERT(regs.sp == args.end());
    
    JS_ASSERT(callee.nonLazyScript() == script);

    StackFrame::Flags flags = ToFrameFlags(initial);
    StackFrame *fp = getCallFrame(cx, report, args, &callee, script, &flags);
    if (!fp)
        return false;

    
    fp->initCallFrame(cx, callee, script, args.length(), flags);

    



    regs.prepareToRun(*fp, script);
    return true;
}

JS_ALWAYS_INLINE bool
ContextStack::pushInlineFrame(JSContext *cx, FrameRegs &regs, const CallArgs &args,
                              JSFunction &callee, HandleScript script,
                              InitialFrameFlags initial, Value **stackLimit)
{
    AssertCanGC();
    if (!pushInlineFrame(cx, regs, args, callee, script, initial))
        return false;
    *stackLimit = space().conservativeEnd_;
    return true;
}

JS_ALWAYS_INLINE StackFrame *
ContextStack::getFixupFrame(JSContext *cx, MaybeReportError report,
                            const CallArgs &args, JSFunction *fun, HandleScript script,
                            void *ncode, InitialFrameFlags initial, Value **stackLimit)
{
    AssertCanGC();
    JS_ASSERT(onTop());
    JS_ASSERT(fun->nonLazyScript() == args.callee().toFunction()->nonLazyScript());
    JS_ASSERT(fun->nonLazyScript() == script);

    StackFrame::Flags flags = ToFrameFlags(initial);
    StackFrame *fp = getCallFrame(cx, report, args, fun, script, &flags);
    if (!fp)
        return NULL;

    
    fp->initFixupFrame(cx->fp(), flags, ncode, args.length());

    *stackLimit = space().conservativeEnd_;
    return fp;
}

JS_ALWAYS_INLINE void
ContextStack::popInlineFrame(FrameRegs &regs)
{
    JS_ASSERT(onTop());
    JS_ASSERT(&regs == &seg_->regs());

    StackFrame *fp = regs.fp();
    Value *newsp = fp->actuals() - 1;
    JS_ASSERT(newsp >= fp->prev()->base());

    newsp[-1] = fp->returnValue();
    regs.popFrame(newsp);
}

inline void
ContextStack::popFrameAfterOverflow()
{
    
    FrameRegs &regs = seg_->regs();
    StackFrame *fp = regs.fp();
    regs.popFrame(fp->actuals() + fp->numActualArgs());
}

inline UnrootedScript
ContextStack::currentScript(jsbytecode **ppc,
                            MaybeAllowCrossCompartment allowCrossCompartment) const
{
    AutoAssertNoGC nogc;

    if (ppc)
        *ppc = NULL;

    if (!hasfp())
        return NULL;

    FrameRegs &regs = this->regs();
    StackFrame *fp = regs.fp();

#ifdef JS_ION
    if (fp->beginsIonActivation()) {
        RootedScript script(cx_);
        ion::GetPcScript(cx_, &script, ppc);
        if (!allowCrossCompartment && script->compartment() != cx_->compartment)
            return UnrootedScript(NULL);
        return script;
    }
#endif

#ifdef JS_METHODJIT
    mjit::CallSite *inlined = regs.inlined();
    if (inlined) {
        mjit::JITChunk *chunk = fp->jit()->chunk(regs.pc);
        JS_ASSERT(inlined->inlineIndex < chunk->nInlineFrames);
        mjit::InlineFrame *frame = &chunk->inlineFrames()[inlined->inlineIndex];
        UnrootedScript script = frame->fun->nonLazyScript();
        if (!allowCrossCompartment && script->compartment() != cx_->compartment)
            return UnrootedScript(NULL);
        if (ppc)
            *ppc = script->code + inlined->pcOffset;
        return script;
    }
#endif

    UnrootedScript script = fp->script();
    if (!allowCrossCompartment && script->compartment() != cx_->compartment)
        return UnrootedScript(NULL);

    if (ppc)
        *ppc = fp->pcQuadratic(*this);
    return script;
}

inline HandleObject
ContextStack::currentScriptedScopeChain() const
{
    return fp()->scopeChain();
}

template <class Op>
inline void
StackIter::ionForEachCanonicalActualArg(JSContext *cx, Op op)
{
    JS_ASSERT(isIon());
#ifdef JS_ION
    ionInlineFrames_.forEachCanonicalActualArg(cx, op, 0, -1);
#endif
}

inline void *
AbstractFramePtr::maybeHookData() const
{
    if (isStackFrame())
        return asStackFrame()->maybeHookData();
    JS_NOT_REACHED("Invalid frame");
    return NULL;
}

inline void
AbstractFramePtr::setHookData(void *data) const
{
    if (isStackFrame()) {
        asStackFrame()->setHookData(data);
        return;
    }
    JS_NOT_REACHED("Invalid frame");
}

inline void
AbstractFramePtr::setReturnValue(const Value &rval) const
{
    if (isStackFrame()) {
        asStackFrame()->setReturnValue(rval);
        return;
    }
    JS_NOT_REACHED("Invalid frame");
}

inline UnrootedObject
AbstractFramePtr::scopeChain() const
{
    if (isStackFrame())
        return asStackFrame()->scopeChain();
    JS_NOT_REACHED("Invalid frame");
    return NULL;
}

inline CallObject &
AbstractFramePtr::callObj() const
{
    if (isStackFrame())
        return asStackFrame()->callObj();
    JS_NOT_REACHED("Invalid frame");
    return asStackFrame()->callObj();
}

inline JSCompartment *
AbstractFramePtr::compartment() const
{
    return scopeChain()->compartment();
}

inline unsigned
AbstractFramePtr::numActualArgs() const
{
    if (isStackFrame())
        return asStackFrame()->numActualArgs();
    JS_NOT_REACHED("Invalid frame");
    return 0;
}
inline unsigned
AbstractFramePtr::numFormalArgs() const
{
    if (isStackFrame())
        return asStackFrame()->numFormalArgs();
    JS_NOT_REACHED("Invalid frame");
    return 0;
}

inline Value &
AbstractFramePtr::unaliasedVar(unsigned i, MaybeCheckAliasing checkAliasing)
{
    if (isStackFrame())
        return asStackFrame()->unaliasedVar(i, checkAliasing);
    JS_NOT_REACHED("Invalid frame");
    return asStackFrame()->unaliasedVar(i);
}

inline Value &
AbstractFramePtr::unaliasedLocal(unsigned i, MaybeCheckAliasing checkAliasing)
{
    if (isStackFrame())
        return asStackFrame()->unaliasedLocal(i, checkAliasing);
    JS_NOT_REACHED("Invalid frame");
    return asStackFrame()->unaliasedLocal(i);
}

inline Value &
AbstractFramePtr::unaliasedFormal(unsigned i, MaybeCheckAliasing checkAliasing)
{
    if (isStackFrame())
        return asStackFrame()->unaliasedFormal(i, checkAliasing);
    JS_NOT_REACHED("Invalid frame");
    return asStackFrame()->unaliasedFormal(i);
}

} 
#endif 
