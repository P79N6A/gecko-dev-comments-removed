






#include "jscntxt.h"
#include "jscompartment.h"
#include "jsinterp.h"
#include "Bailouts.h"
#include "SnapshotReader.h"
#include "Ion.h"
#include "IonCompartment.h"
#include "IonSpewer.h"
#include "jsinfer.h"
#include "jsanalyze.h"
#include "jsinferinlines.h"
#include "IonFrames-inl.h"

using namespace js;
using namespace js::ion;

















SnapshotIterator::SnapshotIterator(const IonBailoutIterator &iter)
  : SnapshotReader(iter.ionScript()->snapshots() + iter.snapshotOffset(),
                   iter.ionScript()->snapshots() + iter.ionScript()->snapshotsSize()),
    fp_(iter.jsFrame()),
    machine_(iter.machineState()),
    ionScript_(iter.ionScript())
{
}

void
IonBailoutIterator::dump() const
{
    if (type_ == IonFrame_OptimizedJS) {
        InlineFrameIterator frames(GetIonContext()->cx, this);
        for (;;) {
            frames.dump();
            if (!frames.more())
                break;
            ++frames;
        }
    } else {
        IonFrameIterator::dump();
    }
}

static RawScript
GetBailedJSScript(JSContext *cx)
{
    
    
    IonJSFrameLayout *frame = reinterpret_cast<IonJSFrameLayout*>(cx->mainThread().ionTop);
    switch (GetCalleeTokenTag(frame->calleeToken())) {
      case CalleeToken_Function: {
        JSFunction *fun = CalleeTokenToFunction(frame->calleeToken());
        return fun->nonLazyScript();
      }
      case CalleeToken_Script:
        return CalleeTokenToScript(frame->calleeToken());
      default:
        JS_NOT_REACHED("unexpected callee token kind");
        return NULL;
    }
}

void
StackFrame::initFromBailout(JSContext *cx, SnapshotIterator &iter)
{
    uint32_t exprStackSlots = iter.slots() - script()->nfixed;

#ifdef TRACK_SNAPSHOTS
    iter.spewBailingFrom();
#endif
    IonSpew(IonSpew_Bailouts, " expr stack slots %u, is function frame %u",
            exprStackSlots, isFunctionFrame());

    if (iter.bailoutKind() == Bailout_ArgumentCheck) {
        
        
        
        iter.skip();
        flags_ &= ~StackFrame::HAS_SCOPECHAIN;
    } else {
        Value v = iter.read();
        if (v.isObject()) {
            scopeChain_ = &v.toObject();
            flags_ |= StackFrame::HAS_SCOPECHAIN;
            if (isFunctionFrame() && fun()->isHeavyweight())
                flags_ |= StackFrame::HAS_CALL_OBJ;
        } else {
            JS_ASSERT(v.isUndefined());
        }
    }

    
    
    
    if (cx->runtime->spsProfiler.enabled())
        setPushedSPSFrame();

    if (isFunctionFrame()) {
        Value thisv = iter.read();
        formals()[-1] = thisv;

        
        
        if (isConstructing())
            JS_ASSERT(!thisv.isPrimitive());

        JS_ASSERT(iter.slots() >= CountArgSlots(fun()));
        IonSpew(IonSpew_Bailouts, " frame slots %u, nargs %u, nfixed %u",
                iter.slots(), fun()->nargs, script()->nfixed);

        for (uint32_t i = 0; i < fun()->nargs; i++) {
            Value arg = iter.read();
            formals()[i] = arg;
        }
    }
    exprStackSlots -= CountArgSlots(maybeFun());

    for (uint32_t i = 0; i < script()->nfixed; i++) {
        Value slot = iter.read();
        slots()[i] = slot;
    }

    IonSpew(IonSpew_Bailouts, " pushing %u expression stack slots", exprStackSlots);
    FrameRegs &regs = cx->regs();
    for (uint32_t i = 0; i < exprStackSlots; i++) {
        Value v;

        
        
        
        if (!iter.moreFrames() && i == exprStackSlots - 1 && cx->runtime->hasIonReturnOverride())
            v = iter.skip();
        else
            v = iter.read();

        *regs.sp++ = v;
    }
    unsigned pcOff = iter.pcOffset();
    regs.pc = script()->code + pcOff;

    if (iter.resumeAfter())
        regs.pc = GetNextPc(regs.pc);

    IonSpew(IonSpew_Bailouts, " new PC is offset %u within script %p (line %d)",
            pcOff, (void *)script(), PCToLineNumber(script(), regs.pc));

    
    
    
    JS_ASSERT_IF(JSOp(*regs.pc) != JSOP_FUNAPPLY,
                 exprStackSlots == js_ReconstructStackDepth(cx, script(), regs.pc));
}

static StackFrame *
PushInlinedFrame(JSContext *cx, StackFrame *callerFrame)
{
    
    
    
    
    FrameRegs &regs = cx->regs();
    JS_ASSERT(js_CodeSpec[*regs.pc].format & JOF_INVOKE);
    int callerArgc = GET_ARGC(regs.pc);
    if (JSOp(*regs.pc) == JSOP_FUNAPPLY)
        callerArgc = callerFrame->nactual();
    const Value &calleeVal = regs.sp[-callerArgc - 2];

    RootedFunction fun(cx, calleeVal.toObject().toFunction());
    RootedScript script(cx, fun->nonLazyScript());
    CallArgs inlineArgs = CallArgsFromSp(callerArgc, regs.sp);

    
    
    regs.sp = inlineArgs.end();

    InitialFrameFlags flags = INITIAL_NONE;
    if (JSOp(*regs.pc) == JSOP_NEW)
        flags = INITIAL_CONSTRUCT;

    if (!cx->stack.pushInlineFrame(cx, regs, inlineArgs, fun, script, flags, DONT_REPORT_ERROR))
        return NULL;

    StackFrame *fp = cx->stack.fp();
    JS_ASSERT(fp == regs.fp());
    JS_ASSERT(fp->prev() == callerFrame);

    fp->formals()[-2].setObject(*fun);

    return fp;
}

static uint32_t
ConvertFrames(JSContext *cx, IonActivation *activation, IonBailoutIterator &it)
{
    IonSpew(IonSpew_Bailouts, "Bailing out %s:%u, IonScript %p",
            it.script()->filename(), it.script()->lineno, (void *) it.ionScript());
    IonSpew(IonSpew_Bailouts, " reading from snapshot offset %u size %u",
            it.snapshotOffset(), it.ionScript()->snapshotsSize());
#ifdef DEBUG
    
    
    
    
    if (it.script()->ion == it.ionScript()) {
        IonSpew(IonSpew_Bailouts, " Current script use count is %u",
                it.script()->getUseCount());
    }
#endif

    
    
    it.ionScript()->setBailoutExpected();

    
    
    
    
    BailoutClosure *br = js_new<BailoutClosure>();
    if (!br)
        return BAILOUT_RETURN_FATAL_ERROR;
    activation->setBailout(br);

    StackFrame *fp;
    if (it.isEntryJSFrame() && cx->fp()->runningInIon() && activation->entryfp()) {
        
        
        
        
        
        
        
        
        
        JS_ASSERT(cx->fp() == activation->entryfp());
        fp = cx->fp();
        cx->regs().sp = fp->base();
    } else {
        br->constructFrame();
        if (!cx->stack.pushBailoutArgs(cx, it, br->argsGuard()))
            return BAILOUT_RETURN_FATAL_ERROR;

        fp = cx->stack.pushBailoutFrame(cx, it, *br->argsGuard(), br->frameGuard());
    }

    if (!fp)
        return BAILOUT_RETURN_OVERRECURSED;

    br->setEntryFrame(fp);

    JSFunction *callee = it.maybeCallee();
    if (callee)
        fp->formals()[-2].setObject(*callee);

    if (it.isConstructing())
        fp->setConstructing();

    SnapshotIterator iter(it);

    while (true) {
        IonSpew(IonSpew_Bailouts, " restoring frame");
        fp->initFromBailout(cx, iter);

        if (!iter.moreFrames())
             break;
        iter.nextFrame();

        fp = PushInlinedFrame(cx, fp);
        if (!fp)
            return BAILOUT_RETURN_OVERRECURSED;
    }

    fp->clearRunningInIon();

    jsbytecode *bailoutPc = fp->script()->code + iter.pcOffset();
    br->setBailoutPc(bailoutPc);

    switch (iter.bailoutKind()) {
      case Bailout_Normal:
        return BAILOUT_RETURN_OK;
      case Bailout_TypeBarrier:
        return BAILOUT_RETURN_TYPE_BARRIER;
      case Bailout_Monitor:
        return BAILOUT_RETURN_MONITOR;
      case Bailout_BoundsCheck:
        return BAILOUT_RETURN_BOUNDS_CHECK;
      case Bailout_ShapeGuard:
        return BAILOUT_RETURN_SHAPE_GUARD;
      case Bailout_CachedShapeGuard:
        return BAILOUT_RETURN_CACHED_SHAPE_GUARD;

      
      
      
      
      case Bailout_ArgumentCheck:
        fp->unsetPushedSPSFrame();
        Probes::enterScript(cx, fp->script(), fp->script()->function(), fp);
        return BAILOUT_RETURN_ARGUMENT_CHECK;
    }

    JS_NOT_REACHED("bad bailout kind");
    return BAILOUT_RETURN_FATAL_ERROR;
}

uint32_t
ion::Bailout(BailoutStack *sp)
{
    JSContext *cx = GetIonContext()->cx;
    
    cx->mainThread().ionTop = NULL;
    IonActivationIterator ionActivations(cx);
    IonBailoutIterator iter(ionActivations, sp);
    IonActivation *activation = ionActivations.activation();

    
    
    

    IonSpew(IonSpew_Bailouts, "Took bailout! Snapshot offset: %d", iter.snapshotOffset());

    uint32_t retval = ConvertFrames(cx, activation, iter);

    EnsureExitFrame(iter.jsFrame());
    return retval;
}

uint32_t
ion::InvalidationBailout(InvalidationBailoutStack *sp, size_t *frameSizeOut)
{
    sp->checkInvariants();

    JSContext *cx = GetIonContext()->cx;

    
    cx->mainThread().ionTop = NULL;
    IonActivationIterator ionActivations(cx);
    IonBailoutIterator iter(ionActivations, sp);
    IonActivation *activation = ionActivations.activation();

    IonSpew(IonSpew_Bailouts, "Took invalidation bailout! Snapshot offset: %d", iter.snapshotOffset());

    
    *frameSizeOut = iter.topFrameSize();

    uint32_t retval = ConvertFrames(cx, activation, iter);

    {
        IonJSFrameLayout *frame = iter.jsFrame();
        IonSpew(IonSpew_Invalidate, "converting to exit frame");
        IonSpew(IonSpew_Invalidate, "   orig calleeToken %p", (void *) frame->calleeToken());
        IonSpew(IonSpew_Invalidate, "   orig frameSize %u", unsigned(frame->prevFrameLocalSize()));
        IonSpew(IonSpew_Invalidate, "   orig ra %p", (void *) frame->returnAddress());

        frame->replaceCalleeToken(NULL);
        EnsureExitFrame(frame);

        IonSpew(IonSpew_Invalidate, "   new  calleeToken %p", (void *) frame->calleeToken());
        IonSpew(IonSpew_Invalidate, "   new  frameSize %u", unsigned(frame->prevFrameLocalSize()));
        IonSpew(IonSpew_Invalidate, "   new  ra %p", (void *) frame->returnAddress());
    }

    iter.ionScript()->decref(cx->runtime->defaultFreeOp());

    if (cx->runtime->hasIonReturnOverride())
        cx->regs().sp[-1] = cx->runtime->takeIonReturnOverride();

    if (retval != BAILOUT_RETURN_FATAL_ERROR) {
        
        
        
        jsbytecode *pc = activation->bailout()->bailoutPc();

        
        
        bool isResumeAfter = GetNextPc(pc) == cx->regs().pc;

        if ((js_CodeSpec[*pc].format & JOF_TYPESET) && isResumeAfter) {
            JS_ASSERT(retval == BAILOUT_RETURN_OK);
            return BAILOUT_RETURN_MONITOR;
        }

        return retval;
    }

    return BAILOUT_RETURN_FATAL_ERROR;
}

static void
ReflowArgTypes(JSContext *cx)
{
    StackFrame *fp = cx->fp();
    unsigned nargs = fp->fun()->nargs;
    RootedScript script(cx, fp->script());

    types::AutoEnterAnalysis enter(cx);

    if (!fp->isConstructing())
        types::TypeScript::SetThis(cx, script, fp->thisValue());
    for (unsigned i = 0; i < nargs; ++i)
        types::TypeScript::SetArgument(cx, script, i, fp->unaliasedFormal(i, DONT_CHECK_ALIASING));
}

uint32_t
ion::ReflowTypeInfo(uint32_t bailoutResult)
{
    JSContext *cx = GetIonContext()->cx;
    IonActivation *activation = cx->mainThread().ionActivation;

    IonSpew(IonSpew_Bailouts, "reflowing type info");

    if (bailoutResult == BAILOUT_RETURN_ARGUMENT_CHECK) {
        IonSpew(IonSpew_Bailouts, "reflowing type info at argument-checked entry");
        ReflowArgTypes(cx);
        return true;
    }

    RootedScript script(cx, cx->fp()->script());
    jsbytecode *pc = activation->bailout()->bailoutPc();

    JS_ASSERT(js_CodeSpec[*pc].format & JOF_TYPESET);

    IonSpew(IonSpew_Bailouts, "reflowing type info at %s:%d pcoff %d", script->filename(),
            script->lineno, pc - script->code);

    types::AutoEnterAnalysis enter(cx);
    if (bailoutResult == BAILOUT_RETURN_TYPE_BARRIER)
        script->analysis()->breakTypeBarriers(cx, pc - script->code, false);
    else
        JS_ASSERT(bailoutResult == BAILOUT_RETURN_MONITOR);

    
    Value &result = cx->regs().sp[-1];
    types::TypeScript::Monitor(cx, script, pc, result);

    return true;
}


bool
ion::EnsureHasScopeObjects(JSContext *cx, StackFrame *fp)
{
    if (fp->isFunctionFrame() &&
        fp->fun()->isHeavyweight() &&
        !fp->hasCallObj())
    {
        return fp->initFunctionScopeObjects(cx);
    }
    return true;
}

uint32_t
ion::BoundsCheckFailure()
{
    JSContext *cx = GetIonContext()->cx;
    RawScript script = GetBailedJSScript(cx);

    IonSpew(IonSpew_Bailouts, "Bounds check failure %s:%d", script->filename(),
            script->lineno);

    if (!script->failedBoundsCheck) {
        script->failedBoundsCheck = true;

        
        IonSpew(IonSpew_Invalidate, "Invalidating due to bounds check failure");

        return Invalidate(cx, script);
    }

    return true;
}

uint32_t
ion::ShapeGuardFailure()
{
    JSContext *cx = GetIonContext()->cx;
    RawScript script = GetBailedJSScript(cx);

    JS_ASSERT(script->hasIonScript());
    JS_ASSERT(!script->ion->invalidated());

    script->failedShapeGuard = true;

    IonSpew(IonSpew_Invalidate, "Invalidating due to shape guard failure");

    return Invalidate(cx, script);
}

uint32_t
ion::CachedShapeGuardFailure()
{
    JSContext *cx = GetIonContext()->cx;
    RawScript script = GetBailedJSScript(cx);

    JS_ASSERT(script->hasIonScript());
    JS_ASSERT(!script->ion->invalidated());

    script->failedShapeGuard = true;

    
    
    for (size_t i = 0; i < script->ion->scriptEntries(); i++)
        mjit::PurgeCaches(script->ion->getScript(i));

    IonSpew(IonSpew_Invalidate, "Invalidating due to shape guard failure");

    return Invalidate(cx, script);
}

uint32_t
ion::ThunkToInterpreter(Value *vp)
{
    JSContext *cx = GetIonContext()->cx;
    IonActivation *activation = cx->mainThread().ionActivation;
    BailoutClosure *br = activation->takeBailout();
    InterpMode resumeMode = JSINTERP_BAILOUT;

    if (!EnsureHasScopeObjects(cx, cx->fp()))
        resumeMode = JSINTERP_RETHROW;

    
    
    
    
    
    
    jsbytecode *pc = cx->regs().pc;
    while (JSOp(*pc) == JSOP_GOTO)
        pc += GET_JUMP_OFFSET(pc);
    if (JSOp(*pc) == JSOP_LOOPENTRY)
        cx->regs().pc = GetNextPc(pc);

    
    
    
    
    
    {
        ScriptFrameIter iter(cx);
        StackFrame *fp = NULL;
        Rooted<JSScript*> script(cx);
        do {
            fp = iter.interpFrame();
            script = iter.script();
            if (script->needsArgsObj()) {
                
                
                
                JS_ASSERT(!fp->hasArgsObj());
                ArgumentsObject *argsobj = ArgumentsObject::createExpected(cx, fp);
                if (!argsobj) {
                    resumeMode = JSINTERP_RETHROW;
                    break;
                }
                
                
                
                
                SetFrameArgumentsObject(cx, fp, script, argsobj);
            }
            ++iter;
        } while (fp != br->entryfp());
    }

    if (activation->entryfp() == br->entryfp()) {
        
        
        
        
        vp->setMagic(JS_ION_BAILOUT);
        js_delete(br);
        return resumeMode == JSINTERP_RETHROW ? Interpret_Error : Interpret_Ok;
    }

    InterpretStatus status = Interpret(cx, br->entryfp(), resumeMode);
    JS_ASSERT_IF(resumeMode == JSINTERP_RETHROW, status == Interpret_Error);

    if (status == Interpret_OSR) {
        
        
        JS_NOT_REACHED("invalid");

        IonSpew(IonSpew_Bailouts, "Performing inline OSR %s:%d",
                cx->fp()->script()->filename(),
                PCToLineNumber(cx->fp()->script(), cx->regs().pc));

        
        
        
        
        
        
        
        StackFrame *fp = cx->fp();

        fp->setRunningInIon();
        vp->setPrivate(fp);
        js_delete(br);
        return Interpret_OSR;
    }

    if (status == Interpret_Ok)
        *vp = br->entryfp()->returnValue();

    
    
    js_delete(br);

    return status;
}

