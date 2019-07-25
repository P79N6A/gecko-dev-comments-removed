






































#ifndef jsinterpinlines_h__
#define jsinterpinlines_h__

inline void
JSStackFrame::initCallFrame(JSContext *cx, JSObject &callee, JSFunction *fun,
                            uint32 nactual, uint32 flagsArg)
{
    JS_ASSERT((flagsArg & ~(JSFRAME_CONSTRUCTING |
                            JSFRAME_OVERFLOW_ARGS |
                            JSFRAME_UNDERFLOW_ARGS)) == 0);
    JS_ASSERT(fun == callee.getFunctionPrivate());

    
    flags_ = JSFRAME_FUNCTION | JSFRAME_HAS_PREVPC | JSFRAME_HAS_SCOPECHAIN | flagsArg;
    exec.fun = fun;
    args.nactual = nactual;  
    scopeChain_ = callee.getParent();
    
    JS_ASSERT(!hasImacropc());
    JS_ASSERT(!hasHookData());
    JS_ASSERT(annotation() == NULL);

    JS_ASSERT(!hasCallObj());
}

inline void
JSStackFrame::initCallFrameCallerHalf(JSContext *cx, uint32 nactual, uint32 flagsArg)
{
    JS_ASSERT((flagsArg & ~(JSFRAME_CONSTRUCTING |
                            JSFRAME_FUNCTION |
                            JSFRAME_OVERFLOW_ARGS |
                            JSFRAME_UNDERFLOW_ARGS)) == 0);
    JSFrameRegs *regs = cx->regs;

    
    flags_ = JSFRAME_FUNCTION | flagsArg;
    args.nactual = nactual;  
    prev_ = regs->fp;
    JS_ASSERT(!hasImacropc());
    JS_ASSERT(!hasHookData());
    JS_ASSERT(annotation() == NULL);

    JS_ASSERT(!hasCallObj());
}





inline void
JSStackFrame::initCallFrameEarlyPrologue(JSFunction *fun, void *ncode)
{
    
    exec.fun = fun;
    ncode_ = ncode;
}





inline void
JSStackFrame::initCallFrameLatePrologue()
{
    SetValueRangeToUndefined(slots(), script()->nfixed);
}

inline void
JSStackFrame::initEvalFrame(JSScript *script, JSStackFrame *prev,
                            jsbytecode *prevpc, uint32 flagsArg)
{
    JS_ASSERT(flagsArg & JSFRAME_EVAL);
    JS_ASSERT((flagsArg & ~(JSFRAME_EVAL | JSFRAME_DEBUGGER)) == 0);
    JS_ASSERT(prev->flags_ & (JSFRAME_FUNCTION | JSFRAME_GLOBAL));

    
    js::Value *dstvp = (js::Value *)this - 2;
    js::Value *srcvp = prev->flags_ & (JSFRAME_GLOBAL | JSFRAME_EVAL)
                       ? (js::Value *)prev - 2
                       : prev->formalArgs() - 2;
    dstvp[0] = srcvp[0];
    dstvp[1] = srcvp[1];
    JS_ASSERT_IF(prev->flags_ & JSFRAME_FUNCTION,
                 dstvp[0].toObject().isFunction());

    
    flags_ = flagsArg | JSFRAME_HAS_PREVPC | JSFRAME_HAS_SCOPECHAIN |
             (prev->flags_ & (JSFRAME_FUNCTION |
                              JSFRAME_GLOBAL |
                              JSFRAME_HAS_CALL_OBJ));
    if (isFunctionFrame()) {
        exec = prev->exec;
        args.script = script;
    } else {
        exec.script = script;
    }
    scopeChain_ = &prev->scopeChain();
    JS_ASSERT_IF(isFunctionFrame(), &callObj() == &prev->callObj());

    setPrev(prev, prevpc);
    JS_ASSERT(!hasImacropc());
    JS_ASSERT(!hasHookData());
    setAnnotation(prev->annotation());
}

inline void
JSStackFrame::initGlobalFrame(JSScript *script, JSObject &chain, uint32 flagsArg)
{
    JS_ASSERT((flagsArg & ~(JSFRAME_EVAL | JSFRAME_DEBUGGER)) == 0);

    
    js::Value *vp = (js::Value *)this - 2;
    vp[0].setUndefined();
    vp[1].setUndefined();  

    
    flags_ = flagsArg | JSFRAME_GLOBAL | JSFRAME_HAS_PREVPC | JSFRAME_HAS_SCOPECHAIN;
    exec.script = script;
    args.script = (JSScript *)0xbad;
    scopeChain_ = &chain;

    prev_ = NULL;
    JS_ASSERT(!hasImacropc());
    JS_ASSERT(!hasHookData());
    JS_ASSERT(annotation() == NULL);
}

inline void
JSStackFrame::initDummyFrame(JSContext *cx, JSObject &chain)
{
    js::PodZero(this);
    flags_ = JSFRAME_DUMMY | JSFRAME_HAS_PREVPC | JSFRAME_HAS_SCOPECHAIN;
    setPrev(cx->regs);
    chain.isGlobal();
    setScopeChainNoCallObj(chain);
}

inline void
JSStackFrame::stealFrameAndSlots(js::Value *vp, JSStackFrame *otherfp,
                                 js::Value *othervp, js::Value *othersp)
{
    JS_ASSERT(vp == (js::Value *)this - (otherfp->formalArgsEnd() - othervp));
    JS_ASSERT(othervp == otherfp->actualArgs() - 2);
    JS_ASSERT(othersp >= otherfp->slots());
    JS_ASSERT(othersp <= otherfp->base() + otherfp->numSlots());

    size_t nbytes = (othersp - othervp) * sizeof(js::Value);
    memcpy(vp, othervp, nbytes);
    JS_ASSERT(vp == actualArgs() - 2);

    





    if (hasCallObj()) {
        callObj().setPrivate(this);
        otherfp->flags_ &= ~JSFRAME_HAS_CALL_OBJ;
    }
    if (hasArgsObj()) {
        argsObj().setPrivate(this);
        otherfp->flags_ &= ~JSFRAME_HAS_ARGS_OBJ;
    }
}

inline js::Value &
JSStackFrame::canonicalActualArg(uintN i) const
{
    if (i < numFormalArgs())
        return formalArg(i);
    JS_ASSERT(i < numActualArgs());
    return actualArgs()[i];
}

template <class Op>
inline void
JSStackFrame::forEachCanonicalActualArg(Op op)
{
    uintN nformal = fun()->nargs;
    js::Value *formals = formalArgsEnd() - nformal;
    uintN nactual = numActualArgs();
    if (nactual <= nformal) {
        uintN i = 0;
        js::Value *actualsEnd = formals + nactual;
        for (js::Value *p = formals; p != actualsEnd; ++p, ++i)
            op(i, p);
    } else {
        uintN i = 0;
        js::Value *formalsEnd = formalArgsEnd();
        for (js::Value *p = formals; p != formalsEnd; ++p, ++i)
            op(i, p);
        js::Value *actuals = formalsEnd - (nactual + 2);
        js::Value *actualsEnd = formals - 2;
        for (js::Value *p = actuals; p != actualsEnd; ++p, ++i)
            op(i, p);
    }
}

template <class Op>
inline void
JSStackFrame::forEachFormalArg(Op op)
{
    js::Value *formals = formalArgsEnd() - fun()->nargs;
    js::Value *formalsEnd = formalArgsEnd();
    uintN i = 0;
    for (js::Value *p = formals; p != formalsEnd; ++p, ++i)
        op(i, p);
}

inline JSObject *
JSStackFrame::computeThisObject(JSContext *cx)
{
    js::Value &thisv = thisValue();
    if (JS_LIKELY(!thisv.isPrimitive()))
        return &thisv.toObject();
    if (!js::ComputeThisFromArgv(cx, &thisv + 1))
        return NULL;
    JS_ASSERT(IsSaneThisObject(thisv.toObject()));
    return &thisv.toObject();
}

inline JSObject &
JSStackFrame::varobj(js::StackSegment *seg) const
{
    JS_ASSERT(seg->contains(this));
    return isFunctionFrame() ? callObj() : seg->getInitialVarObj();
}

inline JSObject &
JSStackFrame::varobj(JSContext *cx) const
{
    JS_ASSERT(cx->activeSegment()->contains(this));
    return isFunctionFrame() ? callObj() : cx->activeSegment()->getInitialVarObj();
}

inline uintN
JSStackFrame::numActualArgs() const
{
    JS_ASSERT(isFunctionFrame() && !isEvalFrame());
    if (JS_UNLIKELY(flags_ & (JSFRAME_OVERFLOW_ARGS | JSFRAME_UNDERFLOW_ARGS)))
        return hasArgsObj() ? argsObj().getArgsInitialLength() : args.nactual;
    return numFormalArgs();
}

inline js::Value *
JSStackFrame::actualArgs() const
{
    JS_ASSERT(isFunctionFrame() && !isEvalFrame());
    js::Value *argv = formalArgsEnd() - numFormalArgs();
    if (JS_UNLIKELY(flags_ & JSFRAME_OVERFLOW_ARGS)) {
        uintN nactual = hasArgsObj() ? argsObj().getArgsInitialLength() : args.nactual;
        return argv - (2 + nactual);
    }
    return argv;
}

inline js::Value *
JSStackFrame::actualArgsEnd() const
{
    JS_ASSERT(isFunctionFrame() && !isEvalFrame());
    if (JS_UNLIKELY(flags_ & JSFRAME_OVERFLOW_ARGS))
        return formalArgsEnd() - (2 + numFormalArgs());
    uintN argc = numActualArgs();
    uintN nmissing = numFormalArgs() - argc;
    return formalArgsEnd() - nmissing;
}

inline void
JSStackFrame::setArgsObj(JSObject &obj)
{
    JS_ASSERT_IF(hasArgsObj(), &obj == args.obj);
    JS_ASSERT_IF(!hasArgsObj(), numActualArgs() == obj.getArgsInitialLength());
    args.obj = &obj;
    flags_ |= JSFRAME_HAS_ARGS_OBJ;
}

inline void
JSStackFrame::clearArgsObj()
{
    JS_ASSERT(hasArgsObj());
    args.nactual = args.obj->getArgsInitialLength();
    flags_ ^= JSFRAME_HAS_ARGS_OBJ;
}

inline void
JSStackFrame::setScopeChainNoCallObj(JSObject &obj)
{
#ifdef DEBUG
    JS_ASSERT(&obj != NULL);
    JSObject *callObjBefore = maybeCallObj();
    if (!hasCallObj() && &scopeChain() != sInvalidScopeChain) {
        for (JSObject *pobj = &scopeChain(); pobj; pobj = pobj->getParent())
            JS_ASSERT_IF(pobj->isCall(), pobj->getPrivate() != this);
    }
#endif
    scopeChain_ = &obj;
    flags_ |= JSFRAME_HAS_SCOPECHAIN;
    JS_ASSERT(callObjBefore == maybeCallObj());
}

inline void
JSStackFrame::setScopeChainAndCallObj(JSObject &obj)
{
    JS_ASSERT(&obj != NULL);
    JS_ASSERT(!hasCallObj() && obj.isCall() && obj.getPrivate() == this);
    scopeChain_ = &obj;
    flags_ |= JSFRAME_HAS_SCOPECHAIN | JSFRAME_HAS_CALL_OBJ;
}

inline void
JSStackFrame::clearCallObj()
{
    JS_ASSERT(hasCallObj());
    flags_ ^= JSFRAME_HAS_CALL_OBJ;
}

inline JSObject &
JSStackFrame::callObj() const
{
    JS_ASSERT(hasCallObj());
    JSObject *pobj = &scopeChain();
    while (JS_UNLIKELY(pobj->getClass() != &js_CallClass)) {
        JS_ASSERT(js_IsCacheableNonGlobalScope(pobj) || pobj->isWith());
        pobj = pobj->getParent();
    }
    return *pobj;
}

inline JSObject *
JSStackFrame::maybeCallObj() const
{
    return hasCallObj() ? &callObj() : NULL;
}

namespace js {

inline void
PutActivationObjects(JSContext *cx, JSStackFrame *fp)
{
    JS_ASSERT(fp->isFunctionFrame() && !fp->isEvalFrame());

    
    if (fp->hasCallObj()) {
        js_PutCallObject(cx, fp);
    } else if (fp->hasArgsObj()) {
        js_PutArgsObject(cx, fp);
    }
}

}

#endif 
