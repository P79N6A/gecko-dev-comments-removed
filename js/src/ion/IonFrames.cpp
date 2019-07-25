








































#include "Ion.h"
#include "IonFrames.h"
#include "jsobj.h"
#include "jsscript.h"
#include "jsfun.h"
#include "IonCompartment.h"
#include "IonFrames-inl.h"
#include "Safepoints.h"
#include "IonSpewer.h"

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

int32
FrameRecovery::OffsetOfSlot(int32 slot)
{
    if (slot <= 0)
        return sizeof(IonJSFrameLayout) + -slot;
    return -(slot * STACK_SLOT_SIZE);
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

FrameRecovery
FrameRecovery::FromFrameIterator(const IonFrameIterator& it)
{
    MachineState noRegs;
    FrameRecovery frame(it.prevFp(), it.prevFp() - it.prevFrameLocalSize(), noRegs);

    
    
    IonScript *ionScript = frame.ionScript();
    const SafepointIndex *si = ionScript->getSafepointIndex(it.returnAddress());
    SafepointReader reader(ionScript, si);
    uint32 osiReturnDisplacement = reader.getOsiReturnPointOffset();
    const OsiIndex *oi = ionScript->getOsiIndex(osiReturnDisplacement);

    frame.setSnapshotOffset(oi->snapshotOffset());
    return frame;
}

IonScript *
FrameRecovery::ionScript() const
{
    return ionScript_ ? ionScript_ : script_->ion;
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
    bool invalidated = !currentIonScript || !currentIonScript->containsCodeAddress(returnAddr);
    if (!invalidated)
        return false;

    int32 invalidationDataOffset = ((int32 *) returnAddr)[-1];
    uint8 *ionScriptDataOffset = returnAddr + invalidationDataOffset;
    IonScript *ionScript = (IonScript *) ((uintptr_t *) ionScriptDataOffset)[-1];
    JS_ASSERT(ionScript->containsCodeAddress(returnAddr));
    *ionScriptOut = ionScript;
    return true;
}

CalleeToken
IonFrameIterator::calleeToken() const
{
    JS_ASSERT(type_ == IonFrame_JS);
    return ((IonJSFrameLayout *) current_)->calleeToken();
}

bool
IonFrameIterator::hasScript() const
{
    return type_ == IonFrame_JS;
}

JSScript *
IonFrameIterator::script() const
{
    JS_ASSERT(hasScript());
    JSScript *script = MaybeScriptFromCalleeToken(calleeToken());
    JS_ASSERT(script);
    return script;
}

uint8 *
IonFrameIterator::prevFp() const
{
    JS_ASSERT(type_ != IonFrame_Entry);

    size_t currentSize = SizeOfFramePrefix(type_);
    currentSize += current()->prevFrameLocalSize();

    return current_ + currentSize;
}

IonFrameIterator &
IonFrameIterator::operator++()
{
    JS_ASSERT(type_ != IonFrame_Entry);

    
    
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

void
ion::HandleException(ResumeFromException *rfe)
{
    JSContext *cx = GetIonContext()->cx;

    IonSpew(IonSpew_Invalidate, "handling exception");

    IonFrameIterator iter(cx->runtime->ionTop);
    while (iter.type() != IonFrame_Entry) {
        if (iter.type() == IonFrame_JS) {
            IonScript *ionScript;
            if (iter.checkInvalidation(&ionScript))
                ionScript->decref(cx);
        }

        ++iter;
    }

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
        MarkRoot(trc, CalleeTokenToFunction(token), "ion-callee");
        break;
      case CalleeToken_Script:
        MarkRoot(trc, CalleeTokenToScript(token), "ion-entry");
        break;
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
        
        
    } else if (CalleeTokenIsFunction(layout->calleeToken())) {
        JSFunction *fun = CalleeTokenToFunction(layout->calleeToken());

        
        Value *argv = layout->argv();
        for (size_t i = 0; i < fun->nargs; i++)
            gc::MarkRoot(trc, argv[i], "ion-argv");

        ionScript = fun->script()->ion;
    } else {
        ionScript = CalleeTokenToScript(layout->calleeToken())->ion;
    }

    const SafepointIndex *si = ionScript->getSafepointIndex(frame.returnAddressToFp());

    SafepointReader safepoint(ionScript, si);

    (void) safepoint.getOsiReturnPointOffset();

    GeneralRegisterSet actual, spilled;
    safepoint.getGcRegs(&actual, &spilled);
    
    
    JS_ASSERT(actual.empty() && spilled.empty());

    
    
    uint32 slot;
    while (safepoint.getGcSlot(&slot)) {
        uintptr_t *ref = layout->slotRef(slot);
        gc::MarkRootThingOrValue(trc, *ref, "ion-gc-slot");
    }

    while (safepoint.getValueSlot(&slot)) {
        Value *v = (Value *)layout->slotRef(slot);
        gc::MarkRoot(trc, *v, "ion-gc-slot");
    }
}

static void
MarkIonActivation(JSTracer *trc, uint8 *top)
{
    for (IonFrameIterator frames(top); frames.more(); ++frames) {
        switch (frames.type()) {
          case IonFrame_Exit:
            
            break;
          case IonFrame_JS:
            MarkIonJSFrame(trc, frames);
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

static inline jsbytecode *
GetNextPc(jsbytecode *pc)
{
    return pc + js_CodeSpec[JSOp(*pc)].length;
}

void
ion::GetPcScript(JSContext *cx, JSScript **scriptRes, jsbytecode **pcRes)
{
    JS_ASSERT(cx->fp()->runningInIon());
    FrameRecovery fr = FrameRecovery::FromFrameIterator(
        IonFrameIterator(cx->runtime->ionTop));

    
    
    
    SnapshotIterator si(fr);

    
    JSFunction *fun = fr.callee();
    JSScript *script = fr.script();
    jsbytecode *pc = script->code + si.pcOffset();

    
    while (si.moreFrames()) {
        JS_ASSERT(JSOp(*pc) == JSOP_CALL);

        
        int callerArgc = GET_ARGC(pc);
        uint32 funSlot = (si.slots() - 1) - callerArgc - 1;

        
        while (funSlot--) {
            JS_ASSERT(si.more());
            si.skip(si.readSlot());
        }
        Value funValue = si.read();
        while (si.more())
            si.skip(si.readSlot());

        
        fun = funValue.toObject().toFunction();
        script = fun->script();
        si.readFrame();
        pc = script->code + si.pcOffset();
    }

    
    
    do {
        pc--;
    } while (!script->analysis()->maybeCode(pc));

    
    *scriptRes = script;
    *pcRes = pc;
}
