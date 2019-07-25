








































#include "jscntxt.h"
#include "jscompartment.h"
#include "Bailouts.h"
#include "Snapshots.h"
#include "Ion.h"
#include "IonCompartment.h"

using namespace js;
using namespace js::ion;

class IonFrameIterator
{
    IonScript *ionScript_;
    BailoutEnvironment *env_;
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
            return env_->readSlot(loc.stackSlot());
        return env_->readReg(loc.reg());
    }

  public:
    IonFrameIterator(IonScript *ionScript, BailoutEnvironment *env, const uint8 *start, const uint8 *end)
      : ionScript_(ionScript),
        env_(env),
        reader_(start, end)
    {
    }

    Value read() {
        SnapshotReader::Slot slot = reader_.readSlot();
        switch (slot.mode()) {
          case SnapshotReader::DOUBLE_REG:
            return DoubleValue(env_->readFloatReg(slot.floatReg()));

          case SnapshotReader::TYPED_REG:
            return FromTypedPayload(slot.knownType(), env_->readReg(slot.reg()));

          case SnapshotReader::TYPED_STACK:
          {
            JSValueType type = slot.knownType();
            if (type == JSVAL_TYPE_DOUBLE)
                return DoubleValue(env_->readDoubleSlot(slot.stackSlot()));
            return FromTypedPayload(type, env_->readSlot(slot.stackSlot()));
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
            return ionScript_->getConstant(slot.constantIndex());

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

    bool nextFrame() {
        reader_.finishReading();
        return false;
    }
};

static void
RestoreOneFrame(JSContext *cx, StackFrame *fp, IonFrameIterator &iter)
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
ConvertFrames(JSContext *cx, IonActivation *activation, BailoutEnvironment *env)
{
    IonFramePrefix *top = env->top();

    
    JSScript *script;
    IonScript *ionScript;
    JSFunction *fun = NULL;
    JSObject *callee = NULL;
    if (IsCalleeTokenFunction(top->calleeToken())) {
        callee = CalleeTokenToFunction(top->calleeToken());
        fun = callee->getFunctionPrivate();
        script = fun->script();
    } else {
        script = CalleeTokenToScript(top->calleeToken());
    }
    ionScript = script->ion;

    
    uint32 snapshotOffset;
    if (env->frameClass() != FrameSizeClass::None()) {
        BailoutId id = env->bailoutId();
        snapshotOffset = ionScript->bailoutToSnapshot(id);
    } else {
        snapshotOffset = env->snapshotOffset();
    }

    JS_ASSERT(snapshotOffset < ionScript->snapshotsSize());
    const uint8 *start = ionScript->snapshots() + snapshotOffset;
    const uint8 *end = ionScript->snapshots() + ionScript->snapshotsSize();
    IonFrameIterator iter(ionScript, env, start, end);

    
    
    
    
    cx->stack.repointRegs(&activation->oldFrameRegs());

    BailoutClosure *br = cx->new_<BailoutClosure>();
    if (!br)
        return false;
    activation->setBailout(br);

    
    
    JS_ASSERT(callee);

    StackFrame *fp = cx->stack.pushBailoutFrame(cx, callee, fun, script, br->frameGuard());
    if (!fp)
        return false;

    br->setEntryFrame(fp);

    if (callee)
        fp->formalArgs()[-2].setObject(*callee);

    for (;;) {
        RestoreOneFrame(cx, fp, iter);
        if (!iter.nextFrame())
            break;

        
        JS_NOT_REACHED("NYI");
    }

    return true;
}

uint32
ion::Bailout(void **sp)
{
    JSContext *cx = GetIonContext()->cx;
    IonCompartment *ioncompartment = cx->compartment->ionCompartment();
    IonActivation *activation = ioncompartment->activation();
    BailoutEnvironment env(ioncompartment, sp);

    if (!ConvertFrames(cx, activation, &env))
        return BAILOUT_RETURN_FATAL_ERROR;

    return BAILOUT_RETURN_OK;
}

JSBool
ion::ThunkToInterpreter(IonFramePrefix *top, Value *vp)
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

uint32
ion::HandleException(IonFramePrefix *top)
{
    JSContext *cx = GetIonContext()->cx;
    IonCompartment *ioncompartment = cx->compartment->ionCompartment();

    
    JS_ASSERT(top->isEntryFrame());

    
    
    if (BailoutClosure *closure = ioncompartment->activation()->maybeTakeBailout())
        cx->delete_(closure);

    top->setReturnAddress(ioncompartment->returnError()->raw());
    return 0;
}

