








































#include "Ion.h"
#include "IonFrames.h"
#include "jsobj.h"
#include "jsscript.h"
#include "jsfun.h"
#include "IonCompartment.h"
#include "IonFrames-inl.h"
#include "Safepoints.h"
#include "IonSpewer.h"
#include "IonMacroAssembler.h"
#include "gc/Marking.h"
#include "SnapshotReader.h"
#include "Safepoints.h"

using namespace js;
using namespace js::ion;

JSScript *
ion::MaybeScriptFromCalleeToken(CalleeToken token)
{
    switch (GetCalleeTokenTag(token)) {
      case CalleeToken_Script:
        return CalleeTokenToScript(token);
      case CalleeToken_Function:
        return CalleeTokenToFunction(token)->script();
    }
    JS_NOT_REACHED("invalid callee token tag");
    return NULL;
}

FrameRecovery::FrameRecovery(uint8 *fp, uint8 *sp, const MachineState &machine)
  : fp_((IonJSFrameLayout *)fp),
    sp_(sp),
    machine_(machine),
    ionScript_(NULL)
{
    unpackCalleeToken(fp_->calleeToken());
}

void
FrameRecovery::unpackCalleeToken(CalleeToken token)
{
    switch (GetCalleeTokenTag(token)) {
      case CalleeToken_Function:
        callee_ = CalleeTokenToFunction(token);
        script_ = callee_->script();
        break;
      case CalleeToken_Script:
        callee_ = NULL;
        script_ = CalleeTokenToScript(token);
        break;
      default:
        JS_NOT_REACHED("invalid callee token tag");
    }
}

void
FrameRecovery::setIonScript(IonScript *ionScript)
{
    ionScript_ = ionScript;
}

void
FrameRecovery::setBailoutId(BailoutId bailoutId)
{
    snapshotOffset_ = ionScript()->bailoutToSnapshot(bailoutId);
}

FrameRecovery
FrameRecovery::FromBailoutId(uint8 *fp, uint8 *sp, const MachineState &machine,
                             BailoutId bailoutId)
{
    FrameRecovery frame(fp, sp, machine);
    frame.setBailoutId(bailoutId);
    return frame;
}

FrameRecovery
FrameRecovery::FromSnapshot(uint8 *fp, uint8 *sp, const MachineState &machine,
                            SnapshotOffset snapshotOffset)
{
    FrameRecovery frame(fp, sp, machine);
    frame.setSnapshotOffset(snapshotOffset);
    return frame;
}

IonScript *
FrameRecovery::ionScript() const
{
    return ionScript_ ? ionScript_ : script_->ion;
}

IonFrameIterator::IonFrameIterator(const IonActivationIterator &activations)
    : current_(activations.top()),
      type_(IonFrame_Exit),
      returnAddressToFp_(NULL),
      frameSize_(0),
      cachedSafepointIndex_(NULL),
      activation_(activations.activation())
{
}

IonFrameIterator::IonFrameIterator(IonJSFrameLayout *fp)
  : current_((uint8 *)fp),
    type_(IonFrame_JS),
    returnAddressToFp_(fp->returnAddress()),
    frameSize_(fp->prevFrameLocalSize())
{
}

bool
IonFrameIterator::checkInvalidation() const
{
    IonScript *dummy;
    return checkInvalidation(&dummy);
}

bool
IonFrameIterator::checkInvalidation(IonScript **ionScriptOut) const
{
    uint8 *returnAddr = returnAddressToFp();
    JSScript *script = this->script();
    
    
    IonScript *currentIonScript = script->ion;
    bool invalidated = !script->hasIonScript() ||
        !currentIonScript->containsReturnAddress(returnAddr);
    if (!invalidated)
        return false;

    int32 invalidationDataOffset = ((int32 *) returnAddr)[-1];
    uint8 *ionScriptDataOffset = returnAddr + invalidationDataOffset;
    IonScript *ionScript = (IonScript *) Assembler::getPointer(ionScriptDataOffset);
    JS_ASSERT(ionScript->containsReturnAddress(returnAddr));
    *ionScriptOut = ionScript;
    return true;
}

CalleeToken
IonFrameIterator::calleeToken() const
{
    return ((IonJSFrameLayout *) current_)->calleeToken();
}

JSFunction *
IonFrameIterator::callee() const
{
    JS_ASSERT(isFunctionFrame());
    return CalleeTokenToFunction(calleeToken());
}

JSFunction *
IonFrameIterator::maybeCallee() const
{
    if (isFunctionFrame())
        return callee();
    return NULL;
}

bool
IonFrameIterator::isFunctionFrame() const
{
    return js::ion::CalleeTokenIsFunction(calleeToken());
}

JSScript *
IonFrameIterator::script() const
{
    JS_ASSERT(isScripted());
    JSScript *script = MaybeScriptFromCalleeToken(calleeToken());
    JS_ASSERT(script);
    return script;
}

uint8 *
IonFrameIterator::prevFp() const
{
    JS_ASSERT(type_ != IonFrame_Entry);

    size_t currentSize = SizeOfFramePrefix(type_);
    
    
    
    if (prevType() == IonFrame_Bailed_Rectifier) {
        JS_ASSERT(type_ == IonFrame_Exit);
        currentSize = SizeOfFramePrefix(IonFrame_JS);
    }
    currentSize += current()->prevFrameLocalSize();
    return current_ + currentSize;
}

IonFrameIterator &
IonFrameIterator::operator++()
{
    JS_ASSERT(type_ != IonFrame_Entry);

    frameSize_ = prevFrameLocalSize();
    cachedSafepointIndex_ = NULL;

    
    
    if (current()->prevType() == IonFrame_Entry) {
        type_ = IonFrame_Entry;
        return *this;
    }

    
    
    uint8 *prev = prevFp();
    type_ = current()->prevType();
    returnAddressToFp_ = current()->returnAddress();
    current_ = prev;
    return *this;
}

MachineState
IonFrameIterator::machineState() const
{
    SafepointReader reader(ionScript(), safepoint());

    GeneralRegisterSet gcRegs = reader.gcSpills();
    GeneralRegisterSet allRegs = reader.allSpills();

    
    
    
    
    uintptr_t *spillBase = reinterpret_cast<uintptr_t *>(fp()) + ionScript()->frameSize();

    MachineState machine;
    for (GeneralRegisterIterator iter(allRegs); iter.more(); iter++, spillBase++) {
        Register reg = *iter;
        if (!gcRegs.has(reg))
            continue;
        machine.setRegisterLocation(reg, spillBase);
    }

    return machine;
}

static void
CloseLiveIterator(JSContext *cx, const InlineFrameIterator &frame, uint32 localSlot)
{
    SnapshotIterator si = frame.snapshotIterator();

    
    uint32 base = CountArgSlots(frame.maybeCallee()) + frame.script()->nfixed;
    uint32 skipSlots = base + localSlot - 1;

    for (unsigned i = 0; i < skipSlots; i++)
        si.skip();

    Value v = si.read();
    JSObject *obj = &v.toObject();

    if (cx->isExceptionPending())
        UnwindIteratorForUncatchableException(cx, obj);
    else
        UnwindIteratorForException(cx, obj);
}

static void
CloseLiveIterators(JSContext *cx, const InlineFrameIterator &frame)
{
    JSScript *script = frame.script();
    jsbytecode *pc = frame.pc();

    if (!JSScript::isValidOffset(script->trynotesOffset))
        return;

    JSTryNote *tn = script->trynotes()->vector;
    JSTryNote *tnEnd = tn + script->trynotes()->length;

    for (; tn != tnEnd; ++tn) {
        if (uint32(pc - script->code) < tn->start)
            continue;
        if (uint32(pc - script->code) >= tn->start + tn->length)
            continue;

        if (tn->kind != JSTRY_ITER)
            continue;

        JS_ASSERT(JSOp(*(script->code + tn->start + tn->length)) == JSOP_ENDITER);
        JS_ASSERT(tn->stackDepth > 0);

        uint32 localSlot = tn->stackDepth;
        CloseLiveIterator(cx, frame, localSlot);
    }
}

void
ion::HandleException(ResumeFromException *rfe)
{
    JSContext *cx = GetIonContext()->cx;

    IonSpew(IonSpew_Invalidate, "handling exception");

    IonFrameIterator iter(cx->runtime->ionTop);
    while (!iter.isEntry()) {
        if (iter.isScripted()) {
            
            
            InlineFrameIterator frames(&iter);
            for (;;) {
                CloseLiveIterators(cx, frames);
                if (!frames.more())
                    break;
                ++frames;
            }

            IonScript *ionScript;
            if (iter.checkInvalidation(&ionScript))
                ionScript->decref(cx->runtime->defaultFreeOp());
        }

        ++iter;
    }

    
    
    
    
    if (cx->runtime->hasIonReturnOverride())
        cx->runtime->takeIonReturnOverride();

    rfe->stackPointer = iter.fp();
}

IonActivationIterator::IonActivationIterator(JSContext *cx)
  : top_(cx->runtime->ionTop),
    activation_(cx->runtime->ionActivation)
{
}

IonActivationIterator::IonActivationIterator(JSRuntime *rt)
  : top_(rt->ionTop),
    activation_(rt->ionActivation)
{
}

IonActivationIterator &
IonActivationIterator::operator++()
{
    JS_ASSERT(activation_);
    top_ = activation_->prevIonTop();
    activation_ = activation_->prev();
    return *this;
}

bool
IonActivationIterator::more() const
{
    return !!activation_;
}

static void
MarkCalleeToken(JSTracer *trc, CalleeToken token)
{
    switch (GetCalleeTokenTag(token)) {
      case CalleeToken_Function:
      {
        JSFunction *fun = CalleeTokenToFunction(token);
        MarkObjectRoot(trc, &fun, "ion-callee");
        JS_ASSERT(fun == CalleeTokenToFunction(token));
        break;
      }
      case CalleeToken_Script:
      {
        JSScript *script = CalleeTokenToScript(token);
        MarkScriptRoot(trc, &script, "ion-entry");
        JS_ASSERT(script == CalleeTokenToScript(token));
        break;
      }
      default:
        JS_NOT_REACHED("unknown callee token type");
    }
}

static void
MarkIonJSFrame(JSTracer *trc, const IonFrameIterator &frame)
{
    IonJSFrameLayout *layout = (IonJSFrameLayout *)frame.fp();
    
    MarkCalleeToken(trc, layout->calleeToken());

    IonScript *ionScript;
    if (frame.checkInvalidation(&ionScript)) {
        
        
        
        IonScript::Trace(trc, ionScript);
    } else if (CalleeTokenIsFunction(layout->calleeToken())) {
        JSFunction *fun = CalleeTokenToFunction(layout->calleeToken());

        
        Value *argv = layout->argv();
        for (size_t i = 0; i < fun->nargs; i++)
            gc::MarkValueRoot(trc, &argv[i], "ion-argv");

        ionScript = fun->script()->ion;
    } else {
        ionScript = CalleeTokenToScript(layout->calleeToken())->ion;
    }

    const SafepointIndex *si = ionScript->getSafepointIndex(frame.returnAddressToFp());

    SafepointReader safepoint(ionScript, si);

    
    JS_ASSERT(safepoint.gcSpills().empty());
    JS_ASSERT(safepoint.allSpills().empty());

    
    
    uint32 slot;
    while (safepoint.getGcSlot(&slot)) {
        uintptr_t *ref = layout->slotRef(slot);
        gc::MarkThingOrValueRoot(trc, ref, "ion-gc-slot");
    }

    while (safepoint.getValueSlot(&slot)) {
        Value *v = (Value *)layout->slotRef(slot);
        gc::MarkValueRoot(trc, v, "ion-gc-slot");
    }
}

static void
MarkIonActivation(JSTracer *trc, uint8 *top)
{
    for (IonFrameIterator frames(top); !frames.done(); ++frames) {
        switch (frames.type()) {
          case IonFrame_Exit:
            
            break;
          case IonFrame_JS:
            MarkIonJSFrame(trc, frames);
            break;
          case IonFrame_Rectifier:
            
            
            
            break;
          default:
            JS_NOT_REACHED("unexpected frame type");
            break;
        }
    }
}

void
ion::MarkIonActivations(JSRuntime *rt, JSTracer *trc)
{
    for (IonActivationIterator activations(rt); activations.more(); ++activations)
        MarkIonActivation(trc, activations.top());
}

void
ion::GetPcScript(JSContext *cx, JSScript **scriptRes, jsbytecode **pcRes)
{
#ifdef DEBUG
    
    bool enableBailoutSpew = false;
    if (IonSpewEnabled(IonSpew_Bailouts)) {
        enableBailoutSpew = true;
        DisableChannel(IonSpew_Bailouts);
    }
#endif

    JS_ASSERT(cx->fp()->runningInIon());
    IonSpew(IonSpew_Snapshots, "Recover PC & Script from the last frame.");

    
    IonFrameIterator it(cx->runtime->ionTop);
    ++it;
    InlineFrameIterator ifi(&it);

    
    *scriptRes = ifi.script();
    if (pcRes)
        *pcRes = ifi.pc();

#ifdef DEBUG
    if (enableBailoutSpew)
        EnableChannel(IonSpew_Bailouts);
#endif
}

void
OsiIndex::fixUpOffset(MacroAssembler &masm)
{
    callPointDisplacement_ = masm.actualOffset(callPointDisplacement_);
}

uint32
OsiIndex::returnPointDisplacement() const
{
    
    
    
    return callPointDisplacement_ + Assembler::patchWrite_NearCallSize();
}

SnapshotIterator::SnapshotIterator(IonScript *ionScript, SnapshotOffset snapshotOffset,
                                   IonJSFrameLayout *fp, const MachineState &machine)
  : SnapshotReader(ionScript->snapshots() + snapshotOffset,
                   ionScript->snapshots() + ionScript->snapshotsSize()),
    fp_(fp),
    machine_(machine),
    ionScript_(ionScript)
{
    JS_ASSERT(snapshotOffset < ionScript->snapshotsSize());
}

SnapshotIterator::SnapshotIterator(const IonFrameIterator &iter)
  : SnapshotReader(iter.ionScript()->snapshots() + iter.osiIndex()->snapshotOffset(),
                   iter.ionScript()->snapshots() + iter.ionScript()->snapshotsSize()),
    fp_(iter.jsFrame()),
    machine_(iter.machineState()),
    ionScript_(iter.ionScript())
{
}

SnapshotIterator::SnapshotIterator()
  : SnapshotReader(NULL, NULL),
    fp_(NULL),
    ionScript_(NULL)
{
}

bool
SnapshotIterator::hasLocation(const SnapshotReader::Location &loc)
{
    return loc.isStackSlot() && machine_.has(loc.reg());
}

uintptr_t
SnapshotIterator::fromLocation(const SnapshotReader::Location &loc)
{
    if (loc.isStackSlot())
        return ReadFrameSlot(fp_, loc.stackSlot());
    return machine_.read(loc.reg());
}

Value
SnapshotIterator::FromTypedPayload(JSValueType type, uintptr_t payload)
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

bool
SnapshotIterator::slotReadable(const Slot &slot)
{
    switch (slot.mode()) {
      case SnapshotReader::DOUBLE_REG:
        return machine_.has(slot.floatReg());

      case SnapshotReader::TYPED_REG:
        return machine_.has(slot.reg());

      case SnapshotReader::UNTYPED:
#if defined(JS_NUNBOX32)
          return hasLocation(slot.type()) && hasLocation(slot.payload());
#elif defined(JS_PUNBOX64)
          return hasLocation(slot.value());
#endif

      default:
        return true;
    }
}

Value
SnapshotIterator::slotValue(const Slot &slot)
{
    switch (slot.mode()) {
      case SnapshotReader::DOUBLE_REG:
        return DoubleValue(machine_.read(slot.floatReg()));

      case SnapshotReader::TYPED_REG:
        return FromTypedPayload(slot.knownType(), machine_.read(slot.reg()));

      case SnapshotReader::TYPED_STACK:
      {
        JSValueType type = slot.knownType();
        if (type == JSVAL_TYPE_DOUBLE)
            return DoubleValue(ReadFrameDoubleSlot(fp_, slot.stackSlot()));
        return FromTypedPayload(type, ReadFrameSlot(fp_, slot.stackSlot()));
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

IonScript *
IonFrameIterator::ionScript() const
{
    JS_ASSERT(type() == IonFrame_JS);

    IonScript *ionScript;
    if (checkInvalidation(&ionScript))
        return ionScript;
    return script()->ionScript();
}

const SafepointIndex *
IonFrameIterator::safepoint() const
{
    if (!cachedSafepointIndex_)
        cachedSafepointIndex_ = ionScript()->getSafepointIndex(returnAddressToFp());
    return cachedSafepointIndex_;
}

const OsiIndex *
IonFrameIterator::osiIndex() const
{
    SafepointReader reader(ionScript(), safepoint());
    return ionScript()->getOsiIndex(reader.osiReturnPointOffset());
}

InlineFrameIterator::InlineFrameIterator(const IonFrameIterator *iter)
  : frame_(iter),
    framesRead_(0),
    callee_(NULL),
    script_(NULL)
{
    if (frame_) {
        start_ = SnapshotIterator(*frame_);
        findNextFrame();
    }
}

void
InlineFrameIterator::findNextFrame()
{
    JS_ASSERT(more());

    si_ = start_;

    
    callee_ = frame_->maybeCallee();
    script_ = frame_->script();
    pc_ = script_->code + si_.pcOffset();

    
    
    unsigned remaining = start_.frameCount() - framesRead_ - 1;
    for (unsigned i = 0; i < remaining; i++) {
        JS_ASSERT(js_CodeSpec[*pc_].format & JOF_INVOKE);

        
        unsigned skipCount = (si_.slots() - 1) - GET_ARGC(pc_) - 1;
        for (unsigned j = 0; j < skipCount; j++)
            si_.skip();

        Value funval = si_.read();

        
        while (si_.moreSlots())
            si_.skip();

        si_.nextFrame();

        callee_ = funval.toObject().toFunction();
        script_ = callee_->script();
        pc_ = script_->code + si_.pcOffset();
    }

    framesRead_++;
}

InlineFrameIterator
InlineFrameIterator::operator++()
{
    InlineFrameIterator iter(*this);
    findNextFrame();
    return iter;
}

bool
InlineFrameIterator::isFunctionFrame() const
{
    return !!callee_;
}

MachineState
MachineState::FromBailout(uintptr_t regs[Registers::Total],
                          double fpregs[FloatRegisters::Total])
{
    MachineState machine;

    for (unsigned i = 0; i < Registers::Total; i++)
        machine.setRegisterLocation(Register::FromCode(i), &regs[i]);
    for (unsigned i = 0; i < FloatRegisters::Total; i++)
        machine.setRegisterLocation(FloatRegister::FromCode(i), &fpregs[i]);

    return machine;
}

bool
InlineFrameIterator::isConstructing() const
{
    
    if (more()) {
        InlineFrameIterator parent(*this);
        ++parent;

        
        JS_ASSERT(js_CodeSpec[*parent.pc()].format & JOF_INVOKE);

        return (JSOp)*parent.pc() == JSOP_NEW;
    }

    return frame_->isConstructing();
}

bool
IonFrameIterator::isConstructing() const
{
    IonFrameIterator parent(*this);

    
    do {
        ++parent;
    } while (!parent.done() && !parent.isScripted());

    if (parent.isScripted()) {
        
        InlineFrameIterator inlinedParent(&parent);
        JS_ASSERT(js_CodeSpec[*inlinedParent.pc()].format & JOF_INVOKE);

        return (JSOp)*inlinedParent.pc() == JSOP_NEW;
    }

    JS_ASSERT(parent.done());
    return activation_->entryfp()->isConstructing();
}

JSObject *
InlineFrameIterator::thisObject() const
{
    
    SnapshotIterator s(si_);

    
    s.skip();

    
    
    Value v = s.read();
    JS_ASSERT(v.isObject());
    return &v.toObject();
}

unsigned
InlineFrameIterator::numActualArgs() const
{
    
    if (more()) {
        InlineFrameIterator parent(*this);
        ++parent;

        
        JS_ASSERT(js_CodeSpec[*parent.pc()].format & JOF_INVOKE);

        return GET_ARGC(parent.pc());
    }

    return frame_->numActualArgs();
}

unsigned
IonFrameIterator::numActualArgs() const
{
    IonFrameIterator parent(*this);

    
    do {
        ++parent;
    } while (!parent.done() && !parent.isScripted());

    if (parent.isScripted()) {
        
        InlineFrameIterator inlinedParent(&parent);
        JS_ASSERT(js_CodeSpec[*inlinedParent.pc()].format & JOF_INVOKE);

        return GET_ARGC(inlinedParent.pc());
    }

    JS_ASSERT(parent.done());
    return activation_->entryfp()->numActualArgs();
}
