






#include "Ion.h"
#include "IonFrames.h"
#include "jsobj.h"
#include "jsscript.h"
#include "jsfun.h"
#include "BaselineJIT.h"
#include "BaselineIC.h"
#include "IonCompartment.h"
#include "IonFrames-inl.h"
#include "IonFrameIterator-inl.h"
#include "Safepoints.h"
#include "IonSpewer.h"
#include "IonMacroAssembler.h"
#include "PcScriptCache.h"
#include "PcScriptCache-inl.h"
#include "gc/Marking.h"
#include "SnapshotReader.h"
#include "Safepoints.h"
#include "VMFunctions.h"

using namespace js;
using namespace js::ion;

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
  : current_((uint8_t *)fp),
    type_(IonFrame_OptimizedJS),
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
    AutoAssertNoGC nogc;
    uint8_t *returnAddr = returnAddressToFp();
    RawScript script = this->script();
    
    
    IonScript *currentIonScript = script->ion;
    bool invalidated = !script->hasIonScript() ||
        !currentIonScript->containsReturnAddress(returnAddr);
    if (!invalidated)
        return false;

    int32_t invalidationDataOffset = ((int32_t *) returnAddr)[-1];
    uint8_t *ionScriptDataOffset = returnAddr + invalidationDataOffset;
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
    if (isScripted()) {
        JS_ASSERT(isFunctionFrame());
        return CalleeTokenToFunction(calleeToken());
    }

    JS_ASSERT(isNative());
    return exitFrame()->nativeExit()->vp()[0].toObject().toFunction();
}

JSFunction *
IonFrameIterator::maybeCallee() const
{
    if ((isScripted() && isFunctionFrame()) || isNative())
        return callee();
    return NULL;
}

bool
IonFrameIterator::isNative() const
{
    if (type_ != IonFrame_Exit)
        return false;
    return exitFrame()->footer()->ionCode() == NULL;
}

bool
IonFrameIterator::isOOLNativeGetter() const
{
    if (type_ != IonFrame_Exit)
        return false;
    return exitFrame()->footer()->ionCode() == ION_FRAME_OOL_NATIVE_GETTER;
}

bool
IonFrameIterator::isOOLPropertyOp() const
{
    if (type_ != IonFrame_Exit)
        return false;
    return exitFrame()->footer()->ionCode() == ION_FRAME_OOL_PROPERTY_OP;
}

bool
IonFrameIterator::isDOMExit() const
{
    if (type_ != IonFrame_Exit)
        return false;
    return exitFrame()->isDomExit();
}

bool
IonFrameIterator::isFunctionFrame() const
{
    return js::ion::CalleeTokenIsFunction(calleeToken());
}

bool
IonFrameIterator::isEntryJSFrame() const
{
    if (prevType() == IonFrame_OptimizedJS || prevType() == IonFrame_Bailed_JS)
        return false;

    if (prevType() == IonFrame_BaselineStub || prevType() == IonFrame_Bailed_BaselineStub)
        return false;

    if (prevType() == IonFrame_Entry)
        return true;

    IonFrameIterator iter(*this);
    ++iter;
    for (; !iter.done(); ++iter) {
        if (iter.isScripted())
            return false;
    }
    return true;
}

UnrootedScript
IonFrameIterator::script() const
{
    AutoAssertNoGC nogc;
    JS_ASSERT(isScripted());
    RawScript script = ScriptFromCalleeToken(calleeToken());
    JS_ASSERT(script);
    return script;
}

void
IonFrameIterator::baselineScriptAndPc(MutableHandleScript scriptRes, jsbytecode **pcRes) const
{
    AutoAssertNoGC nogc;
    JS_ASSERT(isBaselineJS());
    scriptRes.set(script());
    uint8_t *retAddr = returnAddressToFp();
    *pcRes = scriptRes->baseline->icEntryFromReturnAddress(retAddr).pc(scriptRes);
}

Value *
IonFrameIterator::nativeVp() const
{
    JS_ASSERT(isNative());
    return exitFrame()->nativeExit()->vp();
}

Value *
IonFrameIterator::actualArgs() const
{
    return jsFrame()->argv() + 1;
}

uint8_t *
IonFrameIterator::prevFp() const
{
    size_t currentSize = SizeOfFramePrefix(type_);
    
    
    
    if (prevType() == IonFrame_Bailed_Rectifier ||
        prevType() == IonFrame_Bailed_JS ||
        prevType() == IonFrame_Bailed_BaselineStub)
    {
        JS_ASSERT(type_ == IonFrame_Exit);
        currentSize = SizeOfFramePrefix(IonFrame_OptimizedJS);
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

    
    
    uint8_t *prev = prevFp();
    type_ = current()->prevType();
    if (type_ == IonFrame_Bailed_JS)
        type_ = IonFrame_OptimizedJS;
    else if (type_ == IonFrame_Bailed_BaselineStub)
        type_ = IonFrame_BaselineStub;
    returnAddressToFp_ = current()->returnAddress();
    current_ = prev;
    return *this;
}

uintptr_t *
IonFrameIterator::spillBase() const
{
    
    
    
    
    return reinterpret_cast<uintptr_t *>(fp() - ionScript()->frameSize());
}

MachineState
IonFrameIterator::machineState() const
{
    SafepointReader reader(ionScript(), safepoint());
    uintptr_t *spill = spillBase();

    
    
    
    
    MachineState machine;
    for (GeneralRegisterIterator iter(reader.allSpills()); iter.more(); iter++)
        machine.setRegisterLocation(*iter, --spill);

    return machine;
}

static void
CloseLiveIterator(JSContext *cx, const InlineFrameIterator &frame, uint32_t localSlot)
{
    AssertCanGC();
    SnapshotIterator si = frame.snapshotIterator();

    
    uint32_t base = CountArgSlots(frame.maybeCallee()) + frame.script()->nfixed;
    uint32_t skipSlots = base + localSlot - 1;

    for (unsigned i = 0; i < skipSlots; i++)
        si.skip();

    Value v = si.read();
    RootedObject obj(cx, &v.toObject());

    if (cx->isExceptionPending())
        UnwindIteratorForException(cx, obj);
    else
        UnwindIteratorForUncatchableException(cx, obj);
}

static void
CloseLiveIterators(JSContext *cx, const InlineFrameIterator &frame)
{
    AssertCanGC();
    RootedScript script(cx, frame.script());
    jsbytecode *pc = frame.pc();

    if (!script->hasTrynotes())
        return;

    JSTryNote *tn = script->trynotes()->vector;
    JSTryNote *tnEnd = tn + script->trynotes()->length;

    uint32_t pcOffset = uint32_t(pc - script->main());
    for (; tn != tnEnd; ++tn) {
        if (pcOffset < tn->start)
            continue;
        if (pcOffset >= tn->start + tn->length)
            continue;

        if (tn->kind != JSTRY_ITER)
            continue;

        JS_ASSERT(JSOp(*(script->main() + tn->start + tn->length)) == JSOP_ENDITER);
        JS_ASSERT(tn->stackDepth > 0);

        uint32_t localSlot = tn->stackDepth;
        CloseLiveIterator(cx, frame, localSlot);
    }
}

void
ion::HandleException(ResumeFromException *rfe)
{
    AssertCanGC();
    JSContext *cx = GetIonContext()->cx;

    IonSpew(IonSpew_Invalidate, "handling exception");

    
    
    js_delete(cx->runtime->ionActivation->maybeTakeBailout());

    IonFrameIterator iter(cx->runtime->ionTop);
    while (!iter.isEntry()) {
        if (iter.isOptimizedJS()) {
            
            
            InlineFrameIterator frames(&iter);
            for (;;) {
                CloseLiveIterators(cx, frames);

                
                
                
                AutoAssertNoGC nogc;
                RawScript script = frames.script();
                Probes::exitScript(cx, script, script->function(), NULL);
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

void
IonActivationIterator::settle()
{
    while (activation_ && activation_->empty()) {
        top_ = activation_->prevIonTop();
        activation_ = activation_->prev();
    }
}

IonActivationIterator::IonActivationIterator(JSContext *cx)
  : top_(cx->runtime->ionTop),
    activation_(cx->runtime->ionActivation)
{
    settle();
}

IonActivationIterator::IonActivationIterator(JSRuntime *rt)
  : top_(rt->ionTop),
    activation_(rt->ionActivation)
{
    settle();
}

IonActivationIterator &
IonActivationIterator::operator++()
{
    JS_ASSERT(activation_);
    top_ = activation_->prevIonTop();
    activation_ = activation_->prev();
    settle();
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
        UnrootedScript script = CalleeTokenToScript(token);
        MarkScriptRoot(trc, &script, "ion-entry");
        JS_ASSERT(script == CalleeTokenToScript(token));
        break;
      }
      default:
        JS_NOT_REACHED("unknown callee token type");
    }
}

static inline uintptr_t
ReadAllocation(const IonFrameIterator &frame, const LAllocation *a)
{
    if (a->isGeneralReg()) {
        Register reg = a->toGeneralReg()->reg();
        return frame.machineState().read(reg);
    }
    if (a->isStackSlot()) {
        uint32_t slot = a->toStackSlot()->slot();
        return *frame.jsFrame()->slotRef(slot);
    }
    uint32_t index = a->toArgument()->index();
    uint8_t *argv = reinterpret_cast<uint8_t *>(frame.jsFrame()->argv());
    return *reinterpret_cast<uintptr_t *>(argv + index);
}

static void
MarkActualArguments(JSTracer *trc, const IonFrameIterator &frame)
{
    IonJSFrameLayout *layout = frame.jsFrame();
    JS_ASSERT(CalleeTokenIsFunction(layout->calleeToken()));

    size_t nargs = frame.numActualArgs();

    
    Value *argv = layout->argv();
    for (size_t i = 0; i < nargs + 1; i++)
        gc::MarkValueRoot(trc, &argv[i], "ion-argv");
}

size_t
IonFrameIterator::numBaselineStackValues() const
{
    JS_ASSERT(isBaselineJS());

    
    uint8_t *base = fp() - BaselineFrame::FramePointerOffset;

    
    
    size_t size = *reinterpret_cast<size_t *>(base + BaselineFrame::reverseOffsetOfFrameSize());
    JS_ASSERT(size >= BaselineFrame::FramePointerOffset + BaselineFrame::Size());
    JS_ASSERT(size <= frameSize());

    
    size -= BaselineFrame::FramePointerOffset + BaselineFrame::Size();
    JS_ASSERT((size % sizeof(Value)) == 0);

    return size / sizeof(Value);
}

Value
IonFrameIterator::baselineStackValue(size_t index) const
{
    JS_ASSERT(isBaselineJS());
    JS_ASSERT(index < numBaselineStackValues());

    
    uint8_t *base = fp() - BaselineFrame::FramePointerOffset;

    return *reinterpret_cast<Value *>(base + BaselineFrame::reverseOffsetOfLocal(index));
}

static void
MarkBaselineJSFrame(JSTracer *trc, const IonFrameIterator &frame)
{
    IonJSFrameLayout *layout = frame.jsFrame();
    MarkCalleeToken(trc, layout->calleeToken());

    if (CalleeTokenIsFunction(layout->calleeToken()))
        MarkActualArguments(trc, frame);

    uint8_t *base = frame.fp() - BaselineFrame::FramePointerOffset;

    
    JSObject **scope = reinterpret_cast<JSObject **>(base + BaselineFrame::reverseOffsetOfScopeChain());
    gc::MarkObjectRoot(trc, scope, "baseline-scopechain");

    
    size_t nvalues = frame.numBaselineStackValues();
    if (nvalues > 0) {
        
        Value *last = reinterpret_cast<Value *>(base + BaselineFrame::reverseOffsetOfLocal(nvalues - 1));
        gc::MarkValueRootRange(trc, nvalues, last, "baseline-stack");
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
        ionScript = CalleeTokenToFunction(layout->calleeToken())->nonLazyScript()->ion;
    } else {
        ionScript = CalleeTokenToScript(layout->calleeToken())->ion;
    }

    if (CalleeTokenIsFunction(layout->calleeToken()))
        MarkActualArguments(trc, frame);

    const SafepointIndex *si = ionScript->getSafepointIndex(frame.returnAddressToFp());

    SafepointReader safepoint(ionScript, si);

    
    
    uint32_t slot;
    while (safepoint.getGcSlot(&slot)) {
        uintptr_t *ref = layout->slotRef(slot);
        gc::MarkGCThingRoot(trc, reinterpret_cast<void **>(ref), "ion-gc-slot");
    }

    while (safepoint.getValueSlot(&slot)) {
        Value *v = (Value *)layout->slotRef(slot);
        gc::MarkValueRoot(trc, v, "ion-gc-slot");
    }

    uintptr_t *spill = frame.spillBase();
    GeneralRegisterSet gcRegs = safepoint.gcSpills();
    GeneralRegisterSet valueRegs = safepoint.valueSpills();
    for (GeneralRegisterIterator iter(safepoint.allSpills()); iter.more(); iter++) {
        --spill;
        if (gcRegs.has(*iter))
            gc::MarkGCThingRoot(trc, reinterpret_cast<void **>(spill), "ion-gc-spill");
        else if (valueRegs.has(*iter))
            gc::MarkValueRoot(trc, reinterpret_cast<Value *>(spill), "ion-value-spill");
    }

#ifdef JS_NUNBOX32
    LAllocation type, payload;
    while (safepoint.getNunboxSlot(&type, &payload)) {
        jsval_layout layout;
        layout.s.tag = (JSValueTag)ReadAllocation(frame, &type);
        layout.s.payload.uintptr = ReadAllocation(frame, &payload);

        Value v = IMPL_TO_JSVAL(layout);
        gc::MarkValueRoot(trc, &v, "ion-torn-value");
        JS_ASSERT(v == IMPL_TO_JSVAL(layout));
    }
#endif
}

void
IonActivationIterator::ionStackRange(uintptr_t *&min, uintptr_t *&end)
{
    IonFrameIterator frames(top());

    IonExitFrameLayout *exitFrame = frames.exitFrame();
    IonExitFooterFrame *footer = exitFrame->footer();
    const VMFunction *f = footer->function();
    if (exitFrame->isWrapperExit() && f->outParam == Type_Handle)
        min = reinterpret_cast<uintptr_t *>(footer->outVp());
    else
        min = reinterpret_cast<uintptr_t *>(footer);

    while (!frames.done())
        ++frames;

    end = reinterpret_cast<uintptr_t *>(frames.prevFp());
}

static void
MarkIonExitFrame(JSTracer *trc, const IonFrameIterator &frame)
{
    IonExitFooterFrame *footer = frame.exitFrame()->footer();

    
    
    
    
    
    JS_ASSERT(uintptr_t(footer->ionCode()) != uintptr_t(-1));

    
    
    
    if (frame.isNative()) {
        IonNativeExitFrameLayout *native = frame.exitFrame()->nativeExit();
        size_t len = native->argc() + 2;
        Value *vp = native->vp();
        gc::MarkValueRootRange(trc, len, vp, "ion-native-args");
        return;
    }

    if (frame.isOOLNativeGetter()) {
        IonOOLNativeGetterExitFrameLayout *oolgetter = frame.exitFrame()->oolNativeGetterExit();
        gc::MarkIonCodeRoot(trc, oolgetter->stubCode(), "ion-ool-getter-code");
        gc::MarkValueRoot(trc, oolgetter->vp(), "ion-ool-getter-callee");
        gc::MarkValueRoot(trc, oolgetter->thisp(), "ion-ool-getter-this");
        return;
    }
 
    if (frame.isOOLPropertyOp()) {
        IonOOLPropertyOpExitFrameLayout *oolgetter = frame.exitFrame()->oolPropertyOpExit();
        gc::MarkIonCodeRoot(trc, oolgetter->stubCode(), "ion-ool-property-op-code");
        gc::MarkValueRoot(trc, oolgetter->vp(), "ion-ool-property-op-vp");
        gc::MarkIdRoot(trc, oolgetter->id(), "ion-ool-property-op-id");
        gc::MarkObjectRoot(trc, oolgetter->obj(), "ion-ool-property-op-obj");
        return;
    }

    if (frame.isDOMExit()) {
        IonDOMExitFrameLayout *dom = frame.exitFrame()->DOMExit();
        gc::MarkObjectRoot(trc, dom->thisObjAddress(), "ion-dom-args");
        if (dom->isSetterFrame()) {
            gc::MarkValueRoot(trc, dom->vp(), "ion-dom-args");
        } else if (dom->isMethodFrame()) {
            IonDOMMethodExitFrameLayout *method =
                reinterpret_cast<IonDOMMethodExitFrameLayout *>(dom);
            size_t len = method->argc() + 2;
            Value *vp = method->vp();
            gc::MarkValueRootRange(trc, len, vp, "ion-dom-args");
        }
        return;
    }

    MarkIonCodeRoot(trc, footer->addressOfIonCode(), "ion-exit-code");

    const VMFunction *f = footer->function();
    if (f == NULL || f->explicitArgs == 0)
        return;

    
    uint8_t *argBase = frame.exitFrame()->argBase();
    for (uint32_t explicitArg = 0; explicitArg < f->explicitArgs; explicitArg++) {
        switch (f->argRootType(explicitArg)) {
          case VMFunction::RootNone:
            break;
          case VMFunction::RootObject: {
            
            JSObject **pobj = reinterpret_cast<JSObject **>(argBase);
            if (*pobj)
                gc::MarkObjectRoot(trc, pobj, "ion-vm-args");
            break;
          }
          case VMFunction::RootString:
          case VMFunction::RootPropertyName:
            gc::MarkStringRoot(trc, reinterpret_cast<JSString**>(argBase), "ion-vm-args");
            break;
          case VMFunction::RootFunction:
            gc::MarkObjectRoot(trc, reinterpret_cast<JSFunction**>(argBase), "ion-vm-args");
            break;
          case VMFunction::RootValue:
            gc::MarkValueRoot(trc, reinterpret_cast<Value*>(argBase), "ion-vm-args");
            break;
          case VMFunction::RootCell:
            gc::MarkGCThingRoot(trc, reinterpret_cast<void **>(argBase), "ion-vm-args");
            break;
        }

        switch (f->argProperties(explicitArg)) {
          case VMFunction::WordByValue:
          case VMFunction::WordByRef:
            argBase += sizeof(void *);
            break;
          case VMFunction::DoubleByValue:
          case VMFunction::DoubleByRef:
            argBase += 2 * sizeof(void *);
            break;
        }
    }

    if (f->outParam == Type_Handle)
        gc::MarkValueRoot(trc, footer->outVp(), "ion-vm-outvp");
}

static void
MarkIonActivation(JSTracer *trc, const IonActivationIterator &activations)
{
    for (IonFrameIterator frames(activations); !frames.done(); ++frames) {
        switch (frames.type()) {
          case IonFrame_Exit:
            MarkIonExitFrame(trc, frames);
            break;
          case IonFrame_BaselineJS:
            MarkBaselineJSFrame(trc, frames);
            break;
          case IonFrame_BaselineStub:
            
            break;
          case IonFrame_OptimizedJS:
            MarkIonJSFrame(trc, frames);
            break;
          case IonFrame_Bailed_JS:
            JS_NOT_REACHED("invalid");
            break;
          case IonFrame_Rectifier:
          case IonFrame_Bailed_Rectifier:
            break;
          case IonFrame_Osr:
            
            
            
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
        MarkIonActivation(trc, activations);
}

void
ion::AutoTempAllocatorRooter::trace(JSTracer *trc)
{
    for (CompilerRootNode *root = temp->rootList(); root != NULL; root = root->next)
        gc::MarkGCThingRoot(trc, root->address(), "ion-compiler-root");
}

void
ion::GetPcScript(JSContext *cx, MutableHandleScript scriptRes, jsbytecode **pcRes)
{
    JS_ASSERT(cx->fp()->beginsIonActivation());
    IonSpew(IonSpew_Snapshots, "Recover PC & Script from the last frame.");

    JSRuntime *rt = cx->runtime;

    
    IonFrameIterator it(rt->ionTop);
    uint8_t *retAddr = it.returnAddress();
    uint32_t hash = PcScriptCache::Hash(retAddr);
    JS_ASSERT(retAddr != NULL);

    
    if (JS_UNLIKELY(rt->ionPcScriptCache == NULL)) {
        rt->ionPcScriptCache = (PcScriptCache *)js_malloc(sizeof(struct PcScriptCache));
        if (rt->ionPcScriptCache)
            rt->ionPcScriptCache->clear(rt->gcNumber);
    }

    
    if (rt->ionPcScriptCache && rt->ionPcScriptCache->get(rt, hash, retAddr, scriptRes, pcRes))
        return;

    
    ++it; 
    jsbytecode *pc = NULL;

    if (it.isOptimizedJS()) {
        InlineFrameIterator ifi(&it);
        scriptRes.set(ifi.script());
        pc = ifi.pc();
    } else {
        if (it.isBaselineStub())
            ++it;
        JS_ASSERT(it.isBaselineJS());
        it.baselineScriptAndPc(scriptRes, &pc);
    }

    if (pcRes)
        *pcRes = pc;

    
    if (rt->ionPcScriptCache)
        rt->ionPcScriptCache->add(hash, retAddr, pc, (RawScript)scriptRes);
}

void
OsiIndex::fixUpOffset(MacroAssembler &masm)
{
    callPointDisplacement_ = masm.actualOffset(callPointDisplacement_);
}

uint32_t
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
    return loc.isStackSlot() || machine_.has(loc.reg());
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
    JS_ASSERT(type() == IonFrame_OptimizedJS);

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
    if (iter) {
        start_ = SnapshotIterator(*iter);
        findNextFrame();
    }
}

InlineFrameIterator::InlineFrameIterator(const InlineFrameIterator *iter)
  : frame_(iter->frame_),
    framesRead_(0),
    callee_(NULL),
    script_(NULL)
{
    if (frame_) {
        start_ = SnapshotIterator(*frame_);
        
        
        framesRead_ = iter->framesRead_ - 1;
        findNextFrame();
    }
}

void
InlineFrameIterator::findNextFrame()
{
    AutoAssertNoGC nogc;
    JS_ASSERT(more());

    si_ = start_;

    
    callee_ = frame_->maybeCallee();
    script_ = frame_->script();
    pc_ = script_->code + si_.pcOffset();
#ifdef DEBUG
    numActualArgs_ = 0xbad;
#endif

    
    
    unsigned remaining = start_.frameCount() - framesRead_ - 1;
    for (unsigned i = 0; i < remaining; i++) {
        JS_ASSERT(js_CodeSpec[*pc_].format & JOF_INVOKE);

        
        numActualArgs_ = GET_ARGC(pc_);

        
        unsigned skipCount = (si_.slots() - 1) - numActualArgs_ - 1;
        for (unsigned j = 0; j < skipCount; j++)
            si_.skip();

        Value funval = si_.read();

        
        while (si_.moreSlots())
            si_.skip();

        si_.nextFrame();

        callee_ = funval.toObject().toFunction();
        script_ = callee_->nonLazyScript();
        pc_ = script_->code + si_.pcOffset();
    }

    framesRead_++;
}

InlineFrameIterator &
InlineFrameIterator::operator++()
{
    findNextFrame();
    return *this;
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

        
        if (IsGetterPC(parent.pc()) || IsSetterPC(parent.pc()))
            return false;

        
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

    if (parent.isOptimizedJS()) {
        
        InlineFrameIterator inlinedParent(&parent);

        
        if (IsGetterPC(inlinedParent.pc()) || IsSetterPC(inlinedParent.pc()))
            return false;

        JS_ASSERT(js_CodeSpec[*inlinedParent.pc()].format & JOF_INVOKE);

        return (JSOp)*inlinedParent.pc() == JSOP_NEW;
    }

    if (parent.isBaselineJS()) {
        JSContext *cx = GetIonContext()->cx;
        RootedScript script(cx);
        jsbytecode *pc;

        parent.baselineScriptAndPc(&script, &pc);

        JS_ASSERT(js_CodeSpec[*pc].format & JOF_INVOKE);

        return JSOp(*pc) == JSOP_NEW;
    }

    JS_ASSERT(parent.done());

    
    
    if (!activation_->entryfp())
        return false;

    
    
    
    if (activation_->entryfp()->callingIntoIon())
        return false;
    JS_ASSERT(activation_->entryfp()->runningInIon());
    return activation_->entryfp()->isConstructing();
}

JSObject *
InlineFrameIterator::scopeChain() const
{
    SnapshotIterator s(si_);

    
    Value v = s.read();
    JS_ASSERT(v.isObject());
    return &v.toObject();
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
    
    
    
    
    
    if (more())
        return numActualArgs_;

    return frame_->numActualArgs();
}

unsigned
IonFrameIterator::numActualArgs() const
{
    if (isScripted())
        return jsFrame()->numActualArgs();

    JS_ASSERT(isNative());
    return exitFrame()->nativeExit()->argc();
}

void
SnapshotIterator::warnUnreadableSlot()
{
    fprintf(stderr, "Warning! Tried to access unreadable IonMonkey slot (possible f.arguments).\n");
}

void
IonFrameIterator::dump() const
{
    switch (type_) {
      case IonFrame_Entry:
        fprintf(stderr, " Entry frame\n");
        fprintf(stderr, "  Frame size: %u\n", unsigned(current()->prevFrameLocalSize()));
        break;
      case IonFrame_BaselineJS:
        dumpBaseline();
        break;
      case IonFrame_BaselineStub:
      case IonFrame_Bailed_BaselineStub:
        fprintf(stderr, " Baseline stub frame\n");
        fprintf(stderr, "  Frame size: %u\n", unsigned(current()->prevFrameLocalSize()));
        break;
      case IonFrame_OptimizedJS:
      {
        InlineFrameIterator frames(this);
        for (;;) {
            frames.dump();
            if (!frames.more())
                break;
            ++frames;
        }
        break;
      }
      case IonFrame_Rectifier:
      case IonFrame_Bailed_Rectifier:
        fprintf(stderr, " Rectifier frame\n");
        fprintf(stderr, "  Frame size: %u\n", unsigned(current()->prevFrameLocalSize()));
        break;
      case IonFrame_Bailed_JS:
        fprintf(stderr, "Warning! Bailed JS frames are not observable.\n");
        break;
      case IonFrame_Exit:
        break;
      case IonFrame_Osr:
        fprintf(stderr, "Warning! OSR frame are not defined yet.\n");
        break;
    };
    fputc('\n', stderr);
}

struct DumpOp {
    DumpOp(unsigned int i) : i_(i) {}

    unsigned int i_;
    void operator()(const Value& v) {
        fprintf(stderr, "  actual (arg %d): ", i_);
#ifdef DEBUG
        js_DumpValue(v);
#else
        fprintf(stderr, "?\n");
#endif
        i_++;
    }
};

void
IonFrameIterator::dumpBaseline() const
{
    AutoAssertNoGC nogc;
    JS_ASSERT(isBaselineJS());

    fprintf(stderr, " JS Baseline frame\n");
    if (isFunctionFrame()) {
        fprintf(stderr, "  callee fun: ");
#ifdef DEBUG
        js_DumpObject(callee());
#else
        fprintf(stderr, "?\n");
#endif
    } else {
        fprintf(stderr, "  global frame, no callee\n");
    }

    fprintf(stderr, "  file %s line %u\n",
            script()->filename, (unsigned) script()->lineno);

    JSContext *cx = GetIonContext()->cx;
    RootedScript script(cx);
    jsbytecode *pc;
    baselineScriptAndPc(&script, &pc);

    fprintf(stderr, "  script = %p, pc = %p (offset %u)\n", (void *)script, pc, uint32_t(pc - script->code));
    fprintf(stderr, "  current op: %s\n", js_CodeName[*pc]);

    fprintf(stderr, "  actual args: %d\n", numActualArgs());

    uint8_t *base = fp() - BaselineFrame::FramePointerOffset;
    Value *v = reinterpret_cast<Value *>(base + BaselineFrame::reverseOffsetOfLocal(0));
    size_t nvalues = numBaselineStackValues();

    for (unsigned i = 0; i < nvalues; i++) {
        fprintf(stderr, "  slot %u: ", i);
#ifdef DEBUG
        js_DumpValue(*v);
#else
        fprintf(stderr, "?\n");
#endif
        v--;
    }
}

void
InlineFrameIterator::dump() const
{
    AutoAssertNoGC nogc;
    if (more())
        fprintf(stderr, " JS frame (inlined)\n");
    else
        fprintf(stderr, " JS frame\n");

    bool isFunction = false;
    if (isFunctionFrame()) {
        isFunction = true;
        fprintf(stderr, "  callee fun: ");
#ifdef DEBUG
        js_DumpObject(callee());
#else
        fprintf(stderr, "?\n");
#endif
    } else {
        fprintf(stderr, "  global frame, no callee\n");
    }

    fprintf(stderr, "  file %s line %u\n",
            script()->filename, (unsigned) script()->lineno);

    fprintf(stderr, "  script = %p, pc = %p\n", (void*) script(), pc());
    fprintf(stderr, "  current op: %s\n", js_CodeName[*pc()]);

    if (!more()) {
        numActualArgs();
    }

    SnapshotIterator si = snapshotIterator();
    fprintf(stderr, "  slots: %u\n", si.slots() - 1);
    for (unsigned i = 0; i < si.slots() - 1; i++) {
        if (isFunction) {
            if (i == 0)
                fprintf(stderr, "  scope chain: ");
            else if (i == 1)
                fprintf(stderr, "  this: ");
            else if (i - 2 < callee()->nargs)
                fprintf(stderr, "  formal (arg %d): ", i - 2);
            else {
                if (i - 2 == callee()->nargs && numActualArgs() > callee()->nargs) {
                    DumpOp d(callee()->nargs);
                    forEachCanonicalActualArg(d, d.i_, numActualArgs() - d.i_);
                }

                fprintf(stderr, "  slot %d: ", i - 2 - callee()->nargs);
            }
        } else
            fprintf(stderr, "  slot %u: ", i);
#ifdef DEBUG
        js_DumpValue(si.maybeRead());
#else
        fprintf(stderr, "?\n");
#endif
    }

    fputc('\n', stderr);
}
