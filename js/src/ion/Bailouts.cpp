








































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

    bool nextFrame() {
        reader_.finishReadingFrame();
        return reader_.remainingFrameCount() > 0;
    }
};

static void
RestoreOneFrame(JSContext *cx, StackFrame *fp, IonBailoutIterator &iter)
{
    uint32 exprStackSlots = iter.slots() - fp->script()->nfixed;

    IonSpew(IonSpew_Bailouts, "expr stack slots %u, is function frame %u",
            exprStackSlots, fp->isFunctionFrame());
    if (fp->isFunctionFrame()) {
        JS_ASSERT(iter.slots() >= fp->fun()->nargs + 1U);
        IonSpew(IonSpew_Bailouts, "frame slots %u, nargs %u, nfixed %u",
                iter.slots(), fp->fun()->nargs, fp->script()->nfixed);
        Value thisv = iter.read();
        fp->formalArgs()[-1] = thisv;

        for (uint32 i = 0; i < fp->fun()->nargs; i++) {
            Value arg = iter.read();
            fp->formalArgs()[i] = arg;
        }

        exprStackSlots -= (fp->fun()->nargs + 1);
    }

    for (uint32 i = 0; i < fp->script()->nfixed; i++) {
        Value slot = iter.read();
        fp->slots()[i] = slot;
    }

    IonSpew(IonSpew_Bailouts, " pushing %u expression stack slots", exprStackSlots);
    FrameRegs &regs = cx->regs();
    for (uint32 i = 0; i < exprStackSlots; i++) {
        Value v = iter.read();
        *regs.sp++ = v;
    }
    uintN pcOff = iter.pcOffset();
    regs.pc = fp->script()->code + pcOff;

    IonSpew(IonSpew_Bailouts, " new PC is offset %u within script %p",
            pcOff, (void *) fp->script());
    JS_ASSERT(exprStackSlots == js_ReconstructStackDepth(cx, fp->script(), regs.pc));
}

static StackFrame *
PushInlinedFrame(JSContext *cx, StackFrame *callerFrame)
{
    
    
    
    
    FrameRegs &regs = cx->regs();
    JS_ASSERT(JSOp(*regs.pc) == JSOP_CALL);
    uintN callerArgc = GET_ARGC(regs.pc);
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

    
    
    JS_ASSERT(in.callee());

    StackFrame *fp = cx->stack.pushBailoutFrame(cx, *in.callee(), in.script(), br->frameGuard());
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

    switch (iter.bailoutKind()) {
      case Bailout_Normal:
        return BAILOUT_RETURN_OK;
      case Bailout_TypeBarrier:
        return BAILOUT_RETURN_TYPE_BARRIER;
      case Bailout_ArgumentCheck:
        return BAILOUT_RETURN_ARGUMENT_CHECK;
    }

    return true;
}

uint32
ion::Bailout(BailoutStack *sp)
{
    JSContext *cx = GetIonContext()->cx;
    IonCompartment *ioncompartment = cx->compartment->ionCompartment();
    IonActivation *activation = cx->threadData()->ionActivation;
    FrameRecovery in = FrameRecoveryFromBailout(ioncompartment, sp);

    uint32 retval = ConvertFrames(cx, activation, in);
    if (retval != BAILOUT_RETURN_FATAL_ERROR)
        return retval;

    cx->delete_(activation->maybeTakeBailout());
    return BAILOUT_RETURN_FATAL_ERROR;
}

uint32
ion::InvalidationBailout(InvalidationBailoutStack *sp, size_t *frameSizeOut)
{
    JSContext *cx = GetIonContext()->cx;
    IonCompartment *ioncompartment = cx->compartment->ionCompartment();
    IonActivation *activation = cx->threadData()->ionActivation;
    FrameRecovery in = FrameRecoveryFromInvalidation(ioncompartment, sp);

    
    *frameSizeOut = in.frameSize();

    uint32 retval = ConvertFrames(cx, activation, in);

    
    
    
    {
        IonJSFrameLayout *frame = in.fp();
        IonSpew(IonSpew_Invalidate, "converting to exit frame");
        IonSpew(IonSpew_Invalidate, "   orig calleeToken %p", (void *) frame->calleeToken());
        IonSpew(IonSpew_Invalidate, "   orig frameSize %u", unsigned(frame->prevFrameLocalSize()));
        IonSpew(IonSpew_Invalidate, "   orig ra %p", (void *) frame->returnAddress());

        InvalidationRecord *record = CalleeTokenToInvalidationRecord(frame->calleeToken());
        Foreground::delete_<InvalidationRecord>(record);
        
        frame->replaceCalleeToken(InvalidationRecordToToken(NULL));

        uint32 callerFrameSize = frame->prevFrameLocalSize() +
                                 sizeof(IonJSFrameLayout) - sizeof(IonExitFrameLayout);
        frame->setFrameDescriptor(callerFrameSize, IonFrame_Exit);

        IonSpew(IonSpew_Invalidate, "   new  calleeToken %p", (void *) frame->calleeToken());
        IonSpew(IonSpew_Invalidate, "   new  frameSize %u", unsigned(frame->prevFrameLocalSize()));
        IonSpew(IonSpew_Invalidate, "   new  ra %p", (void *) frame->returnAddress());
    }

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
    IonActivation *activation = cx->threadData()->ionActivation;

    if (bailoutResult == BAILOUT_RETURN_ARGUMENT_CHECK) {
        IonSpew(IonSpew_Bailouts, "reflowing type info at argument-checked entry");
        ReflowArgTypes(cx);
        return !activation->failedInvalidation();
    }

    JSScript *script = cx->fp()->script();
    jsbytecode *pc = cx->regs().pc;
    IonSpew(IonSpew_Bailouts, "reflowing type info at %s:%d pcoff %d", script->filename,
            script->lineno, pc - script->code);
    if (script->hasAnalysis() && script->analysis()->ranInference()) {
        types::AutoEnterTypeInference enter(cx);
        script->analysis()->breakTypeBarriers(cx, pc - script->code, false);
    }

    
    Value &result = cx->regs().sp[-1];
    types::TypeScript::Monitor(cx, script, pc, result);

    return !activation->failedInvalidation();
}

JSBool
ion::ThunkToInterpreter(Value *vp)
{
    JSContext *cx = GetIonContext()->cx;
    IonActivation *activation = JS_THREAD_DATA(cx)->ionActivation;
    BailoutClosure *br = activation->takeBailout();

    bool ok = Interpret(cx, br->entryfp(), JSINTERP_BAILOUT);

    if (ok)
        *vp = br->entryfp()->returnValue();

    
    
    cx->delete_(br);

    return ok ? JS_TRUE : JS_FALSE;
}
