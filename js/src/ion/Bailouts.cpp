








































#include "jscntxt.h"
#include "jscompartment.h"
#include "jsinterp.h"
#include "Bailouts.h"
#include "Snapshots.h"
#include "Ion.h"
#include "IonCompartment.h"
#include "IonSpewer.h"
#include "jsinfer.h"
#include "jsanalyze.h"
#include "jsinferinlines.h"
#include "IonFrames-inl.h"

using namespace js;
using namespace js::ion;

class IonBailoutIterator
{
    IonScript *ionScript_;
    FrameRecovery &in_;
    SnapshotReader reader_;

    static Value FromTypedPayload(JSValueType type, uintptr_t payload)
    {
        switch (type) {
          case JSVAL_TYPE_INT32:
            return Int32Value(payload);
          case JSVAL_TYPE_BOOLEAN:
            return BooleanValue(!!payload);
          case JSVAL_TYPE_STRING:
            return StringValue(reinterpret_cast<JSString *>(payload));
          case JSVAL_TYPE_OBJECT:
            return ObjectValue(*reinterpret_cast<JSObject *>(payload));
          default:
            JS_NOT_REACHED("unexpected type - needs payload");
            return UndefinedValue();
        }
    }

    uintptr_t fromLocation(const SnapshotReader::Location &loc) {
        if (loc.isStackSlot())
            return in_.readSlot(loc.stackSlot());
        return in_.machine().readReg(loc.reg());
    }

  public:
    IonBailoutIterator(FrameRecovery &in, const uint8 *start, const uint8 *end)
      : in_(in),
        reader_(start, end)
    {
    }

    Value readBogus() {
        reader_.readSlot();
        return UndefinedValue();
    }

    Value read() {
        SnapshotReader::Slot slot = reader_.readSlot();
        switch (slot.mode()) {
          case SnapshotReader::DOUBLE_REG:
            return DoubleValue(in_.machine().readFloatReg(slot.floatReg()));

          case SnapshotReader::TYPED_REG:
            return FromTypedPayload(slot.knownType(), in_.machine().readReg(slot.reg()));

          case SnapshotReader::TYPED_STACK:
          {
            JSValueType type = slot.knownType();
            if (type == JSVAL_TYPE_DOUBLE)
                return DoubleValue(in_.readDoubleSlot(slot.stackSlot()));
            return FromTypedPayload(type, in_.readSlot(slot.stackSlot()));
          }

          case SnapshotReader::UNTYPED:
          {
              jsval_layout layout;
#if defined(JS_NUNBOX32)
              layout.s.tag = (JSValueTag)fromLocation(slot.type());
              layout.s.payload.word = fromLocation(slot.payload());
#elif defined(JS_PUNBOX64)
              layout.asBits = fromLocation(slot.value());
#endif
              return IMPL_TO_JSVAL(layout);
          }

          case SnapshotReader::JS_UNDEFINED:
            return UndefinedValue();

          case SnapshotReader::JS_NULL:
            return NullValue();

          case SnapshotReader::JS_INT32:
            return Int32Value(slot.int32Value());

          case SnapshotReader::CONSTANT:
            return in_.ionScript()->getConstant(slot.constantIndex());

          default:
            JS_NOT_REACHED("huh?");
            return UndefinedValue();
        }
    }

    uint32 slots() const {
        return reader_.slots();
    }
    uint32 pcOffset() const {
        return reader_.pcOffset();
    }
    BailoutKind bailoutKind() const {
        return reader_.bailoutKind();
    }
    bool resumeAfter() const {
        if (hasNextFrame())
            return false;
        return reader_.resumeAfter();
    }
    bool hasNextFrame() const {
        return reader_.remainingFrameCount() > 1;
    }
    bool nextFrame() {
        reader_.finishReadingFrame();
        return reader_.remainingFrameCount() > 0;
    }
};

static void
RestoreOneFrame(JSContext *cx, StackFrame *fp, IonBailoutIterator &iter)
{
    uint32 exprStackSlots = iter.slots() - fp->script()->nfixed;

    IonSpew(IonSpew_Bailouts, " expr stack slots %u, is function frame %u",
            exprStackSlots, fp->isFunctionFrame());

    
    
    
    
    Value scopeChainv = iter.read();
    if (scopeChainv.isObject()) {
        
        
        
        if (iter.bailoutKind() != Bailout_ArgumentCheck)
            fp->setScopeChainNoCallObj(scopeChainv.toObject());
        else
            scopeChainv = ObjectValue(*fp->fun()->environment());
    } else {
        JS_ASSERT(scopeChainv.isUndefined());
    }

    if (fp->isFunctionFrame()) {
        Value thisv = iter.read();
        fp->formalArgs()[-1] = thisv;

        JS_ASSERT(iter.slots() >= CountArgSlots(fp->fun()));
        IonSpew(IonSpew_Bailouts, " frame slots %u, nargs %u, nfixed %u",
                iter.slots(), fp->fun()->nargs, fp->script()->nfixed);

        for (uint32 i = 0; i < fp->fun()->nargs; i++) {
            Value arg = iter.read();
            fp->formalArgs()[i] = arg;
        }
    }
    exprStackSlots -= CountArgSlots(fp->maybeFun());

    for (uint32 i = 0; i < fp->script()->nfixed; i++) {
        Value slot = iter.read();
        fp->slots()[i] = slot;
    }

    IonSpew(IonSpew_Bailouts, " pushing %u expression stack slots", exprStackSlots);
    FrameRegs &regs = cx->regs();
    for (uint32 i = 0; i < exprStackSlots; i++) {
        Value v;

        
        
        
        
        
        if (!iter.hasNextFrame() && i == exprStackSlots - 1 && cx->runtime->hasIonReturnOverride())
            v = iter.readBogus();
        else
            v = iter.read();

        *regs.sp++ = v;
    }
    uintN pcOff = iter.pcOffset();
    regs.pc = fp->script()->code + pcOff;

    if (iter.resumeAfter())
        regs.pc = GetNextPc(regs.pc);

    IonSpew(IonSpew_Bailouts, " new PC is offset %u within script %p (line %d)",
            pcOff, (void *)fp->script(), js_PCToLineNumber(cx, fp->script(), regs.pc));
    JS_ASSERT(exprStackSlots == js_ReconstructStackDepth(cx, fp->script(), regs.pc));
}

static StackFrame *
PushInlinedFrame(JSContext *cx, StackFrame *callerFrame)
{
    
    
    
    
    FrameRegs &regs = cx->regs();
    JS_ASSERT(JSOp(*regs.pc) == JSOP_CALL);
    int callerArgc = GET_ARGC(regs.pc);
    const Value &calleeVal = regs.sp[-callerArgc - 2];

    JSFunction *fun = calleeVal.toObject().toFunction();
    JSScript *script = fun->script();
    CallArgs inlineArgs = CallArgsFromArgv(fun->nargs, regs.sp - callerArgc);
    
    
    
    regs.sp = inlineArgs.end();

    if (!cx->stack.pushInlineFrame(cx, regs, inlineArgs, *fun, script, INITIAL_NONE))
        return NULL;

    StackFrame *fp = cx->stack.fp();
    JS_ASSERT(fp == regs.fp());
    JS_ASSERT(fp->prev() == callerFrame);
    
    fp->formalArgs()[-2].setObject(*fun);

    return fp;
}

static uint32
ConvertFrames(JSContext *cx, IonActivation *activation, FrameRecovery &in)
{
    IonSpew(IonSpew_Bailouts, "Bailing out %s:%u, IonScript %p",
            in.script()->filename, in.script()->lineno, (void *) in.ionScript());
    IonSpew(IonSpew_Bailouts, " reading from snapshot offset %u size %u",
            in.snapshotOffset(), in.ionScript()->snapshotsSize());

    JS_ASSERT(in.snapshotOffset() < in.ionScript()->snapshotsSize());
    const uint8 *start = in.ionScript()->snapshots() + in.snapshotOffset();
    const uint8 *end = in.ionScript()->snapshots() + in.ionScript()->snapshotsSize();
    IonBailoutIterator iter(in, start, end);

    
    in.ionScript()->forbidOsr();

    BailoutClosure *br = cx->new_<BailoutClosure>();
    if (!br)
        return BAILOUT_RETURN_FATAL_ERROR;
    activation->setBailout(br);

    StackFrame *fp;
    if (in.callee()) {
        
        fp = cx->stack.pushBailoutFrame(cx, *in.callee(), in.script(), br->frameGuard());
    } else {
        
        
        
        
        JSObject *prevScopeChain = &cx->fp()->scopeChain();
        Value thisv = cx->fp()->thisValue();
        fp = cx->stack.pushBailoutFrame(cx, in.script(), *prevScopeChain, thisv, br->frameGuard());
    }

    if (!fp)
        return BAILOUT_RETURN_FATAL_ERROR;

    br->setEntryFrame(fp);

    if (in.callee())
        fp->formalArgs()[-2].setObject(*in.callee());

    for (size_t i = 0;; ++i) {
        IonSpew(IonSpew_Bailouts, " restoring frame %u (lower is older)", i);
        RestoreOneFrame(cx, fp, iter);
        if (!iter.nextFrame())
            break;

        fp = PushInlinedFrame(cx, fp);
        if (!fp)
            return BAILOUT_RETURN_FATAL_ERROR;
    }

    jsbytecode *bailoutPc = fp->script()->code + iter.pcOffset();
    br->setBailoutPc(bailoutPc);

    switch (iter.bailoutKind()) {
      case Bailout_Normal:
        return BAILOUT_RETURN_OK;
      case Bailout_TypeBarrier:
        return BAILOUT_RETURN_TYPE_BARRIER;
      case Bailout_ArgumentCheck:
        return BAILOUT_RETURN_ARGUMENT_CHECK;
      case Bailout_Monitor:
        return BAILOUT_RETURN_MONITOR;
      case Bailout_RecompileCheck:
        return BAILOUT_RETURN_RECOMPILE_CHECK;
    }

    JS_NOT_REACHED("bad bailout kind");
    return BAILOUT_RETURN_FATAL_ERROR;
}

static inline void
EnsureExitFrame(IonCommonFrameLayout *frame)
{
    if (frame->prevType() == IonFrame_Entry) {
        
        
        return;
    }

    if (frame->prevType() == IonFrame_Rectifier) {
        
        
        
        
        frame->changePrevType(IonFrame_Bailed_Rectifier);
        return;
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    uint32 callerFrameSize = frame->prevFrameLocalSize() +
        IonJSFrameLayout::Size() - IonExitFrameLayout::Size();
    frame->setFrameDescriptor(callerFrameSize, frame->prevType());
}

uint32
ion::Bailout(BailoutStack *sp)
{
    JSContext *cx = GetIonContext()->cx;
    IonCompartment *ioncompartment = cx->compartment->ionCompartment();
    IonActivation *activation = cx->runtime->ionActivation;
    FrameRecovery in = FrameRecoveryFromBailout(ioncompartment, sp);

    IonSpew(IonSpew_Bailouts, "Took bailout! Snapshot offset: %d", in.snapshotOffset());

    uint32 retval = ConvertFrames(cx, activation, in);

    EnsureExitFrame(in.fp());

    if (retval != BAILOUT_RETURN_FATAL_ERROR)
        return retval;

    cx->delete_(activation->maybeTakeBailout());
    return BAILOUT_RETURN_FATAL_ERROR;
}

uint32
ion::InvalidationBailout(InvalidationBailoutStack *sp, size_t *frameSizeOut)
{
    sp->checkInvariants();

    JSContext *cx = GetIonContext()->cx;
    IonCompartment *ioncompartment = cx->compartment->ionCompartment();
    IonActivation *activation = cx->runtime->ionActivation;
    FrameRecovery in = FrameRecoveryFromInvalidation(ioncompartment, sp);

    IonSpew(IonSpew_Bailouts, "Took invalidation bailout! Snapshot offset: %d", in.snapshotOffset());

    
    *frameSizeOut = in.frameSize();

    uint32 retval = ConvertFrames(cx, activation, in);

    {
        IonJSFrameLayout *frame = in.fp();
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

    
    
    
    jsbytecode *pc = activation->bailout()->bailoutPc();
    if (js_CodeSpec[*pc].format & JOF_TYPESET)
        types::TypeScript::Monitor(cx, cx->fp()->script(), pc, cx->regs().sp[-1]);

    in.ionScript()->decref(cx);

    if (cx->runtime->hasIonReturnOverride())
        cx->regs().sp[-1] = cx->runtime->takeIonReturnOverride();

    if (retval != BAILOUT_RETURN_FATAL_ERROR)
        return retval;

    cx->delete_(activation->maybeTakeBailout());
    return BAILOUT_RETURN_FATAL_ERROR;
}

static void
ReflowArgTypes(JSContext *cx)
{
    StackFrame *fp = cx->fp();
    uintN nargs = fp->fun()->nargs;
    JSScript *script = fp->script();

    types::AutoEnterTypeInference enter(cx);

    if (!fp->isConstructing())
        types::TypeScript::SetThis(cx, script, fp->thisValue());
    for (uintN i = 0; i < nargs; ++i)
        types::TypeScript::SetArgument(cx, script, i, fp->formalArg(i));
}

uint32
ion::ReflowTypeInfo(uint32 bailoutResult)
{
    JSContext *cx = GetIonContext()->cx;
    IonActivation *activation = cx->runtime->ionActivation;

    IonSpew(IonSpew_Bailouts, "reflowing type info");

    if (bailoutResult == BAILOUT_RETURN_ARGUMENT_CHECK) {
        IonSpew(IonSpew_Bailouts, "reflowing type info at argument-checked entry");
        ReflowArgTypes(cx);
        return true;
    }

    JSScript *script = cx->fp()->script();
    jsbytecode *pc = activation->bailout()->bailoutPc();

    JS_ASSERT(script->hasAnalysis());
    JS_ASSERT(script->analysis()->ranInference());

    JS_ASSERT(js_CodeSpec[*pc].format & JOF_TYPESET);

    IonSpew(IonSpew_Bailouts, "reflowing type info at %s:%d pcoff %d", script->filename,
            script->lineno, pc - script->code);

    types::AutoEnterTypeInference enter(cx);
    if (bailoutResult == BAILOUT_RETURN_TYPE_BARRIER)
        script->analysis()->breakTypeBarriers(cx, pc - script->code, false);
    else
        JS_ASSERT(bailoutResult == BAILOUT_RETURN_MONITOR);

    
    Value &result = cx->regs().sp[-1];
    types::TypeScript::Monitor(cx, script, pc, result);

    return true;
}

uint32
ion::RecompileForInlining()
{
    JSContext *cx = GetIonContext()->cx;
    JSScript *script = cx->fp()->script();

    IonSpew(IonSpew_Inlining, "Recompiling script to inline calls %s:%d", script->filename,
            script->lineno);

    
    Vector<types::RecompileInfo> scripts(cx);
    if (!scripts.append(types::RecompileInfo(script)))
        return BAILOUT_RETURN_FATAL_ERROR;

    Invalidate(cx, scripts,  false);

    
    JS_ASSERT(script->getUseCount() >= js_IonOptions.usesBeforeInlining);

    return true;
}

JSBool
ion::ThunkToInterpreter(Value *vp)
{
    JSContext *cx = GetIonContext()->cx;
    IonActivation *activation = cx->runtime->ionActivation;
    BailoutClosure *br = activation->takeBailout();

    bool ok = Interpret(cx, br->entryfp(), JSINTERP_BAILOUT);

    if (ok)
        *vp = br->entryfp()->returnValue();

    
    
    cx->delete_(br);

    return ok ? JS_TRUE : JS_FALSE;
}
