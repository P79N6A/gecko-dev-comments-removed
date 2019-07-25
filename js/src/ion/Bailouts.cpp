








































#include "jscntxt.h"
#include "jscompartment.h"
#include "jsinterp.h"
#include "Bailouts.h"
#include "Snapshots.h"
#include "Ion.h"
#include "IonCompartment.h"
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
        reader_.finishReading();
        return false;
    }
};

static void
RestoreOneFrame(JSContext *cx, StackFrame *fp, IonBailoutIterator &iter)
{
    uint32 exprStackSlots = iter.slots() - fp->script()->nfixed;

    if (fp->isFunctionFrame()) {
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

    FrameRegs &regs = cx->regs();
    for (uint32 i = 0; i < exprStackSlots; i++) {
        Value v = iter.read();
        *regs.sp++ = v;
    }
    regs.pc = fp->script()->code + iter.pcOffset();
}

static bool
ConvertFrames(JSContext *cx, IonActivation *activation, FrameRecovery &in)
{
    JS_ASSERT(in.snapshotOffset() < in.ionScript()->snapshotsSize());
    const uint8 *start = in.ionScript()->snapshots() + in.snapshotOffset();
    const uint8 *end = in.ionScript()->snapshots() + in.ionScript()->snapshotsSize();
    IonBailoutIterator iter(in, start, end);

    
    
    
    
    cx->stack.repointRegs(&activation->oldFrameRegs());

    BailoutClosure *br = cx->new_<BailoutClosure>();
    if (!br)
        return false;
    activation->setBailout(br);

    
    
    JS_ASSERT(in.callee());

    StackFrame *fp = cx->stack.pushBailoutFrame(cx, in.callee(), in.fun(), in.script(),
                                                br->frameGuard());
    if (!fp)
        return false;

    br->setEntryFrame(fp);

    if (in.callee())
        fp->formalArgs()[-2].setObject(*in.callee());

    for (;;) {
        RestoreOneFrame(cx, fp, iter);
        if (!iter.nextFrame())
            break;

        
        JS_NOT_REACHED("NYI");
    }

    switch (iter.bailoutKind()) {
      case Bailout_Normal:
        break;

      case Bailout_TypeBarrier:
      {
        JSScript *script = cx->fp()->script();
        if (script->hasAnalysis() && script->analysis()->ranInference()) {
            types::AutoEnterTypeInference enter(cx);
            script->analysis()->breakTypeBarriers(cx, cx->regs().pc - script->code, false);
        }

        
        Value &result = cx->regs().sp[-1];
        types::TypeScript::Monitor(cx, script, cx->regs().pc, result);
        break;
      }
    }

    return true;
}

uint32
ion::Bailout(BailoutStack *sp)
{
    JSContext *cx = GetIonContext()->cx;
    IonCompartment *ioncompartment = cx->compartment->ionCompartment();
    IonActivation *activation = ioncompartment->activation();
    FrameRecovery in = FrameRecoveryFromBailout(ioncompartment, sp);

    if (!ConvertFrames(cx, activation, in)) {
        if (BailoutClosure *br = activation->maybeTakeBailout())
            cx->delete_(br);
        return BAILOUT_RETURN_FATAL_ERROR;
    }

    return BAILOUT_RETURN_OK;
}

JSBool
ion::ThunkToInterpreter(Value *vp)
{
    JSContext *cx = GetIonContext()->cx;
    IonActivation *activation = cx->compartment->ionCompartment()->activation();
    BailoutClosure *br = activation->takeBailout();

    bool ok = Interpret(cx, br->entryfp(), JSINTERP_BAILOUT);

    if (ok)
        *vp = br->entryfp()->returnValue();

    
    
    cx->delete_(br);

    JS_ASSERT(&cx->regs() == &activation->oldFrameRegs());
    cx->stack.repointRegs(NULL);

    return ok ? JS_TRUE : JS_FALSE;
}

