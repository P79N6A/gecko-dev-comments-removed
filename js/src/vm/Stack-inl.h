







































#ifndef Stack_inl_h__
#define Stack_inl_h__

#include "jscntxt.h"
#include "jscompartment.h"

#include "Stack.h"

#include "ArgumentsObject-inl.h"

namespace js {




class StackSegment
{
    
    ContextStack        *stack_;

    
    StackSegment        *previousInContext_;

    
    StackSegment        *previousInMemory_;

    
    StackFrame          *initialFrame_;

    
    FrameRegs           *suspendedRegs_;

    
    JSObject            *initialVarObj_;

    
    bool                saved_;

    
#if JS_BITS_PER_WORD == 32
    void                *padding;
#endif

    



#define NON_NULL_SUSPENDED_REGS ((FrameRegs *)0x1)

  public:
    StackSegment()
      : stack_(NULL), previousInContext_(NULL), previousInMemory_(NULL),
        initialFrame_(NULL), suspendedRegs_(NON_NULL_SUSPENDED_REGS),
        initialVarObj_(NULL), saved_(false)
    {
        JS_ASSERT(empty());
    }

    

    Value *valueRangeBegin() const {
        return (Value *)(this + 1);
    }

    




















    bool empty() const {
        JS_ASSERT(!!stack_ == !!initialFrame_);
        JS_ASSERT_IF(!stack_, suspendedRegs_ == NON_NULL_SUSPENDED_REGS && !saved_);
        return !stack_;
    }

    bool isActive() const {
        JS_ASSERT_IF(!suspendedRegs_, stack_ && !saved_);
        JS_ASSERT_IF(!stack_, suspendedRegs_ == NON_NULL_SUSPENDED_REGS);
        return !suspendedRegs_;
    }

    bool isSuspended() const {
        JS_ASSERT_IF(!stack_ || !suspendedRegs_, !saved_);
        JS_ASSERT_IF(!stack_, suspendedRegs_ == NON_NULL_SUSPENDED_REGS);
        return stack_ && suspendedRegs_;
    }

    

    bool isSaved() const {
        JS_ASSERT_IF(saved_, isSuspended());
        return saved_;
    }

    

    void joinContext(ContextStack &stack, StackFrame &frame) {
        JS_ASSERT(empty());
        stack_ = &stack;
        initialFrame_ = &frame;
        suspendedRegs_ = NULL;
        JS_ASSERT(isActive());
    }

    void leaveContext() {
        JS_ASSERT(isActive());
        stack_ = NULL;
        initialFrame_ = NULL;
        suspendedRegs_ = NON_NULL_SUSPENDED_REGS;
        JS_ASSERT(empty());
    }

    ContextStack &stack() const {
        JS_ASSERT(!empty());
        return *stack_;
    }

    ContextStack *maybeStack() const {
        return stack_;
    }

#undef NON_NULL_SUSPENDED_REGS

    

    void suspend(FrameRegs &regs) {
        JS_ASSERT(isActive());
        JS_ASSERT(contains(regs.fp()));
        suspendedRegs_ = &regs;
        JS_ASSERT(isSuspended());
    }

    void resume() {
        JS_ASSERT(isSuspended());
        suspendedRegs_ = NULL;
        JS_ASSERT(isActive());
    }

    

    void save(FrameRegs &regs) {
        JS_ASSERT(!isSuspended());
        suspend(regs);
        saved_ = true;
        JS_ASSERT(isSaved());
    }

    void restore() {
        JS_ASSERT(isSaved());
        saved_ = false;
        resume();
        JS_ASSERT(!isSuspended());
    }

    

    StackFrame *initialFrame() const {
        JS_ASSERT(!empty());
        return initialFrame_;
    }

    FrameRegs &currentRegs() const {
        JS_ASSERT(!empty());
        return isActive() ? stack_->regs() : suspendedRegs();
    }

    StackFrame *currentFrame() const {
        return currentRegs().fp();
    }

    StackFrame *currentFrameOrNull() const {
        return empty() ? NULL : currentFrame();
    }

    

    FrameRegs &suspendedRegs() const {
        JS_ASSERT(isSuspended());
        return *suspendedRegs_;
    }

    StackFrame *suspendedFrame() const {
        return suspendedRegs_->fp();
    }

    

    void setPreviousInContext(StackSegment *seg) {
        previousInContext_ = seg;
    }

    StackSegment *previousInContext() const  {
        return previousInContext_;
    }

    void setPreviousInMemory(StackSegment *seg) {
        previousInMemory_ = seg;
    }

    StackSegment *previousInMemory() const  {
        return previousInMemory_;
    }

    void setInitialVarObj(JSObject *obj) {
        JS_ASSERT(!empty());
        initialVarObj_ = obj;
    }

    bool hasInitialVarObj() {
        JS_ASSERT(!empty());
        return initialVarObj_ != NULL;
    }

    JSObject &initialVarObj() const {
        JS_ASSERT(!empty() && initialVarObj_);
        return *initialVarObj_;
    }

    bool contains(const StackFrame *fp) const;

    StackFrame *computeNextFrame(StackFrame *fp) const;
};

static const size_t VALUES_PER_STACK_SEGMENT = sizeof(StackSegment) / sizeof(Value);
JS_STATIC_ASSERT(sizeof(StackSegment) % sizeof(Value) == 0);



inline void
StackFrame::initPrev(JSContext *cx)
{
    JS_ASSERT(flags_ & HAS_PREVPC);
    if (FrameRegs *regs = cx->maybeRegs()) {
        prev_ = regs->fp();
        prevpc_ = regs->pc;
        JS_ASSERT_IF(!prev_->isDummyFrame() && !prev_->hasImacropc(),
                     uint32(prevpc_ - prev_->script()->code) < prev_->script()->length);
    } else {
        prev_ = NULL;
#ifdef DEBUG
        prevpc_ = (jsbytecode *)0xbadc;
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
StackFrame::initCallFrame(JSContext *cx, JSObject &callee, JSFunction *fun,
                          uint32 nactual, uint32 flagsArg)
{
    JS_ASSERT((flagsArg & ~(CONSTRUCTING |
                            OVERFLOW_ARGS |
                            UNDERFLOW_ARGS)) == 0);
    JS_ASSERT(fun == callee.getFunctionPrivate());

    
    flags_ = FUNCTION | HAS_PREVPC | HAS_SCOPECHAIN | flagsArg;
    exec.fun = fun;
    args.nactual = nactual;  
    scopeChain_ = callee.getParent();
    initPrev(cx);
    JS_ASSERT(!hasImacropc());
    JS_ASSERT(!hasHookData());
    JS_ASSERT(annotation() == NULL);
    JS_ASSERT(!hasCallObj());
}

inline void
StackFrame::resetInvokeCallFrame()
{
    

    putActivationObjects();

    JS_ASSERT(!(flags_ & ~(FUNCTION |
                           OVERFLOW_ARGS |
                           UNDERFLOW_ARGS |
                           OVERRIDE_ARGS |
                           HAS_PREVPC |
                           HAS_RVAL |
                           HAS_SCOPECHAIN |
                           HAS_ANNOTATION |
                           HAS_HOOK_DATA |
                           HAS_CALL_OBJ |
                           HAS_ARGS_OBJ |
                           FINISHED_IN_INTERP)));

    




    JS_ASSERT_IF(flags_ & HAS_CALL_OBJ, callObj().getPrivate() == NULL);
    JS_ASSERT_IF(flags_ & HAS_ARGS_OBJ, argsObj().getPrivate() == NULL);

    flags_ &= FUNCTION |
              OVERFLOW_ARGS |
              HAS_PREVPC |
              UNDERFLOW_ARGS;

    JS_ASSERT(exec.fun == callee().getFunctionPrivate());
    scopeChain_ = callee().getParent();
}

inline void
StackFrame::initCallFrameCallerHalf(JSContext *cx, uint32 flagsArg,
                                    void *ncode)
{
    JS_ASSERT((flagsArg & ~(CONSTRUCTING |
                            FUNCTION |
                            OVERFLOW_ARGS |
                            UNDERFLOW_ARGS)) == 0);

    flags_ = FUNCTION | flagsArg;
    prev_ = cx->fp();
    ncode_ = ncode;
}





inline void
StackFrame::initCallFrameEarlyPrologue(JSFunction *fun, uint32 nactual)
{
    exec.fun = fun;
    if (flags_ & (OVERFLOW_ARGS | UNDERFLOW_ARGS))
        args.nactual = nactual;
}





inline void
StackFrame::initCallFrameLatePrologue()
{
    SetValueRangeToUndefined(slots(), script()->nfixed);
}

inline void
StackFrame::initEvalFrame(JSContext *cx, JSScript *script, StackFrame *prev, uint32 flagsArg)
{
    JS_ASSERT(flagsArg & EVAL);
    JS_ASSERT((flagsArg & ~(EVAL | DEBUGGER)) == 0);
    JS_ASSERT(prev->isScriptFrame());

    
    Value *dstvp = (Value *)this - 2;
    Value *srcvp = prev->hasArgs()
                   ? prev->formalArgs() - 2
                   : (Value *)prev - 2;
    dstvp[0] = srcvp[0];
    dstvp[1] = srcvp[1];
    JS_ASSERT_IF(prev->isFunctionFrame(),
                 dstvp[0].toObject().isFunction());

    
    flags_ = flagsArg | HAS_PREVPC | HAS_SCOPECHAIN |
             (prev->flags_ & (FUNCTION | GLOBAL));
    if (isFunctionFrame()) {
        exec = prev->exec;
        args.script = script;
    } else {
        exec.script = script;
    }

    scopeChain_ = &prev->scopeChain();
    prev_ = prev;
    prevpc_ = prev->pcQuadratic(cx);
    JS_ASSERT(!hasImacropc());
    JS_ASSERT(!hasHookData());
    setAnnotation(prev->annotation());
}

inline void
StackFrame::initGlobalFrame(JSScript *script, JSObject &chain, StackFrame *prev, uint32 flagsArg)
{
    JS_ASSERT((flagsArg & ~(EVAL | DEBUGGER)) == 0);

    
    Value *vp = (Value *)this - 2;
    vp[0].setUndefined();
    vp[1].setUndefined();  

    
    flags_ = flagsArg | GLOBAL | HAS_PREVPC | HAS_SCOPECHAIN;
    exec.script = script;
    args.script = (JSScript *)0xbad;
    scopeChain_ = &chain;
    prev_ = prev;
    JS_ASSERT(!hasImacropc());
    JS_ASSERT(!hasHookData());
    JS_ASSERT(annotation() == NULL);
}

inline void
StackFrame::initDummyFrame(JSContext *cx, JSObject &chain)
{
    PodZero(this);
    flags_ = DUMMY | HAS_PREVPC | HAS_SCOPECHAIN;
    initPrev(cx);
    chain.isGlobal();
    setScopeChainNoCallObj(chain);
}

inline void
StackFrame::stealFrameAndSlots(Value *vp, StackFrame *otherfp,
                               Value *othervp, Value *othersp)
{
    JS_ASSERT(vp == (Value *)this - (otherfp->formalArgsEnd() - othervp));
    JS_ASSERT(othervp == otherfp->actualArgs() - 2);
    JS_ASSERT(othersp >= otherfp->slots());
    JS_ASSERT(othersp <= otherfp->base() + otherfp->numSlots());

    PodCopy(vp, othervp, othersp - othervp);
    JS_ASSERT(vp == this->actualArgs() - 2);

    
    if (otherfp->hasOverflowArgs())
        Debug_SetValueRangeToCrashOnTouch(othervp, othervp + 2 + otherfp->numFormalArgs());

    





    if (hasCallObj()) {
        JSObject &obj = callObj();
        obj.setPrivate(this);
        otherfp->flags_ &= ~HAS_CALL_OBJ;
        if (js_IsNamedLambda(fun())) {
            JSObject *env = obj.getParent();
            JS_ASSERT(env->getClass() == &js_DeclEnvClass);
            env->setPrivate(this);
        }
    }
    if (hasArgsObj()) {
        ArgumentsObject &argsobj = argsObj();
        if (argsobj.isNormalArguments())
            argsobj.setPrivate(this);
        else
            JS_ASSERT(!argsobj.getPrivate());
        otherfp->flags_ &= ~HAS_ARGS_OBJ;
    }
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

JS_ALWAYS_INLINE void
StackFrame::clearMissingArgs()
{
    if (flags_ & UNDERFLOW_ARGS)
        SetValueRangeToUndefined(formalArgs() + numActualArgs(), formalArgsEnd());
}

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
                pobj = pobj->getParent();
            JS_ASSERT(pobj);
        } else {
            for (JSObject *pobj = &obj; pobj; pobj = pobj->getParent())
                JS_ASSERT_IF(pobj->isCall(), pobj->getPrivate() != this);
        }
    }
#endif
    scopeChain_ = &obj;
    flags_ |= HAS_SCOPECHAIN;
}

inline void
StackFrame::setScopeChainWithOwnCallObj(JSObject &obj)
{
    JS_ASSERT(&obj != NULL);
    JS_ASSERT(!hasCallObj() && obj.isCall() && obj.getPrivate() == this);
    scopeChain_ = &obj;
    flags_ |= HAS_SCOPECHAIN | HAS_CALL_OBJ;
}

inline JSObject &
StackFrame::callObj() const
{
    JS_ASSERT_IF(isNonEvalFunctionFrame() || isStrictEvalFrame(), hasCallObj());

    JSObject *pobj = &scopeChain();
    while (JS_UNLIKELY(pobj->getClass() != &js_CallClass)) {
        JS_ASSERT(IsCacheableNonGlobalScope(pobj) || pobj->isWith());
        pobj = pobj->getParent();
    }
    return *pobj;
}

inline void
StackFrame::putActivationObjects()
{
    if (flags_ & (HAS_ARGS_OBJ | HAS_CALL_OBJ)) {
        
        if (hasCallObj())
            js_PutCallObject(this);
        else if (hasArgsObj())
            js_PutArgsObject(this);
    }
}

inline void
StackFrame::markActivationObjectsAsPut()
{
    if (flags_ & (HAS_ARGS_OBJ | HAS_CALL_OBJ)) {
        if (hasArgsObj() && !argsObj().getPrivate()) {
            args.nactual = args.obj->initialLength();
            flags_ &= ~HAS_ARGS_OBJ;
        }
        if (hasCallObj() && !callObj().getPrivate()) {
            






            scopeChain_ = isFunctionFrame()
                          ? callee().getParent()
                          : scopeChain_->getParent();
            flags_ &= ~HAS_CALL_OBJ;
        }
    }
}



JS_ALWAYS_INLINE void
StackSpace::pushOverride(Value *top, StackOverride *prev)
{
    *prev = override_;

    override_.top = top;
#ifdef DEBUG
    override_.seg = seg_;
    override_.frame = seg_->currentFrameOrNull();
#endif

    JS_ASSERT(prev->top < override_.top);
}

JS_ALWAYS_INLINE void
StackSpace::popOverride(const StackOverride &prev)
{
    JS_ASSERT(prev.top < override_.top);

    JS_ASSERT_IF(seg_->empty(), override_.frame == NULL);
    JS_ASSERT_IF(!seg_->empty(), override_.frame == seg_->currentFrame());
    JS_ASSERT(override_.seg == seg_);

    override_ = prev;
}

JS_ALWAYS_INLINE Value *
StackSpace::activeFirstUnused() const
{
    JS_ASSERT(seg_->isActive());

    Value *max = Max(seg_->stack().regs().sp, override_.top);
    JS_ASSERT(max == firstUnused());
    return max;
}

#ifdef JS_TRACER
JS_ALWAYS_INLINE bool
StackSpace::ensureEnoughSpaceToEnterTrace()
{
    ptrdiff_t needed = TraceNativeStorage::MAX_NATIVE_STACK_SLOTS +
                       TraceNativeStorage::MAX_CALL_STACK_ENTRIES * VALUES_PER_STACK_FRAME;
#ifdef XP_WIN
    return ensureSpace(NULL, firstUnused(), needed);
#else
    return end_ - firstUnused() > needed;
#endif
}
#endif

STATIC_POSTCONDITION(!return || ubound(from) >= nvals)
JS_ALWAYS_INLINE bool
StackSpace::ensureSpace(JSContext *maybecx, Value *from, ptrdiff_t nvals) const
{
    JS_ASSERT(from >= firstUnused());
#ifdef XP_WIN
    JS_ASSERT(from <= commitEnd_);
    if (commitEnd_ - from < nvals)
        return bumpCommit(maybecx, from, nvals);
    return true;
#else
    if (end_ - from < nvals) {
        js_ReportOverRecursed(maybecx);
        return false;
    }
    return true;
#endif
}

inline Value *
StackSpace::getStackLimit(JSContext *cx)
{
    Value *limit;
#ifdef XP_WIN
    limit = commitEnd_;
#else
    limit = end_;
#endif

    
    FrameRegs &regs = cx->regs();
    uintN minSpace = regs.fp()->numSlots() + VALUES_PER_STACK_FRAME;
    if (regs.sp + minSpace > limit) {
        js_ReportOverRecursed(cx);
        return NULL;
    }

    return limit;
}



JS_ALWAYS_INLINE bool
ContextStack::isCurrentAndActive() const
{
    assertSegmentsInSync();
    return seg_ && seg_->isActive() && seg_ == space().currentSegment();
}

namespace detail {

struct OOMCheck
{
    JS_ALWAYS_INLINE bool
    operator()(JSContext *cx, StackSpace &space, Value *from, uintN nvals)
    {
        return space.ensureSpace(cx, from, nvals);
    }
};

struct LimitCheck
{
    Value **limit;

    LimitCheck(Value **limit) : limit(limit) {}

    JS_ALWAYS_INLINE bool
    operator()(JSContext *cx, StackSpace &space, Value *from, uintN nvals)
    {
        



        nvals += VALUES_PER_STACK_FRAME;

        JS_ASSERT(from < *limit);
        if (*limit - from >= ptrdiff_t(nvals))
            return true;
        return space.tryBumpLimit(cx, from, nvals, limit);
    }
};

}  

template <class Check>
JS_ALWAYS_INLINE StackFrame *
ContextStack::getCallFrame(JSContext *cx, Value *firstUnused, uintN nactual,
                           JSFunction *fun, JSScript *script, uint32 *flags,
                           Check check) const
{
    JS_ASSERT(fun->script() == script);
    JS_ASSERT(space().firstUnused() == firstUnused);

    uintN nvals = VALUES_PER_STACK_FRAME + script->nslots;
    uintN nformal = fun->nargs;

    

    if (nactual == nformal) {
        if (JS_UNLIKELY(!check(cx, space(), firstUnused, nvals)))
            return NULL;
        return reinterpret_cast<StackFrame *>(firstUnused);
    }

    if (nactual < nformal) {
        *flags |= StackFrame::UNDERFLOW_ARGS;
        uintN nmissing = nformal - nactual;
        if (JS_UNLIKELY(!check(cx, space(), firstUnused, nmissing + nvals)))
            return NULL;
        SetValueRangeToUndefined(firstUnused, nmissing);
        return reinterpret_cast<StackFrame *>(firstUnused + nmissing);
    }

    *flags |= StackFrame::OVERFLOW_ARGS;
    uintN ncopy = 2 + nformal;
    if (JS_UNLIKELY(!check(cx, space(), firstUnused, ncopy + nvals)))
        return NULL;

    Value *dst = firstUnused;
    Value *src = firstUnused - (2 + nactual);
    PodCopy(dst, src, ncopy);
    Debug_SetValueRangeToCrashOnTouch(src, ncopy);
    return reinterpret_cast<StackFrame *>(firstUnused + ncopy);
}

JS_ALWAYS_INLINE StackFrame *
ContextStack::getInlineFrame(JSContext *cx, Value *sp, uintN nactual,
                             JSFunction *fun, JSScript *script, uint32 *flags) const
{
    JS_ASSERT(isCurrentAndActive());
    JS_ASSERT(cx->regs().sp == sp);

    return getCallFrame(cx, sp, nactual, fun, script, flags, detail::OOMCheck());
}

JS_ALWAYS_INLINE StackFrame *
ContextStack::getInlineFrameWithinLimit(JSContext *cx, Value *sp, uintN nactual,
                                        JSFunction *fun, JSScript *script, uint32 *flags,
                                        Value **limit) const
{
    JS_ASSERT(isCurrentAndActive());
    JS_ASSERT(cx->regs().sp == sp);

    return getCallFrame(cx, sp, nactual, fun, script, flags, detail::LimitCheck(limit));
}

JS_ALWAYS_INLINE void
ContextStack::pushInlineFrame(JSScript *script, StackFrame *fp, FrameRegs &regs)
{
    JS_ASSERT(isCurrentAndActive());
    JS_ASSERT(regs_ == &regs && script == fp->script());

    regs.prepareToRun(fp, script);
}

JS_ALWAYS_INLINE void
ContextStack::popInlineFrame()
{
    JS_ASSERT(isCurrentAndActive());

    StackFrame *fp = regs_->fp();
    fp->putActivationObjects();

    Value *newsp = fp->actualArgs() - 1;
    JS_ASSERT(newsp >= fp->prev()->base());

    newsp[-1] = fp->returnValue();
    regs_->popFrame(newsp);
}

JS_ALWAYS_INLINE bool
ContextStack::pushInvokeArgs(JSContext *cx, uintN argc, InvokeArgsGuard *argsGuard)
{
    if (!isCurrentAndActive())
        return pushInvokeArgsSlow(cx, argc, argsGuard);

    Value *start = space().activeFirstUnused();
    uintN vplen = 2 + argc;
    if (!space().ensureSpace(cx, start, vplen))
        return false;

    Value *vp = start;
    ImplicitCast<CallArgs>(*argsGuard) = CallArgsFromVp(argc, vp);

    



    space().pushOverride(vp + vplen, &argsGuard->prevOverride_);

    argsGuard->stack_ = this;
    return true;
}

JS_ALWAYS_INLINE void
ContextStack::popInvokeArgs(const InvokeArgsGuard &argsGuard)
{
    if (argsGuard.seg_) {
        popInvokeArgsSlow(argsGuard);
        return;
    }

    JS_ASSERT(isCurrentAndActive());
    space().popOverride(argsGuard.prevOverride_);
}

JS_ALWAYS_INLINE
InvokeArgsGuard::~InvokeArgsGuard()
{
    if (JS_UNLIKELY(!pushed()))
        return;
    stack_->popInvokeArgs(*this);
}

JS_ALWAYS_INLINE StackFrame *
ContextStack::getInvokeFrame(JSContext *cx, const CallArgs &args,
                             JSFunction *fun, JSScript *script,
                             uint32 *flags, InvokeFrameGuard *frameGuard) const
{
    uintN argc = args.argc();
    Value *start = args.argv() + argc;
    JS_ASSERT(start == space().firstUnused());
    StackFrame *fp = getCallFrame(cx, start, argc, fun, script, flags, detail::OOMCheck());
    if (!fp)
        return NULL;

    frameGuard->regs_.prepareToRun(fp, script);
    return fp;
}

JS_ALWAYS_INLINE void
ContextStack::pushInvokeFrame(const CallArgs &args, InvokeFrameGuard *frameGuard)
{
    JS_ASSERT(space().firstUnused() == args.argv() + args.argc());

    if (JS_UNLIKELY(space().seg_->empty())) {
        pushInvokeFrameSlow(frameGuard);
        return;
    }

    frameGuard->prevRegs_ = regs_;
    regs_ = &frameGuard->regs_;
    JS_ASSERT(isCurrentAndActive());

    frameGuard->stack_ = this;
}

JS_ALWAYS_INLINE void
ContextStack::popInvokeFrame(const InvokeFrameGuard &frameGuard)
{
    JS_ASSERT(isCurrentAndActive());
    JS_ASSERT(&frameGuard.regs_ == regs_);

    if (JS_UNLIKELY(seg_->initialFrame() == regs_->fp())) {
        popInvokeFrameSlow(frameGuard);
        return;
    }

    regs_->fp()->putActivationObjects();
    regs_ = frameGuard.prevRegs_;
}

JS_ALWAYS_INLINE void
InvokeFrameGuard::pop()
{
    JS_ASSERT(pushed());
    stack_->popInvokeFrame(*this);
    stack_ = NULL;
}

JS_ALWAYS_INLINE
InvokeFrameGuard::~InvokeFrameGuard()
{
    if (pushed())
        pop();
}

JS_ALWAYS_INLINE JSObject &
ContextStack::currentVarObj() const
{
    if (regs_->fp()->hasCallObj())
        return regs_->fp()->callObj();
    return seg_->initialVarObj();
}

inline StackFrame *
ContextStack::findFrameAtLevel(uintN targetLevel) const
{
    StackFrame *fp = regs_->fp();
    while (true) {
        JS_ASSERT(fp && fp->isScriptFrame());
        if (fp->script()->staticLevel == targetLevel)
            break;
        fp = fp->prev();
    }
    return fp;
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

    



    StackFrame *fp = reinterpret_cast<StackFrame *>(getPrivate());
    if (fp == JS_ARGUMENTS_OBJECT_ON_TRACE)
        return false;

    




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

    StackFrame *fp = reinterpret_cast<StackFrame *>(getPrivate());

    
    if (!fp) {
        Value *srcbeg = elements() + start;
        Value *srcend = srcbeg + count;
        for (Value *dst = vp, *src = srcbeg; src < srcend; ++dst, ++src) {
            if (src->isMagic(JS_ARGS_HOLE))
                return false;
            *dst = *src;
        }
        return true;
    }

    
    if (fp == JS_ARGUMENTS_OBJECT_ON_TRACE)
        return false;

    
    JS_ASSERT(fp->numActualArgs() <= JS_ARGS_LENGTH_MAX);
    return fp->forEachCanonicalActualArg(detail::CopyNonHoleArgsTo(this, vp), start, count);
}

} 

#endif 
