







































#ifndef Stack_inl_h__
#define Stack_inl_h__

#include "jscntxt.h"
#include "jscompartment.h"

#include "Stack.h"

#include "jsscriptinlines.h"
#include "ArgumentsObject-inl.h"
#include "CallObject-inl.h"

#include "methodjit/MethodJIT.h"

namespace js {






static inline bool
IsCacheableNonGlobalScope(JSObject *obj)
{
    bool cacheable = (obj->isCall() || obj->isBlock() || obj->isDeclEnv());

    JS_ASSERT_IF(cacheable, !obj->getOps()->lookupProperty);
    return cacheable;
}

inline JSObject &
StackFrame::scopeChain() const
{
    JS_ASSERT_IF(!(flags_ & HAS_SCOPECHAIN), isFunctionFrame());
    if (!(flags_ & HAS_SCOPECHAIN)) {
        scopeChain_ = callee().toFunction()->environment();
        flags_ |= HAS_SCOPECHAIN;
    }
    return *scopeChain_;
}

inline JSObject &
StackFrame::varObj()
{
    JSObject *obj = &scopeChain();
    while (!obj->isVarObj())
        obj = obj->getParentOrScopeChain();
    return *obj;
}

inline JSCompartment *
StackFrame::compartment() const
{
    JS_ASSERT_IF(isScriptFrame(), scopeChain().compartment() == script()->compartment());
    return scopeChain().compartment();
}

inline void
StackFrame::initPrev(JSContext *cx)
{
    JS_ASSERT(flags_ & HAS_PREVPC);
    if (FrameRegs *regs = cx->maybeRegs()) {
        prev_ = regs->fp();
        prevpc_ = regs->pc;
        prevInline_ = regs->inlined();
        JS_ASSERT_IF(!prev_->isDummyFrame() && !prev_->hasImacropc(),
                     uint32(prevpc_ - prev_->script()->code) < prev_->script()->length);
    } else {
        prev_ = NULL;
#ifdef DEBUG
        prevpc_ = (jsbytecode *)0xbadc;
        prevInline_ = (JSInlinedSite *)0xbadc;
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
                          JSScript *script, uint32 nactual, StackFrame::Flags flagsArg)
{
    JS_ASSERT((flagsArg & ~(CONSTRUCTING |
                            LOWERED_CALL_APPLY |
                            OVERFLOW_ARGS |
                            UNDERFLOW_ARGS)) == 0);
    JS_ASSERT(script == callee.toFunction()->script());

    
    flags_ = FUNCTION | HAS_PREVPC | HAS_SCOPECHAIN | flagsArg;
    exec.fun = &callee;
    args.nactual = nactual;
    scopeChain_ = callee.toFunction()->environment();
    ncode_ = NULL;
    initPrev(cx);
    JS_ASSERT(!hasImacropc());
    JS_ASSERT(!hasHookData());
    JS_ASSERT(annotation() == NULL);
    JS_ASSERT(!hasCallObj());

    SetValueRangeToUndefined(slots(), script->nfixed);
}

inline void
StackFrame::resetCallFrame(JSScript *script)
{
    JS_ASSERT(script == this->script());

    if (flags_ & UNDERFLOW_ARGS)
        SetValueRangeToUndefined(formalArgs() + numActualArgs(), formalArgsEnd());

    JS_ASSERT(!(flags_ & ~(FUNCTION |
                           OVERFLOW_ARGS |
                           UNDERFLOW_ARGS |
                           OVERRIDE_ARGS |
                           HAS_PREVPC |
                           HAS_RVAL |
                           HAS_SCOPECHAIN |
                           HAS_ANNOTATION |
                           HAS_HOOK_DATA |
                           FINISHED_IN_INTERP |
                           DOWN_FRAMES_EXPANDED)));

    flags_ &= FUNCTION |
              OVERFLOW_ARGS |
              HAS_PREVPC |
              UNDERFLOW_ARGS;

    JS_ASSERT(exec.fun->script() == callee().toFunction()->script());
    scopeChain_ = callee().toFunction()->environment();

    SetValueRangeToUndefined(slots(), script->nfixed);
}

inline void
StackFrame::initJitFrameCallerHalf(StackFrame *prev, StackFrame::Flags flags, void *ncode)
{
    JS_ASSERT((flags & ~(CONSTRUCTING |
                         LOWERED_CALL_APPLY |
                         FUNCTION |
                         OVERFLOW_ARGS |
                         UNDERFLOW_ARGS)) == 0);

    flags_ = FUNCTION | flags;
    prev_ = prev;
    ncode_ = ncode;
}





inline void
StackFrame::initJitFrameEarlyPrologue(JSFunction *fun, uint32 nactual)
{
    exec.fun = fun;
    args.nactual = nactual;
}





inline bool
StackFrame::initJitFrameLatePrologue(JSContext *cx, Value **limit)
{
    *limit = cx->stack.space().getStackLimit(cx, DONT_REPORT_ERROR);
    if (!*limit) {
        cx->stack.popFrameAfterOverflow();
        js_ReportOverRecursed(cx);
        return false;
    }

    scopeChain();
    SetValueRangeToUndefined(slots(), script()->nfixed);
    return true;
}

inline void
StackFrame::overwriteCallee(JSObject &newCallee)
{
    JS_ASSERT(callee().toFunction()->script() == newCallee.toFunction()->script());
    mutableCalleev().setObject(newCallee);
}

inline Value &
StackFrame::canonicalActualArg(uintN i) const
{
    if (i < numFormalArgs())
        return formalArg(i);
    JS_ASSERT(i < numActualArgs());
    return actualArgs()[i];
}

template <class Op>
inline bool
StackFrame::forEachCanonicalActualArg(Op op, uintN start , uintN count )
{
    uintN nformal = fun()->nargs;
    JS_ASSERT(start <= nformal);

    Value *formals = formalArgsEnd() - nformal;
    uintN nactual = numActualArgs();
    if (count == uintN(-1))
        count = nactual - start;

    uintN end = start + count;
    JS_ASSERT(end >= start);
    JS_ASSERT(end <= nactual);

    if (end <= nformal) {
        Value *p = formals + start;
        for (; start < end; ++p, ++start) {
            if (!op(start, p))
                return false;
        }
    } else {
        for (Value *p = formals + start; start < nformal; ++p, ++start) {
            if (!op(start, p))
                return false;
        }
        JS_ASSERT(start >= nformal);
        Value *actuals = formals - (nactual + 2) + start;
        for (Value *p = actuals; start < end; ++p, ++start) {
            if (!op(start, p))
                return false;
        }
    }
    return true;
}

template <class Op>
inline bool
StackFrame::forEachFormalArg(Op op)
{
    Value *formals = formalArgsEnd() - fun()->nargs;
    Value *formalsEnd = formalArgsEnd();
    uintN i = 0;
    for (Value *p = formals; p != formalsEnd; ++p, ++i) {
        if (!op(i, p))
            return false;
    }
    return true;
}

struct CopyTo
{
    Value *dst;
    CopyTo(Value *dst) : dst(dst) {}
    bool operator()(uintN, Value *src) {
        *dst++ = *src;
        return true;
    }
};

inline uintN
StackFrame::numActualArgs() const
{
    







    JS_ASSERT(hasArgs());
    if (JS_UNLIKELY(flags_ & (OVERFLOW_ARGS | UNDERFLOW_ARGS)))
        return hasArgsObj() ? argsObj().initialLength() : args.nactual;
    return numFormalArgs();
}

inline Value *
StackFrame::actualArgs() const
{
    JS_ASSERT(hasArgs());
    Value *argv = formalArgs();
    if (JS_UNLIKELY(flags_ & OVERFLOW_ARGS)) {
        uintN nactual = hasArgsObj() ? argsObj().initialLength() : args.nactual;
        return argv - (2 + nactual);
    }
    return argv;
}

inline Value *
StackFrame::actualArgsEnd() const
{
    JS_ASSERT(hasArgs());
    if (JS_UNLIKELY(flags_ & OVERFLOW_ARGS))
        return formalArgs() - 2;
    return formalArgs() + numActualArgs();
}

inline void
StackFrame::setArgsObj(ArgumentsObject &obj)
{
    JS_ASSERT_IF(hasArgsObj(), &obj == args.obj);
    JS_ASSERT_IF(!hasArgsObj(), numActualArgs() == obj.initialLength());
    args.obj = &obj;
    flags_ |= HAS_ARGS_OBJ;
}

inline void
StackFrame::setScopeChainNoCallObj(JSObject &obj)
{
#ifdef DEBUG
    JS_ASSERT(&obj != NULL);
    if (&obj != sInvalidScopeChain) {
        if (hasCallObj()) {
            JSObject *pobj = &obj;
            while (pobj && pobj->getPrivate() != this)
                pobj = pobj->getParentOrScopeChain();
            JS_ASSERT(pobj);
        } else {
            for (JSObject *pobj = &obj; pobj->isScope(); pobj = pobj->getParentOrScopeChain())
                JS_ASSERT_IF(pobj->isCall(), pobj->getPrivate() != this);
        }
    }
#endif
    scopeChain_ = &obj;
    flags_ |= HAS_SCOPECHAIN;
}

inline void
StackFrame::setScopeChainWithOwnCallObj(CallObject &obj)
{
    JS_ASSERT(&obj != NULL);
    JS_ASSERT(!hasCallObj() && obj.maybeStackFrame() == this);
    scopeChain_ = &obj;
    flags_ |= HAS_SCOPECHAIN | HAS_CALL_OBJ;
}

inline CallObject &
StackFrame::callObj() const
{
    JS_ASSERT_IF(isNonEvalFunctionFrame() || isStrictEvalFrame(), hasCallObj());

    JSObject *pobj = &scopeChain();
    while (JS_UNLIKELY(!pobj->isCall()))
        pobj = pobj->getParentOrScopeChain();
    return pobj->asCall();
}

inline bool
StackFrame::maintainNestingState() const
{
    



    return isNonEvalFunctionFrame() && !isGeneratorFrame() && script()->nesting();
}

inline bool
StackFrame::functionPrologue(JSContext *cx)
{
    JS_ASSERT(isNonEvalFunctionFrame());

    JSFunction *fun = this->fun();

    if (fun->isHeavyweight()) {
        if (!CreateFunCallObject(cx, this))
            return false;
    } else {
        
        scopeChain();
    }

    if (script()->nesting()) {
        JS_ASSERT(maintainNestingState());
        types::NestingPrologue(cx, this);
    }

    return true;
}

inline void
StackFrame::functionEpilogue()
{
    JS_ASSERT(isNonEvalFunctionFrame());

    if (flags_ & (HAS_ARGS_OBJ | HAS_CALL_OBJ)) {
        
        if (hasCallObj())
            js_PutCallObject(this);
        else if (hasArgsObj())
            js_PutArgsObject(this);
    }

    if (maintainNestingState())
        types::NestingEpilogue(this);
}

inline void
StackFrame::markFunctionEpilogueDone()
{
    if (flags_ & (HAS_ARGS_OBJ | HAS_CALL_OBJ)) {
        if (hasArgsObj() && !argsObj().maybeStackFrame()) {
            args.nactual = args.obj->initialLength();
            flags_ &= ~HAS_ARGS_OBJ;
        }
        if (hasCallObj() && !callObj().maybeStackFrame()) {
            






            scopeChain_ = isFunctionFrame()
                          ? callee().toFunction()->environment()
                          : scopeChain_->scopeChain();
            flags_ &= ~HAS_CALL_OBJ;
        }
    }

    




    if (maintainNestingState())
        script()->nesting()->activeFrames++;
}



#ifdef JS_TRACER
JS_ALWAYS_INLINE bool
StackSpace::ensureEnoughSpaceToEnterTrace(JSContext *cx)
{
    ptrdiff_t needed = TraceNativeStorage::MAX_NATIVE_STACK_SLOTS +
                       TraceNativeStorage::MAX_CALL_STACK_ENTRIES * VALUES_PER_STACK_FRAME;
    return ensureSpace(cx, DONT_REPORT_ERROR, firstUnused(), needed);
}
#endif

STATIC_POSTCONDITION(!return || ubound(from) >= nvals)
JS_ALWAYS_INLINE bool
StackSpace::ensureSpace(JSContext *cx, MaybeReportError report, Value *from, ptrdiff_t nvals,
                        JSCompartment *dest) const
{
    assertInvariants();
    JS_ASSERT(from >= firstUnused());
#ifdef XP_WIN
    JS_ASSERT(from <= commitEnd_);
#endif
    if (JS_UNLIKELY(conservativeEnd_ - from < nvals))
        return ensureSpaceSlow(cx, report, from, nvals, dest);
    return true;
}

inline Value *
StackSpace::getStackLimit(JSContext *cx, MaybeReportError report)
{
    FrameRegs &regs = cx->regs();
    uintN nvals = regs.fp()->numSlots() + STACK_JIT_EXTRA;
    return ensureSpace(cx, report, regs.sp, nvals)
           ? conservativeEnd_
           : NULL;
}



JS_ALWAYS_INLINE StackFrame *
ContextStack::getCallFrame(JSContext *cx, MaybeReportError report, const CallArgs &args,
                           JSFunction *fun, JSScript *script,  uint32 *flags) const
{
    JS_ASSERT(fun->script() == script);
    uintN nformal = fun->nargs;

    Value *firstUnused = args.end();
    JS_ASSERT(firstUnused == space().firstUnused());

    
    uintN nvals = VALUES_PER_STACK_FRAME + script->nslots + StackSpace::STACK_JIT_EXTRA;

    

    if (args.length() == nformal) {
        if (!space().ensureSpace(cx, report, firstUnused, nvals))
            return NULL;
        return reinterpret_cast<StackFrame *>(firstUnused);
    }

    if (args.length() < nformal) {
        *flags = StackFrame::Flags(*flags | StackFrame::UNDERFLOW_ARGS);
        uintN nmissing = nformal - args.length();
        if (!space().ensureSpace(cx, report, firstUnused, nmissing + nvals))
            return NULL;
        SetValueRangeToUndefined(firstUnused, nmissing);
        return reinterpret_cast<StackFrame *>(firstUnused + nmissing);
    }

    *flags = StackFrame::Flags(*flags | StackFrame::OVERFLOW_ARGS);
    uintN ncopy = 2 + nformal;
    if (!space().ensureSpace(cx, report, firstUnused, ncopy + nvals))
        return NULL;
    Value *dst = firstUnused;
    Value *src = args.base();
    PodCopy(dst, src, ncopy);
    return reinterpret_cast<StackFrame *>(firstUnused + ncopy);
}

JS_ALWAYS_INLINE bool
ContextStack::pushInlineFrame(JSContext *cx, FrameRegs &regs, const CallArgs &args,
                              JSFunction &callee, JSScript *script,
                              InitialFrameFlags initial)
{
    JS_ASSERT(onTop());
    JS_ASSERT(regs.sp == args.end());
    
    JS_ASSERT(script == callee.toFunction()->script());

     uint32 flags = ToFrameFlags(initial);
    StackFrame *fp = getCallFrame(cx, REPORT_ERROR, args, &callee, script, &flags);
    if (!fp)
        return false;

    
    fp->initCallFrame(cx, callee, script, args.length(), (StackFrame::Flags) flags);

    



    regs.prepareToRun(*fp, script);
    return true;
}

JS_ALWAYS_INLINE bool
ContextStack::pushInlineFrame(JSContext *cx, FrameRegs &regs, const CallArgs &args,
                              JSFunction &callee, JSScript *script,
                              InitialFrameFlags initial, Value **stackLimit)
{
    if (!pushInlineFrame(cx, regs, args, callee, script, initial))
        return false;
    *stackLimit = space().conservativeEnd_;
    return true;
}

JS_ALWAYS_INLINE StackFrame *
ContextStack::getFixupFrame(JSContext *cx, MaybeReportError report,
                            const CallArgs &args, JSFunction *fun, JSScript *script,
                            void *ncode, InitialFrameFlags initial, Value **stackLimit)
{
    JS_ASSERT(onTop());
    JS_ASSERT(fun->script() == args.callee().toFunction()->script());
    JS_ASSERT(fun->script() == script);

     uint32 flags = ToFrameFlags(initial);
    StackFrame *fp = getCallFrame(cx, report, args, fun, script, &flags);
    if (!fp)
        return NULL;

    
    fp->initJitFrameCallerHalf(cx->fp(), (StackFrame::Flags) flags, ncode);
    fp->initJitFrameEarlyPrologue(fun, args.length());

    *stackLimit = space().conservativeEnd_;
    return fp;
}

JS_ALWAYS_INLINE void
ContextStack::popInlineFrame(FrameRegs &regs)
{
    JS_ASSERT(onTop());
    JS_ASSERT(&regs == &seg_->regs());

    StackFrame *fp = regs.fp();
    fp->functionEpilogue();

    Value *newsp = fp->actualArgs() - 1;
    JS_ASSERT(newsp >= fp->prev()->base());

    newsp[-1] = fp->returnValue();
    regs.popFrame(newsp);
}

inline void
ContextStack::popFrameAfterOverflow()
{
    
    FrameRegs &regs = seg_->regs();
    StackFrame *fp = regs.fp();
    regs.popFrame(fp->actualArgsEnd());
}

inline JSScript *
ContextStack::currentScript(jsbytecode **ppc) const
{
    if (ppc)
        *ppc = NULL;

    FrameRegs *regs = maybeRegs();
    StackFrame *fp = regs ? regs->fp() : NULL;
    while (fp && fp->isDummyFrame())
        fp = fp->prev();
    if (!fp)
        return NULL;

#ifdef JS_METHODJIT
    mjit::CallSite *inlined = regs->inlined();
    if (inlined) {
        JS_ASSERT(inlined->inlineIndex < fp->jit()->nInlineFrames);
        mjit::InlineFrame *frame = &fp->jit()->inlineFrames()[inlined->inlineIndex];
        JSScript *script = frame->fun->script();
        if (script->compartment() != cx_->compartment)
            return NULL;
        if (ppc)
            *ppc = script->code + inlined->pcOffset;
        return script;
    }
#endif

    JSScript *script = fp->script();
    if (script->compartment() != cx_->compartment)
        return NULL;

    if (ppc) {
        if (fp->hasImacropc())
            *ppc = fp->imacropc();
        else
            *ppc = fp->pcQuadratic(*this);
    }
    return script;
}

inline JSObject *
ContextStack::currentScriptedScopeChain() const
{
    return &fp()->scopeChain();
}



namespace detail {

struct STATIC_SKIP_INFERENCE CopyNonHoleArgsTo
{
    CopyNonHoleArgsTo(ArgumentsObject *argsobj, Value *dst) : argsobj(*argsobj), dst(dst) {}
    ArgumentsObject &argsobj;
    Value *dst;
    bool operator()(uint32 argi, Value *src) {
        if (argsobj.element(argi).isMagic(JS_ARGS_HOLE))
            return false;
        *dst++ = *src;
        return true;
    }
};

} 

inline bool
ArgumentsObject::getElement(uint32 i, Value *vp)
{
    if (i >= initialLength())
        return false;

    *vp = element(i);

    



    if (vp->isMagic(JS_ARGS_HOLE))
        return false;

    



    if (onTrace())
        return false;

    




    StackFrame *fp = maybeStackFrame();
    JS_ASSERT_IF(isStrictArguments(), !fp);
    if (fp)
        *vp = fp->canonicalActualArg(i);
    return true;
}

inline bool
ArgumentsObject::getElements(uint32 start, uint32 count, Value *vp)
{
    JS_ASSERT(start + count >= start);

    uint32 length = initialLength();
    if (start > length || start + count > length)
        return false;

    StackFrame *fp = maybeStackFrame();

    
    if (!fp) {
        const Value *srcbeg = elements() + start;
        const Value *srcend = srcbeg + count;
        const Value *src = srcbeg;
        for (Value *dst = vp; src < srcend; ++dst, ++src) {
            if (src->isMagic(JS_ARGS_HOLE))
                return false;
            *dst = *src;
        }
        return true;
    }

    
    if (onTrace())
        return false;

    
    JS_ASSERT(fp->numActualArgs() <= StackSpace::ARGS_LENGTH_MAX);
    return fp->forEachCanonicalActualArg(detail::CopyNonHoleArgsTo(this, vp), start, count);
}

} 
#endif 
