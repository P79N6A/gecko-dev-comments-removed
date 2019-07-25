







































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

    
    bool                saved_;

    



#define NON_NULL_SUSPENDED_REGS ((FrameRegs *)0x1)

  public:
    StackSegment()
      : stack_(NULL), previousInContext_(NULL), previousInMemory_(NULL),
        initialFrame_(NULL), suspendedRegs_(NON_NULL_SUSPENDED_REGS),
        saved_(false)
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
                          JSScript *script, uint32 nactual, StackFrame::Flags flagsArg)
{
    JS_ASSERT((flagsArg & ~(CONSTRUCTING |
                            OVERFLOW_ARGS |
                            UNDERFLOW_ARGS)) == 0);
    JS_ASSERT(fun == callee.getFunctionPrivate());
    JS_ASSERT(script == fun->script());

    
    flags_ = FUNCTION | HAS_PREVPC | HAS_SCOPECHAIN | flagsArg;
    exec.fun = fun;
    args.nactual = nactual;  
    scopeChain_ = callee.getParent();
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

    

    putActivationObjects();

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

    SetValueRangeToUndefined(slots(), script->nfixed);
}

inline void
StackFrame::initJitFrameCallerHalf(JSContext *cx, StackFrame::Flags flags,
                                    void *ncode)
{
    JS_ASSERT((flags & ~(CONSTRUCTING |
                         FUNCTION |
                         OVERFLOW_ARGS |
                         UNDERFLOW_ARGS)) == 0);

    flags_ = FUNCTION | flags;
    prev_ = cx->fp();
    ncode_ = ncode;
}





inline void
StackFrame::initJitFrameEarlyPrologue(JSFunction *fun, uint32 nactual)
{
    exec.fun = fun;
    if (flags_ & (OVERFLOW_ARGS | UNDERFLOW_ARGS))
        args.nactual = nactual;
}





inline void
StackFrame::initJitFrameLatePrologue()
{
    SetValueRangeToUndefined(slots(), script()->nfixed);
}

inline void
StackFrame::initExecuteFrame(JSScript *script, StackFrame *prev, const Value &thisv,
                             JSObject &scopeChain, ExecuteType type)
{
    




    flags_ = type | HAS_SCOPECHAIN;
    if (!(flags_ & GLOBAL))
        flags_ |= (prev->flags_ & (FUNCTION | GLOBAL));

    Value *dstvp = (Value *)this - 2;
    dstvp[1] = thisv;

    if (isFunctionFrame()) {
        dstvp[0] = prev->calleev();
        exec = prev->exec;
        args.script = script;
    } else {
        JS_ASSERT(isGlobalFrame());
        dstvp[0] = NullValue();
        exec.script = script;
#ifdef DEBUG
        args.script = (JSScript *)0xbad;
#endif
    }

    scopeChain_ = &scopeChain;
    prev_ = prev;
#ifdef DEBUG
    ncode_ = (void *)0xbad;
    Debug_SetValueRangeToCrashOnTouch(&rval_, 1);
    prevpc_ = (jsbytecode *)0xbad;
    hookData_ = (void *)0xbad;
    annotation_ = (void *)0xbad;
#endif

    if (flags_ & HAS_ANNOTATION)
        annotation_ = prev->annotation_;
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
    JS_ASSERT(vp == (Value *)this - ((Value *)otherfp - othervp));
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

JS_ALWAYS_INLINE bool
OOMCheck::operator()(JSContext *cx, StackSpace &space, Value *from, uintN nvals)
{
    return space.ensureSpace(cx, from, nvals);
}

JS_ALWAYS_INLINE bool
LimitCheck::operator()(JSContext *cx, StackSpace &space, Value *from, uintN nvals)
{
    



    nvals += VALUES_PER_STACK_FRAME;
    JS_ASSERT(from < *limit);
    if (*limit - from >= ptrdiff_t(nvals))
        return true;
    return space.tryBumpLimit(cx, from, nvals, limit);
}

template <class Check>
JS_ALWAYS_INLINE StackFrame *
ContextStack::getCallFrame(JSContext *cx, const CallArgs &args,
                           JSFunction *fun, JSScript *script,
                           StackFrame::Flags *flags, Check check) const
{
    JS_ASSERT(fun->script() == script);
    JS_ASSERT(space().firstUnused() == args.end());

    Value *firstUnused = args.end();
    uintN nvals = VALUES_PER_STACK_FRAME + script->nslots;
    uintN nformal = fun->nargs;

    

    if (args.argc() == nformal) {
        if (JS_UNLIKELY(!check(cx, space(), firstUnused, nvals)))
            return NULL;
        return reinterpret_cast<StackFrame *>(firstUnused);
    }

    if (args.argc() < nformal) {
        *flags = StackFrame::Flags(*flags | StackFrame::UNDERFLOW_ARGS);
        uintN nmissing = nformal - args.argc();
        if (JS_UNLIKELY(!check(cx, space(), firstUnused, nmissing + nvals)))
            return NULL;
        SetValueRangeToUndefined(firstUnused, nmissing);
        return reinterpret_cast<StackFrame *>(firstUnused + nmissing);
    }

    *flags = StackFrame::Flags(*flags | StackFrame::OVERFLOW_ARGS);
    uintN ncopy = 2 + nformal;
    if (JS_UNLIKELY(!check(cx, space(), firstUnused, ncopy + nvals)))
        return NULL;

    Value *dst = firstUnused;
    Value *src = args.base();
    PodCopy(dst, src, ncopy);
    Debug_SetValueRangeToCrashOnTouch(src, ncopy);
    return reinterpret_cast<StackFrame *>(firstUnused + ncopy);
}

template <class Check>
JS_ALWAYS_INLINE bool
ContextStack::pushInlineFrame(JSContext *cx, FrameRegs &regs, const CallArgs &args,
                              JSObject &callee, JSFunction *fun, JSScript *script,
                              MaybeConstruct construct, Check check)
{
    JS_ASSERT(isCurrentAndActive());
    JS_ASSERT(&regs == &cx->regs());
    JS_ASSERT(regs.sp == args.end());
    
    JS_ASSERT(callee.getFunctionPrivate() == fun);
    JS_ASSERT(fun->script() == script);

    StackFrame::Flags flags = ToFrameFlags(construct);
    StackFrame *fp = getCallFrame(cx, args, fun, script, &flags, check);
    if (!fp)
        return false;

    
    fp->initCallFrame(cx, callee, fun, script, args.argc(), flags);
    regs.prepareToRun(fp, script);
    return true;
}

JS_ALWAYS_INLINE StackFrame *
ContextStack::getFixupFrame(JSContext *cx, FrameRegs &regs, const CallArgs &args,
                            JSFunction *fun, JSScript *script, void *ncode,
                            MaybeConstruct construct, LimitCheck check)
{
    JS_ASSERT(isCurrentAndActive());
    JS_ASSERT(&regs == &cx->regs());
    JS_ASSERT(regs.sp == args.end());
    JS_ASSERT(args.callee().getFunctionPrivate() == fun);
    JS_ASSERT(fun->script() == script);

    StackFrame::Flags flags = ToFrameFlags(construct);
    StackFrame *fp = getCallFrame(cx, args, fun, script, &flags, check);
    if (!fp)
        return NULL;

    
    fp->initJitFrameCallerHalf(cx, flags, ncode);
    fp->initJitFrameEarlyPrologue(fun, args.argc());
    return fp;
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

JS_ALWAYS_INLINE bool
ContextStack::pushInvokeFrame(JSContext *cx, const CallArgs &args, MaybeConstruct construct,
                              JSObject &callee, JSFunction *fun, JSScript *script,
                              InvokeFrameGuard *ifg)
{
    JS_ASSERT(callee == args.callee());
    JS_ASSERT(callee.getFunctionPrivate() == fun);
    JS_ASSERT(fun->script() == script);
    JS_ASSERT(args.end() == space().firstUnused());

    
    StackFrame::Flags flags = ToFrameFlags(construct);
    StackFrame *fp = getCallFrame(cx, args, fun, script, &flags, OOMCheck());
    if (!fp)
        return false;

    
    ifg->regs_.prepareToRun(fp, script);
    fp->initCallFrame(cx, callee, fun, script, args.argc(), flags);

    if (JS_UNLIKELY(space().seg_->empty())) {
        pushInvokeFrameSlow(ifg);
        return true;
    }

    
    ifg->prevRegs_ = regs_;
    regs_ = &ifg->regs_;

    
    ifg->stack_ = this;
    return true;
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
