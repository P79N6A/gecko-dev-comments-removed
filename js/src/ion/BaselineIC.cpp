





#include "BaselineJIT.h"
#include "BaselineCompiler.h"
#include "BaselineHelpers.h"
#include "BaselineIC.h"
#include "IonLinker.h"
#include "IonSpewer.h"
#include "VMFunctions.h"
#include "IonFrames-inl.h"

#include "builtin/Eval.h"

#include "jsinterpinlines.h"

namespace js {
namespace ion {

#ifdef DEBUG
void
FallbackICSpew(JSContext *cx, ICFallbackStub *stub, const char *fmt, ...)
{
    if (IonSpewEnabled(IonSpew_BaselineICFallback)) {
        RootedScript script(cx, GetTopIonJSScript(cx));
        jsbytecode *pc = stub->icEntry()->pc(script);

        char fmtbuf[100];
        va_list args;
        va_start(args, fmt);
        vsnprintf(fmtbuf, 100, fmt, args);
        va_end(args);

        IonSpew(IonSpew_BaselineICFallback,
                "Fallback hit for (%s:%d) (pc=%d,line=%d,uses=%d,stubs=%d): %s",
                script->filename(),
                script->lineno,
                (int) (pc - script->code),
                PCToLineNumber(script, pc),
                script->getUseCount(),
                (int) stub->numOptimizedStubs(),
                fmtbuf);
    }
}

void
TypeFallbackICSpew(JSContext *cx, ICTypeMonitor_Fallback *stub, const char *fmt, ...)
{
    if (IonSpewEnabled(IonSpew_BaselineICFallback)) {
        RootedScript script(cx, GetTopIonJSScript(cx));
        jsbytecode *pc = stub->icEntry()->pc(script);

        char fmtbuf[100];
        va_list args;
        va_start(args, fmt);
        vsnprintf(fmtbuf, 100, fmt, args);
        va_end(args);

        IonSpew(IonSpew_BaselineICFallback,
                "Type monitor fallback hit for (%s:%d) (pc=%d,line=%d,uses=%d,stubs=%d): %s",
                script->filename(),
                script->lineno,
                (int) (pc - script->code),
                PCToLineNumber(script, pc),
                script->getUseCount(),
                (int) stub->numOptimizedMonitorStubs(),
                fmtbuf);
    }
}

#else
#define FallbackICSpew(...)
#define TypeFallbackICSpew(...)
#endif


ICFallbackStub *
ICEntry::fallbackStub() const
{
    return firstStub()->getChainFallback();
}


ICStubConstIterator &
ICStubConstIterator::operator++()
{
    JS_ASSERT(currentStub_ != NULL);
    currentStub_ = currentStub_->next();
    return *this;
}


ICStubIterator::ICStubIterator(ICFallbackStub *fallbackStub, bool end)
  : icEntry_(fallbackStub->icEntry()),
    fallbackStub_(fallbackStub),
    previousStub_(NULL),
    currentStub_(end ? fallbackStub : icEntry_->firstStub()),
    unlinked_(false)
{ }

ICStubIterator &
ICStubIterator::operator++()
{
    JS_ASSERT(currentStub_->next() != NULL);
    if (!unlinked_)
        previousStub_ = currentStub_;
    currentStub_ = currentStub_->next();
    unlinked_ = false;
    return *this;
}

void
ICStubIterator::unlink(Zone *zone)
{
    JS_ASSERT(currentStub_->next() != NULL);
    JS_ASSERT(currentStub_ != fallbackStub_);
    JS_ASSERT(!unlinked_);
    fallbackStub_->unlinkStub(zone, previousStub_, currentStub_);

    
    unlinked_ = true;
}


void
ICStub::markCode(JSTracer *trc, const char *name)
{
    IonCode *stubIonCode = ionCode();
    MarkIonCodeUnbarriered(trc, &stubIonCode, name);
}

void
ICStub::updateCode(IonCode *code)
{
    
#ifdef JSGC_INCREMENTAL
    IonCode::writeBarrierPre(ionCode());
#endif
    stubCode_ = code->raw();
}

 void
ICStub::trace(JSTracer *trc)
{
    markCode(trc, "baseline-stub-ioncode");

    
    
    
    
    if (isMonitoredFallback()) {
        ICTypeMonitor_Fallback *lastMonStub = toMonitoredFallbackStub()->fallbackMonitorStub();
        for (ICStubConstIterator iter = lastMonStub->firstMonitorStub(); !iter.atEnd(); iter++) {
            JS_ASSERT_IF(iter->next() == NULL, *iter == lastMonStub);
            iter->markCode(trc, "baseline-monitor-stub-ioncode");
        }
    }

    if (isUpdated()) {
        for (ICStubConstIterator iter = toUpdatedStub()->firstUpdateStub(); !iter.atEnd(); iter++) {
            JS_ASSERT_IF(iter->next() == NULL, iter->isTypeUpdate_Fallback());
            iter->markCode(trc, "baseline-update-stub-ioncode");
        }
    }

    switch (kind()) {
      case ICStub::Call_Scripted: {
        ICCall_Scripted *callStub = toCall_Scripted();
        MarkScript(trc, &callStub->calleeScript(), "baseline-callscripted-callee");
        break;
      }
      case ICStub::Call_Native: {
        ICCall_Native *callStub = toCall_Native();
        MarkObject(trc, &callStub->callee(), "baseline-callnative-callee");
        break;
      }
      case ICStub::GetElem_Native: {
        ICGetElem_Native *getElemStub = toGetElem_Native();
        MarkShape(trc, &getElemStub->shape(), "baseline-getelem-native-shape");
        gc::MarkValue(trc, &getElemStub->idval(), "baseline-getelem-native-idval");
        break;
      }
      case ICStub::GetElem_NativePrototype: {
        ICGetElem_NativePrototype *getElemStub = toGetElem_NativePrototype();
        MarkShape(trc, &getElemStub->shape(), "baseline-getelem-nativeproto-shape");
        gc::MarkValue(trc, &getElemStub->idval(), "baseline-getelem-nativeproto-idval");
        MarkObject(trc, &getElemStub->holder(), "baseline-getelem-nativeproto-holder");
        MarkShape(trc, &getElemStub->holderShape(), "baseline-getelem-nativeproto-holdershape");
        break;
      }
      case ICStub::GetElem_Dense: {
        ICGetElem_Dense *getElemStub = toGetElem_Dense();
        MarkShape(trc, &getElemStub->shape(), "baseline-getelem-dense-shape");
        break;
      }
      case ICStub::GetElem_TypedArray: {
        ICGetElem_TypedArray *getElemStub = toGetElem_TypedArray();
        MarkShape(trc, &getElemStub->shape(), "baseline-getelem-typedarray-shape");
        break;
      }
      case ICStub::SetElem_Dense: {
        ICSetElem_Dense *setElemStub = toSetElem_Dense();
        MarkShape(trc, &setElemStub->shape(), "baseline-getelem-dense-shape");
        MarkTypeObject(trc, &setElemStub->type(), "baseline-setelem-dense-type");
        break;
      }
      case ICStub::SetElem_DenseAdd: {
        ICSetElem_DenseAdd *setElemStub = toSetElem_DenseAdd();
        MarkTypeObject(trc, &setElemStub->type(), "baseline-setelem-denseadd-type");

        JS_STATIC_ASSERT(ICSetElem_DenseAdd::MAX_PROTO_CHAIN_DEPTH == 4);

        switch (setElemStub->protoChainDepth()) {
          case 0: setElemStub->toImpl<0>()->traceShapes(trc); break;
          case 1: setElemStub->toImpl<1>()->traceShapes(trc); break;
          case 2: setElemStub->toImpl<2>()->traceShapes(trc); break;
          case 3: setElemStub->toImpl<3>()->traceShapes(trc); break;
          case 4: setElemStub->toImpl<4>()->traceShapes(trc); break;
          default: JS_NOT_REACHED("Invalid proto stub.");
        }
        break;
      }
      case ICStub::SetElem_TypedArray: {
        ICSetElem_TypedArray *setElemStub = toSetElem_TypedArray();
        MarkShape(trc, &setElemStub->shape(), "baseline-setelem-typedarray-shape");
        break;
      }
      case ICStub::TypeMonitor_SingleObject: {
        ICTypeMonitor_SingleObject *monitorStub = toTypeMonitor_SingleObject();
        MarkObject(trc, &monitorStub->object(), "baseline-monitor-singleobject");
        break;
      }
      case ICStub::TypeMonitor_TypeObject: {
        ICTypeMonitor_TypeObject *monitorStub = toTypeMonitor_TypeObject();
        MarkTypeObject(trc, &monitorStub->type(), "baseline-monitor-typeobject");
        break;
      }
      case ICStub::TypeUpdate_SingleObject: {
        ICTypeUpdate_SingleObject *updateStub = toTypeUpdate_SingleObject();
        MarkObject(trc, &updateStub->object(), "baseline-update-singleobject");
        break;
      }
      case ICStub::TypeUpdate_TypeObject: {
        ICTypeUpdate_TypeObject *updateStub = toTypeUpdate_TypeObject();
        MarkTypeObject(trc, &updateStub->type(), "baseline-update-typeobject");
        break;
      }
      case ICStub::Profiler_PushFunction: {
        ICProfiler_PushFunction *pushFunStub = toProfiler_PushFunction();
        MarkScript(trc, &pushFunStub->script(), "baseline-profilerpushfunction-stub-script");
        break;
      }
      case ICStub::GetName_Global: {
        ICGetName_Global *globalStub = toGetName_Global();
        MarkShape(trc, &globalStub->shape(), "baseline-global-stub-shape");
        break;
      }
      case ICStub::GetName_Scope0:
        static_cast<ICGetName_Scope<0>*>(this)->traceScopes(trc);
        break;
      case ICStub::GetName_Scope1:
        static_cast<ICGetName_Scope<1>*>(this)->traceScopes(trc);
        break;
      case ICStub::GetName_Scope2:
        static_cast<ICGetName_Scope<2>*>(this)->traceScopes(trc);
        break;
      case ICStub::GetName_Scope3:
        static_cast<ICGetName_Scope<3>*>(this)->traceScopes(trc);
        break;
      case ICStub::GetName_Scope4:
        static_cast<ICGetName_Scope<4>*>(this)->traceScopes(trc);
        break;
      case ICStub::GetName_Scope5:
        static_cast<ICGetName_Scope<5>*>(this)->traceScopes(trc);
        break;
      case ICStub::GetName_Scope6:
        static_cast<ICGetName_Scope<6>*>(this)->traceScopes(trc);
        break;
      case ICStub::GetIntrinsic_Constant: {
        ICGetIntrinsic_Constant *constantStub = toGetIntrinsic_Constant();
        gc::MarkValue(trc, &constantStub->value(), "baseline-getintrinsic-constant-value");
        break;
      }
      case ICStub::GetProp_String: {
        ICGetProp_String *propStub = toGetProp_String();
        MarkShape(trc, &propStub->stringProtoShape(), "baseline-getpropstring-stub-shape");
        break;
      }
      case ICStub::GetProp_Native: {
        ICGetProp_Native *propStub = toGetProp_Native();
        MarkShape(trc, &propStub->shape(), "baseline-getpropnative-stub-shape");
        break;
      }
      case ICStub::GetProp_NativePrototype: {
        ICGetProp_NativePrototype *propStub = toGetProp_NativePrototype();
        MarkShape(trc, &propStub->shape(), "baseline-getpropnativeproto-stub-shape");
        MarkObject(trc, &propStub->holder(), "baseline-getpropnativeproto-stub-holder");
        MarkShape(trc, &propStub->holderShape(), "baseline-getpropnativeproto-stub-holdershape");
        break;
      }
      case ICStub::GetProp_CallListBaseNative:
      case ICStub::GetProp_CallListBaseWithGenerationNative: {
        ICGetPropCallListBaseNativeStub *propStub;
        if (kind() ==  ICStub::GetProp_CallListBaseNative)
            propStub = toGetProp_CallListBaseNative();
        else
            propStub = toGetProp_CallListBaseWithGenerationNative();
        MarkShape(trc, &propStub->shape(), "baseline-getproplistbasenative-stub-shape");
        if (propStub->expandoShape()) {
            MarkShape(trc, &propStub->expandoShape(),
                      "baseline-getproplistbasenative-stub-expandoshape");
        }
        MarkObject(trc, &propStub->holder(), "baseline-getproplistbasenative-stub-holder");
        MarkShape(trc, &propStub->holderShape(), "baseline-getproplistbasenative-stub-holdershape");
        MarkObject(trc, &propStub->getter(), "baseline-getproplistbasenative-stub-getter");
        break;
      }
      case ICStub::GetProp_ListBaseShadowed: {
        ICGetProp_ListBaseShadowed *propStub = toGetProp_ListBaseShadowed();
        MarkShape(trc, &propStub->shape(), "baseline-getproplistbaseshadowed-stub-shape");
        MarkString(trc, &propStub->name(), "baseline-getproplistbaseshadowed-stub-name");
        break;
      }
      case ICStub::GetProp_CallScripted: {
        ICGetProp_CallScripted *callStub = toGetProp_CallScripted();
        MarkShape(trc, &callStub->shape(), "baseline-getpropcallscripted-stub-shape");
        MarkObject(trc, &callStub->holder(), "baseline-getpropcallscripted-stub-holder");
        MarkShape(trc, &callStub->holderShape(), "baseline-getpropcallscripted-stub-holdershape");
        MarkObject(trc, &callStub->getter(), "baseline-getpropcallscripted-stub-getter");
        break;
      }
      case ICStub::GetProp_CallNative: {
        ICGetProp_CallNative *callStub = toGetProp_CallNative();
        MarkShape(trc, &callStub->shape(), "baseline-getpropcallnative-stub-shape");
        MarkObject(trc, &callStub->holder(), "baseline-getpropcallnative-stub-holder");
        MarkShape(trc, &callStub->holderShape(), "baseline-getpropcallnative-stub-holdershape");
        MarkObject(trc, &callStub->getter(), "baseline-getpropcallnative-stub-getter");
        break;
      }
      case ICStub::SetProp_Native: {
        ICSetProp_Native *propStub = toSetProp_Native();
        MarkShape(trc, &propStub->shape(), "baseline-setpropnative-stub-shape");
        MarkTypeObject(trc, &propStub->type(), "baseline-setpropnative-stub-type");
        break;
      }
      case ICStub::SetProp_NativeAdd: {
        ICSetProp_NativeAdd *propStub = toSetProp_NativeAdd();
        MarkTypeObject(trc, &propStub->type(), "baseline-setpropnativeadd-stub-type");
        MarkShape(trc, &propStub->newShape(), "baseline-setpropnativeadd-stub-newshape");
        JS_STATIC_ASSERT(ICSetProp_NativeAdd::MAX_PROTO_CHAIN_DEPTH == 4);
        switch (propStub->protoChainDepth()) {
          case 0: propStub->toImpl<0>()->traceShapes(trc); break;
          case 1: propStub->toImpl<1>()->traceShapes(trc); break;
          case 2: propStub->toImpl<2>()->traceShapes(trc); break;
          case 3: propStub->toImpl<3>()->traceShapes(trc); break;
          case 4: propStub->toImpl<4>()->traceShapes(trc); break;
          default: JS_NOT_REACHED("Invalid proto stub.");
        }
        break;
      }
      case ICStub::SetProp_CallScripted: {
        ICSetProp_CallScripted *callStub = toSetProp_CallScripted();
        MarkShape(trc, &callStub->shape(), "baseline-setpropcallscripted-stub-shape");
        MarkObject(trc, &callStub->holder(), "baseline-setpropcallscripted-stub-holder");
        MarkShape(trc, &callStub->holderShape(), "baseline-setpropcallscripted-stub-holdershape");
        MarkObject(trc, &callStub->setter(), "baseline-setpropcallscripted-stub-setter");
        break;
      }
      case ICStub::SetProp_CallNative: {
        ICSetProp_CallNative *callStub = toSetProp_CallNative();
        MarkShape(trc, &callStub->shape(), "baseline-setpropcallnative-stub-shape");
        MarkObject(trc, &callStub->holder(), "baseline-setpropcallnative-stub-holder");
        MarkShape(trc, &callStub->holderShape(), "baseline-setpropcallnative-stub-holdershape");
        MarkObject(trc, &callStub->setter(), "baseline-setpropcallnative-stub-setter");
        break;
      }
      default:
        break;
    }
}

void
ICFallbackStub::unlinkStub(Zone *zone, ICStub *prev, ICStub *stub)
{
    JS_ASSERT(stub->next());

    
    if (stub->next() == this) {
        JS_ASSERT(lastStubPtrAddr_ == stub->addressOfNext());
        if (prev)
            lastStubPtrAddr_ = prev->addressOfNext();
        else
            lastStubPtrAddr_ = icEntry()->addressOfFirstStub();
        *lastStubPtrAddr_ = this;
    } else {
        if (prev) {
            JS_ASSERT(prev->next() == stub);
            prev->setNext(stub->next());
        } else {
            JS_ASSERT(icEntry()->firstStub() == stub);
            icEntry()->setFirstStub(stub->next());
        }
    }

    JS_ASSERT(numOptimizedStubs_ > 0);
    numOptimizedStubs_--;

    if (zone->needsBarrier()) {
        
        
        stub->trace(zone->barrierTracer());
    }

    if (ICStub::CanMakeCalls(stub->kind()) && stub->isMonitored()) {
        
        
        
        
        ICTypeMonitor_Fallback *monitorFallback = toMonitoredFallbackStub()->fallbackMonitorStub();
        stub->toMonitoredStub()->resetFirstMonitorStub(monitorFallback);
    }

#ifdef DEBUG
    
    
    
    
    if (!ICStub::CanMakeCalls(stub->kind()))
        stub->stubCode_ = (uint8_t *)0xbad;
#endif
}

void
ICFallbackStub::unlinkStubsWithKind(JSContext *cx, ICStub::Kind kind)
{
    for (ICStubIterator iter = beginChain(); !iter.atEnd(); iter++) {
        if (iter->kind() == kind)
            iter.unlink(cx->zone());
    }
}

void
ICTypeMonitor_Fallback::resetMonitorStubChain(Zone *zone)
{
    if (zone->needsBarrier()) {
        
        
        
        this->trace(zone->barrierTracer());
    }

    firstMonitorStub_ = this;
    numOptimizedMonitorStubs_ = 0;

    if (hasFallbackStub_) {
        lastMonitorStubPtrAddr_ = NULL;

        
        for (ICStubConstIterator iter = mainFallbackStub_->beginChainConst();
             !iter.atEnd(); iter++)
        {
            if (!iter->isMonitored())
                continue;
            iter->toMonitoredStub()->resetFirstMonitorStub(this);
        }
    } else {
        icEntry_->setFirstStub(this);
        lastMonitorStubPtrAddr_ = icEntry_->addressOfFirstStub();
    }
}

ICMonitoredStub::ICMonitoredStub(Kind kind, IonCode *stubCode, ICStub *firstMonitorStub)
  : ICStub(kind, ICStub::Monitored, stubCode),
    firstMonitorStub_(firstMonitorStub)
{
    
    
    JS_ASSERT_IF(firstMonitorStub_->isTypeMonitor_Fallback(),
                 firstMonitorStub_->toTypeMonitor_Fallback()->firstMonitorStub() ==
                    firstMonitorStub_);
}

bool
ICMonitoredFallbackStub::initMonitoringChain(JSContext *cx, ICStubSpace *space)
{
    JS_ASSERT(fallbackMonitorStub_ == NULL);

    ICTypeMonitor_Fallback::Compiler compiler(cx, this);
    ICTypeMonitor_Fallback *stub = compiler.getStub(space);
    if (!stub)
        return false;
    fallbackMonitorStub_ = stub;
    return true;
}

bool
ICMonitoredFallbackStub::addMonitorStubForValue(JSContext *cx, HandleScript script, HandleValue val)
{
    return fallbackMonitorStub_->addMonitorStubForValue(cx, script, val);
}

bool
ICUpdatedStub::initUpdatingChain(JSContext *cx, ICStubSpace *space)
{
    JS_ASSERT(firstUpdateStub_ == NULL);

    ICTypeUpdate_Fallback::Compiler compiler(cx);
    ICTypeUpdate_Fallback *stub = compiler.getStub(space);
    if (!stub)
        return false;

    firstUpdateStub_ = stub;
    return true;
}

IonCode *
ICStubCompiler::getStubCode()
{
    IonCompartment *ion = cx->compartment->ionCompartment();

    
    uint32_t stubKey = getKey();
    IonCode *stubCode = ion->getStubCode(stubKey);
    if (stubCode)
        return stubCode;

    
    MacroAssembler masm;
#ifdef JS_CPU_ARM
    masm.setSecondScratchReg(BaselineSecondScratchReg);
#endif

    AutoFlushCache afc("ICStubCompiler::getStubCode", cx->runtime->ionRuntime());
    if (!generateStubCode(masm))
        return NULL;
    Linker linker(masm);
    Rooted<IonCode *> newStubCode(cx, linker.newCode(cx, JSC::BASELINE_CODE));
    if (!newStubCode)
        return NULL;

    
    if (!postGenerateStubCode(masm, newStubCode))
        return NULL;

    
    if (cx->zone()->needsBarrier())
        newStubCode->togglePreBarriers(true);

    
    if (!ion->putStubCode(stubKey, newStubCode))
        return NULL;

    JS_ASSERT(entersStubFrame_ == ICStub::CanMakeCalls(kind));

    return newStubCode;
}

bool
ICStubCompiler::tailCallVM(const VMFunction &fun, MacroAssembler &masm)
{
    IonCompartment *ion = cx->compartment->ionCompartment();
    IonCode *code = ion->getVMWrapper(fun);
    if (!code)
        return false;

    uint32_t argSize = fun.explicitStackSlots() * sizeof(void *);
    EmitTailCallVM(code, masm, argSize);
    return true;
}

bool
ICStubCompiler::callVM(const VMFunction &fun, MacroAssembler &masm)
{
    IonCompartment *ion = cx->compartment->ionCompartment();
    IonCode *code = ion->getVMWrapper(fun);
    if (!code)
        return false;

    EmitCallVM(code, masm);
    return true;
}

bool
ICStubCompiler::callTypeUpdateIC(MacroAssembler &masm, uint32_t objectOffset)
{
    IonCompartment *ion = cx->compartment->ionCompartment();
    IonCode *code = ion->getVMWrapper(DoTypeUpdateFallbackInfo);
    if (!code)
        return false;

    EmitCallTypeUpdateIC(masm, code, objectOffset);
    return true;
}

void
ICStubCompiler::enterStubFrame(MacroAssembler &masm, Register scratch)
{
    EmitEnterStubFrame(masm, scratch);
#ifdef DEBUG
    entersStubFrame_ = true;
#endif
}

void
ICStubCompiler::leaveStubFrame(MacroAssembler &masm, bool calledIntoIon)
{
    JS_ASSERT(entersStubFrame_);
    EmitLeaveStubFrame(masm, calledIntoIon);
}

void
ICStubCompiler::guardProfilingEnabled(MacroAssembler &masm, Register scratch, Label *skip)
{
    
    JS_ASSERT(kind == ICStub::Call_Scripted      || kind == ICStub::Call_AnyScripted     ||
              kind == ICStub::Call_Native        || kind == ICStub::GetProp_CallScripted ||
              kind == ICStub::GetProp_CallNative || kind == ICStub::GetProp_CallListBaseNative ||
              kind == ICStub::Call_ScriptedApplyArguments ||
              kind == ICStub::GetProp_CallListBaseWithGenerationNative ||
              kind == ICStub::GetProp_ListBaseShadowed ||
              kind == ICStub::SetProp_CallScripted || kind == ICStub::SetProp_CallNative);

    
    
    
    JS_ASSERT(entersStubFrame_);
    masm.loadPtr(Address(BaselineFrameReg, 0), scratch);
    masm.branchTest32(Assembler::Zero,
                      Address(scratch, BaselineFrame::reverseOffsetOfFlags()),
                      Imm32(BaselineFrame::HAS_PUSHED_SPS_FRAME),
                      skip);

    
    uint32_t *enabledAddr = cx->runtime->spsProfiler.addressOfEnabled();
    masm.branch32(Assembler::Equal, AbsoluteAddress(enabledAddr), Imm32(0), skip);
}




static bool
IsTopFrameConstructing(JSContext *cx)
{
    IonFrameIterator iter(cx);
    JS_ASSERT(iter.type() == IonFrame_Exit);

    ++iter;
    JS_ASSERT(iter.type() == IonFrame_BaselineStub);

    ++iter;
    JS_ASSERT(iter.isBaselineJS());

    return iter.isConstructing();
}

static bool
EnsureCanEnterIon(JSContext *cx, ICUseCount_Fallback *stub, BaselineFrame *frame,
                  HandleScript script, jsbytecode *pc, void **jitcodePtr)
{
    JS_ASSERT(jitcodePtr);
    JS_ASSERT(!*jitcodePtr);

    bool isLoopEntry = (JSOp(*pc) == JSOP_LOOPENTRY);

    bool isConstructing = IsTopFrameConstructing(cx);
    MethodStatus stat;
    if (isLoopEntry) {
        IonSpew(IonSpew_BaselineOSR, "  Compile at loop entry!");
        stat = CanEnterAtBranch(cx, script, frame, pc, isConstructing);
    } else if (frame->isFunctionFrame()) {
        IonSpew(IonSpew_BaselineOSR, "  Compile function from top for later entry!");
        stat = CompileFunctionForBaseline(cx, script, frame, isConstructing);
    } else {
        return true;
    }

    if (stat == Method_Error) {
        IonSpew(IonSpew_BaselineOSR, "  Compile with Ion errored!");
        return false;
    }

    if (stat == Method_CantCompile)
        IonSpew(IonSpew_BaselineOSR, "  Can't compile with Ion!");
    else if (stat == Method_Skipped)
        IonSpew(IonSpew_BaselineOSR, "  Skipped compile with Ion!");
    else if (stat == Method_Compiled)
        IonSpew(IonSpew_BaselineOSR, "  Compiled with Ion!");
    else
        JS_NOT_REACHED("Invalid MethodStatus!");

    
    if (stat != Method_Compiled) {
        
        
        bool bailoutExpected = script->hasIonScript() && script->ionScript()->bailoutExpected();
        if (stat == Method_CantCompile || bailoutExpected) {
            IonSpew(IonSpew_BaselineOSR, "  Reset UseCount cantCompile=%s bailoutExpected=%s!",
                    stat == Method_CantCompile ? "yes" : "no",
                    bailoutExpected ? "yes" : "no");
            script->resetUseCount();
        }
        return true;
    }

    if (isLoopEntry) {
        IonSpew(IonSpew_BaselineOSR, "  OSR possible!");
        IonScript *ion = script->ionScript();
        *jitcodePtr = ion->method()->raw() + ion->osrEntryOffset();
    }

    return true;
}


























struct IonOsrTempData
{
    void *jitcode;
    uint8_t *stackFrame;
};

static IonOsrTempData *
PrepareOsrTempData(JSContext *cx, ICUseCount_Fallback *stub, BaselineFrame *frame,
                   HandleScript script, jsbytecode *pc, void *jitcode)
{
    
    size_t numLocalsAndStackVals = frame->numValueSlots();
    size_t numFormalArgs = frame->isFunctionFrame() ? frame->numFormalArgs() : 0;

    
    
    
    
    
    
    
    

    size_t stackFrameSpace = (sizeof(Value) * numLocalsAndStackVals) + sizeof(StackFrame)
                           + (sizeof(Value) * (numFormalArgs + 1));
    size_t ionOsrTempDataSpace = sizeof(IonOsrTempData);

    size_t totalSpace = AlignBytes(stackFrameSpace, sizeof(Value)) +
                        AlignBytes(ionOsrTempDataSpace, sizeof(Value));

    IonOsrTempData *info = (IonOsrTempData *)cx->runtime->getIonRuntime(cx)->allocateOsrTempData(totalSpace);
    if (!info)
        return NULL;

    memset(info, 0, totalSpace);

    info->jitcode = jitcode;

    uint8_t *stackFrameStart = (uint8_t *)info + AlignBytes(ionOsrTempDataSpace, sizeof(Value));
    info->stackFrame = stackFrameStart + (numFormalArgs * sizeof(Value)) + sizeof(Value);

    
    
    

    
    memcpy(stackFrameStart, frame->argv() - 1, (numFormalArgs + 1) * sizeof(Value));

    
    uint8_t *stackFrame = info->stackFrame;
    *((JSObject **) (stackFrame + StackFrame::offsetOfScopeChain())) = frame->scopeChain();
    if (frame->isFunctionFrame()) {
        
        *((JSFunction **) (stackFrame + StackFrame::offsetOfExec())) = frame->fun();
        *((uint32_t *) (stackFrame + StackFrame::offsetOfFlags())) = StackFrame::FUNCTION;
    } else {
        *((JSScript **) (stackFrame + StackFrame::offsetOfExec())) = frame->script();
        *((uint32_t *) (stackFrame + StackFrame::offsetOfFlags())) = 0;
    }

    
    
    
    Value *stackFrameLocalsStart = (Value *) (stackFrame + sizeof(StackFrame));
    for (size_t i = 0; i < numLocalsAndStackVals; i++)
        stackFrameLocalsStart[i] = *(frame->valueSlot(i));

    IonSpew(IonSpew_BaselineOSR, "Allocated IonOsrTempData at %p", (void *) info);
    IonSpew(IonSpew_BaselineOSR, "Jitcode is %p", info->jitcode);

    
    return info;
}

static bool
DoUseCountFallback(JSContext *cx, ICUseCount_Fallback *stub, BaselineFrame *frame,
                   IonOsrTempData **infoPtr)
{
    JS_ASSERT(infoPtr);
    *infoPtr = NULL;

    
    if (!ion::IsEnabled(cx))
        return true;

    RootedScript script(cx, frame->script());
    jsbytecode *pc = stub->icEntry()->pc(script);
    bool isLoopEntry = JSOp(*pc) == JSOP_LOOPENTRY;

    FallbackICSpew(cx, stub, "UseCount(%d)", isLoopEntry ? int(pc - script->code) : int(-1));

    if (!script->canIonCompile()) {
        
        
        
        script->resetUseCount();
        return true;
    }

    JS_ASSERT(!script->isIonCompilingOffThread());

    
    
    if (script->hasIonScript() && !isLoopEntry) {
        IonSpew(IonSpew_BaselineOSR, "IonScript exists, but not at loop entry!");
        
        
        
        return true;
    }

    
    IonSpew(IonSpew_BaselineOSR,
            "UseCount for %s:%d reached %d at pc %p, trying to switch to Ion!",
            script->filename(), script->lineno, (int) script->getUseCount(), (void *) pc);
    void *jitcode = NULL;
    if (!EnsureCanEnterIon(cx, stub, frame, script, pc, &jitcode))
        return false;

    
    JS_ASSERT_IF(!isLoopEntry, !jitcode);
    if (!jitcode)
        return true;

    
    IonSpew(IonSpew_BaselineOSR, "Got jitcode.  Preparing for OSR into ion.");
    IonOsrTempData *info = PrepareOsrTempData(cx, stub, frame, script, pc, jitcode);
    if (!info)
        return false;
    *infoPtr = info;

    return true;
}

typedef bool (*DoUseCountFallbackFn)(JSContext *, ICUseCount_Fallback *, BaselineFrame *frame,
                                     IonOsrTempData **infoPtr);
static const VMFunction DoUseCountFallbackInfo =
    FunctionInfo<DoUseCountFallbackFn>(DoUseCountFallback);

bool
ICUseCount_Fallback::Compiler::generateStubCode(MacroAssembler &masm)
{
    
    
    masm.movePtr(BaselineFrameReg, R0.scratchReg());

    
    enterStubFrame(masm, R1.scratchReg());

    Label noCompiledCode;
    
    {
        
        masm.subPtr(Imm32(sizeof(void *)), BaselineStackReg);
        masm.push(BaselineStackReg);

        
        masm.loadBaselineFramePtr(R0.scratchReg(), R0.scratchReg());
        masm.push(R0.scratchReg());

        
        masm.push(BaselineStubReg);

        if (!callVM(DoUseCountFallbackInfo, masm))
            return false;

        
        masm.pop(R0.scratchReg());

        leaveStubFrame(masm);

        
        masm.branchPtr(Assembler::Equal, R0.scratchReg(), ImmWord((void*) NULL), &noCompiledCode);
    }

    
    GeneralRegisterSet regs(availableGeneralRegs(0));
    Register osrDataReg = R0.scratchReg();
    regs.take(osrDataReg);
    regs.takeUnchecked(OsrFrameReg);

    Register scratchReg = regs.takeAny();

    
    
    
    
    
    
    

    
    masm.movePtr(BaselineFrameReg, BaselineStackReg);

    
    
    masm.pop(scratchReg);

    
    masm.loadPtr(Address(osrDataReg, offsetof(IonOsrTempData, jitcode)), scratchReg);
    masm.loadPtr(Address(osrDataReg, offsetof(IonOsrTempData, stackFrame)), OsrFrameReg);
    masm.jump(scratchReg);

    
    masm.bind(&noCompiledCode);
    EmitReturnFromIC(masm);
    return true;
}





static bool
DoProfilerFallback(JSContext *cx, BaselineFrame *frame, ICProfiler_Fallback *stub)
{
    RootedScript script(cx, frame->script());
    RootedFunction func(cx, frame->maybeFun());
    mozilla::DebugOnly<ICEntry *> icEntry = stub->icEntry();

    FallbackICSpew(cx, stub, "Profiler");

    SPSProfiler *profiler = &cx->runtime->spsProfiler;

    
    JS_ASSERT(profiler->enabled());
    if (!cx->runtime->spsProfiler.enter(cx, script, func))
        return false;
    frame->setPushedSPSFrame();

    
    
    JS_ASSERT_IF(icEntry->firstStub() != stub,
                 icEntry->firstStub()->isProfiler_PushFunction() &&
                 icEntry->firstStub()->next() == stub);
    stub->unlinkStubsWithKind(cx, ICStub::Profiler_PushFunction);
    JS_ASSERT(icEntry->firstStub() == stub);

    
    const char *string = profiler->profileString(cx, script, func);
    if (string == NULL)
        return false;

    IonSpew(IonSpew_BaselineIC, "  Generating Profiler_PushFunction stub for %s:%d",
            script->filename(), script->lineno);

    
    ICProfiler_PushFunction::Compiler compiler(cx, string, script);
    ICStub *optStub = compiler.getStub(compiler.getStubSpace(script));
    if (!optStub)
        return false;
    stub->addNewStub(optStub);

    return true;
}

typedef bool (*DoProfilerFallbackFn)(JSContext *, BaselineFrame *frame, ICProfiler_Fallback *);
static const VMFunction DoProfilerFallbackInfo =
    FunctionInfo<DoProfilerFallbackFn>(DoProfilerFallback);

bool
ICProfiler_Fallback::Compiler::generateStubCode(MacroAssembler &masm)
{
    EmitRestoreTailCallReg(masm);

    masm.push(BaselineStubReg);         
    masm.pushBaselineFramePtr(BaselineFrameReg, R0.scratchReg()); 

    return tailCallVM(DoProfilerFallbackInfo, masm);
}

bool
ICProfiler_PushFunction::Compiler::generateStubCode(MacroAssembler &masm)
{

    Register scratch = R0.scratchReg();
    Register scratch2 = R1.scratchReg();

    
#ifdef DEBUG
    Label spsEnabled;
    uint32_t *enabledAddr = cx->runtime->spsProfiler.addressOfEnabled();
    masm.branch32(Assembler::NotEqual, AbsoluteAddress(enabledAddr), Imm32(0), &spsEnabled);
    masm.breakpoint();
    masm.bind(&spsEnabled);
#endif

    
    masm.spsPushFrame(&cx->runtime->spsProfiler,
                      Address(BaselineStubReg, ICProfiler_PushFunction::offsetOfStr()),
                      Address(BaselineStubReg, ICProfiler_PushFunction::offsetOfScript()),
                      scratch,
                      scratch2);

    
    Address flagsOffset(BaselineFrameReg, BaselineFrame::reverseOffsetOfFlags());
    masm.or32(Imm32(BaselineFrame::HAS_PUSHED_SPS_FRAME), flagsOffset);

    EmitReturnFromIC(masm);

    return true;
}





bool
ICTypeMonitor_Fallback::addMonitorStubForValue(JSContext *cx, HandleScript script, HandleValue val)
{
    bool wasDetachedMonitorChain = lastMonitorStubPtrAddr_ == NULL;
    JS_ASSERT_IF(wasDetachedMonitorChain, numOptimizedMonitorStubs_ == 0);

    if (numOptimizedMonitorStubs_ >= MAX_OPTIMIZED_STUBS) {
        
        
        return true;
    }

    if (val.isPrimitive()) {
        JS_ASSERT(!val.isMagic());
        JSValueType type = val.isDouble() ? JSVAL_TYPE_DOUBLE : val.extractNonDoubleType();

        
        ICTypeMonitor_PrimitiveSet *existingStub = NULL;
        for (ICStubConstIterator iter = firstMonitorStub(); !iter.atEnd(); iter++) {
            if (iter->isTypeMonitor_PrimitiveSet()) {
                existingStub = iter->toTypeMonitor_PrimitiveSet();
                if (existingStub->containsType(type))
                    return true;
            }
        }

        ICTypeMonitor_PrimitiveSet::Compiler compiler(cx, existingStub, type);
        ICStub *stub = existingStub ? compiler.updateStub()
                                    : compiler.getStub(compiler.getStubSpace(script));
        if (!stub)
            return false;

        IonSpew(IonSpew_BaselineIC, "  %s TypeMonitor stub %p for primitive type %d",
                existingStub ? "Modified existing" : "Created new", stub, type);

        if (!existingStub) {
            JS_ASSERT(!hasStub(TypeMonitor_PrimitiveSet));
            addOptimizedMonitorStub(stub);
        }

    } else if (val.toObject().hasSingletonType()) {
        RootedObject obj(cx, &val.toObject());

        
        for (ICStubConstIterator iter = firstMonitorStub(); !iter.atEnd(); iter++) {
            if (iter->isTypeMonitor_SingleObject() &&
                iter->toTypeMonitor_SingleObject()->object() == obj)
            {
                return true;
            }
        }

        ICTypeMonitor_SingleObject::Compiler compiler(cx, obj);
        ICStub *stub = compiler.getStub(compiler.getStubSpace(script));
        if (!stub)
            return false;

        IonSpew(IonSpew_BaselineIC, "  Added TypeMonitor stub %p for singleton %p",
                stub, obj.get());

        addOptimizedMonitorStub(stub);

    } else {
        RootedTypeObject type(cx, val.toObject().type());

        
        for (ICStubConstIterator iter = firstMonitorStub(); !iter.atEnd(); iter++) {
            if (iter->isTypeMonitor_TypeObject() &&
                iter->toTypeMonitor_TypeObject()->type() == type)
            {
                return true;
            }
        }

        ICTypeMonitor_TypeObject::Compiler compiler(cx, type);
        ICStub *stub = compiler.getStub(compiler.getStubSpace(script));
        if (!stub)
            return false;

        IonSpew(IonSpew_BaselineIC, "  Added TypeMonitor stub %p for TypeObject %p",
                stub, type.get());

        addOptimizedMonitorStub(stub);
    }

    bool firstMonitorStubAdded = wasDetachedMonitorChain && (numOptimizedMonitorStubs_ > 0);

    if (firstMonitorStubAdded) {
        
        
        
        ICStub *firstStub = mainFallbackStub_->icEntry()->firstStub();
        for (ICStubConstIterator iter = firstStub; !iter.atEnd(); iter++) {
            
            
            if (!iter->isMonitored())
                continue;

            
            
            
            JS_ASSERT(iter->toMonitoredStub()->firstMonitorStub() == this);
            iter->toMonitoredStub()->updateFirstMonitorStub(firstMonitorStub_);
        }
    }

    return true;
}

static bool
DoTypeMonitorFallback(JSContext *cx, BaselineFrame *frame, ICTypeMonitor_Fallback *stub,
                      HandleValue value, MutableHandleValue res)
{
    RootedScript script(cx, frame->script());
    jsbytecode *pc = stub->icEntry()->pc(script);
    TypeFallbackICSpew(cx, stub, "TypeMonitor");

    uint32_t argument;
    if (stub->monitorsThis()) {
        JS_ASSERT(pc == script->code);
        types::TypeScript::SetThis(cx, script, value);
    } else if (stub->monitorsArgument(&argument)) {
        JS_ASSERT(pc == script->code);
        types::TypeScript::SetArgument(cx, script, argument, value);
    } else {
        types::TypeScript::Monitor(cx, script, pc, value);
    }

    if (!stub->addMonitorStubForValue(cx, script, value))
        return false;

    
    res.set(value);
    return true;
}

typedef bool (*DoTypeMonitorFallbackFn)(JSContext *, BaselineFrame *, ICTypeMonitor_Fallback *,
                                        HandleValue, MutableHandleValue);
static const VMFunction DoTypeMonitorFallbackInfo =
    FunctionInfo<DoTypeMonitorFallbackFn>(DoTypeMonitorFallback);

bool
ICTypeMonitor_Fallback::Compiler::generateStubCode(MacroAssembler &masm)
{
    JS_ASSERT(R0 == JSReturnOperand);

    
    EmitRestoreTailCallReg(masm);

    masm.pushValue(R0);
    masm.push(BaselineStubReg);
    masm.pushBaselineFramePtr(BaselineFrameReg, R0.scratchReg());

    return tailCallVM(DoTypeMonitorFallbackInfo, masm);
}

bool
ICTypeMonitor_PrimitiveSet::Compiler::generateStubCode(MacroAssembler &masm)
{
    Label success;
    if ((flags_ & TypeToFlag(JSVAL_TYPE_INT32)) && !(flags_ & TypeToFlag(JSVAL_TYPE_DOUBLE)))
        masm.branchTestInt32(Assembler::Equal, R0, &success);

    if (flags_ & TypeToFlag(JSVAL_TYPE_DOUBLE))
        masm.branchTestNumber(Assembler::Equal, R0, &success);

    if (flags_ & TypeToFlag(JSVAL_TYPE_UNDEFINED))
        masm.branchTestUndefined(Assembler::Equal, R0, &success);

    if (flags_ & TypeToFlag(JSVAL_TYPE_BOOLEAN))
        masm.branchTestBoolean(Assembler::Equal, R0, &success);

    if (flags_ & TypeToFlag(JSVAL_TYPE_STRING))
        masm.branchTestString(Assembler::Equal, R0, &success);

    
    
    
    
    
    



    JS_ASSERT(!(flags_ & TypeToFlag(JSVAL_TYPE_OBJECT)));

    if (flags_ & TypeToFlag(JSVAL_TYPE_NULL))
        masm.branchTestNull(Assembler::Equal, R0, &success);

    EmitStubGuardFailure(masm);

    masm.bind(&success);
    EmitReturnFromIC(masm);
    return true;
}

bool
ICTypeMonitor_SingleObject::Compiler::generateStubCode(MacroAssembler &masm)
{
    Label failure;
    masm.branchTestObject(Assembler::NotEqual, R0, &failure);

    
    Register obj = masm.extractObject(R0, ExtractTemp0);
    Address expectedObject(BaselineStubReg, ICTypeMonitor_SingleObject::offsetOfObject());
    masm.branchPtr(Assembler::NotEqual, expectedObject, obj, &failure);

    EmitReturnFromIC(masm);

    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}

bool
ICTypeMonitor_TypeObject::Compiler::generateStubCode(MacroAssembler &masm)
{
    Label failure;
    masm.branchTestObject(Assembler::NotEqual, R0, &failure);

    
    Register obj = masm.extractObject(R0, ExtractTemp0);
    masm.loadPtr(Address(obj, JSObject::offsetOfType()), R1.scratchReg());

    Address expectedType(BaselineStubReg, ICTypeMonitor_TypeObject::offsetOfType());
    masm.branchPtr(Assembler::NotEqual, expectedType, R1.scratchReg(), &failure);

    EmitReturnFromIC(masm);

    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}

bool
ICUpdatedStub::addUpdateStubForValue(JSContext *cx, HandleScript script, HandleObject obj,
                                     jsid id, HandleValue val)
{
    if (numOptimizedStubs_ >= MAX_OPTIMIZED_STUBS) {
        
        
        return true;
    }

    types::EnsureTrackPropertyTypes(cx, obj, id);

    if (val.isPrimitive()) {
        JSValueType type = val.isDouble() ? JSVAL_TYPE_DOUBLE : val.extractNonDoubleType();

        
        ICTypeUpdate_PrimitiveSet *existingStub = NULL;
        for (ICStubConstIterator iter = firstUpdateStub_; !iter.atEnd(); iter++) {
            if (iter->isTypeUpdate_PrimitiveSet()) {
                existingStub = iter->toTypeUpdate_PrimitiveSet();
                if (existingStub->containsType(type))
                    return true;
            }
        }

        ICTypeUpdate_PrimitiveSet::Compiler compiler(cx, existingStub, type);
        ICStub *stub = existingStub ? compiler.updateStub()
                                    : compiler.getStub(compiler.getStubSpace(script));
        if (!stub)
            return false;
        if (!existingStub) {
            JS_ASSERT(!hasTypeUpdateStub(TypeUpdate_PrimitiveSet));
            addOptimizedUpdateStub(stub);
        }

        IonSpew(IonSpew_BaselineIC, "  %s TypeUpdate stub %p for primitive type %d",
                existingStub ? "Modified existing" : "Created new", stub, type);

    } else if (val.toObject().hasSingletonType()) {
        RootedObject obj(cx, &val.toObject());

        
        for (ICStubConstIterator iter = firstUpdateStub_; !iter.atEnd(); iter++) {
            if (iter->isTypeUpdate_SingleObject() &&
                iter->toTypeUpdate_SingleObject()->object() == obj)
            {
                return true;
            }
        }

        ICTypeUpdate_SingleObject::Compiler compiler(cx, obj);
        ICStub *stub = compiler.getStub(compiler.getStubSpace(script));
        if (!stub)
            return false;

        IonSpew(IonSpew_BaselineIC, "  Added TypeUpdate stub %p for singleton %p", stub, obj.get());

        addOptimizedUpdateStub(stub);

    } else {
        RootedTypeObject type(cx, val.toObject().type());

        
        for (ICStubConstIterator iter = firstUpdateStub_; !iter.atEnd(); iter++) {
            if (iter->isTypeUpdate_TypeObject() &&
                iter->toTypeUpdate_TypeObject()->type() == type)
            {
                return true;
            }
        }

        ICTypeUpdate_TypeObject::Compiler compiler(cx, type);
        ICStub *stub = compiler.getStub(compiler.getStubSpace(script));
        if (!stub)
            return false;

        IonSpew(IonSpew_BaselineIC, "  Added TypeUpdate stub %p for TypeObject %p",
                stub, type.get());

        addOptimizedUpdateStub(stub);
    }

    return true;
}




static bool
DoTypeUpdateFallback(JSContext *cx, BaselineFrame *frame, ICUpdatedStub *stub, HandleValue objval,
                     HandleValue value)
{
    FallbackICSpew(cx, stub->getChainFallback(), "TypeUpdate(%s)",
                   ICStub::KindString(stub->kind()));

    RootedScript script(cx, frame->script());
    RootedObject obj(cx, &objval.toObject());
    RootedId id(cx);

    switch(stub->kind()) {
      case ICStub::SetElem_Dense:
      case ICStub::SetElem_DenseAdd: {
        JS_ASSERT(obj->isNative());
        id = JSID_VOID;
        types::AddTypePropertyId(cx, obj, id, value);
        break;
      }
      case ICStub::SetProp_Native:
      case ICStub::SetProp_NativeAdd: {
        JS_ASSERT(obj->isNative());
        jsbytecode *pc = stub->getChainFallback()->icEntry()->pc(script);
        id = NameToId(script->getName(pc));
        types::AddTypePropertyId(cx, obj, id, value);
        break;
      }
      default:
        JS_NOT_REACHED("Invalid stub");
        return false;
    }

    return stub->addUpdateStubForValue(cx, script, obj, id, value);
}

typedef bool (*DoTypeUpdateFallbackFn)(JSContext *, BaselineFrame *, ICUpdatedStub *, HandleValue,
                                       HandleValue);
const VMFunction DoTypeUpdateFallbackInfo =
    FunctionInfo<DoTypeUpdateFallbackFn>(DoTypeUpdateFallback);

bool
ICTypeUpdate_Fallback::Compiler::generateStubCode(MacroAssembler &masm)
{
    
    masm.move32(Imm32(0), R1.scratchReg());
    EmitReturnFromIC(masm);
    return true;
}

bool
ICTypeUpdate_PrimitiveSet::Compiler::generateStubCode(MacroAssembler &masm)
{
    Label success;
    if ((flags_ & TypeToFlag(JSVAL_TYPE_INT32)) && !(flags_ & TypeToFlag(JSVAL_TYPE_DOUBLE)))
        masm.branchTestInt32(Assembler::Equal, R0, &success);

    if (flags_ & TypeToFlag(JSVAL_TYPE_DOUBLE))
        masm.branchTestNumber(Assembler::Equal, R0, &success);

    if (flags_ & TypeToFlag(JSVAL_TYPE_UNDEFINED))
        masm.branchTestUndefined(Assembler::Equal, R0, &success);

    if (flags_ & TypeToFlag(JSVAL_TYPE_BOOLEAN))
        masm.branchTestBoolean(Assembler::Equal, R0, &success);

    if (flags_ & TypeToFlag(JSVAL_TYPE_STRING))
        masm.branchTestString(Assembler::Equal, R0, &success);

    
    
    
    
    
    



    JS_ASSERT(!(flags_ & TypeToFlag(JSVAL_TYPE_OBJECT)));

    if (flags_ & TypeToFlag(JSVAL_TYPE_NULL))
        masm.branchTestNull(Assembler::Equal, R0, &success);

    EmitStubGuardFailure(masm);

    
    masm.bind(&success);
    masm.mov(Imm32(1), R1.scratchReg());
    EmitReturnFromIC(masm);

    return true;
}

bool
ICTypeUpdate_SingleObject::Compiler::generateStubCode(MacroAssembler &masm)
{
    Label failure;
    masm.branchTestObject(Assembler::NotEqual, R0, &failure);

    
    Register obj = masm.extractObject(R0, R1.scratchReg());
    Address expectedObject(BaselineStubReg, ICTypeUpdate_SingleObject::offsetOfObject());
    masm.branchPtr(Assembler::NotEqual, expectedObject, obj, &failure);

    
    masm.mov(Imm32(1), R1.scratchReg());
    EmitReturnFromIC(masm);

    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}

bool
ICTypeUpdate_TypeObject::Compiler::generateStubCode(MacroAssembler &masm)
{
    Label failure;
    masm.branchTestObject(Assembler::NotEqual, R0, &failure);

    
    Register obj = masm.extractObject(R0, R1.scratchReg());
    masm.loadPtr(Address(obj, JSObject::offsetOfType()), R1.scratchReg());

    Address expectedType(BaselineStubReg, ICTypeUpdate_TypeObject::offsetOfType());
    masm.branchPtr(Assembler::NotEqual, expectedType, R1.scratchReg(), &failure);

    
    masm.mov(Imm32(1), R1.scratchReg());
    EmitReturnFromIC(masm);

    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}





static bool
DoThisFallback(JSContext *cx, ICThis_Fallback *stub, HandleValue thisv, MutableHandleValue ret)
{
    FallbackICSpew(cx, stub, "This");

    ret.set(thisv);
    bool modified;
    if (!BoxNonStrictThis(cx, ret, &modified))
        return false;
    return true;
}

typedef bool (*DoThisFallbackFn)(JSContext *, ICThis_Fallback *, HandleValue, MutableHandleValue);
static const VMFunction DoThisFallbackInfo = FunctionInfo<DoThisFallbackFn>(DoThisFallback);

bool
ICThis_Fallback::Compiler::generateStubCode(MacroAssembler &masm)
{
    JS_ASSERT(R0 == JSReturnOperand);

    
    EmitRestoreTailCallReg(masm);

    masm.pushValue(R0);
    masm.push(BaselineStubReg);

    return tailCallVM(DoThisFallbackInfo, masm);
}





static bool
DoNewArray(JSContext *cx, ICNewArray_Fallback *stub, uint32_t length,
           HandleTypeObject type, MutableHandleValue res)
{
    FallbackICSpew(cx, stub, "NewArray");

    JSObject *obj = NewInitArray(cx, length, type);
    if (!obj)
        return false;

    res.setObject(*obj);
    return true;
}

typedef bool(*DoNewArrayFn)(JSContext *, ICNewArray_Fallback *, uint32_t, HandleTypeObject,
                            MutableHandleValue);
static const VMFunction DoNewArrayInfo = FunctionInfo<DoNewArrayFn>(DoNewArray);

bool
ICNewArray_Fallback::Compiler::generateStubCode(MacroAssembler &masm)
{
    EmitRestoreTailCallReg(masm);

    masm.push(R1.scratchReg()); 
    masm.push(R0.scratchReg()); 
    masm.push(BaselineStubReg); 

    return tailCallVM(DoNewArrayInfo, masm);
}





static bool
DoNewObject(JSContext *cx, ICNewObject_Fallback *stub, HandleObject templateObject,
            MutableHandleValue res)
{
    FallbackICSpew(cx, stub, "NewObject");

    JSObject *obj = NewInitObject(cx, templateObject);
    if (!obj)
        return false;

    res.setObject(*obj);
    return true;
}

typedef bool(*DoNewObjectFn)(JSContext *, ICNewObject_Fallback *, HandleObject,
                             MutableHandleValue);
static const VMFunction DoNewObjectInfo = FunctionInfo<DoNewObjectFn>(DoNewObject);

bool
ICNewObject_Fallback::Compiler::generateStubCode(MacroAssembler &masm)
{
    EmitRestoreTailCallReg(masm);

    masm.push(R0.scratchReg()); 
    masm.push(BaselineStubReg); 

    return tailCallVM(DoNewObjectInfo, masm);
}





static bool
DoCompareFallback(JSContext *cx, BaselineFrame *frame, ICCompare_Fallback *stub, HandleValue lhs,
                  HandleValue rhs, MutableHandleValue ret)
{
    jsbytecode *pc = stub->icEntry()->pc(frame->script());
    JSOp op = JSOp(*pc);

    FallbackICSpew(cx, stub, "Compare(%s)", js_CodeName[op]);

    
    if (op == JSOP_CASE)
        op = JSOP_STRICTEQ;

    
    
    RootedValue lhsCopy(cx, lhs);
    RootedValue rhsCopy(cx, rhs);

    
    JSBool out;
    switch(op) {
      case JSOP_LT:
        if (!LessThan(cx, &lhsCopy, &rhsCopy, &out))
            return false;
        break;
      case JSOP_LE:
        if (!LessThanOrEqual(cx, &lhsCopy, &rhsCopy, &out))
            return false;
        break;
      case JSOP_GT:
        if (!GreaterThan(cx, &lhsCopy, &rhsCopy, &out))
            return false;
        break;
      case JSOP_GE:
        if (!GreaterThanOrEqual(cx, &lhsCopy, &rhsCopy, &out))
            return false;
        break;
      case JSOP_EQ:
        if (!LooselyEqual<true>(cx, &lhsCopy, &rhsCopy, &out))
            return false;
        break;
      case JSOP_NE:
        if (!LooselyEqual<false>(cx, &lhsCopy, &rhsCopy, &out))
            return false;
        break;
      case JSOP_STRICTEQ:
        if (!StrictlyEqual<true>(cx, &lhsCopy, &rhsCopy, &out))
            return false;
        break;
      case JSOP_STRICTNE:
        if (!StrictlyEqual<false>(cx, &lhsCopy, &rhsCopy, &out))
            return false;
        break;
      default:
        JS_ASSERT(!"Unhandled baseline compare op");
        return false;
    }

    ret.setBoolean(out);

    
    if (stub->numOptimizedStubs() >= ICCompare_Fallback::MAX_OPTIMIZED_STUBS) {
        
        
        return true;
    }

    JSScript *script = frame->script();

    
    if (lhs.isInt32() && rhs.isInt32()) {
        IonSpew(IonSpew_BaselineIC, "  Generating %s(Int32, Int32) stub", js_CodeName[op]);
        ICCompare_Int32::Compiler compiler(cx, op);
        ICStub *int32Stub = compiler.getStub(compiler.getStubSpace(script));
        if (!int32Stub)
            return false;

        stub->addNewStub(int32Stub);
        return true;
    }

    if (!cx->runtime->jitSupportsFloatingPoint && (lhs.isNumber() || rhs.isNumber()))
        return true;

    if (lhs.isNumber() && rhs.isNumber()) {
        IonSpew(IonSpew_BaselineIC, "  Generating %s(Number, Number) stub", js_CodeName[op]);

        
        stub->unlinkStubsWithKind(cx, ICStub::Compare_Int32);

        ICCompare_Double::Compiler compiler(cx, op);
        ICStub *doubleStub = compiler.getStub(compiler.getStubSpace(script));
        if (!doubleStub)
            return false;

        stub->addNewStub(doubleStub);
        return true;
    }

    if ((lhs.isNumber() && rhs.isUndefined()) ||
        (lhs.isUndefined() && rhs.isNumber()))
    {
        IonSpew(IonSpew_BaselineIC, "  Generating %s(%s, %s) stub", js_CodeName[op],
                    rhs.isUndefined() ? "Number" : "Undefined",
                    rhs.isUndefined() ? "Undefined" : "Number");
        ICCompare_NumberWithUndefined::Compiler compiler(cx, op, lhs.isUndefined());
        ICStub *doubleStub = compiler.getStub(compiler.getStubSpace(script));
        if (!stub)
            return false;

        stub->addNewStub(doubleStub);
        return true;
    }

    if (lhs.isBoolean() && rhs.isBoolean()) {
        IonSpew(IonSpew_BaselineIC, "  Generating %s(Boolean, Boolean) stub", js_CodeName[op]);
        ICCompare_Boolean::Compiler compiler(cx, op);
        ICStub *booleanStub = compiler.getStub(compiler.getStubSpace(script));
        if (!booleanStub)
            return false;

        stub->addNewStub(booleanStub);
        return true;
    }

    if ((lhs.isBoolean() && rhs.isInt32()) || (lhs.isInt32() && rhs.isBoolean())) {
        IonSpew(IonSpew_BaselineIC, "  Generating %s(%s, %s) stub", js_CodeName[op],
                    rhs.isInt32() ? "Boolean" : "Int32",
                    rhs.isInt32() ? "Int32" : "Boolean");
        ICCompare_Int32WithBoolean::Compiler compiler(cx, op, lhs.isInt32());
        ICStub *optStub = compiler.getStub(compiler.getStubSpace(script));
        if (!optStub)
            return false;

        stub->addNewStub(optStub);
        return true;
    }

    if (IsEqualityOp(op)) {
        if (lhs.isString() && rhs.isString() && !stub->hasStub(ICStub::Compare_String)) {
            IonSpew(IonSpew_BaselineIC, "  Generating %s(String, String) stub", js_CodeName[op]);
            ICCompare_String::Compiler compiler(cx, op);
            ICStub *stringStub = compiler.getStub(compiler.getStubSpace(script));
            if (!stringStub)
                return false;

            stub->addNewStub(stringStub);
            return true;
        }

        if (lhs.isObject() && rhs.isObject()) {
            JS_ASSERT(!stub->hasStub(ICStub::Compare_Object));
            IonSpew(IonSpew_BaselineIC, "  Generating %s(Object, Object) stub", js_CodeName[op]);
            ICCompare_Object::Compiler compiler(cx, op);
            ICStub *objectStub = compiler.getStub(compiler.getStubSpace(script));
            if (!objectStub)
                return false;

            stub->addNewStub(objectStub);
            return true;
        }

        if ((lhs.isObject() || lhs.isNull() || lhs.isUndefined()) &&
            (rhs.isObject() || rhs.isNull() || rhs.isUndefined()) &&
            !stub->hasStub(ICStub::Compare_ObjectWithUndefined))
        {
            IonSpew(IonSpew_BaselineIC, "  Generating %s(Obj/Null/Undef, Obj/Null/Undef) stub",
                    js_CodeName[op]);
            bool lhsIsUndefined = lhs.isNull() || lhs.isUndefined();
            bool compareWithNull = lhs.isNull() || rhs.isNull();
            ICCompare_ObjectWithUndefined::Compiler compiler(cx, op,
                                                             lhsIsUndefined, compareWithNull);
            ICStub *objectStub = compiler.getStub(compiler.getStubSpace(script));
            if (!objectStub)
                return false;

            stub->addNewStub(objectStub);
            return true;
        }
    }

    return true;
}

typedef bool (*DoCompareFallbackFn)(JSContext *, BaselineFrame *, ICCompare_Fallback *,
                                    HandleValue, HandleValue, MutableHandleValue);
static const VMFunction DoCompareFallbackInfo =
    FunctionInfo<DoCompareFallbackFn>(DoCompareFallback, PopValues(2));

bool
ICCompare_Fallback::Compiler::generateStubCode(MacroAssembler &masm)
{
    JS_ASSERT(R0 == JSReturnOperand);

    
    EmitRestoreTailCallReg(masm);

    
    masm.pushValue(R0);
    masm.pushValue(R1);

    
    masm.pushValue(R1);
    masm.pushValue(R0);
    masm.push(BaselineStubReg);
    masm.pushBaselineFramePtr(BaselineFrameReg, R0.scratchReg());

    return tailCallVM(DoCompareFallbackInfo, masm);
}





bool
ICCompare_String::Compiler::generateStubCode(MacroAssembler &masm)
{
    Label failure;
    masm.branchTestString(Assembler::NotEqual, R0, &failure);
    masm.branchTestString(Assembler::NotEqual, R1, &failure);

    JS_ASSERT(IsEqualityOp(op));

    Register left = masm.extractString(R0, ExtractTemp0);
    Register right = masm.extractString(R1, ExtractTemp1);

    GeneralRegisterSet regs(availableGeneralRegs(2));
    Register scratchReg = regs.takeAny();
    
    Register scratchReg2;
    if (regs.empty()) {
        scratchReg2 = BaselineStubReg;
        masm.push(BaselineStubReg);
    } else {
        scratchReg2 = regs.takeAny();
    }
    JS_ASSERT(scratchReg2 != scratchReg);

    Label inlineCompareFailed;
    masm.compareStrings(op, left, right, scratchReg2, scratchReg, &inlineCompareFailed);
    masm.tagValue(JSVAL_TYPE_BOOLEAN, scratchReg2, R0);
    if (scratchReg2 == BaselineStubReg)
        masm.pop(BaselineStubReg);
    EmitReturnFromIC(masm);

    masm.bind(&inlineCompareFailed);
    if (scratchReg2 == BaselineStubReg)
        masm.pop(BaselineStubReg);
    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}





bool
ICCompare_Boolean::Compiler::generateStubCode(MacroAssembler &masm)
{
    Label failure;
    masm.branchTestBoolean(Assembler::NotEqual, R0, &failure);
    masm.branchTestBoolean(Assembler::NotEqual, R1, &failure);

    Register left = masm.extractInt32(R0, ExtractTemp0);
    Register right = masm.extractInt32(R1, ExtractTemp1);

    
    Assembler::Condition cond = JSOpToCondition(op, true);
    masm.cmp32(left, right);
    masm.emitSet(cond, left);

    
    masm.tagValue(JSVAL_TYPE_BOOLEAN, left, R0);
    EmitReturnFromIC(masm);

    
    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}





bool
ICCompare_NumberWithUndefined::Compiler::generateStubCode(MacroAssembler &masm)
{
    ValueOperand numberOperand, undefinedOperand;
    if (lhsIsUndefined) {
        numberOperand = R1;
        undefinedOperand = R0;
    } else {
        numberOperand = R0;
        undefinedOperand = R1;
    }

    Label failure;
    masm.branchTestNumber(Assembler::NotEqual, numberOperand, &failure);
    masm.branchTestUndefined(Assembler::NotEqual, undefinedOperand, &failure);

    
    
    masm.moveValue(BooleanValue(op == JSOP_NE || op == JSOP_STRICTNE), R0);

    EmitReturnFromIC(masm);

    
    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}





bool
ICCompare_Object::Compiler::generateStubCode(MacroAssembler &masm)
{
    Label failure;
    masm.branchTestObject(Assembler::NotEqual, R0, &failure);
    masm.branchTestObject(Assembler::NotEqual, R1, &failure);

    JS_ASSERT(IsEqualityOp(op));

    Register left = masm.extractObject(R0, ExtractTemp0);
    Register right = masm.extractObject(R1, ExtractTemp1);

    Label ifTrue;
    masm.branchPtr(JSOpToCondition(op, true), left, right, &ifTrue);

    masm.moveValue(BooleanValue(false), R0);
    EmitReturnFromIC(masm);

    masm.bind(&ifTrue);
    masm.moveValue(BooleanValue(true), R0);
    EmitReturnFromIC(masm);

    
    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}





bool
ICCompare_ObjectWithUndefined::Compiler::generateStubCode(MacroAssembler &masm)
{
    JS_ASSERT(IsEqualityOp(op));

    ValueOperand objectOperand, undefinedOperand;
    if (lhsIsUndefined) {
        objectOperand = R1;
        undefinedOperand = R0;
    } else {
        objectOperand = R0;
        undefinedOperand = R1;
    }

    Label failure;
    if (compareWithNull)
        masm.branchTestNull(Assembler::NotEqual, undefinedOperand, &failure);
    else
        masm.branchTestUndefined(Assembler::NotEqual, undefinedOperand, &failure);

    Label notObject;
    masm.branchTestObject(Assembler::NotEqual, objectOperand, &notObject);

    if (op == JSOP_STRICTEQ || op == JSOP_STRICTNE) {
        
        masm.moveValue(BooleanValue(op == JSOP_STRICTNE), R0);
        EmitReturnFromIC(masm);
    } else {
        
        Label emulatesUndefined;
        Register obj = masm.extractObject(objectOperand, ExtractTemp0);
        masm.loadPtr(Address(obj, JSObject::offsetOfType()), obj);
        masm.loadPtr(Address(obj, offsetof(types::TypeObject, clasp)), obj);
        masm.branchTest32(Assembler::NonZero,
                          Address(obj, Class::offsetOfFlags()),
                          Imm32(JSCLASS_EMULATES_UNDEFINED),
                          &emulatesUndefined);
        masm.moveValue(BooleanValue(op == JSOP_NE), R0);
        EmitReturnFromIC(masm);
        masm.bind(&emulatesUndefined);
        masm.moveValue(BooleanValue(op == JSOP_EQ), R0);
        EmitReturnFromIC(masm);
    }

    masm.bind(&notObject);

    
    if (compareWithNull)
        masm.branchTestNull(Assembler::NotEqual, objectOperand, &failure);
    else
        masm.branchTestUndefined(Assembler::NotEqual, objectOperand, &failure);

    masm.moveValue(BooleanValue(op == JSOP_STRICTEQ || op == JSOP_EQ), R0);
    EmitReturnFromIC(masm);

    
    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}





bool
ICCompare_Int32WithBoolean::Compiler::generateStubCode(MacroAssembler &masm)
{
    Label failure;
    ValueOperand int32Val;
    ValueOperand boolVal;
    if (lhsIsInt32_) {
        int32Val = R0;
        boolVal = R1;
    } else {
        boolVal = R0;
        int32Val = R1;
    }
    masm.branchTestBoolean(Assembler::NotEqual, boolVal, &failure);
    masm.branchTestInt32(Assembler::NotEqual, int32Val, &failure);

    if (op_ == JSOP_STRICTEQ || op_ == JSOP_STRICTNE) {
        
        masm.moveValue(BooleanValue(op_ == JSOP_STRICTNE), R0);
        EmitReturnFromIC(masm);
    } else {
        Register boolReg = masm.extractBoolean(boolVal, ExtractTemp0);
        Register int32Reg = masm.extractInt32(int32Val, ExtractTemp1);

        
        Assembler::Condition cond = JSOpToCondition(op_, true);
        masm.cmp32(lhsIsInt32_ ? int32Reg : boolReg,
                   lhsIsInt32_ ? boolReg : int32Reg);
        masm.emitSet(cond, R0.scratchReg());

        
        masm.tagValue(JSVAL_TYPE_BOOLEAN, R0.scratchReg(), R0);
        EmitReturnFromIC(masm);
    }

    
    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}





static bool
DoToBoolFallback(JSContext *cx, BaselineFrame *frame, ICToBool_Fallback *stub, HandleValue arg,
                 MutableHandleValue ret)
{
    FallbackICSpew(cx, stub, "ToBool");

    bool cond = ToBoolean(arg);
    ret.setBoolean(cond);

    
    if (stub->numOptimizedStubs() >= ICToBool_Fallback::MAX_OPTIMIZED_STUBS) {
        
        
        return true;
    }

    JS_ASSERT(!arg.isBoolean());

    JSScript *script = frame->script();

    
    if (arg.isInt32()) {
        IonSpew(IonSpew_BaselineIC, "  Generating ToBool(Int32) stub.");
        ICToBool_Int32::Compiler compiler(cx);
        ICStub *int32Stub = compiler.getStub(compiler.getStubSpace(script));
        if (!int32Stub)
            return false;

        stub->addNewStub(int32Stub);
        return true;
    }

    if (arg.isDouble() && cx->runtime->jitSupportsFloatingPoint) {
        IonSpew(IonSpew_BaselineIC, "  Generating ToBool(Double) stub.");
        ICToBool_Double::Compiler compiler(cx);
        ICStub *doubleStub = compiler.getStub(compiler.getStubSpace(script));
        if (!doubleStub)
            return false;

        stub->addNewStub(doubleStub);
        return true;
    }

    if (arg.isString()) {
        IonSpew(IonSpew_BaselineIC, "  Generating ToBool(String) stub");
        ICToBool_String::Compiler compiler(cx);
        ICStub *stringStub = compiler.getStub(compiler.getStubSpace(script));
        if (!stringStub)
            return false;

        stub->addNewStub(stringStub);
        return true;
    }

    if (arg.isNull() || arg.isUndefined()) {
        ICToBool_NullUndefined::Compiler compiler(cx);
        ICStub *nilStub = compiler.getStub(compiler.getStubSpace(script));
        if (!nilStub)
            return false;

        stub->addNewStub(nilStub);
        return true;
    }

    if (arg.isObject()) {
        IonSpew(IonSpew_BaselineIC, "  Generating ToBool(Object) stub.");
        ICToBool_Object::Compiler compiler(cx);
        ICStub *objStub = compiler.getStub(compiler.getStubSpace(script));
        if (!objStub)
            return false;

        stub->addNewStub(objStub);
        return true;
    }

    return true;
}

typedef bool (*pf)(JSContext *, BaselineFrame *, ICToBool_Fallback *, HandleValue,
                   MutableHandleValue);
static const VMFunction fun = FunctionInfo<pf>(DoToBoolFallback);

bool
ICToBool_Fallback::Compiler::generateStubCode(MacroAssembler &masm)
{
    JS_ASSERT(R0 == JSReturnOperand);

    
    EmitRestoreTailCallReg(masm);

    
    masm.pushValue(R0);
    masm.push(BaselineStubReg);
    masm.pushBaselineFramePtr(BaselineFrameReg, R0.scratchReg());

    return tailCallVM(fun, masm);
}





bool
ICToBool_Int32::Compiler::generateStubCode(MacroAssembler &masm)
{
    Label failure;
    masm.branchTestInt32(Assembler::NotEqual, R0, &failure);

    Label ifFalse;
    Assembler::Condition cond = masm.testInt32Truthy(false, R0);
    masm.j(cond, &ifFalse);

    masm.moveValue(BooleanValue(true), R0);
    EmitReturnFromIC(masm);

    masm.bind(&ifFalse);
    masm.moveValue(BooleanValue(false), R0);
    EmitReturnFromIC(masm);

    
    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}





bool
ICToBool_String::Compiler::generateStubCode(MacroAssembler &masm)
{
    Label failure;
    masm.branchTestString(Assembler::NotEqual, R0, &failure);

    Label ifFalse;
    Assembler::Condition cond = masm.testStringTruthy(false, R0);
    masm.j(cond, &ifFalse);

    masm.moveValue(BooleanValue(true), R0);
    EmitReturnFromIC(masm);

    masm.bind(&ifFalse);
    masm.moveValue(BooleanValue(false), R0);
    EmitReturnFromIC(masm);

    
    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}





bool
ICToBool_NullUndefined::Compiler::generateStubCode(MacroAssembler &masm)
{
    Label failure, ifFalse;
    masm.branchTestNull(Assembler::Equal, R0, &ifFalse);
    masm.branchTestUndefined(Assembler::NotEqual, R0, &failure);

    masm.bind(&ifFalse);
    masm.moveValue(BooleanValue(false), R0);
    EmitReturnFromIC(masm);

    
    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}





bool
ICToBool_Double::Compiler::generateStubCode(MacroAssembler &masm)
{
    Label failure, ifTrue;
    masm.branchTestDouble(Assembler::NotEqual, R0, &failure);
    masm.unboxDouble(R0, FloatReg0);
    Assembler::Condition cond = masm.testDoubleTruthy(true, FloatReg0);
    masm.j(cond, &ifTrue);

    masm.moveValue(BooleanValue(false), R0);
    EmitReturnFromIC(masm);

    masm.bind(&ifTrue);
    masm.moveValue(BooleanValue(true), R0);
    EmitReturnFromIC(masm);

    
    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}





bool
ICToBool_Object::Compiler::generateStubCode(MacroAssembler &masm)
{
    Label failure, ifFalse, slowPath;
    masm.branchTestObject(Assembler::NotEqual, R0, &failure);

    Register objReg = masm.extractObject(R0, ExtractTemp0);
    Register scratch = R1.scratchReg();
    Assembler::Condition cond = masm.branchTestObjectTruthy(false, objReg, scratch, &slowPath);
    masm.j(cond, &ifFalse);

    
    masm.moveValue(BooleanValue(true), R0);
    EmitReturnFromIC(masm);

    masm.bind(&ifFalse);
    masm.moveValue(BooleanValue(false), R0);
    EmitReturnFromIC(masm);

    masm.bind(&slowPath);
    masm.setupUnalignedABICall(1, scratch);
    masm.passABIArg(objReg);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, ObjectEmulatesUndefined));
    masm.xor32(Imm32(1), ReturnReg);
    masm.tagValue(JSVAL_TYPE_BOOLEAN, ReturnReg, R0);
    EmitReturnFromIC(masm);

    
    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}





static bool
DoToNumberFallback(JSContext *cx, ICToNumber_Fallback *stub, HandleValue arg, MutableHandleValue ret)
{
    FallbackICSpew(cx, stub, "ToNumber");
    ret.set(arg);
    return ToNumber(cx, ret.address());
}

typedef bool (*DoToNumberFallbackFn)(JSContext *, ICToNumber_Fallback *, HandleValue, MutableHandleValue);
static const VMFunction DoToNumberFallbackInfo =
    FunctionInfo<DoToNumberFallbackFn>(DoToNumberFallback, PopValues(1));

bool
ICToNumber_Fallback::Compiler::generateStubCode(MacroAssembler &masm)
{
    JS_ASSERT(R0 == JSReturnOperand);

    
    EmitRestoreTailCallReg(masm);

    
    masm.pushValue(R0);

    
    masm.pushValue(R0);
    masm.push(BaselineStubReg);

    return tailCallVM(DoToNumberFallbackInfo, masm);
}






#if defined(_MSC_VER)
# pragma optimize("g", off)
#endif
static bool
DoBinaryArithFallback(JSContext *cx, BaselineFrame *frame, ICBinaryArith_Fallback *stub,
                      HandleValue lhs, HandleValue rhs, MutableHandleValue ret)
{
    RootedScript script(cx, frame->script());
    jsbytecode *pc = stub->icEntry()->pc(script);
    JSOp op = JSOp(*pc);
    FallbackICSpew(cx, stub, "BinaryArith(%s,%d,%d)", js_CodeName[op],
            int(lhs.isDouble() ? JSVAL_TYPE_DOUBLE : lhs.extractNonDoubleType()),
            int(rhs.isDouble() ? JSVAL_TYPE_DOUBLE : rhs.extractNonDoubleType()));

    
    
    RootedValue lhsCopy(cx, lhs);
    RootedValue rhsCopy(cx, rhs);

    
    switch(op) {
      case JSOP_ADD:
        
        if (!AddValues(cx, script, pc, &lhsCopy, &rhsCopy, ret.address()))
            return false;
        break;
      case JSOP_SUB:
        if (!SubValues(cx, script, pc, &lhsCopy, &rhsCopy, ret.address()))
            return false;
        break;
      case JSOP_MUL:
        if (!MulValues(cx, script, pc, &lhsCopy, &rhsCopy, ret.address()))
            return false;
        break;
      case JSOP_DIV:
        if (!DivValues(cx, script, pc, &lhsCopy, &rhsCopy, ret.address()))
            return false;
        break;
      case JSOP_MOD:
        if (!ModValues(cx, script, pc, &lhsCopy, &rhsCopy, ret.address()))
            return false;
        break;
      case JSOP_BITOR: {
        int32_t result;
        if (!BitOr(cx, lhs, rhs, &result))
            return false;
        ret.setInt32(result);
        break;
      }
      case JSOP_BITXOR: {
        int32_t result;
        if (!BitXor(cx, lhs, rhs, &result))
            return false;
        ret.setInt32(result);
        break;
      }
      case JSOP_BITAND: {
        int32_t result;
        if (!BitAnd(cx, lhs, rhs, &result))
            return false;
        ret.setInt32(result);
        break;
      }
      case JSOP_LSH: {
        int32_t result;
        if (!BitLsh(cx, lhs, rhs, &result))
            return false;
        ret.setInt32(result);
        break;
      }
      case JSOP_RSH: {
        int32_t result;
        if (!BitRsh(cx, lhs, rhs, &result))
            return false;
        ret.setInt32(result);
        break;
      }
      case JSOP_URSH: {
        if (!UrshOperation(cx, script, pc, lhs, rhs, ret.address()))
            return false;
        break;
      }
      default:
        JS_NOT_REACHED("Unhandled baseline arith op");
        return false;
    }

    
    if (stub->numOptimizedStubs() >= ICBinaryArith_Fallback::MAX_OPTIMIZED_STUBS) {
        
        
        return true;
    }

    
    if (op == JSOP_ADD) {
        if (lhs.isString() && rhs.isString()) {
            IonSpew(IonSpew_BaselineIC, "  Generating %s(String, String) stub", js_CodeName[op]);
            JS_ASSERT(ret.isString());
            ICBinaryArith_StringConcat::Compiler compiler(cx);
            ICStub *strcatStub = compiler.getStub(compiler.getStubSpace(script));
            if (!strcatStub)
                return false;
            stub->addNewStub(strcatStub);
            return true;
        }

        if ((lhs.isString() && rhs.isObject()) || (lhs.isObject() && rhs.isString())) {
            IonSpew(IonSpew_BaselineIC, "  Generating %s(%s, %s) stub", js_CodeName[op],
                    lhs.isString() ? "String" : "Object",
                    lhs.isString() ? "Object" : "String");
            JS_ASSERT(ret.isString());
            ICBinaryArith_StringObjectConcat::Compiler compiler(cx, lhs.isString());
            ICStub *strcatStub = compiler.getStub(compiler.getStubSpace(script));
            if (!strcatStub)
                return false;
            stub->addNewStub(strcatStub);
            return true;
        }
    }

    if (((lhs.isBoolean() && (rhs.isBoolean() || rhs.isInt32())) ||
         (rhs.isBoolean() && (lhs.isBoolean() || lhs.isInt32()))) &&
        (op == JSOP_ADD || op == JSOP_SUB || op == JSOP_BITOR || op == JSOP_BITAND ||
         op == JSOP_BITXOR))
    {
        IonSpew(IonSpew_BaselineIC, "  Generating %s(%s, %s) stub", js_CodeName[op],
                lhs.isBoolean() ? "Boolean" : "Int32", rhs.isBoolean() ? "Boolean" : "Int32");
        ICBinaryArith_BooleanWithInt32::Compiler compiler(cx, op, lhs.isBoolean(), rhs.isBoolean());
        ICStub *arithStub = compiler.getStub(compiler.getStubSpace(script));
        if (!arithStub)
            return false;
        stub->addNewStub(arithStub);
        return true;
    }

    
    if (!lhs.isNumber() || !rhs.isNumber())
        return true;

    JS_ASSERT(ret.isNumber());

    if (lhs.isDouble() || rhs.isDouble() || ret.isDouble()) {
        if (!cx->runtime->jitSupportsFloatingPoint)
            return true;

        switch (op) {
          case JSOP_ADD:
          case JSOP_SUB:
          case JSOP_MUL:
          case JSOP_DIV:
          case JSOP_MOD: {
            
            stub->unlinkStubsWithKind(cx, ICStub::BinaryArith_Int32);
            IonSpew(IonSpew_BaselineIC, "  Generating %s(Double, Double) stub", js_CodeName[op]);

            ICBinaryArith_Double::Compiler compiler(cx, op);
            ICStub *doubleStub = compiler.getStub(compiler.getStubSpace(script));
            if (!doubleStub)
                return false;
            stub->addNewStub(doubleStub);
            return true;
          }
          default:
            break;
        }
    }

    
    if (lhs.isInt32() && rhs.isInt32()) {
        bool allowDouble = ret.isDouble();
        IonSpew(IonSpew_BaselineIC, "  Generating %s(Int32, Int32%s) stub", js_CodeName[op],
                allowDouble ? " => Double" : "");
        ICBinaryArith_Int32::Compiler compilerInt32(cx, op, allowDouble);
        ICStub *int32Stub = compilerInt32.getStub(compilerInt32.getStubSpace(script));
        if (!int32Stub)
            return false;
        stub->addNewStub(int32Stub);
        return true;
    }

    
    if (((lhs.isDouble() && rhs.isInt32()) || (lhs.isInt32() && rhs.isDouble())) &&
        ret.isInt32())
    {
        switch(op) {
          case JSOP_BITOR:
          case JSOP_BITXOR:
          case JSOP_BITAND: {
            IonSpew(IonSpew_BaselineIC, "  Generating %s(%s, %s) stub", js_CodeName[op],
                        lhs.isDouble() ? "Double" : "Int32",
                        lhs.isDouble() ? "Int32" : "Double");
            ICBinaryArith_DoubleWithInt32::Compiler compiler(cx, op, lhs.isDouble());
            ICStub *optStub = compiler.getStub(compiler.getStubSpace(script));
            if (!optStub)
                return false;
            stub->addNewStub(optStub);
            return true;
          }
          default:
            break;
        }
    }

    return true;
}
#if defined(_MSC_VER)
# pragma optimize("", on)
#endif

typedef bool (*DoBinaryArithFallbackFn)(JSContext *, BaselineFrame *, ICBinaryArith_Fallback *,
                                        HandleValue, HandleValue, MutableHandleValue);
static const VMFunction DoBinaryArithFallbackInfo =
    FunctionInfo<DoBinaryArithFallbackFn>(DoBinaryArithFallback, PopValues(2));

bool
ICBinaryArith_Fallback::Compiler::generateStubCode(MacroAssembler &masm)
{
    JS_ASSERT(R0 == JSReturnOperand);

    
    EmitRestoreTailCallReg(masm);

    
    masm.pushValue(R0);
    masm.pushValue(R1);

    
    masm.pushValue(R1);
    masm.pushValue(R0);
    masm.push(BaselineStubReg);
    masm.pushBaselineFramePtr(BaselineFrameReg, R0.scratchReg());

    return tailCallVM(DoBinaryArithFallbackInfo, masm);
}

static bool
DoConcatStrings(JSContext *cx, HandleValue lhs, HandleValue rhs, MutableHandleValue res)
{
    JS_ASSERT(lhs.isString());
    JS_ASSERT(rhs.isString());
    JSString *lstr = lhs.toString();
    JSString *rstr = rhs.toString();
    JSString *result = ConcatStrings<NoGC>(cx, lstr, rstr);
    if (result) {
        res.set(StringValue(result));
        return true;
    }

    RootedString rootedl(cx, lstr), rootedr(cx, rstr);
    result = ConcatStrings<CanGC>(cx, rootedl, rootedr);
    if (!result)
        return false;

    res.set(StringValue(result));
    return true;
}

typedef bool (*DoConcatStringsFn)(JSContext *, HandleValue, HandleValue, MutableHandleValue);
static const VMFunction DoConcatStringsInfo = FunctionInfo<DoConcatStringsFn>(DoConcatStrings);

bool
ICBinaryArith_StringConcat::Compiler::generateStubCode(MacroAssembler &masm)
{
    Label failure;
    masm.branchTestString(Assembler::NotEqual, R0, &failure);
    masm.branchTestString(Assembler::NotEqual, R1, &failure);

    
    EmitRestoreTailCallReg(masm);

    masm.pushValue(R1);
    masm.pushValue(R0);
    if (!tailCallVM(DoConcatStringsInfo, masm))
        return false;

    
    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}

static JSString *
ConvertObjectToStringForConcat(JSContext *cx, HandleValue obj)
{
    JS_ASSERT(obj.isObject());
    RootedValue rootedObj(cx, obj);
    if (!ToPrimitive(cx, &rootedObj))
        return NULL;
    return ToString<CanGC>(cx, rootedObj);
}

static bool
DoConcatStringObject(JSContext *cx, bool lhsIsString, HandleValue lhs, HandleValue rhs,
                     MutableHandleValue res)
{
    JSString *lstr = NULL;
    JSString *rstr = NULL;
    if (lhsIsString) {
        
        JS_ASSERT(lhs.isString() && rhs.isObject());
        rstr = ConvertObjectToStringForConcat(cx, rhs);
        if (!rstr)
            return false;

        
        lstr = lhs.toString();
    } else {
        JS_ASSERT(rhs.isString() && lhs.isObject());
        
        lstr = ConvertObjectToStringForConcat(cx, lhs);
        if (!lstr)
            return false;

        
        rstr = rhs.toString();
    }

    JSString *str = ConcatStrings<NoGC>(cx, lstr, rstr);
    if (!str) {
        RootedString nlstr(cx, lstr), nrstr(cx, rstr);
        str = ConcatStrings<CanGC>(cx, nlstr, nrstr);
        if (!str)
            return false;
    }

    
    

    res.setString(str);
    return true;
}

typedef bool (*DoConcatStringObjectFn)(JSContext *, bool lhsIsString, HandleValue, HandleValue,
                                       MutableHandleValue);
static const VMFunction DoConcatStringObjectInfo =
    FunctionInfo<DoConcatStringObjectFn>(DoConcatStringObject, PopValues(2));

bool
ICBinaryArith_StringObjectConcat::Compiler::generateStubCode(MacroAssembler &masm)
{
    Label failure;
    if (lhsIsString_) {
        masm.branchTestString(Assembler::NotEqual, R0, &failure);
        masm.branchTestObject(Assembler::NotEqual, R1, &failure);
    } else {
        masm.branchTestObject(Assembler::NotEqual, R0, &failure);
        masm.branchTestString(Assembler::NotEqual, R1, &failure);
    }

    
    EmitRestoreTailCallReg(masm);

    
    masm.pushValue(R0);
    masm.pushValue(R1);

    
    masm.pushValue(R1);
    masm.pushValue(R0);
    masm.push(Imm32(lhsIsString_));
    if (!tailCallVM(DoConcatStringObjectInfo, masm))
        return false;

    
    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}

bool
ICBinaryArith_Double::Compiler::generateStubCode(MacroAssembler &masm)
{
    Label failure;
    masm.ensureDouble(R0, FloatReg0, &failure);
    masm.ensureDouble(R1, FloatReg1, &failure);

    switch (op) {
      case JSOP_ADD:
        masm.addDouble(FloatReg1, FloatReg0);
        break;
      case JSOP_SUB:
        masm.subDouble(FloatReg1, FloatReg0);
        break;
      case JSOP_MUL:
        masm.mulDouble(FloatReg1, FloatReg0);
        break;
      case JSOP_DIV:
        masm.divDouble(FloatReg1, FloatReg0);
        break;
      case JSOP_MOD:
        masm.setupUnalignedABICall(2, R0.scratchReg());
        masm.passABIArg(FloatReg0);
        masm.passABIArg(FloatReg1);
        masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, NumberMod), MacroAssembler::DOUBLE);
        JS_ASSERT(ReturnFloatReg == FloatReg0);
        break;
      default:
        JS_NOT_REACHED("Unexpected op");
        return false;
    }

    masm.boxDouble(FloatReg0, R0);
    EmitReturnFromIC(masm);

    
    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}

bool
ICBinaryArith_BooleanWithInt32::Compiler::generateStubCode(MacroAssembler &masm)
{
    Label failure;
    if (lhsIsBool_)
        masm.branchTestBoolean(Assembler::NotEqual, R0, &failure);
    else
        masm.branchTestInt32(Assembler::NotEqual, R0, &failure);

    if (rhsIsBool_)
        masm.branchTestBoolean(Assembler::NotEqual, R1, &failure);
    else
        masm.branchTestInt32(Assembler::NotEqual, R1, &failure);

    Register lhsReg = lhsIsBool_ ? masm.extractBoolean(R0, ExtractTemp0)
                                 : masm.extractInt32(R0, ExtractTemp0);
    Register rhsReg = rhsIsBool_ ? masm.extractBoolean(R1, ExtractTemp1)
                                 : masm.extractInt32(R1, ExtractTemp1);

    JS_ASSERT(op_ == JSOP_ADD || op_ == JSOP_SUB ||
              op_ == JSOP_BITOR || op_ == JSOP_BITXOR || op_ == JSOP_BITAND);

    switch(op_) {
      case JSOP_ADD: {
        Label fixOverflow;

        masm.add32(rhsReg, lhsReg);
        masm.j(Assembler::Overflow, &fixOverflow);
        masm.tagValue(JSVAL_TYPE_INT32, lhsReg, R0);
        EmitReturnFromIC(masm);

        masm.bind(&fixOverflow);
        masm.sub32(rhsReg, lhsReg);
        masm.jump(&failure);
        break;
      }
      case JSOP_SUB: {
        Label fixOverflow;

        masm.sub32(rhsReg, lhsReg);
        masm.j(Assembler::Overflow, &fixOverflow);
        masm.tagValue(JSVAL_TYPE_INT32, lhsReg, R0);
        EmitReturnFromIC(masm);

        masm.bind(&fixOverflow);
        masm.add32(rhsReg, lhsReg);
        masm.jump(&failure);
        break;
      }
      case JSOP_BITOR: {
        masm.orPtr(rhsReg, lhsReg);
        masm.tagValue(JSVAL_TYPE_INT32, lhsReg, R0);
        EmitReturnFromIC(masm);
        break;
      }
      case JSOP_BITXOR: {
        masm.xorPtr(rhsReg, lhsReg);
        masm.tagValue(JSVAL_TYPE_INT32, lhsReg, R0);
        EmitReturnFromIC(masm);
        break;
      }
      case JSOP_BITAND: {
        masm.andPtr(rhsReg, lhsReg);
        masm.tagValue(JSVAL_TYPE_INT32, lhsReg, R0);
        EmitReturnFromIC(masm);
        break;
      }
      default:
       JS_NOT_REACHED("Unhandled op for BinaryArith_BooleanWithInt32.");
       return false;
    }

    
    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}

bool
ICBinaryArith_DoubleWithInt32::Compiler::generateStubCode(MacroAssembler &masm)
{
    JS_ASSERT(op == JSOP_BITOR || op == JSOP_BITAND || op == JSOP_BITXOR);

    Label failure;
    Register intReg;
    Register scratchReg;
    if (lhsIsDouble_) {
        masm.branchTestDouble(Assembler::NotEqual, R0, &failure);
        masm.branchTestInt32(Assembler::NotEqual, R1, &failure);
        intReg = masm.extractInt32(R1, ExtractTemp0);
        masm.unboxDouble(R0, FloatReg0);
        scratchReg = R0.scratchReg();
    } else {
        masm.branchTestInt32(Assembler::NotEqual, R0, &failure);
        masm.branchTestDouble(Assembler::NotEqual, R1, &failure);
        intReg = masm.extractInt32(R0, ExtractTemp0);
        masm.unboxDouble(R1, FloatReg0);
        scratchReg = R1.scratchReg();
    }

    
    {
        Label doneTruncate;
        Label truncateABICall;
        masm.branchTruncateDouble(FloatReg0, scratchReg, &truncateABICall);
        masm.jump(&doneTruncate);

        masm.bind(&truncateABICall);
        masm.push(intReg);
        masm.setupUnalignedABICall(1, scratchReg);
        masm.passABIArg(FloatReg0);
        masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, js::ToInt32));
        masm.storeCallResult(scratchReg);
        masm.pop(intReg);

        masm.bind(&doneTruncate);
    }

    Register intReg2 = scratchReg;
    
    switch(op) {
      case JSOP_BITOR:
        masm.orPtr(intReg, intReg2);
        break;
      case JSOP_BITXOR:
        masm.xorPtr(intReg, intReg2);
        break;
      case JSOP_BITAND:
        masm.andPtr(intReg, intReg2);
        break;
      default:
       JS_NOT_REACHED("Unhandled op for BinaryArith_DoubleWithInt32.");
       return false;
    }
    masm.tagValue(JSVAL_TYPE_INT32, intReg2, R0);
    EmitReturnFromIC(masm);

    
    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}






#if defined(_MSC_VER)
# pragma optimize("g", off)
#endif
static bool
DoUnaryArithFallback(JSContext *cx, BaselineFrame *frame, ICUnaryArith_Fallback *stub,
                     HandleValue val, MutableHandleValue res)
{
    RootedScript script(cx, frame->script());
    jsbytecode *pc = stub->icEntry()->pc(script);
    JSOp op = JSOp(*pc);
    FallbackICSpew(cx, stub, "UnaryArith(%s)", js_CodeName[op]);

    switch (op) {
      case JSOP_BITNOT: {
        int32_t result;
        if (!BitNot(cx, val, &result))
            return false;
        res.setInt32(result);
        break;
      }
      case JSOP_NEG:
        if (!NegOperation(cx, script, pc, val, res))
            return false;
        break;
      default:
        JS_NOT_REACHED("Unexpected op");
        return false;
    }

    if (stub->numOptimizedStubs() >= ICUnaryArith_Fallback::MAX_OPTIMIZED_STUBS) {
        
        return true;
    }

    if (val.isInt32() && res.isInt32()) {
        IonSpew(IonSpew_BaselineIC, "  Generating %s(Int32 => Int32) stub", js_CodeName[op]);
        ICUnaryArith_Int32::Compiler compiler(cx, op);
        ICStub *int32Stub = compiler.getStub(compiler.getStubSpace(script));
        if (!int32Stub)
            return false;
        stub->addNewStub(int32Stub);
        return true;
    }

    if (val.isNumber() && res.isNumber() &&
        op == JSOP_NEG &&
        cx->runtime->jitSupportsFloatingPoint)
    {
        IonSpew(IonSpew_BaselineIC, "  Generating %s(Number => Number) stub", js_CodeName[op]);
        
        stub->unlinkStubsWithKind(cx, ICStub::UnaryArith_Int32);

        ICUnaryArith_Double::Compiler compiler(cx, op);
        ICStub *doubleStub = compiler.getStub(compiler.getStubSpace(script));
        if (!doubleStub)
            return false;
        stub->addNewStub(doubleStub);
        return true;
    }

    return true;
}
#if defined(_MSC_VER)
# pragma optimize("", on)
#endif

typedef bool (*DoUnaryArithFallbackFn)(JSContext *, BaselineFrame *, ICUnaryArith_Fallback *,
                                       HandleValue, MutableHandleValue);
static const VMFunction DoUnaryArithFallbackInfo =
    FunctionInfo<DoUnaryArithFallbackFn>(DoUnaryArithFallback, PopValues(1));

bool
ICUnaryArith_Fallback::Compiler::generateStubCode(MacroAssembler &masm)
{
    JS_ASSERT(R0 == JSReturnOperand);

    
    EmitRestoreTailCallReg(masm);

    
    masm.pushValue(R0);

    
    masm.pushValue(R0);
    masm.push(BaselineStubReg);
    masm.pushBaselineFramePtr(BaselineFrameReg, R0.scratchReg());

    return tailCallVM(DoUnaryArithFallbackInfo, masm);
}

bool
ICUnaryArith_Double::Compiler::generateStubCode(MacroAssembler &masm)
{
    Label failure;
    masm.ensureDouble(R0, FloatReg0, &failure);

    JS_ASSERT(op == JSOP_NEG);
    masm.negateDouble(FloatReg0);
    masm.boxDouble(FloatReg0, R0);

    EmitReturnFromIC(masm);

    
    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}





static void GetFixedOrDynamicSlotOffset(HandleObject obj, uint32_t slot,
                                        bool *isFixed, uint32_t *offset)
{
    JS_ASSERT(isFixed);
    JS_ASSERT(offset);
    *isFixed = obj->isFixedSlot(slot);
    *offset = *isFixed ? JSObject::getFixedSlotOffset(slot)
                       : obj->dynamicSlotIndex(slot) * sizeof(Value);
}

static bool
IsCacheableListBase(JSObject *obj)
{
    if (!obj->isProxy())
        return false;

    BaseProxyHandler *handler = GetProxyHandler(obj);

    if (handler->family() != GetListBaseHandlerFamily())
        return false;

    if (obj->numFixedSlots() <= GetListBaseExpandoSlot())
        return false;

    return true;
}

static JSObject *
GetListBaseProto(JSObject *obj)
{
    JS_ASSERT(IsCacheableListBase(obj));
    return obj->getTaggedProto().toObjectOrNull();
}

static void
GenerateListBaseChecks(JSContext *cx, MacroAssembler &masm, Register object,
                       Address checkProxyHandlerAddr,
                       Address *checkExpandoShapeAddr,
                       Address *expandoAndGenerationAddr,
                       Address *generationAddr,
                       Register scratch,
                       GeneralRegisterSet &listBaseRegSet,
                       Label *checkFailed)
{
    
    
    
    
    Address handlerAddr(object, JSObject::getFixedSlotOffset(JSSLOT_PROXY_HANDLER));
    Address expandoAddr(object, JSObject::getFixedSlotOffset(GetListBaseExpandoSlot()));

    
    masm.loadPtr(checkProxyHandlerAddr, scratch);
    masm.branchPrivatePtr(Assembler::NotEqual, handlerAddr, scratch, checkFailed);

    
    if (!checkExpandoShapeAddr)
        return;

    
    
    ValueOperand tempVal = listBaseRegSet.takeAnyValue();
    masm.pushValue(tempVal);

    Label failListBaseCheck;
    Label listBaseOk;

    if (expandoAndGenerationAddr) {
        JS_ASSERT(generationAddr);

        masm.loadPtr(*expandoAndGenerationAddr, tempVal.scratchReg());
        masm.branchPrivatePtr(Assembler::NotEqual, expandoAddr, tempVal.scratchReg(),
                              &failListBaseCheck);

        masm.load32(*generationAddr, scratch);
        masm.branch32(Assembler::NotEqual,
                      Address(tempVal.scratchReg(), offsetof(ExpandoAndGeneration, expando)),
                      scratch, &failListBaseCheck);

        masm.loadValue(Address(tempVal.scratchReg(), 0), tempVal);
    } else {
        masm.loadValue(expandoAddr, tempVal);
    }

    
    
    masm.branchTestUndefined(Assembler::Equal, tempVal, &listBaseOk);

    
    
    
    masm.loadPtr(*checkExpandoShapeAddr, scratch);
    masm.branchPtr(Assembler::Equal, scratch, ImmWord((void*)NULL), &failListBaseCheck);

    
    
    masm.branchTestObject(Assembler::NotEqual, tempVal, &failListBaseCheck);
    Register objReg = masm.extractObject(tempVal, tempVal.scratchReg());
    masm.branchTestObjShape(Assembler::Equal, objReg, scratch, &listBaseOk);

    
    masm.bind(&failListBaseCheck);
    masm.popValue(tempVal);
    masm.jump(checkFailed);

    
    masm.bind(&listBaseOk);
    masm.popValue(tempVal);
}




static bool
EffectlesslyLookupProperty(JSContext *cx, HandleObject obj, HandlePropertyName name,
                           MutableHandleObject holder, MutableHandleShape shape,
                           bool *checkListBase=NULL,
                           ListBaseShadowsResult *shadowsResult=NULL,
                           bool *listBaseHasGeneration=NULL)
{
    shape.set(NULL);
    holder.set(NULL);

    bool isListBase = false;
    if (checkListBase)
        *checkListBase = false;

    
    RootedObject checkObj(cx, obj);
    if (checkListBase && IsCacheableListBase(obj)) {
        JS_ASSERT(listBaseHasGeneration);
        JS_ASSERT(shadowsResult);

        *checkListBase = isListBase = true;
        if (obj->hasUncacheableProto())
            return true;

        RootedId id(cx, NameToId(name));
        *shadowsResult = GetListBaseShadowsCheck()(cx, obj, id);
        if (*shadowsResult == ShadowCheckFailed)
            return false;

        if (*shadowsResult == Shadows) {
            holder.set(obj);
            return true;
        }

        *listBaseHasGeneration = (*shadowsResult == DoesntShadowUnique);

        checkObj = GetListBaseProto(obj);
    }

    if (!isListBase && !obj->isNative())
        return true;

    if (checkObj->hasIdempotentProtoChain()) {
        if (!JSObject::lookupProperty(cx, checkObj, name, holder, shape))
            return false;
    } else if (checkObj->isNative()) {
        shape.set(checkObj->nativeLookup(cx, NameToId(name)));
        if (shape)
            holder.set(checkObj);
    }
    return true;
}

static bool
IsCacheableProtoChain(JSObject *obj, JSObject *holder, bool isListBase=false)
{
    JS_ASSERT_IF(isListBase, IsCacheableListBase(obj));
    JS_ASSERT_IF(!isListBase, obj->isNative());

    
    
    if (obj->hasUncacheableProto())
        return false;

    JSObject *cur = obj;
    while (cur != holder) {
        
        
        
        JSObject *proto;
        if (isListBase && cur == obj)
            proto = cur->getTaggedProto().toObjectOrNull();
        else
            proto = cur->getProto();

        if (!proto || !proto->isNative())
            return false;

        if (proto->hasUncacheableProto())
            return false;

        cur = proto;
    }
    return true;
}

static bool
IsCacheableGetPropReadSlot(JSObject *obj, JSObject *holder, Shape *shape, bool isListBase=false)
{
    if (!shape || !IsCacheableProtoChain(obj, holder, isListBase))
        return false;

    if (!shape->hasSlot() || !shape->hasDefaultGetter())
        return false;

    return true;
}

static bool
IsCacheableGetPropCall(JSObject *obj, JSObject *holder, Shape *shape, bool *isScripted,
                       bool isListBase=false)
{
    JS_ASSERT(isScripted);

    
    if (obj == holder)
        return false;

    if (!shape || !IsCacheableProtoChain(obj, holder, isListBase))
        return false;

    if (shape->hasSlot() || shape->hasDefaultGetter())
        return false;

    if (!shape->hasGetterValue())
        return false;

    if (!shape->getterValue().isObject() || !shape->getterObject()->isFunction())
        return false;

    JSFunction *func = shape->getterObject()->toFunction();
    if (func->isNative()) {
        *isScripted = false;
        return true;
    }

    if (!func->hasScript())
        return false;

    JSScript *script = func->nonLazyScript();
    if (!script->hasIonScript() && !script->hasBaselineScript())
        return false;

    *isScripted = true;
    return true;
}

static bool
IsCacheableSetPropWriteSlot(JSObject *obj, Shape *oldShape, JSObject *holder, Shape *shape)
{
    if (!shape)
        return false;

    
    if (obj->lastProperty() != oldShape)
        return false;

    
    if (obj != holder)
        return false;

    if (!shape->hasSlot() || !shape->hasDefaultSetter() || !shape->writable())
        return false;

    return true;
}

static bool
IsCacheableSetPropAddSlot(JSContext *cx, HandleObject obj, HandleShape oldShape, uint32_t oldSlots,
                          HandleId id, HandleObject holder, HandleShape shape,
                          size_t *protoChainDepth)
{
    if (!shape)
        return false;

    
    if (obj != holder || shape != obj->lastProperty())
        return false;

    
    if (!obj->isExtensible() || obj->lastProperty()->previous() != oldShape)
        return false;

    
    if (shape->inDictionary() || !shape->hasSlot() || !shape->hasDefaultSetter() ||
        !shape->writable())
    {
        return false;
    }

    
    if (obj->getClass()->resolve != JS_ResolveStub)
        return false;

    size_t chainDepth = 0;
    
    
    for (JSObject *proto = obj->getProto(); proto; proto = proto->getProto()) {
        chainDepth++;
        
        if (!proto->isNative())
            return false;

        
        Shape *protoShape = proto->nativeLookup(cx, id);
        if (protoShape && !protoShape->hasDefaultSetter())
            return false;

        
        
        if (proto->getClass()->resolve != JS_ResolveStub)
             return false;
    }

    
    
    
    if (obj->numDynamicSlots() != oldSlots)
        return false;

    *protoChainDepth = chainDepth;
    return true;
}

static bool
IsCacheableSetPropCall(JSObject *obj, JSObject *holder, Shape *shape, bool *isScripted)
{
    JS_ASSERT(isScripted);

    
    if (obj == holder)
        return false;

    if (!shape || !IsCacheableProtoChain(obj, holder))
        return false;

    if (shape->hasSlot() || shape->hasDefaultSetter())
        return false;

    if (!shape->hasSetterValue())
        return false;

    if (!shape->setterValue().isObject() || !shape->setterObject()->isFunction())
        return false;

    JSFunction *func = shape->setterObject()->toFunction();
    if (func->isNative()) {
        *isScripted = false;
        return true;
    }

    if (!func->hasScript())
        return false;

    JSScript *script = func->nonLazyScript();
    if (!script->hasIonScript() && !script->hasBaselineScript())
        return false;

    *isScripted = true;
    return true;
}

static bool
TypedArrayGetElemStubExists(ICGetElem_Fallback *stub, HandleObject obj)
{
    for (ICStubConstIterator iter = stub->beginChainConst(); !iter.atEnd(); iter++) {
        if (!iter->isGetElem_TypedArray())
            continue;
        if (obj->lastProperty() == iter->toGetElem_TypedArray()->shape())
            return true;
    }
    return false;
}

static bool
ArgumentsGetElemStubExists(ICGetElem_Fallback *stub, ICGetElem_Arguments::Which which)
{
    for (ICStubConstIterator iter = stub->beginChainConst(); !iter.atEnd(); iter++) {
        if (!iter->isGetElem_Arguments())
            continue;
        if (iter->toGetElem_Arguments()->which() == which)
            return true;
    }
    return false;
}


static bool TryAttachNativeGetElemStub(JSContext *cx, HandleScript script,
                                       ICGetElem_Fallback *stub, HandleObject obj,
                                       HandleValue key)
{
    RootedId id(cx);
    if (!ValueToId<CanGC>(cx, key, &id))
        return false;

    uint32_t dummy;
    if (!JSID_IS_ATOM(id) || JSID_TO_ATOM(id)->isIndex(&dummy))
        return true;

    RootedPropertyName propName(cx, JSID_TO_ATOM(id)->asPropertyName());

    RootedShape shape(cx);
    RootedObject holder(cx);
    if (!EffectlesslyLookupProperty(cx, obj, propName, &holder, &shape))
        return false;

    if (!IsCacheableGetPropReadSlot(obj, holder, shape))
        return true;

    bool isFixedSlot;
    uint32_t offset;
    GetFixedOrDynamicSlotOffset(holder, shape->slot(), &isFixedSlot, &offset);

    ICStub *monitorStub = stub->fallbackMonitorStub()->firstMonitorStub();
    ICStub::Kind kind = (obj == holder) ? ICStub::GetElem_Native : ICStub::GetElem_NativePrototype;

    IonSpew(IonSpew_BaselineIC, "  Generating GetElem(Native %s) stub (obj=%p, shape=%p, holder=%p, holderShape=%p)",
                (obj == holder) ? "direct" : "prototype",
                obj.get(), obj->lastProperty(), holder.get(), holder->lastProperty());

    ICGetElemNativeCompiler compiler(cx, kind, monitorStub, obj, holder, key,
                                     isFixedSlot, offset);
    ICStub *newStub = compiler.getStub(compiler.getStubSpace(script));
    if (!newStub)
        return false;

    stub->addNewStub(newStub);
    return true;
}

static bool
TypedArrayRequiresFloatingPoint(JSObject *obj)
{
    uint32_t type = TypedArray::type(obj);
    return (type == TypedArray::TYPE_UINT32 ||
            type == TypedArray::TYPE_FLOAT32 ||
            type == TypedArray::TYPE_FLOAT64);
}

static bool
TryAttachGetElemStub(JSContext *cx, HandleScript script, ICGetElem_Fallback *stub,
                     HandleValue lhs, HandleValue rhs, HandleValue res)
{
    
    if (lhs.isString() && rhs.isInt32() && res.isString() &&
        !stub->hasStub(ICStub::GetElem_String))
    {
        IonSpew(IonSpew_BaselineIC, "  Generating GetElem(String[Int32]) stub");
        ICGetElem_String::Compiler compiler(cx);
        ICStub *stringStub = compiler.getStub(compiler.getStubSpace(script));
        if (!stringStub)
            return false;

        stub->addNewStub(stringStub);
        return true;
    }

    if (lhs.isMagic(JS_OPTIMIZED_ARGUMENTS) && rhs.isInt32() &&
        !ArgumentsGetElemStubExists(stub, ICGetElem_Arguments::Magic))
    {
        IonSpew(IonSpew_BaselineIC, "  Generating GetElem(MagicArgs[Int32]) stub");
        ICGetElem_Arguments::Compiler compiler(cx, stub->fallbackMonitorStub()->firstMonitorStub(),
                                               ICGetElem_Arguments::Magic);
        ICStub *argsStub = compiler.getStub(compiler.getStubSpace(script));
        if (!argsStub)
            return false;

        stub->addNewStub(argsStub);
        return true;
    }

    
    if (!lhs.isObject())
        return true;
    RootedObject obj(cx, &lhs.toObject());

    
    if (obj->isArguments() && rhs.isInt32()) {
        ICGetElem_Arguments::Which which = ICGetElem_Arguments::Normal;
        if (obj->isStrictArguments())
            which = ICGetElem_Arguments::Strict;
        if (!ArgumentsGetElemStubExists(stub, which)) {
            IonSpew(IonSpew_BaselineIC, "  Generating GetElem(ArgsObj[Int32]) stub");
            ICGetElem_Arguments::Compiler compiler(
                cx, stub->fallbackMonitorStub()->firstMonitorStub(), which);
            ICStub *argsStub = compiler.getStub(compiler.getStubSpace(script));
            if (!argsStub)
                return false;

            stub->addNewStub(argsStub);
            return true;
        }
    }

    if (obj->isNative()) {
        
        if (rhs.isInt32()) {
            IonSpew(IonSpew_BaselineIC, "  Generating GetElem(Native[Int32] dense) stub");
            ICGetElem_Dense::Compiler compiler(cx, stub->fallbackMonitorStub()->firstMonitorStub(),
                                               obj->lastProperty());
            ICStub *denseStub = compiler.getStub(compiler.getStubSpace(script));
            if (!denseStub)
                return false;

            stub->addNewStub(denseStub);
            return true;
        }

        
        if (rhs.isString()) {
            if (!TryAttachNativeGetElemStub(cx, script, stub, obj, rhs))
                return false;
        }
    }

    
    if (obj->isTypedArray() && rhs.isInt32() && res.isNumber() &&
        !TypedArrayGetElemStubExists(stub, obj))
    {
        if (!cx->runtime->jitSupportsFloatingPoint && TypedArrayRequiresFloatingPoint(obj))
            return true;

        IonSpew(IonSpew_BaselineIC, "  Generating GetElem(TypedArray[Int32]) stub");
        ICGetElem_TypedArray::Compiler compiler(cx, obj->lastProperty(), TypedArray::type(obj));
        ICStub *typedArrayStub = compiler.getStub(compiler.getStubSpace(script));
        if (!typedArrayStub)
            return false;

        stub->addNewStub(typedArrayStub);
        return true;
    }

    
    
    
    if (!obj->isNative() && !obj->isTypedArray())
        stub->noteNonNativeAccess();

    return true;
}

static bool
DoGetElemFallback(JSContext *cx, BaselineFrame *frame, ICGetElem_Fallback *stub, HandleValue lhs,
                  HandleValue rhs, MutableHandleValue res)
{
    RootedScript script(cx, frame->script());
    jsbytecode *pc = stub->icEntry()->pc(script);
    JSOp op = JSOp(*pc);
    FallbackICSpew(cx, stub, "GetElem(%s)", js_CodeName[op]);

    JS_ASSERT(op == JSOP_GETELEM || op == JSOP_CALLELEM);

    
    RootedValue lhsCopy(cx, lhs);

    bool isOptimizedArgs = false;
    if (lhs.isMagic(JS_OPTIMIZED_ARGUMENTS)) {
        
        if (!GetElemOptimizedArguments(cx, frame, &lhsCopy, rhs, res, &isOptimizedArgs))
            return false;
        if (isOptimizedArgs)
            types::TypeScript::Monitor(cx, script, pc, res);
    }

    if (!isOptimizedArgs) {
        if (!GetElementOperation(cx, op, &lhsCopy, rhs, res))
            return false;
        types::TypeScript::Monitor(cx, script, pc, res);
    }

    
    if (!stub->addMonitorStubForValue(cx, script, res))
        return false;

    if (stub->numOptimizedStubs() >= ICGetElem_Fallback::MAX_OPTIMIZED_STUBS) {
        
        
        return true;
    }

    
    if (!TryAttachGetElemStub(cx, script, stub, lhs, rhs, res))
        return false;

    return true;
}

typedef bool (*DoGetElemFallbackFn)(JSContext *, BaselineFrame *, ICGetElem_Fallback *,
                                    HandleValue, HandleValue, MutableHandleValue);
static const VMFunction DoGetElemFallbackInfo =
    FunctionInfo<DoGetElemFallbackFn>(DoGetElemFallback, PopValues(2));

bool
ICGetElem_Fallback::Compiler::generateStubCode(MacroAssembler &masm)
{
    JS_ASSERT(R0 == JSReturnOperand);

    
    EmitRestoreTailCallReg(masm);

    
    masm.pushValue(R0);
    masm.pushValue(R1);

    
    masm.pushValue(R1);
    masm.pushValue(R0);
    masm.push(BaselineStubReg);
    masm.pushBaselineFramePtr(BaselineFrameReg, R0.scratchReg());

    return tailCallVM(DoGetElemFallbackInfo, masm);
}





bool
ICGetElemNativeCompiler::generateStubCode(MacroAssembler &masm)
{
    Label failure;
    Label failurePopR1;
    bool popR1 = false;

    masm.branchTestObject(Assembler::NotEqual, R0, &failure);

    Address idValAddr(BaselineStubReg, ICGetElemNativeStub::offsetOfIdval());
    masm.branchTestValue(Assembler::NotEqual, idValAddr, R1, &failure);

    GeneralRegisterSet regs(availableGeneralRegs(2));
    Register scratchReg = regs.takeAny();

    
    Register objReg = masm.extractObject(R0, ExtractTemp0);

    
    masm.loadPtr(Address(objReg, JSObject::offsetOfShape()), scratchReg);
    Address shapeAddr(BaselineStubReg, ICGetElemNativeStub::offsetOfShape());
    masm.branchPtr(Assembler::NotEqual, shapeAddr, scratchReg, &failure);

    Register holderReg;
    if (obj_ == holder_) {
        holderReg = objReg;
    } else {
        
        if (regs.empty()) {
            masm.push(R1.scratchReg());
            popR1 = true;
            holderReg = R1.scratchReg();
        } else {
            holderReg = regs.takeAny();
        }
        masm.loadPtr(Address(BaselineStubReg, ICGetElem_NativePrototype::offsetOfHolder()),
                     holderReg);
        masm.loadPtr(Address(BaselineStubReg, ICGetElem_NativePrototype::offsetOfHolderShape()),
                     scratchReg);
        masm.branchTestObjShape(Assembler::NotEqual, holderReg, scratchReg,
                                popR1 ? &failurePopR1 : &failure);
    }

    
    if (!isFixedSlot_)
        masm.loadPtr(Address(holderReg, JSObject::offsetOfSlots()), holderReg);

    masm.load32(Address(BaselineStubReg, ICGetElem_Native::offsetOfOffset()), scratchReg);
    masm.loadValue(BaseIndex(holderReg, scratchReg, TimesOne), R0);

    if (popR1)
        masm.pop(R1.scratchReg());
    
    EmitEnterTypeMonitorIC(masm);

    
    if (popR1) {
        masm.bind(&failurePopR1);
        masm.pop(R1.scratchReg());
    }
    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}





bool
ICGetElem_String::Compiler::generateStubCode(MacroAssembler &masm)
{
    Label failure;
    masm.branchTestString(Assembler::NotEqual, R0, &failure);
    masm.branchTestInt32(Assembler::NotEqual, R1, &failure);

    GeneralRegisterSet regs(availableGeneralRegs(2));
    Register scratchReg = regs.takeAny();

    
    Register str = masm.extractString(R0, ExtractTemp0);

    
    Address lengthAndFlagsAddr(str, JSString::offsetOfLengthAndFlags());
    masm.loadPtr(lengthAndFlagsAddr, scratchReg);

    
    masm.branchTest32(Assembler::Zero, scratchReg, Imm32(JSString::FLAGS_MASK), &failure);

    
    Register key = masm.extractInt32(R1, ExtractTemp1);

    
    masm.rshiftPtr(Imm32(JSString::LENGTH_SHIFT), scratchReg);
    masm.branch32(Assembler::BelowOrEqual, scratchReg, key, &failure);

    
    Address charsAddr(str, JSString::offsetOfChars());
    masm.loadPtr(charsAddr, scratchReg);
    masm.load16ZeroExtend(BaseIndex(scratchReg, key, TimesTwo, 0), scratchReg);

    
    masm.branch32(Assembler::AboveOrEqual, scratchReg, Imm32(StaticStrings::UNIT_STATIC_LIMIT),
                  &failure);

    
    masm.movePtr(ImmWord(&cx->compartment->rt->staticStrings.unitStaticTable), str);
    masm.loadPtr(BaseIndex(str, scratchReg, ScalePointer), str);

    
    masm.tagValue(JSVAL_TYPE_STRING, str, R0);
    EmitReturnFromIC(masm);

    
    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}





bool
ICGetElem_Dense::Compiler::generateStubCode(MacroAssembler &masm)
{
    Label failure;
    masm.branchTestObject(Assembler::NotEqual, R0, &failure);
    masm.branchTestInt32(Assembler::NotEqual, R1, &failure);

    GeneralRegisterSet regs(availableGeneralRegs(2));
    Register scratchReg = regs.takeAny();

    
    Register obj = masm.extractObject(R0, ExtractTemp0);
    masm.loadPtr(Address(BaselineStubReg, ICGetElem_Dense::offsetOfShape()), scratchReg);
    masm.branchTestObjShape(Assembler::NotEqual, obj, scratchReg, &failure);

    
    masm.loadPtr(Address(obj, JSObject::offsetOfElements()), scratchReg);

    
    Register key = masm.extractInt32(R1, ExtractTemp1);

    
    Address initLength(scratchReg, ObjectElements::offsetOfInitializedLength());
    masm.branch32(Assembler::BelowOrEqual, initLength, key, &failure);

    
    BaseIndex element(scratchReg, key, TimesEight);
    masm.branchTestMagic(Assembler::Equal, element, &failure);
    masm.loadValue(element, R0);

    
    EmitEnterTypeMonitorIC(masm);

    
    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}





bool
ICGetElem_TypedArray::Compiler::generateStubCode(MacroAssembler &masm)
{
    Label failure;
    masm.branchTestObject(Assembler::NotEqual, R0, &failure);
    masm.branchTestInt32(Assembler::NotEqual, R1, &failure);

    GeneralRegisterSet regs(availableGeneralRegs(2));
    Register scratchReg = regs.takeAny();

    
    Register obj = masm.extractObject(R0, ExtractTemp0);
    masm.loadPtr(Address(BaselineStubReg, ICGetElem_TypedArray::offsetOfShape()), scratchReg);
    masm.branchTestObjShape(Assembler::NotEqual, obj, scratchReg, &failure);

    
    Register key = masm.extractInt32(R1, ExtractTemp1);

    
    masm.unboxInt32(Address(obj, TypedArray::lengthOffset()), scratchReg);
    masm.branch32(Assembler::BelowOrEqual, scratchReg, key, &failure);

    
    masm.loadPtr(Address(obj, TypedArray::dataOffset()), scratchReg);

    
    BaseIndex source(scratchReg, key, ScaleFromElemWidth(TypedArray::slotWidth(type_)));
    masm.loadFromTypedArray(type_, source, R0, false, scratchReg, &failure);

    
    EmitReturnFromIC(masm);

    
    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}




bool
ICGetElem_Arguments::Compiler::generateStubCode(MacroAssembler &masm)
{
    Label failure;
    if (which_ == ICGetElem_Arguments::Magic) {
        
        masm.branchTestMagicValue(Assembler::NotEqual, R0, JS_OPTIMIZED_ARGUMENTS, &failure);

        
        masm.branchTest32(Assembler::NonZero,
                          Address(BaselineFrameReg, BaselineFrame::reverseOffsetOfFlags()),
                          Imm32(BaselineFrame::HAS_ARGS_OBJ),
                          &failure);

        
        masm.branchTestInt32(Assembler::NotEqual, R1, &failure);
        Register idx = masm.extractInt32(R1, ExtractTemp1);

        GeneralRegisterSet regs(availableGeneralRegs(2));
        Register scratch = regs.takeAny();

        
        Address actualArgs(BaselineFrameReg, BaselineFrame::offsetOfNumActualArgs());
        masm.loadPtr(actualArgs, scratch);

        
        masm.branch32(Assembler::AboveOrEqual, idx, scratch, &failure);

        
        JS_ASSERT(sizeof(Value) == 8);
        masm.movePtr(BaselineFrameReg, scratch);
        masm.addPtr(Imm32(BaselineFrame::offsetOfArg(0)), scratch);
        BaseIndex element(scratch, idx, TimesEight);
        masm.loadValue(element, R0);

        
        EmitEnterTypeMonitorIC(masm);

        masm.bind(&failure);
        EmitStubGuardFailure(masm);
        return true;
    }

    JS_ASSERT(which_ == ICGetElem_Arguments::Strict ||
              which_ == ICGetElem_Arguments::Normal);

    bool isStrict = which_ == ICGetElem_Arguments::Strict;
    Class *clasp = isStrict ? &StrictArgumentsObjectClass : &NormalArgumentsObjectClass;

    GeneralRegisterSet regs(availableGeneralRegs(2));
    Register scratchReg = regs.takeAny();

    
    masm.branchTestObject(Assembler::NotEqual, R0, &failure);
    Register objReg = masm.extractObject(R0, ExtractTemp0);
    masm.branchTestObjClass(Assembler::NotEqual, objReg, scratchReg, clasp, &failure);

    
    masm.branchTestInt32(Assembler::NotEqual, R1, &failure);
    Register idxReg = masm.extractInt32(R1, ExtractTemp1);

    
    masm.unboxInt32(Address(objReg, ArgumentsObject::getInitialLengthSlotOffset()), scratchReg);

    
    masm.branchTest32(Assembler::NonZero,
                      scratchReg,
                      Imm32(ArgumentsObject::LENGTH_OVERRIDDEN_BIT),
                      &failure);

    
    masm.rshiftPtr(Imm32(ArgumentsObject::PACKED_BITS_COUNT), scratchReg);
    masm.branch32(Assembler::AboveOrEqual, idxReg, scratchReg, &failure);

    
    
    
    Label failureReconstructInputs;
    regs = availableGeneralRegs(0);
    if (regs.has(objReg))
        regs.take(objReg);
    if (regs.has(idxReg))
        regs.take(idxReg);
    if (regs.has(scratchReg))
        regs.take(scratchReg);
    Register argData = regs.takeAny();
    Register tempReg = regs.takeAny();

    
    masm.loadPrivate(Address(objReg, ArgumentsObject::getDataSlotOffset()), argData);

    
    masm.loadPtr(Address(argData, offsetof(ArgumentsData, deletedBits)), scratchReg);

    
    masm.movePtr(idxReg, tempReg);
    masm.rshiftPtr(Imm32(JS_BITS_PER_WORD_LOG2), tempReg);
    masm.loadPtr(BaseIndex(scratchReg, tempReg, ScaleFromElemWidth(sizeof(size_t))), scratchReg);

    
    masm.branchPtr(Assembler::NotEqual, scratchReg, ImmWord((size_t)0), &failureReconstructInputs);

    
    masm.addPtr(Imm32(ArgumentsData::offsetOfArgs()), argData);
    regs.add(scratchReg);
    regs.add(tempReg);
    ValueOperand tempVal = regs.takeAnyValue();
    masm.loadValue(BaseIndex(argData, idxReg, ScaleFromElemWidth(sizeof(Value))), tempVal);

    
    masm.branchTestMagic(Assembler::Equal, tempVal, &failureReconstructInputs);

    
    masm.moveValue(tempVal, R0);

    
    EmitEnterTypeMonitorIC(masm);

    
    
    masm.bind(&failureReconstructInputs);
    masm.tagValue(JSVAL_TYPE_OBJECT, objReg, R0);
    masm.tagValue(JSVAL_TYPE_INT32, idxReg, R1);

    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}





static bool
DenseSetElemStubExists(JSContext *cx, ICStub::Kind kind, ICSetElem_Fallback *stub, HandleObject obj)
{
    JS_ASSERT(kind == ICStub::SetElem_Dense || kind == ICStub::SetElem_DenseAdd);

    for (ICStubConstIterator iter = stub->beginChainConst(); !iter.atEnd(); iter++) {
        if (kind == ICStub::SetElem_Dense && iter->isSetElem_Dense()) {
            ICSetElem_Dense *dense = iter->toSetElem_Dense();
            if (obj->lastProperty() == dense->shape() && obj->getType(cx) == dense->type())
                return true;
        }

        if (kind == ICStub::SetElem_DenseAdd && iter->isSetElem_DenseAdd()) {
            ICSetElem_DenseAdd *dense = iter->toSetElem_DenseAdd();
            if (obj->lastProperty() == dense->toImplUnchecked<0>()->shape(0) &&
                obj->getType(cx) == dense->type())
            {
                return true;
            }
        }
    }
    return false;
}

static bool
TypedArraySetElemStubExists(ICSetElem_Fallback *stub, HandleObject obj, bool expectOOB)
{
    for (ICStubConstIterator iter = stub->beginChainConst(); !iter.atEnd(); iter++) {
        if (!iter->isSetElem_TypedArray())
            continue;
        ICSetElem_TypedArray *taStub = iter->toSetElem_TypedArray();
        if (obj->lastProperty() == taStub->shape() && taStub->expectOutOfBounds() == expectOOB)
            return true;
    }
    return false;
}

static bool
RemoveExistingTypedArraySetElemStub(JSContext *cx, ICSetElem_Fallback *stub, HandleObject obj)
{
    for (ICStubIterator iter = stub->beginChain(); !iter.atEnd(); iter++) {
        if (!iter->isSetElem_TypedArray())
            continue;

        if (obj->lastProperty() != iter->toSetElem_TypedArray()->shape())
            continue;

        
        
        JS_ASSERT(!iter->toSetElem_TypedArray()->expectOutOfBounds());
        iter.unlink(cx->zone());
        return true;
    }
    return false;
}

static bool
CanOptimizeDenseSetElem(JSContext *cx, HandleObject obj, uint32_t index,
                        HandleShape oldShape, uint32_t oldCapacity, uint32_t oldInitLength,
                        bool *isAddingCaseOut, size_t *protoDepthOut)
{
    uint32_t initLength = obj->getDenseInitializedLength();
    uint32_t capacity = obj->getDenseCapacity();

    *isAddingCaseOut = false;
    *protoDepthOut = 0;

    
    if (initLength < oldInitLength || capacity < oldCapacity)
        return false;

    RootedShape shape(cx, obj->lastProperty());

    
    if (oldShape != shape)
        return false;

    
    if (oldCapacity != capacity)
        return false;

    
    if (index >= initLength)
        return false;

    
    if (!obj->containsDenseElement(index))
        return false;

    
    
    if (oldInitLength == initLength)
        return true;

    
    
    if (oldInitLength + 1 != initLength)
        return false;
    if (index != oldInitLength)
        return false;

    
    
    
    
    RootedObject curObj(cx, obj);
    while (curObj) {
        
        if (!curObj->isNative())
            return false;

        
        if (curObj->isIndexed())
            return false;

        curObj = curObj->getProto();
        if (curObj)
            ++*protoDepthOut;
    }

    if (*protoDepthOut > ICSetElem_DenseAdd::MAX_PROTO_CHAIN_DEPTH)
        return false;

    *isAddingCaseOut = true;

    return true;
}

static bool
DoSetElemFallback(JSContext *cx, BaselineFrame *frame, ICSetElem_Fallback *stub, Value *stack,
                  HandleValue objv, HandleValue index, HandleValue rhs)
{
    RootedScript script(cx, frame->script());
    jsbytecode *pc = stub->icEntry()->pc(script);
    JSOp op = JSOp(*pc);
    FallbackICSpew(cx, stub, "SetElem(%s)", js_CodeName[JSOp(*pc)]);

    JS_ASSERT(op == JSOP_SETELEM ||
              op == JSOP_ENUMELEM ||
              op == JSOP_INITELEM ||
              op == JSOP_INITELEM_ARRAY);

    RootedObject obj(cx, ToObjectFromStack(cx, objv));
    if (!obj)
        return false;

    RootedShape oldShape(cx, obj->lastProperty());

    
    uint32_t oldCapacity = 0;
    uint32_t oldInitLength = 0;
    if (obj->isNative() && index.isInt32() && index.toInt32() >= 0) {
        oldCapacity = obj->getDenseCapacity();
        oldInitLength = obj->getDenseInitializedLength();
    }

    if (op == JSOP_INITELEM) {
        if (!InitElemOperation(cx, obj, index, rhs))
            return false;
    } else if (op == JSOP_INITELEM_ARRAY) {
        JS_ASSERT(uint32_t(index.toInt32()) == GET_UINT24(pc));
        if (!InitArrayElemOperation(cx, pc, obj, index.toInt32(), rhs))
            return false;
    } else {
        if (!SetObjectElement(cx, obj, index, rhs, script->strict, script, pc))
            return false;
    }

    
    JS_ASSERT(stack[2] == objv);
    stack[2] = rhs;

    if (stub->numOptimizedStubs() >= ICSetElem_Fallback::MAX_OPTIMIZED_STUBS) {
        
        
        return true;
    }

    
    if (obj->isNative() &&
        index.isInt32() && index.toInt32() >= 0 &&
        !rhs.isMagic(JS_ELEMENTS_HOLE))
    {
        JS_ASSERT(!obj->isTypedArray());

        bool addingCase;
        size_t protoDepth;

        if (CanOptimizeDenseSetElem(cx, obj, index.toInt32(), oldShape, oldCapacity, oldInitLength,
                                    &addingCase, &protoDepth))
        {
            RootedTypeObject type(cx, obj->getType(cx));
            RootedShape shape(cx, obj->lastProperty());

            if (addingCase && !DenseSetElemStubExists(cx, ICStub::SetElem_DenseAdd, stub, obj)) {
                IonSpew(IonSpew_BaselineIC,
                        "  Generating SetElem_DenseAdd stub "
                        "(shape=%p, type=%p, protoDepth=%u)",
                        obj->lastProperty(), type.get(), protoDepth);
                ICSetElemDenseAddCompiler compiler(cx, obj, protoDepth);
                ICUpdatedStub *denseStub = compiler.getStub(compiler.getStubSpace(script));
                if (!denseStub)
                    return false;
                if (!denseStub->addUpdateStubForValue(cx, script, obj, JSID_VOID, rhs))
                    return false;

                stub->addNewStub(denseStub);
            } else if (!addingCase &&
                       !DenseSetElemStubExists(cx, ICStub::SetElem_Dense, stub, obj))
            {
                IonSpew(IonSpew_BaselineIC,
                        "  Generating SetElem_Dense stub (shape=%p, type=%p)",
                        obj->lastProperty(), type.get());
                ICSetElem_Dense::Compiler compiler(cx, shape, type);
                ICUpdatedStub *denseStub = compiler.getStub(compiler.getStubSpace(script));
                if (!denseStub)
                    return false;
                if (!denseStub->addUpdateStubForValue(cx, script, obj, JSID_VOID, rhs))
                    return false;

                stub->addNewStub(denseStub);
            }
        }

        return true;
    }

    if (obj->isTypedArray() && index.isInt32() && rhs.isNumber()) {
        if (!cx->runtime->jitSupportsFloatingPoint && TypedArrayRequiresFloatingPoint(obj))
            return true;

        uint32_t len = TypedArray::length(obj);
        int32_t idx = index.toInt32();
        bool expectOutOfBounds = (idx < 0) || (static_cast<uint32_t>(idx) >= len);

        if (!TypedArraySetElemStubExists(stub, obj, expectOutOfBounds)) {
            
            if (expectOutOfBounds)
                RemoveExistingTypedArraySetElemStub(cx, stub, obj);

            IonSpew(IonSpew_BaselineIC,
                    "  Generating SetElem_TypedArray stub (shape=%p, type=%u, oob=%s)",
                    obj->lastProperty(), TypedArray::type(obj),
                    expectOutOfBounds ? "yes" : "no");
            ICSetElem_TypedArray::Compiler compiler(cx, obj->lastProperty(), TypedArray::type(obj),
                                                    expectOutOfBounds);
            ICStub *typedArrayStub = compiler.getStub(compiler.getStubSpace(script));
            if (!typedArrayStub)
                return false;

            stub->addNewStub(typedArrayStub);
            return true;
        }
    }

    return true;
}

typedef bool (*DoSetElemFallbackFn)(JSContext *, BaselineFrame *, ICSetElem_Fallback *, Value *,
                                    HandleValue, HandleValue, HandleValue);
static const VMFunction DoSetElemFallbackInfo =
    FunctionInfo<DoSetElemFallbackFn>(DoSetElemFallback, PopValues(2));

bool
ICSetElem_Fallback::Compiler::generateStubCode(MacroAssembler &masm)
{
    JS_ASSERT(R0 == JSReturnOperand);

    EmitRestoreTailCallReg(masm);

    
    
    
    
    masm.pushValue(R1);
    masm.loadValue(Address(BaselineStackReg, sizeof(Value)), R1);
    masm.storeValue(R0, Address(BaselineStackReg, sizeof(Value)));
    masm.pushValue(R1);

    
    masm.pushValue(R1); 

    
    
    masm.mov(BaselineStackReg, R1.scratchReg());
    masm.pushValue(Address(R1.scratchReg(), 2 * sizeof(Value)));
    masm.pushValue(R0); 

    
    
    masm.computeEffectiveAddress(Address(BaselineStackReg, 3 * sizeof(Value)), R0.scratchReg());
    masm.push(R0.scratchReg());

    masm.push(BaselineStubReg);
    masm.pushBaselineFramePtr(BaselineFrameReg, R0.scratchReg());

    return tailCallVM(DoSetElemFallbackInfo, masm);
}





bool
ICSetElem_Dense::Compiler::generateStubCode(MacroAssembler &masm)
{
    
    
    
    Label failure;
    Label failureUnstow;
    masm.branchTestObject(Assembler::NotEqual, R0, &failure);
    masm.branchTestInt32(Assembler::NotEqual, R1, &failure);

    GeneralRegisterSet regs(availableGeneralRegs(2));
    Register scratchReg = regs.takeAny();

    
    Register obj = masm.extractObject(R0, ExtractTemp0);
    masm.loadPtr(Address(BaselineStubReg, ICSetElem_Dense::offsetOfShape()), scratchReg);
    masm.branchTestObjShape(Assembler::NotEqual, obj, scratchReg, &failure);

    
    
    EmitStowICValues(masm, 2);

    
    regs = availableGeneralRegs(0);
    regs.take(R0);

    
    Register typeReg = regs.takeAny();
    masm.loadPtr(Address(BaselineStubReg, ICSetElem_Dense::offsetOfType()), typeReg);
    masm.branchPtr(Assembler::NotEqual, Address(obj, JSObject::offsetOfType()), typeReg,
                   &failureUnstow);
    regs.add(typeReg);

    
    
    masm.loadValue(Address(BaselineStackReg, 2 * sizeof(Value) + ICStackValueOffset), R0);

    
    if (!callTypeUpdateIC(masm, sizeof(Value)))
        return false;

    
    EmitUnstowICValues(masm, 2);

    
    regs = availableGeneralRegs(2);
    scratchReg = regs.takeAny();

    
    masm.loadPtr(Address(obj, JSObject::offsetOfElements()), scratchReg);

    
    Register key = masm.extractInt32(R1, ExtractTemp1);

    
    Address initLength(scratchReg, ObjectElements::offsetOfInitializedLength());
    masm.branch32(Assembler::BelowOrEqual, initLength, key, &failure);

    
    BaseIndex element(scratchReg, key, TimesEight);
    masm.branchTestMagic(Assembler::Equal, element, &failure);

    
    
    
    Label convertDoubles, convertDoublesDone;
    Address elementsFlags(scratchReg, ObjectElements::offsetOfFlags());
    masm.branchTest32(Assembler::NonZero, elementsFlags,
                      Imm32(ObjectElements::CONVERT_DOUBLE_ELEMENTS),
                      &convertDoubles);
    masm.bind(&convertDoublesDone);

    
    Address valueAddr(BaselineStackReg, ICStackValueOffset);
    masm.loadValue(valueAddr, R0);
    EmitPreBarrier(masm, element, MIRType_Value);
    masm.storeValue(R0, element);
    EmitReturnFromIC(masm);

    
    
    
    masm.bind(&convertDoubles);
    if (cx->runtime->jitSupportsFloatingPoint)
        masm.convertInt32ValueToDouble(valueAddr, R0.scratchReg(), &convertDoublesDone);
    else
        masm.breakpoint();
    masm.jump(&convertDoublesDone);

    
    masm.bind(&failureUnstow);
    EmitUnstowICValues(masm, 2);

    
    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}

static bool
GetProtoShapes(JSObject *obj, size_t protoChainDepth, AutoShapeVector *shapes)
{
    JS_ASSERT(shapes->length() == 1);
    JSObject *curProto = obj->getProto();
    for (size_t i = 0; i < protoChainDepth; i++) {
        if (!shapes->append(curProto->lastProperty()))
            return false;
        curProto = curProto->getProto();
    }
    JS_ASSERT(!curProto);
    return true;
}





ICUpdatedStub *
ICSetElemDenseAddCompiler::getStub(ICStubSpace *space)
{
    AutoShapeVector shapes(cx);
    if (!shapes.append(obj_->lastProperty()))
        return NULL;

    if (!GetProtoShapes(obj_, protoChainDepth_, &shapes))
        return NULL;

    JS_STATIC_ASSERT(ICSetElem_DenseAdd::MAX_PROTO_CHAIN_DEPTH == 4);

    ICUpdatedStub *stub = NULL;
    switch (protoChainDepth_) {
      case 0: stub = getStubSpecific<0>(space, &shapes); break;
      case 1: stub = getStubSpecific<1>(space, &shapes); break;
      case 2: stub = getStubSpecific<2>(space, &shapes); break;
      case 3: stub = getStubSpecific<3>(space, &shapes); break;
      case 4: stub = getStubSpecific<4>(space, &shapes); break;
      default: JS_NOT_REACHED("ProtoChainDepth too high.");
    }
    if (!stub || !stub->initUpdatingChain(cx, space))
        return NULL;
    return stub;
}

bool
ICSetElemDenseAddCompiler::generateStubCode(MacroAssembler &masm)
{
    
    
    
    Label failure;
    Label failureUnstow;
    masm.branchTestObject(Assembler::NotEqual, R0, &failure);
    masm.branchTestInt32(Assembler::NotEqual, R1, &failure);

    GeneralRegisterSet regs(availableGeneralRegs(2));
    Register scratchReg = regs.takeAny();

    
    Register obj = masm.extractObject(R0, ExtractTemp0);
    masm.loadPtr(Address(BaselineStubReg, ICSetElem_DenseAddImpl<0>::offsetOfShape(0)),
                 scratchReg);
    masm.branchTestObjShape(Assembler::NotEqual, obj, scratchReg, &failure);

    
    
    EmitStowICValues(masm, 2);

    
    regs = availableGeneralRegs(0);
    regs.take(R0);

    
    Register typeReg = regs.takeAny();
    masm.loadPtr(Address(BaselineStubReg, ICSetElem_DenseAdd::offsetOfType()), typeReg);
    masm.branchPtr(Assembler::NotEqual, Address(obj, JSObject::offsetOfType()), typeReg,
                   &failureUnstow);
    regs.add(typeReg);

    
    scratchReg = regs.takeAny();
    Register protoReg = regs.takeAny();
    for (size_t i = 0; i < protoChainDepth_; i++) {
        masm.loadObjProto(i == 0 ? obj : protoReg, protoReg);
        masm.loadPtr(Address(BaselineStubReg, ICSetElem_DenseAddImpl<0>::offsetOfShape(i + 1)),
                     scratchReg);
        masm.branchTestObjShape(Assembler::NotEqual, protoReg, scratchReg, &failureUnstow);
    }
    regs.add(protoReg);
    regs.add(scratchReg);

    
    
    masm.loadValue(Address(BaselineStackReg, 2 * sizeof(Value) + ICStackValueOffset), R0);

    
    if (!callTypeUpdateIC(masm, sizeof(Value)))
        return false;

    
    EmitUnstowICValues(masm, 2);

    
    regs = availableGeneralRegs(2);
    scratchReg = regs.takeAny();

    
    masm.loadPtr(Address(obj, JSObject::offsetOfElements()), scratchReg);

    
    Register key = masm.extractInt32(R1, ExtractTemp1);

    
    Address initLength(scratchReg, ObjectElements::offsetOfInitializedLength());
    masm.branch32(Assembler::NotEqual, initLength, key, &failure);

    
    Address capacity(scratchReg, ObjectElements::offsetOfCapacity());
    masm.branch32(Assembler::BelowOrEqual, capacity, key, &failure);

    
    masm.add32(Imm32(1), initLength);

    
    Label skipIncrementLength;
    Address length(scratchReg, ObjectElements::offsetOfLength());
    masm.branch32(Assembler::Above, length, key, &skipIncrementLength);
    masm.add32(Imm32(1), length);
    masm.bind(&skipIncrementLength);

    
    
    
    Label convertDoubles, convertDoublesDone;
    Address elementsFlags(scratchReg, ObjectElements::offsetOfFlags());
    masm.branchTest32(Assembler::NonZero, elementsFlags,
                      Imm32(ObjectElements::CONVERT_DOUBLE_ELEMENTS),
                      &convertDoubles);
    masm.bind(&convertDoublesDone);

    
    
    BaseIndex element(scratchReg, key, TimesEight);
    Address valueAddr(BaselineStackReg, ICStackValueOffset);
    masm.loadValue(valueAddr, R0);
    masm.storeValue(R0, element);
    EmitReturnFromIC(masm);

    
    
    
    masm.bind(&convertDoubles);
    if (cx->runtime->jitSupportsFloatingPoint)
        masm.convertInt32ValueToDouble(valueAddr, R0.scratchReg(), &convertDoublesDone);
    else
        masm.breakpoint();
    masm.jump(&convertDoublesDone);

    
    masm.bind(&failureUnstow);
    EmitUnstowICValues(masm, 2);

    
    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}





bool
ICSetElem_TypedArray::Compiler::generateStubCode(MacroAssembler &masm)
{
    Label failure;
    masm.branchTestObject(Assembler::NotEqual, R0, &failure);
    masm.branchTestInt32(Assembler::NotEqual, R1, &failure);

    GeneralRegisterSet regs(availableGeneralRegs(2));
    Register scratchReg = regs.takeAny();

    
    Register obj = masm.extractObject(R0, ExtractTemp0);
    masm.loadPtr(Address(BaselineStubReg, ICSetElem_TypedArray::offsetOfShape()), scratchReg);
    masm.branchTestObjShape(Assembler::NotEqual, obj, scratchReg, &failure);

    
    Register key = masm.extractInt32(R1, ExtractTemp1);

    
    Label oobWrite;
    masm.unboxInt32(Address(obj, TypedArray::lengthOffset()), scratchReg);
    masm.branch32(Assembler::BelowOrEqual, scratchReg, key,
                  expectOutOfBounds_ ? &oobWrite : &failure);

    
    masm.loadPtr(Address(obj, TypedArray::dataOffset()), scratchReg);

    BaseIndex dest(scratchReg, key, ScaleFromElemWidth(TypedArray::slotWidth(type_)));
    Address value(BaselineStackReg, ICStackValueOffset);

    
    
    regs = availableGeneralRegs(0);
    regs.takeUnchecked(obj);
    regs.takeUnchecked(key);
    regs.take(scratchReg);
    Register secondScratch = regs.takeAny();

    if (type_ == TypedArray::TYPE_FLOAT32 || type_ == TypedArray::TYPE_FLOAT64) {
        masm.ensureDouble(value, FloatReg0, &failure);
        masm.storeToTypedFloatArray(type_, FloatReg0, dest);
        EmitReturnFromIC(masm);
    } else if (type_ == TypedArray::TYPE_UINT8_CLAMPED) {
        Label notInt32;
        masm.branchTestInt32(Assembler::NotEqual, value, &notInt32);
        masm.unboxInt32(value, secondScratch);
        masm.clampIntToUint8(secondScratch, secondScratch);

        Label clamped;
        masm.bind(&clamped);
        masm.storeToTypedIntArray(type_, secondScratch, dest);
        EmitReturnFromIC(masm);

        
        
        masm.bind(&notInt32);
        if (cx->runtime->jitSupportsFloatingPoint) {
            masm.branchTestDouble(Assembler::NotEqual, value, &failure);
            masm.unboxDouble(value, FloatReg0);
            masm.clampDoubleToUint8(FloatReg0, secondScratch);
            masm.jump(&clamped);
        } else {
            masm.jump(&failure);
        }
    } else {
        Label notInt32;
        masm.branchTestInt32(Assembler::NotEqual, value, &notInt32);
        masm.unboxInt32(value, secondScratch);

        Label isInt32;
        masm.bind(&isInt32);
        masm.storeToTypedIntArray(type_, secondScratch, dest);
        EmitReturnFromIC(masm);

        
        
        Label failureRestoreRegs;
        masm.bind(&notInt32);
        if (cx->runtime->jitSupportsFloatingPoint) {
            masm.branchTestDouble(Assembler::NotEqual, value, &failure);
            masm.unboxDouble(value, FloatReg0);
            masm.branchTruncateDouble(FloatReg0, secondScratch, &failureRestoreRegs);
            masm.jump(&isInt32);
        } else {
            masm.jump(&failure);
        }

        
        
        masm.bind(&failureRestoreRegs);
        masm.tagValue(JSVAL_TYPE_OBJECT, obj, R0);
        masm.tagValue(JSVAL_TYPE_INT32, key, R1);
    }

    
    masm.bind(&failure);
    EmitStubGuardFailure(masm);

    if (expectOutOfBounds_) {
        masm.bind(&oobWrite);
        EmitReturnFromIC(masm);
    }
    return true;
}





static bool
DoInFallback(JSContext *cx, ICIn_Fallback *stub, HandleValue key, HandleValue objValue,
             MutableHandleValue res)
{
    FallbackICSpew(cx, stub, "In");

    if (!objValue.isObject()) {
        js_ReportValueError(cx, JSMSG_IN_NOT_OBJECT, -1, objValue, NullPtr());
        return false;
    }

    RootedObject obj(cx, &objValue.toObject());

    JSBool cond = false;
    if (!OperatorIn(cx, key, obj, &cond))
        return false;

    res.setBoolean(cond);
    return true;
}

typedef bool (*DoInFallbackFn)(JSContext *, ICIn_Fallback *, HandleValue, HandleValue,
                               MutableHandleValue);
static const VMFunction DoInFallbackInfo =
    FunctionInfo<DoInFallbackFn>(DoInFallback, PopValues(2));

bool
ICIn_Fallback::Compiler::generateStubCode(MacroAssembler &masm)
{
    EmitRestoreTailCallReg(masm);

    
    masm.pushValue(R0);
    masm.pushValue(R1);

    
    masm.pushValue(R1);
    masm.pushValue(R0);
    masm.push(BaselineStubReg);

    return tailCallVM(DoInFallbackInfo, masm);
}


static bool
TryAttachGlobalNameStub(JSContext *cx, HandleScript script, ICGetName_Fallback *stub,
                        HandleObject global, HandlePropertyName name)
{
    JS_ASSERT(global->isGlobal());

    RootedId id(cx, NameToId(name));

    
    RootedShape shape(cx, global->nativeLookup(cx, id));
    if (!shape || !shape->hasDefaultGetter() || !shape->hasSlot())
        return true;

    JS_ASSERT(shape->slot() >= global->numFixedSlots());
    uint32_t slot = shape->slot() - global->numFixedSlots();

    

    ICStub *monitorStub = stub->fallbackMonitorStub()->firstMonitorStub();
    IonSpew(IonSpew_BaselineIC, "  Generating GetName(GlobalName) stub");
    ICGetName_Global::Compiler compiler(cx, monitorStub, global->lastProperty(), slot);
    ICStub *newStub = compiler.getStub(compiler.getStubSpace(script));
    if (!newStub)
        return false;

    stub->addNewStub(newStub);
    return true;
}

static bool
TryAttachScopeNameStub(JSContext *cx, HandleScript script, ICGetName_Fallback *stub,
                       HandleObject initialScopeChain, HandlePropertyName name)
{
    AutoShapeVector shapes(cx);
    RootedId id(cx, NameToId(name));
    RootedObject scopeChain(cx, initialScopeChain);

    Shape *shape = NULL;
    while (scopeChain) {
        if (!shapes.append(scopeChain->lastProperty()))
            return false;

        if (scopeChain->isGlobal()) {
            shape = scopeChain->nativeLookup(cx, id);
            if (shape)
                break;
            return true;
        }

        if (!scopeChain->isScope() || scopeChain->isWith())
            return true;

        
        
        
        shape = scopeChain->nativeLookup(cx, id);
        if (shape)
            break;

        scopeChain = scopeChain->enclosingScope();
    }

    if (!IsCacheableGetPropReadSlot(scopeChain, scopeChain, shape))
        return true;

    bool isFixedSlot;
    uint32_t offset;
    GetFixedOrDynamicSlotOffset(scopeChain, shape->slot(), &isFixedSlot, &offset);

    ICStub *monitorStub = stub->fallbackMonitorStub()->firstMonitorStub();
    ICStub *newStub;

    switch (shapes.length()) {
      case 1: {
        ICGetName_Scope<0>::Compiler compiler(cx, monitorStub, &shapes, isFixedSlot, offset);
        newStub = compiler.getStub(compiler.getStubSpace(script));
        break;
      }
      case 2: {
        ICGetName_Scope<1>::Compiler compiler(cx, monitorStub, &shapes, isFixedSlot, offset);
        newStub = compiler.getStub(compiler.getStubSpace(script));
        break;
      }
      case 3: {
        ICGetName_Scope<2>::Compiler compiler(cx, monitorStub, &shapes, isFixedSlot, offset);
        newStub = compiler.getStub(compiler.getStubSpace(script));
        break;
      }
      case 4: {
        ICGetName_Scope<3>::Compiler compiler(cx, monitorStub, &shapes, isFixedSlot, offset);
        newStub = compiler.getStub(compiler.getStubSpace(script));
        break;
      }
      case 5: {
        ICGetName_Scope<4>::Compiler compiler(cx, monitorStub, &shapes, isFixedSlot, offset);
        newStub = compiler.getStub(compiler.getStubSpace(script));
        break;
      }
      case 6: {
        ICGetName_Scope<5>::Compiler compiler(cx, monitorStub, &shapes, isFixedSlot, offset);
        newStub = compiler.getStub(compiler.getStubSpace(script));
        break;
      }
      case 7: {
        ICGetName_Scope<6>::Compiler compiler(cx, monitorStub, &shapes, isFixedSlot, offset);
        newStub = compiler.getStub(compiler.getStubSpace(script));
        break;
      }
      default:
        return true;
    }

    if (!newStub)
        return false;

    stub->addNewStub(newStub);
    return true;
}

static bool
DoGetNameFallback(JSContext *cx, BaselineFrame *frame, ICGetName_Fallback *stub,
                  HandleObject scopeChain, MutableHandleValue res)
{
    RootedScript script(cx, frame->script());
    jsbytecode *pc = stub->icEntry()->pc(script);
    mozilla::DebugOnly<JSOp> op = JSOp(*pc);
    FallbackICSpew(cx, stub, "GetName(%s)", js_CodeName[JSOp(*pc)]);

    JS_ASSERT(op == JSOP_NAME || op == JSOP_CALLNAME || op == JSOP_GETGNAME || op == JSOP_CALLGNAME);

    RootedPropertyName name(cx, script->getName(pc));

    if (JSOp(pc[JSOP_GETGNAME_LENGTH]) == JSOP_TYPEOF) {
        if (!GetScopeNameForTypeOf(cx, scopeChain, name, res))
            return false;
    } else {
        if (!GetScopeName(cx, scopeChain, name, res))
            return false;
    }

    types::TypeScript::Monitor(cx, script, pc, res);

    
    if (!stub->addMonitorStubForValue(cx, script, res))
        return false;

    
    if (stub->numOptimizedStubs() >= ICGetName_Fallback::MAX_OPTIMIZED_STUBS) {
        
        return true;
    }

    if (js_CodeSpec[*pc].format & JOF_GNAME) {
        if (!TryAttachGlobalNameStub(cx, script, stub, scopeChain, name))
            return false;
    } else {
        if (!TryAttachScopeNameStub(cx, script, stub, scopeChain, name))
            return false;
    }

    return true;
}

typedef bool (*DoGetNameFallbackFn)(JSContext *, BaselineFrame *, ICGetName_Fallback *,
                                    HandleObject, MutableHandleValue);
static const VMFunction DoGetNameFallbackInfo = FunctionInfo<DoGetNameFallbackFn>(DoGetNameFallback);

bool
ICGetName_Fallback::Compiler::generateStubCode(MacroAssembler &masm)
{
    JS_ASSERT(R0 == JSReturnOperand);

    EmitRestoreTailCallReg(masm);

    masm.push(R0.scratchReg());
    masm.push(BaselineStubReg);
    masm.pushBaselineFramePtr(BaselineFrameReg, R0.scratchReg());

    return tailCallVM(DoGetNameFallbackInfo, masm);
}

bool
ICGetName_Global::Compiler::generateStubCode(MacroAssembler &masm)
{
    Label failure;
    Register obj = R0.scratchReg();
    Register scratch = R1.scratchReg();

    
    masm.loadPtr(Address(BaselineStubReg, ICGetName_Global::offsetOfShape()), scratch);
    masm.branchTestObjShape(Assembler::NotEqual, obj, scratch, &failure);

    
    masm.loadPtr(Address(obj, JSObject::offsetOfSlots()), obj);
    masm.load32(Address(BaselineStubReg, ICGetName_Global::offsetOfSlot()), scratch);
    masm.loadValue(BaseIndex(obj, scratch, TimesEight), R0);

    
    EmitEnterTypeMonitorIC(masm);

    
    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}

template <size_t NumHops>
bool
ICGetName_Scope<NumHops>::Compiler::generateStubCode(MacroAssembler &masm)
{
    Label failure;
    GeneralRegisterSet regs(availableGeneralRegs(1));
    Register obj = R0.scratchReg();
    Register walker = regs.takeAny();
    Register scratch = regs.takeAny();

    
    size_t numHops = NumHops;

    for (size_t index = 0; index < NumHops + 1; index++) {
        Register scope = index ? walker : obj;

        
        masm.loadPtr(Address(BaselineStubReg, ICGetName_Scope::offsetOfShape(index)), scratch);
        masm.branchTestObjShape(Assembler::NotEqual, scope, scratch, &failure);

        if (index < numHops)
            masm.extractObject(Address(scope, ScopeObject::offsetOfEnclosingScope()), walker);
    }

    Register scope = NumHops ? walker : obj;

    if (!isFixedSlot_) {
        masm.loadPtr(Address(scope, JSObject::offsetOfSlots()), walker);
        scope = walker;
    }

    masm.load32(Address(BaselineStubReg, ICGetName_Scope::offsetOfOffset()), scratch);
    masm.loadValue(BaseIndex(scope, scratch, TimesOne), R0);

    
    EmitEnterTypeMonitorIC(masm);

    
    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}





static bool
DoBindNameFallback(JSContext *cx, BaselineFrame *frame, ICBindName_Fallback *stub,
                   HandleObject scopeChain, MutableHandleValue res)
{
    jsbytecode *pc = stub->icEntry()->pc(frame->script());
    mozilla::DebugOnly<JSOp> op = JSOp(*pc);
    FallbackICSpew(cx, stub, "BindName(%s)", js_CodeName[JSOp(*pc)]);

    JS_ASSERT(op == JSOP_BINDNAME);

    RootedPropertyName name(cx, frame->script()->getName(pc));

    RootedObject scope(cx);
    if (!LookupNameWithGlobalDefault(cx, name, scopeChain, &scope))
        return false;

    res.setObject(*scope);
    return true;
}

typedef bool (*DoBindNameFallbackFn)(JSContext *, BaselineFrame *, ICBindName_Fallback *,
                                     HandleObject, MutableHandleValue);
static const VMFunction DoBindNameFallbackInfo =
    FunctionInfo<DoBindNameFallbackFn>(DoBindNameFallback);

bool
ICBindName_Fallback::Compiler::generateStubCode(MacroAssembler &masm)
{
    JS_ASSERT(R0 == JSReturnOperand);

    EmitRestoreTailCallReg(masm);

    masm.push(R0.scratchReg());
    masm.push(BaselineStubReg);
    masm.pushBaselineFramePtr(BaselineFrameReg, R0.scratchReg());

    return tailCallVM(DoBindNameFallbackInfo, masm);
}





static bool
DoGetIntrinsicFallback(JSContext *cx, BaselineFrame *frame, ICGetIntrinsic_Fallback *stub,
                       MutableHandleValue res)
{
    RootedScript script(cx, frame->script());
    jsbytecode *pc = stub->icEntry()->pc(script);
    mozilla::DebugOnly<JSOp> op = JSOp(*pc);
    FallbackICSpew(cx, stub, "GetIntrinsic(%s)", js_CodeName[JSOp(*pc)]);

    JS_ASSERT(op == JSOP_GETINTRINSIC || op == JSOP_CALLINTRINSIC);

    if (!GetIntrinsicOperation(cx, pc, res))
        return false;

    
    
    

    types::TypeScript::Monitor(cx, script, pc, res);

    IonSpew(IonSpew_BaselineIC, "  Generating GetIntrinsic optimized stub");
    ICGetIntrinsic_Constant::Compiler compiler(cx, res);
    ICStub *newStub = compiler.getStub(compiler.getStubSpace(script));
    if (!newStub)
        return false;

    stub->addNewStub(newStub);
    return true;
}

typedef bool (*DoGetIntrinsicFallbackFn)(JSContext *, BaselineFrame *, ICGetIntrinsic_Fallback *,
                                         MutableHandleValue);
static const VMFunction DoGetIntrinsicFallbackInfo =
    FunctionInfo<DoGetIntrinsicFallbackFn>(DoGetIntrinsicFallback);

bool
ICGetIntrinsic_Fallback::Compiler::generateStubCode(MacroAssembler &masm)
{
    EmitRestoreTailCallReg(masm);

    masm.push(BaselineStubReg);
    masm.pushBaselineFramePtr(BaselineFrameReg, R0.scratchReg());

    return tailCallVM(DoGetIntrinsicFallbackInfo, masm);
}

bool
ICGetIntrinsic_Constant::Compiler::generateStubCode(MacroAssembler &masm)
{
    masm.loadValue(Address(BaselineStubReg, ICGetIntrinsic_Constant::offsetOfValue()), R0);

    EmitReturnFromIC(masm);
    return true;
}





static bool
TryAttachLengthStub(JSContext *cx, HandleScript script, ICGetProp_Fallback *stub, HandleValue val,
                    HandleValue res, bool *attached)
{
    JS_ASSERT(!*attached);

    if (val.isString()) {
        JS_ASSERT(res.isInt32());
        IonSpew(IonSpew_BaselineIC, "  Generating GetProp(String.length) stub");
        ICGetProp_StringLength::Compiler compiler(cx);
        ICStub *newStub = compiler.getStub(compiler.getStubSpace(script));
        if (!newStub)
            return false;

        *attached = true;
        stub->addNewStub(newStub);
        return true;
    }

    if (val.isMagic(JS_OPTIMIZED_ARGUMENTS) && res.isInt32()) {
        IonSpew(IonSpew_BaselineIC, "  Generating GetProp(MagicArgs.length) stub");
        ICGetProp_ArgumentsLength::Compiler compiler(cx, ICGetProp_ArgumentsLength::Magic);
        ICStub *newStub = compiler.getStub(compiler.getStubSpace(script));
        if (!newStub)
            return false;

        *attached = true;
        stub->addNewStub(newStub);
        return true;
    }

    if (!val.isObject())
        return true;

    RootedObject obj(cx, &val.toObject());

    if (obj->isArray() && res.isInt32()) {
        IonSpew(IonSpew_BaselineIC, "  Generating GetProp(Array.length) stub");
        ICGetProp_ArrayLength::Compiler compiler(cx);
        ICStub *newStub = compiler.getStub(compiler.getStubSpace(script));
        if (!newStub)
            return false;

        *attached = true;
        stub->addNewStub(newStub);
        return true;
    }
    if (obj->isTypedArray()) {
        JS_ASSERT(res.isInt32());
        IonSpew(IonSpew_BaselineIC, "  Generating GetProp(TypedArray.length) stub");
        ICGetProp_TypedArrayLength::Compiler compiler(cx);
        ICStub *newStub = compiler.getStub(compiler.getStubSpace(script));
        if (!newStub)
            return false;

        *attached = true;
        stub->addNewStub(newStub);
        return true;
    }

    if (obj->isArguments() && res.isInt32()) {
        IonSpew(IonSpew_BaselineIC, "  Generating GetProp(ArgsObj.length %s) stub",
                obj->isStrictArguments() ? "Strict" : "Normal");
        ICGetProp_ArgumentsLength::Which which = ICGetProp_ArgumentsLength::Normal;
        if (obj->isStrictArguments())
            which = ICGetProp_ArgumentsLength::Strict;
        ICGetProp_ArgumentsLength::Compiler compiler(cx, which);
        ICStub *newStub = compiler.getStub(compiler.getStubSpace(script));
        if (!newStub)
            return false;

        *attached = true;
        stub->addNewStub(newStub);
        return true;
    }

    return true;
}

static bool
UpdateExistingGenerationalListBaseStub(ICGetProp_Fallback *stub,
                                       HandleObject obj)
{
    Value expandoSlot = obj->getFixedSlot(GetListBaseExpandoSlot());
    JS_ASSERT(!expandoSlot.isObject() && !expandoSlot.isUndefined());
    ExpandoAndGeneration *expandoAndGeneration = (ExpandoAndGeneration*)expandoSlot.toPrivate();
    for (ICStubConstIterator iter = stub->beginChainConst(); !iter.atEnd(); iter++) {
        if (iter->isGetProp_CallListBaseWithGenerationNative()) {
            ICGetProp_CallListBaseWithGenerationNative* updateStub =
                iter->toGetProp_CallListBaseWithGenerationNative();
            if (updateStub->expandoAndGeneration() == expandoAndGeneration) {
                
                uint32_t generation = expandoAndGeneration->generation;
                IonSpew(IonSpew_BaselineIC,
                        "  Updating existing stub with generation, old value: %i, "
                        "new value: %i", updateStub->generation(),
                        generation);
                updateStub->setGeneration(generation);
                return true;
            }
        }
    }
    return false;
}

static bool
TryAttachNativeGetPropStub(JSContext *cx, HandleScript script, jsbytecode *pc,
                           ICGetProp_Fallback *stub, HandlePropertyName name,
                           HandleValue val, HandleValue res, bool *attached)
{
    JS_ASSERT(!*attached);

    if (!val.isObject())
        return true;

    RootedObject obj(cx, &val.toObject());

    bool isListBase;
    bool listBaseHasGeneration;
    ListBaseShadowsResult listBaseShadowsResult;
    RootedShape shape(cx);
    RootedObject holder(cx);
    if (!EffectlesslyLookupProperty(cx, obj, name, &holder, &shape, &isListBase,
                                    &listBaseShadowsResult, &listBaseHasGeneration))
    {
        return false;
    }

    if (!isListBase && !obj->isNative())
        return true;

    ICStub *monitorStub = stub->fallbackMonitorStub()->firstMonitorStub();
    if (!isListBase && IsCacheableGetPropReadSlot(obj, holder, shape)) {
        bool isFixedSlot;
        uint32_t offset;
        GetFixedOrDynamicSlotOffset(holder, shape->slot(), &isFixedSlot, &offset);

        ICStub::Kind kind = (obj == holder) ? ICStub::GetProp_Native
                                            : ICStub::GetProp_NativePrototype;

        IonSpew(IonSpew_BaselineIC, "  Generating GetProp(%s %s) stub",
                    isListBase ? "ListBase" : "Native",
                    (obj == holder) ? "direct" : "prototype");
        ICGetPropNativeCompiler compiler(cx, kind, monitorStub, obj, holder, isFixedSlot, offset);
        ICStub *newStub = compiler.getStub(compiler.getStubSpace(script));
        if (!newStub)
            return false;

        stub->addNewStub(newStub);
        *attached = true;
        return true;
    }

    bool isScripted = false;
    bool cacheableCall = IsCacheableGetPropCall(obj, holder, shape, &isScripted, isListBase);

    
    if (cacheableCall && isScripted && !isListBase) {
        RootedFunction callee(cx, shape->getterObject()->toFunction());
        JS_ASSERT(obj != holder);
        JS_ASSERT(callee->hasScript());

        IonSpew(IonSpew_BaselineIC, "  Generating GetProp(NativeObj/ScriptedGetter %s:%d) stub",
                    callee->nonLazyScript()->filename(), callee->nonLazyScript()->lineno);

        ICGetProp_CallScripted::Compiler compiler(cx, monitorStub, obj, holder, callee,
                                                  pc - script->code);
        ICStub *newStub = compiler.getStub(compiler.getStubSpace(script));
        if (!newStub)
            return false;

        stub->addNewStub(newStub);
        *attached = true;
        return true;
    }

    
    if (cacheableCall && !isScripted) {
        RootedFunction callee(cx, shape->getterObject()->toFunction());
        JS_ASSERT(obj != holder);
        JS_ASSERT(callee->isNative());

        IonSpew(IonSpew_BaselineIC, "  Generating GetProp(%s%s/NativeGetter %p) stub",
                isListBase ? "ListBaseObj" : "NativeObj",
                isListBase && listBaseHasGeneration ? "WithGeneration" : "",
                callee->native());

        ICStub *newStub = NULL;
        if (isListBase) {
            ICStub::Kind kind;
            if (listBaseHasGeneration) {
                if (UpdateExistingGenerationalListBaseStub(stub, obj)) {
                    *attached = true;
                    return true;
                }
                kind = ICStub::GetProp_CallListBaseWithGenerationNative;
            } else {
                kind = ICStub::GetProp_CallListBaseNative;
            }
            ICGetPropCallListBaseNativeCompiler compiler(cx, kind, monitorStub, obj, holder, callee,
                                                            pc - script->code);
            newStub = compiler.getStub(compiler.getStubSpace(script));
        } else {
            ICGetProp_CallNative::Compiler compiler(cx, monitorStub, obj, holder, callee,
                                                    pc - script->code);
            newStub = compiler.getStub(compiler.getStubSpace(script));
        }
        if (!newStub)
            return false;
        stub->addNewStub(newStub);
        *attached = true;
        return true;
    }

    
    if (isListBase && listBaseShadowsResult == Shadows) {
        JS_ASSERT(obj == holder);

        IonSpew(IonSpew_BaselineIC, "  Generating GetProp(ListBaseProxy) stub");
        ICGetProp_ListBaseShadowed::Compiler compiler(cx, monitorStub, obj, name,
                                                      pc - script->code);
        ICStub *newStub = compiler.getStub(compiler.getStubSpace(script));
        if (!newStub)
            return false;
        stub->addNewStub(newStub);
        *attached = true;
        return true;
    }

    return true;
}

static bool
TryAttachStringGetPropStub(JSContext *cx, HandleScript script, ICGetProp_Fallback *stub,
                           HandlePropertyName name, HandleValue val, HandleValue res,
                           bool *attached)
{
    JS_ASSERT(!*attached);
    JS_ASSERT(val.isString());

    RootedObject stringProto(cx, script->global().getOrCreateStringPrototype(cx));
    if (!stringProto)
        return false;

    
    RootedId propId(cx, NameToId(name));
    RootedShape shape(cx, stringProto->nativeLookup(cx, propId));
    if (!shape || !shape->hasSlot() || !shape->hasDefaultGetter())
        return true;

    bool isFixedSlot;
    uint32_t offset;
    GetFixedOrDynamicSlotOffset(stringProto, shape->slot(), &isFixedSlot, &offset);

    ICStub *monitorStub = stub->fallbackMonitorStub()->firstMonitorStub();

    IonSpew(IonSpew_BaselineIC, "  Generating GetProp(String.ID from prototype) stub");
    ICGetProp_String::Compiler compiler(cx, monitorStub, stringProto, isFixedSlot, offset);
    ICStub *newStub = compiler.getStub(compiler.getStubSpace(script));
    if (!newStub)
        return false;

    stub->addNewStub(newStub);
    *attached = true;
    return true;
}

static bool
DoGetPropFallback(JSContext *cx, BaselineFrame *frame, ICGetProp_Fallback *stub,
                  MutableHandleValue val, MutableHandleValue res)
{
    RootedScript script(cx, frame->script());
    jsbytecode *pc = stub->icEntry()->pc(script);
    JSOp op = JSOp(*pc);
    FallbackICSpew(cx, stub, "GetProp(%s)", js_CodeName[op]);

    JS_ASSERT(op == JSOP_GETPROP || op == JSOP_CALLPROP || op == JSOP_LENGTH || op == JSOP_GETXPROP);

    RootedPropertyName name(cx, script->getName(pc));
    RootedId id(cx, NameToId(name));

    if (op == JSOP_LENGTH && val.isMagic(JS_OPTIMIZED_ARGUMENTS)) {
        
        if (IsOptimizedArguments(frame, val.address())) {
            res.setInt32(frame->numActualArgs());

            
            types::TypeScript::Monitor(cx, script, pc, res);
            if (!stub->addMonitorStubForValue(cx, script, res))
                return false;

            bool attached = false;
            if (!TryAttachLengthStub(cx, script, stub, val, res, &attached))
                return false;
            JS_ASSERT(attached);

            return true;
        }
    }

    RootedObject obj(cx, ToObjectFromStack(cx, val));
    if (!obj)
        return false;

    if (obj->getOps()->getProperty) {
        if (!JSObject::getGeneric(cx, obj, obj, id, res))
            return false;
    } else {
        if (!GetPropertyHelper(cx, obj, id, 0, res))
            return false;
    }

#if JS_HAS_NO_SUCH_METHOD
    
    if (op == JSOP_CALLPROP && JS_UNLIKELY(res.isPrimitive()) && val.isObject()) {
        if (!OnUnknownMethod(cx, obj, IdToValue(id), res))
            return false;
    }
#endif

    types::TypeScript::Monitor(cx, script, pc, res);

    
    if (!stub->addMonitorStubForValue(cx, script, res))
        return false;

    if (stub->numOptimizedStubs() >= ICGetProp_Fallback::MAX_OPTIMIZED_STUBS) {
        
        return true;
    }

    bool attached = false;

    if (op == JSOP_LENGTH) {
        if (!TryAttachLengthStub(cx, script, stub, val, res, &attached))
            return false;
        if (attached)
            return true;
    }

    if (!TryAttachNativeGetPropStub(cx, script, pc, stub, name, val, res, &attached))
        return false;
    if (attached)
        return true;

    if (val.isString()) {
        if (!TryAttachStringGetPropStub(cx, script, stub, name, val, res, &attached))
            return false;
        if (attached)
            return true;
    }

    JS_ASSERT(!attached);
    stub->noteUnoptimizableAccess();

    return true;
}

typedef bool (*DoGetPropFallbackFn)(JSContext *, BaselineFrame *, ICGetProp_Fallback *,
                                    MutableHandleValue, MutableHandleValue);
static const VMFunction DoGetPropFallbackInfo =
    FunctionInfo<DoGetPropFallbackFn>(DoGetPropFallback, PopValues(1));

bool
ICGetProp_Fallback::Compiler::generateStubCode(MacroAssembler &masm)
{
    JS_ASSERT(R0 == JSReturnOperand);

    EmitRestoreTailCallReg(masm);

    
    masm.pushValue(R0);

    
    masm.pushValue(R0);
    masm.push(BaselineStubReg);
    masm.pushBaselineFramePtr(BaselineFrameReg, R0.scratchReg());

    return tailCallVM(DoGetPropFallbackInfo, masm);
}

bool
ICGetProp_ArrayLength::Compiler::generateStubCode(MacroAssembler &masm)
{
    Label failure;
    masm.branchTestObject(Assembler::NotEqual, R0, &failure);

    Register scratch = R1.scratchReg();

    
    Register obj = masm.extractObject(R0, ExtractTemp0);
    masm.branchTestObjClass(Assembler::NotEqual, obj, scratch, &ArrayClass, &failure);

    
    masm.loadPtr(Address(obj, JSObject::offsetOfElements()), scratch);
    masm.load32(Address(scratch, ObjectElements::offsetOfLength()), scratch);

    
    masm.branchTest32(Assembler::Signed, scratch, scratch, &failure);

    masm.tagValue(JSVAL_TYPE_INT32, scratch, R0);
    EmitReturnFromIC(masm);

    
    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}

bool
ICGetProp_TypedArrayLength::Compiler::generateStubCode(MacroAssembler &masm)
{
    Label failure;
    masm.branchTestObject(Assembler::NotEqual, R0, &failure);

    Register scratch = R1.scratchReg();

    
    Register obj = masm.extractObject(R0, ExtractTemp0);

    
    masm.loadObjClass(obj, scratch);
    masm.branchPtr(Assembler::Below, scratch, ImmWord(&TypedArray::classes[0]), &failure);
    masm.branchPtr(Assembler::AboveOrEqual, scratch,
                   ImmWord(&TypedArray::classes[TypedArray::TYPE_MAX]), &failure);

    
    masm.loadValue(Address(obj, TypedArray::lengthOffset()), R0);
    EmitReturnFromIC(masm);

    
    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}

bool
ICGetProp_StringLength::Compiler::generateStubCode(MacroAssembler &masm)
{
    Label failure;
    masm.branchTestString(Assembler::NotEqual, R0, &failure);

    
    Register string = masm.extractString(R0, ExtractTemp0);
    masm.loadStringLength(string, string);

    masm.tagValue(JSVAL_TYPE_INT32, string, R0);
    EmitReturnFromIC(masm);

    
    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}

bool
ICGetProp_String::Compiler::generateStubCode(MacroAssembler &masm)
{
    Label failure;
    masm.branchTestString(Assembler::NotEqual, R0, &failure);

    GeneralRegisterSet regs(availableGeneralRegs(1));
    Register holderReg = regs.takeAny();
    Register scratchReg = regs.takeAny();

    
    masm.movePtr(ImmGCPtr(stringPrototype_.get()), holderReg);

    Address shapeAddr(BaselineStubReg, ICGetProp_String::offsetOfStringProtoShape());
    masm.loadPtr(Address(holderReg, JSObject::offsetOfShape()), scratchReg);
    masm.branchPtr(Assembler::NotEqual, shapeAddr, scratchReg, &failure);

    if (!isFixedSlot_)
        masm.loadPtr(Address(holderReg, JSObject::offsetOfSlots()), holderReg);

    masm.load32(Address(BaselineStubReg, ICGetPropNativeStub::offsetOfOffset()), scratchReg);
    masm.loadValue(BaseIndex(holderReg, scratchReg, TimesOne), R0);

    
    EmitEnterTypeMonitorIC(masm);

    
    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}

bool
ICGetPropNativeCompiler::generateStubCode(MacroAssembler &masm)
{
    Label failure;
    GeneralRegisterSet regs(availableGeneralRegs(1));

    
    masm.branchTestObject(Assembler::NotEqual, R0, &failure);

    Register scratch = regs.takeAny();

    
    Register objReg = masm.extractObject(R0, ExtractTemp0);
    masm.loadPtr(Address(BaselineStubReg, ICGetPropNativeStub::offsetOfShape()), scratch);
    masm.branchTestObjShape(Assembler::NotEqual, objReg, scratch, &failure);

    Register holderReg;
    if (obj_ == holder_) {
        holderReg = objReg;
    } else {
        
        holderReg = regs.takeAny();
        masm.loadPtr(Address(BaselineStubReg, ICGetProp_NativePrototype::offsetOfHolder()),
                     holderReg);
        masm.loadPtr(Address(BaselineStubReg, ICGetProp_NativePrototype::offsetOfHolderShape()),
                     scratch);
        masm.branchTestObjShape(Assembler::NotEqual, holderReg, scratch, &failure);
    }

    if (!isFixedSlot_)
        masm.loadPtr(Address(holderReg, JSObject::offsetOfSlots()), holderReg);

    masm.load32(Address(BaselineStubReg, ICGetPropNativeStub::offsetOfOffset()), scratch);
    masm.loadValue(BaseIndex(holderReg, scratch, TimesOne), R0);

    
    EmitEnterTypeMonitorIC(masm);

    
    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}

bool
ICGetProp_CallScripted::Compiler::generateStubCode(MacroAssembler &masm)
{
    Label failure;
    Label failureLeaveStubFrame;
    GeneralRegisterSet regs(availableGeneralRegs(1));
    Register scratch = regs.takeAnyExcluding(BaselineTailCallReg);

    
    masm.branchTestObject(Assembler::NotEqual, R0, &failure);

    
    Register objReg = masm.extractObject(R0, ExtractTemp0);
    masm.loadPtr(Address(BaselineStubReg, ICGetProp_CallScripted::offsetOfShape()), scratch);
    masm.branchTestObjShape(Assembler::NotEqual, objReg, scratch, &failure);

    Register holderReg = regs.takeAny();
    masm.loadPtr(Address(BaselineStubReg, ICGetProp_CallScripted::offsetOfHolder()), holderReg);
    masm.loadPtr(Address(BaselineStubReg, ICGetProp_CallScripted::offsetOfHolderShape()), scratch);
    masm.branchTestObjShape(Assembler::NotEqual, holderReg, scratch, &failure);
    regs.add(holderReg);

    
    enterStubFrame(masm, scratch);

    
    
    Register callee;
    if (regs.has(ArgumentsRectifierReg)) {
        callee = ArgumentsRectifierReg;
        regs.take(callee);
    } else {
        callee = regs.takeAny();
    }
    Register code = regs.takeAny();
    masm.loadPtr(Address(BaselineStubReg, ICGetProp_CallScripted::offsetOfGetter()), callee);
    masm.loadPtr(Address(callee, JSFunction::offsetOfNativeOrScript()), code);
    masm.loadBaselineOrIonRaw(code, code, SequentialExecution, &failureLeaveStubFrame);

    
    
    
    masm.Push(R0);
    EmitCreateStubFrameDescriptor(masm, scratch);
    masm.Push(Imm32(0));  
    masm.Push(callee);
    masm.Push(scratch);

    
    Label noUnderflow;
    masm.load16ZeroExtend(Address(callee, offsetof(JSFunction, nargs)), scratch);
    masm.branch32(Assembler::Equal, scratch, Imm32(0), &noUnderflow);
    {
        
        JS_ASSERT(ArgumentsRectifierReg != code);

        IonCode *argumentsRectifier =
            cx->compartment->ionCompartment()->getArgumentsRectifier(SequentialExecution);

        masm.movePtr(ImmGCPtr(argumentsRectifier), code);
        masm.loadPtr(Address(code, IonCode::offsetOfCode()), code);
        masm.mov(Imm32(0), ArgumentsRectifierReg);
    }

    masm.bind(&noUnderflow);

    
    
    {
        Label skipProfilerUpdate;

        
        GeneralRegisterSet availRegs = availableGeneralRegs(0);
        availRegs.take(ArgumentsRectifierReg);
        availRegs.take(code);
        Register scratch = availRegs.takeAny();
        Register pcIdx = availRegs.takeAny();

        
        guardProfilingEnabled(masm, scratch, &skipProfilerUpdate);

        
        masm.load32(Address(BaselineStubReg, ICGetProp_CallScripted::offsetOfPCOffset()), pcIdx);
        masm.spsUpdatePCIdx(&cx->runtime->spsProfiler, pcIdx, scratch);

        masm.bind(&skipProfilerUpdate);
    }
    masm.callIon(code);

    leaveStubFrame(masm, true);

    
    EmitEnterTypeMonitorIC(masm);

    
    masm.bind(&failureLeaveStubFrame);
    leaveStubFrame(masm, false);

    
    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}

static bool
DoCallNativeGetter(JSContext *cx, HandleFunction callee, HandleObject obj,
                   MutableHandleValue result)
{
    JS_ASSERT(callee->isNative());
    JSNative natfun = callee->native();

    Value vp[2] = { ObjectValue(*callee.get()), ObjectValue(*obj.get()) };
    AutoValueArray rootVp(cx, vp, 2);

    if (!natfun(cx, 0, vp))
        return false;

    result.set(vp[0]);
    return true;
}

typedef bool (*DoCallNativeGetterFn)(JSContext *, HandleFunction, HandleObject, MutableHandleValue);
static const VMFunction DoCallNativeGetterInfo =
    FunctionInfo<DoCallNativeGetterFn>(DoCallNativeGetter);

bool
ICGetProp_CallNative::Compiler::generateStubCode(MacroAssembler &masm)
{
    Label failure;
    GeneralRegisterSet regs(availableGeneralRegs(1));
    Register scratch = regs.takeAnyExcluding(BaselineTailCallReg);

    
    masm.branchTestObject(Assembler::NotEqual, R0, &failure);

    
    Register objReg = masm.extractObject(R0, ExtractTemp0);
    masm.loadPtr(Address(BaselineStubReg, ICGetProp_CallNative::offsetOfShape()), scratch);
    masm.branchTestObjShape(Assembler::NotEqual, objReg, scratch, &failure);

    Register holderReg = regs.takeAny();
    masm.loadPtr(Address(BaselineStubReg, ICGetProp_CallNative::offsetOfHolder()), holderReg);
    masm.loadPtr(Address(BaselineStubReg, ICGetProp_CallNative::offsetOfHolderShape()), scratch);
    masm.branchTestObjShape(Assembler::NotEqual, holderReg, scratch, &failure);
    regs.add(holderReg);

    
    enterStubFrame(masm, scratch);

    
    Register callee = regs.takeAny();
    masm.loadPtr(Address(BaselineStubReg, ICGetProp_CallNative::offsetOfGetter()), callee);

    
    masm.push(objReg);
    masm.push(callee);

    
    regs.add(R0);

    
    {
        Label skipProfilerUpdate;
        Register scratch = regs.takeAny();
        Register pcIdx = regs.takeAny();

        
        guardProfilingEnabled(masm, scratch, &skipProfilerUpdate);

        
        masm.load32(Address(BaselineStubReg, ICGetProp_CallNative::offsetOfPCOffset()), pcIdx);
        masm.spsUpdatePCIdx(&cx->runtime->spsProfiler, pcIdx, scratch);

        masm.bind(&skipProfilerUpdate);
        regs.add(scratch);
        regs.add(pcIdx);
    }
    if (!callVM(DoCallNativeGetterInfo, masm))
        return false;
    leaveStubFrame(masm);

    
    EmitEnterTypeMonitorIC(masm);

    
    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}

bool
ICGetPropCallListBaseNativeCompiler::generateStubCode(MacroAssembler &masm,
                                                      Address* expandoAndGenerationAddr,
                                                      Address* generationAddr)
{
    Label failure;
    GeneralRegisterSet regs(availableGeneralRegs(1));
    Register scratch = regs.takeAnyExcluding(BaselineTailCallReg);

    
    masm.branchTestObject(Assembler::NotEqual, R0, &failure);

    
    Register objReg = masm.extractObject(R0, ExtractTemp0);

    
    masm.loadPtr(Address(BaselineStubReg, ICGetProp_CallListBaseNative::offsetOfShape()), scratch);
    masm.branchTestObjShape(Assembler::NotEqual, objReg, scratch, &failure);

    
    {
        GeneralRegisterSet listBaseRegSet(GeneralRegisterSet::All());
        listBaseRegSet.take(BaselineStubReg);
        listBaseRegSet.take(objReg);
        listBaseRegSet.take(scratch);
        Address expandoShapeAddr(BaselineStubReg, ICGetProp_CallListBaseNative::offsetOfExpandoShape());
        GenerateListBaseChecks(
                cx, masm, objReg,
                Address(BaselineStubReg, ICGetProp_CallListBaseNative::offsetOfProxyHandler()),
                &expandoShapeAddr, expandoAndGenerationAddr, generationAddr,
                scratch,
                listBaseRegSet,
                &failure);
    }

    Register holderReg = regs.takeAny();
    masm.loadPtr(Address(BaselineStubReg, ICGetProp_CallListBaseNative::offsetOfHolder()),
                 holderReg);
    masm.loadPtr(Address(BaselineStubReg, ICGetProp_CallListBaseNative::offsetOfHolderShape()),
                 scratch);
    masm.branchTestObjShape(Assembler::NotEqual, holderReg, scratch, &failure);
    regs.add(holderReg);

    
    enterStubFrame(masm, scratch);

    
    Register callee = regs.takeAny();
    masm.loadPtr(Address(BaselineStubReg, ICGetProp_CallListBaseNative::offsetOfGetter()), callee);

    
    masm.push(objReg);
    masm.push(callee);

    
    regs.add(R0);

    
    {
        Label skipProfilerUpdate;
        Register scratch = regs.takeAny();
        Register pcIdx = regs.takeAny();

        
        guardProfilingEnabled(masm, scratch, &skipProfilerUpdate);

        
        masm.load32(Address(BaselineStubReg, ICGetProp_CallListBaseNative::offsetOfPCOffset()),
                    pcIdx);
        masm.spsUpdatePCIdx(&cx->runtime->spsProfiler, pcIdx, scratch);

        masm.bind(&skipProfilerUpdate);
        regs.add(scratch);
        regs.add(pcIdx);
    }
    if (!callVM(DoCallNativeGetterInfo, masm))
        return false;
    leaveStubFrame(masm);

    
    EmitEnterTypeMonitorIC(masm);

    
    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}

bool
ICGetPropCallListBaseNativeCompiler::generateStubCode(MacroAssembler &masm)
{
    if (kind == ICStub::GetProp_CallListBaseNative)
        return generateStubCode(masm, NULL, NULL);

    Address internalStructAddress(BaselineStubReg,
        ICGetProp_CallListBaseWithGenerationNative::offsetOfInternalStruct());
    Address generationAddress(BaselineStubReg,
        ICGetProp_CallListBaseWithGenerationNative::offsetOfGeneration());
    return generateStubCode(masm, &internalStructAddress, &generationAddress);
}

ICStub *
ICGetPropCallListBaseNativeCompiler::getStub(ICStubSpace *space)
{
    RootedShape shape(cx, obj_->lastProperty());
    RootedShape holderShape(cx, holder_->lastProperty());

    Value expandoSlot = obj_->getFixedSlot(GetListBaseExpandoSlot());
    RootedShape expandoShape(cx, NULL);
    ExpandoAndGeneration *expandoAndGeneration;
    int32_t generation;
    Value expandoVal;
    if (kind == ICStub::GetProp_CallListBaseNative) {
        expandoVal = expandoSlot;
    } else {
        JS_ASSERT(kind == ICStub::GetProp_CallListBaseWithGenerationNative);
        JS_ASSERT(!expandoSlot.isObject() && !expandoSlot.isUndefined());
        expandoAndGeneration = (ExpandoAndGeneration*)expandoSlot.toPrivate();
        expandoVal = expandoAndGeneration->expando;
        generation = expandoAndGeneration->generation;
    }

    if (expandoVal.isObject())
        expandoShape = expandoVal.toObject().lastProperty();

    if (kind == ICStub::GetProp_CallListBaseNative) {
        return ICGetProp_CallListBaseNative::New(
            space, getStubCode(), firstMonitorStub_, shape, GetProxyHandler(obj_),
            expandoShape, holder_, holderShape, getter_, pcOffset_);
    }

    return ICGetProp_CallListBaseWithGenerationNative::New(
        space, getStubCode(), firstMonitorStub_, shape, GetProxyHandler(obj_),
        expandoAndGeneration, generation, expandoShape, holder_, holderShape, getter_,
        pcOffset_);
}

ICStub *
ICGetProp_ListBaseShadowed::Compiler::getStub(ICStubSpace *space)
{
    RootedShape shape(cx, obj_->lastProperty());
    return ICGetProp_ListBaseShadowed::New(space, getStubCode(), firstMonitorStub_,
                                           shape, GetProxyHandler(obj_), name_, pcOffset_);
}

static bool
ProxyGet(JSContext *cx, HandleObject proxy, HandlePropertyName name, MutableHandleValue vp)
{
    RootedId id(cx, NameToId(name));
    return Proxy::get(cx, proxy, proxy, id, vp);
}

typedef bool (*ProxyGetFn)(JSContext *cx, HandleObject proxy, HandlePropertyName name,
                           MutableHandleValue vp);
static const VMFunction ProxyGetInfo = FunctionInfo<ProxyGetFn>(ProxyGet);

bool
ICGetProp_ListBaseShadowed::Compiler::generateStubCode(MacroAssembler &masm)
{
    Label failure;

    GeneralRegisterSet regs(availableGeneralRegs(1));
    
    
    
    Register scratch = regs.takeAnyExcluding(BaselineTailCallReg);

    
    masm.branchTestObject(Assembler::NotEqual, R0, &failure);

    
    Register objReg = masm.extractObject(R0, ExtractTemp0);

    
    masm.loadPtr(Address(BaselineStubReg, ICGetProp_ListBaseShadowed::offsetOfShape()), scratch);
    masm.branchTestObjShape(Assembler::NotEqual, objReg, scratch, &failure);

    
    {
        GeneralRegisterSet listBaseRegSet(GeneralRegisterSet::All());
        listBaseRegSet.take(BaselineStubReg);
        listBaseRegSet.take(objReg);
        listBaseRegSet.take(scratch);
        GenerateListBaseChecks(
                cx, masm, objReg,
                Address(BaselineStubReg, ICGetProp_ListBaseShadowed::offsetOfProxyHandler()),
                NULL, NULL, NULL,
                scratch,
                listBaseRegSet,
                &failure);
    }

    

    
    enterStubFrame(masm, scratch);

    
    masm.loadPtr(Address(BaselineStubReg, ICGetProp_ListBaseShadowed::offsetOfName()), scratch);
    masm.push(scratch);
    masm.push(objReg);

    
    regs.add(R0);

    
    {
        Label skipProfilerUpdate;
        Register scratch = regs.takeAny();
        Register pcIdx = regs.takeAny();

        
        guardProfilingEnabled(masm, scratch, &skipProfilerUpdate);

        
        masm.load32(Address(BaselineStubReg, ICGetProp_ListBaseShadowed::offsetOfPCOffset()), pcIdx);
        masm.spsUpdatePCIdx(&cx->runtime->spsProfiler, pcIdx, scratch);

        masm.bind(&skipProfilerUpdate);
        regs.add(scratch);
        regs.add(pcIdx);
    }
    if (!callVM(ProxyGetInfo, masm))
        return false;
    leaveStubFrame(masm);

    
    EmitEnterTypeMonitorIC(masm);

    
    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}

bool
ICGetProp_ArgumentsLength::Compiler::generateStubCode(MacroAssembler &masm)
{
    Label failure;
    if (which_ == ICGetProp_ArgumentsLength::Magic) {
        
        masm.branchTestMagicValue(Assembler::NotEqual, R0, JS_OPTIMIZED_ARGUMENTS, &failure);

        
        masm.branchTest32(Assembler::NonZero,
                          Address(BaselineFrameReg, BaselineFrame::reverseOffsetOfFlags()),
                          Imm32(BaselineFrame::HAS_ARGS_OBJ),
                          &failure);

        Address actualArgs(BaselineFrameReg, BaselineFrame::offsetOfNumActualArgs());
        masm.loadPtr(actualArgs, R0.scratchReg());
        masm.tagValue(JSVAL_TYPE_INT32, R0.scratchReg(), R0);
        EmitReturnFromIC(masm);

        masm.bind(&failure);
        EmitStubGuardFailure(masm);
        return true;
    }
    JS_ASSERT(which_ == ICGetProp_ArgumentsLength::Strict ||
              which_ == ICGetProp_ArgumentsLength::Normal);

    bool isStrict = which_ == ICGetProp_ArgumentsLength::Strict;
    Class *clasp = isStrict ? &StrictArgumentsObjectClass : &NormalArgumentsObjectClass;

    Register scratchReg = R1.scratchReg();

    
    masm.branchTestObject(Assembler::NotEqual, R0, &failure);
    Register objReg = masm.extractObject(R0, ExtractTemp0);
    masm.branchTestObjClass(Assembler::NotEqual, objReg, scratchReg, clasp, &failure);

    
    masm.unboxInt32(Address(objReg, ArgumentsObject::getInitialLengthSlotOffset()), scratchReg);

    
    masm.branchTest32(Assembler::NonZero,
                      scratchReg,
                      Imm32(ArgumentsObject::LENGTH_OVERRIDDEN_BIT),
                      &failure);

    
    
    masm.rshiftPtr(Imm32(ArgumentsObject::PACKED_BITS_COUNT), scratchReg);
    masm.tagValue(JSVAL_TYPE_INT32, scratchReg, R0);
    EmitReturnFromIC(masm);

    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}

void
BaselineScript::noteAccessedGetter(uint32_t pcOffset)
{
    ICEntry &entry = icEntryFromPCOffset(pcOffset);
    ICFallbackStub *stub = entry.fallbackStub();

    if (stub->isGetProp_Fallback())
        stub->toGetProp_Fallback()->noteAccessedGetter();
}






static bool
TryAttachSetPropStub(JSContext *cx, HandleScript script, jsbytecode *pc, ICSetProp_Fallback *stub,
                     HandleObject obj, HandleShape oldShape, uint32_t oldSlots,
                     HandlePropertyName name, HandleId id, HandleValue rhs, bool *attached)
{
    JS_ASSERT(!*attached);

    if (!obj->isNative() || obj->watched())
        return true;

    RootedShape shape(cx);
    RootedObject holder(cx);
    if (!EffectlesslyLookupProperty(cx, obj, name, &holder, &shape))
        return false;

    size_t chainDepth;
    if (IsCacheableSetPropAddSlot(cx, obj, oldShape, oldSlots, id, holder, shape, &chainDepth)) {
        
        if (chainDepth > ICSetProp_NativeAdd::MAX_PROTO_CHAIN_DEPTH)
            return true;

        bool isFixedSlot;
        uint32_t offset;
        GetFixedOrDynamicSlotOffset(obj, shape->slot(), &isFixedSlot, &offset);

        IonSpew(IonSpew_BaselineIC, "  Generating SetProp(NativeObject.ADD) stub");
        ICSetPropNativeAddCompiler compiler(cx, obj, oldShape, chainDepth, isFixedSlot, offset);
        ICUpdatedStub *newStub = compiler.getStub(compiler.getStubSpace(script));
        if (!newStub)
            return false;
        if (!newStub->addUpdateStubForValue(cx, script, obj, id, rhs))
            return false;

        stub->addNewStub(newStub);
        *attached = true;
        return true;
    }

    if (IsCacheableSetPropWriteSlot(obj, oldShape, holder, shape)) {
        bool isFixedSlot;
        uint32_t offset;
        GetFixedOrDynamicSlotOffset(obj, shape->slot(), &isFixedSlot, &offset);

        IonSpew(IonSpew_BaselineIC, "  Generating SetProp(NativeObject.PROP) stub");
        ICSetProp_Native::Compiler compiler(cx, obj, isFixedSlot, offset);
        ICUpdatedStub *newStub = compiler.getStub(compiler.getStubSpace(script));
        if (!newStub)
            return false;
        if (!newStub->addUpdateStubForValue(cx, script, obj, id, rhs))
            return false;

        stub->addNewStub(newStub);
        *attached = true;
        return true;
    }

    bool isScripted = false;
    bool cacheableCall = IsCacheableSetPropCall(obj, holder, shape, &isScripted);

    
    if (cacheableCall && isScripted) {
        RootedFunction callee(cx, shape->setterObject()->toFunction());
        JS_ASSERT(obj != holder);
        JS_ASSERT(callee->hasScript());

        IonSpew(IonSpew_BaselineIC, "  Generating SetProp(NativeObj/ScriptedSetter %s:%d) stub",
                    callee->nonLazyScript()->filename(), callee->nonLazyScript()->lineno);

        ICSetProp_CallScripted::Compiler compiler(cx, obj, holder, callee, pc - script->code);
        ICStub *newStub = compiler.getStub(compiler.getStubSpace(script));
        if (!newStub)
            return false;

        stub->addNewStub(newStub);
        *attached = true;
        return true;
    }

    
    if (cacheableCall && !isScripted) {
        RootedFunction callee(cx, shape->setterObject()->toFunction());
        JS_ASSERT(obj != holder);
        JS_ASSERT(callee->isNative());

        IonSpew(IonSpew_BaselineIC, "  Generating SetProp(NativeObj/NativeSetter %p) stub",
                    callee->native());

        ICSetProp_CallNative::Compiler compiler(cx, obj, holder, callee, pc - script->code);
        ICStub *newStub = compiler.getStub(compiler.getStubSpace(script));
        if (!newStub)
            return false;

        stub->addNewStub(newStub);
        *attached = true;
        return true;
    }

    return true;
}

static bool
DoSetPropFallback(JSContext *cx, BaselineFrame *frame, ICSetProp_Fallback *stub, HandleValue lhs,
                  HandleValue rhs, MutableHandleValue res)
{
    RootedScript script(cx, frame->script());
    jsbytecode *pc = stub->icEntry()->pc(script);
    JSOp op = JSOp(*pc);
    FallbackICSpew(cx, stub, "SetProp(%s)", js_CodeName[op]);

    JS_ASSERT(op == JSOP_SETPROP || op == JSOP_SETNAME || op == JSOP_SETGNAME || op == JSOP_INITPROP);

    RootedPropertyName name(cx, script->getName(pc));
    RootedId id(cx, NameToId(name));

    RootedObject obj(cx, ToObjectFromStack(cx, lhs));
    if (!obj)
        return false;
    RootedShape oldShape(cx, obj->lastProperty());
    uint32_t oldSlots = obj->numDynamicSlots();

    if (op == JSOP_INITPROP && name != cx->names().proto) {
        JS_ASSERT(obj->isObject());
        if (!DefineNativeProperty(cx, obj, id, rhs, NULL, NULL, JSPROP_ENUMERATE, 0, 0, 0))
            return false;
    } else if (op == JSOP_SETNAME || op == JSOP_SETGNAME) {
        if (!SetNameOperation(cx, script, pc, obj, rhs))
            return false;
    } else if (script->strict) {
        if (!js::SetProperty<true>(cx, obj, id, rhs))
            return false;
    } else {
        if (!js::SetProperty<false>(cx, obj, id, rhs))
            return false;
    }

    
    res.set(rhs);

    if (stub->numOptimizedStubs() >= ICSetProp_Fallback::MAX_OPTIMIZED_STUBS) {
        
        return true;
    }

    bool attached = false;
    if (!TryAttachSetPropStub(cx, script, pc, stub, obj, oldShape, oldSlots, name, id, rhs,
         &attached))
    {
        return false;
    }
    if (attached)
        return true;

    JS_ASSERT(!attached);
    stub->noteUnoptimizableAccess();

    return true;
}

typedef bool (*DoSetPropFallbackFn)(JSContext *, BaselineFrame *, ICSetProp_Fallback *,
                                    HandleValue, HandleValue, MutableHandleValue);
static const VMFunction DoSetPropFallbackInfo =
    FunctionInfo<DoSetPropFallbackFn>(DoSetPropFallback, PopValues(2));

bool
ICSetProp_Fallback::Compiler::generateStubCode(MacroAssembler &masm)
{
    JS_ASSERT(R0 == JSReturnOperand);

    EmitRestoreTailCallReg(masm);

    
    masm.pushValue(R0);
    masm.pushValue(R1);

    
    masm.pushValue(R1);
    masm.pushValue(R0);
    masm.push(BaselineStubReg);
    masm.pushBaselineFramePtr(BaselineFrameReg, R0.scratchReg());

    return tailCallVM(DoSetPropFallbackInfo, masm);
}

bool
ICSetProp_Native::Compiler::generateStubCode(MacroAssembler &masm)
{
    Label failure;

    
    masm.branchTestObject(Assembler::NotEqual, R0, &failure);

    GeneralRegisterSet regs(availableGeneralRegs(2));
    Register scratch = regs.takeAny();

    
    Register objReg = masm.extractObject(R0, ExtractTemp0);
    masm.loadPtr(Address(BaselineStubReg, ICSetProp_Native::offsetOfShape()), scratch);
    masm.branchTestObjShape(Assembler::NotEqual, objReg, scratch, &failure);

    
    masm.loadPtr(Address(BaselineStubReg, ICSetProp_Native::offsetOfType()), scratch);
    masm.branchPtr(Assembler::NotEqual, Address(objReg, JSObject::offsetOfType()), scratch,
                   &failure);

    
    EmitStowICValues(masm, 2);

    
    masm.moveValue(R1, R0);

    
    if (!callTypeUpdateIC(masm, sizeof(Value)))
        return false;

    
    EmitUnstowICValues(masm, 2);

    if (!isFixedSlot_)
        masm.loadPtr(Address(objReg, JSObject::offsetOfSlots()), objReg);

    
    masm.load32(Address(BaselineStubReg, ICSetProp_Native::offsetOfOffset()), scratch);
    EmitPreBarrier(masm, BaseIndex(objReg, scratch, TimesOne), MIRType_Value);
    masm.storeValue(R1, BaseIndex(objReg, scratch, TimesOne));

    
    masm.moveValue(R1, R0);
    EmitReturnFromIC(masm);

    
    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}

ICUpdatedStub *
ICSetPropNativeAddCompiler::getStub(ICStubSpace *space)
{
    AutoShapeVector shapes(cx);
    if (!shapes.append(oldShape_))
        return NULL;

    if (!GetProtoShapes(obj_, protoChainDepth_, &shapes))
        return NULL;

    JS_STATIC_ASSERT(ICSetProp_NativeAdd::MAX_PROTO_CHAIN_DEPTH == 4);

    ICUpdatedStub *stub = NULL;
    switch(protoChainDepth_) {
      case 0: stub = getStubSpecific<0>(space, &shapes); break;
      case 1: stub = getStubSpecific<1>(space, &shapes); break;
      case 2: stub = getStubSpecific<2>(space, &shapes); break;
      case 3: stub = getStubSpecific<3>(space, &shapes); break;
      case 4: stub = getStubSpecific<4>(space, &shapes); break;
      default: JS_NOT_REACHED("ProtoChainDepth too high.");
    }
    if (!stub || !stub->initUpdatingChain(cx, space))
        return NULL;
    return stub;
}

bool
ICSetPropNativeAddCompiler::generateStubCode(MacroAssembler &masm)
{
    Label failure;
    Label failureUnstow;

    
    masm.branchTestObject(Assembler::NotEqual, R0, &failure);

    GeneralRegisterSet regs(availableGeneralRegs(2));
    Register scratch = regs.takeAny();

    
    Register objReg = masm.extractObject(R0, ExtractTemp0);
    masm.loadPtr(Address(BaselineStubReg, ICSetProp_NativeAddImpl<0>::offsetOfShape(0)), scratch);
    masm.branchTestObjShape(Assembler::NotEqual, objReg, scratch, &failure);

    
    masm.loadPtr(Address(BaselineStubReg, ICSetProp_NativeAdd::offsetOfType()), scratch);
    masm.branchPtr(Assembler::NotEqual, Address(objReg, JSObject::offsetOfType()), scratch,
                   &failure);

    
    EmitStowICValues(masm, 2);

    regs = availableGeneralRegs(1);
    scratch = regs.takeAny();
    Register protoReg = regs.takeAny();
    
    for (size_t i = 0; i < protoChainDepth_; i++) {
        masm.loadObjProto(i == 0 ? objReg : protoReg, protoReg);
        masm.loadPtr(Address(BaselineStubReg, ICSetProp_NativeAddImpl<0>::offsetOfShape(i + 1)),
                     scratch);
        masm.branchTestObjShape(Assembler::NotEqual, protoReg, scratch, &failureUnstow);
    }

    

    
    
    masm.loadValue(Address(BaselineStackReg, ICStackValueOffset), R0);

    
    if (!callTypeUpdateIC(masm, sizeof(Value)))
        return false;

    
    EmitUnstowICValues(masm, 2);
    regs = availableGeneralRegs(2);
    scratch = regs.takeAny();

    
    Address shapeAddr(objReg, JSObject::offsetOfShape());
    EmitPreBarrier(masm, shapeAddr, MIRType_Shape);
    masm.loadPtr(Address(BaselineStubReg, ICSetProp_NativeAdd::offsetOfNewShape()), scratch);
    masm.storePtr(scratch, shapeAddr);

    if (!isFixedSlot_)
        masm.loadPtr(Address(objReg, JSObject::offsetOfSlots()), objReg);

    
    
    masm.load32(Address(BaselineStubReg, ICSetProp_NativeAdd::offsetOfOffset()), scratch);
    masm.storeValue(R1, BaseIndex(objReg, scratch, TimesOne));

    
    masm.moveValue(R1, R0);
    EmitReturnFromIC(masm);

    
    masm.bind(&failureUnstow);
    EmitUnstowICValues(masm, 2);

    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}

bool
ICSetProp_CallScripted::Compiler::generateStubCode(MacroAssembler &masm)
{
    Label failure;
    Label failureUnstow;
    Label failureLeaveStubFrame;

    
    masm.branchTestObject(Assembler::NotEqual, R0, &failure);

    
    EmitStowICValues(masm, 2);

    GeneralRegisterSet regs(availableGeneralRegs(1));
    Register scratch = regs.takeAnyExcluding(BaselineTailCallReg);

    
    Register objReg = masm.extractObject(R0, ExtractTemp0);
    masm.loadPtr(Address(BaselineStubReg, ICSetProp_CallScripted::offsetOfShape()), scratch);
    masm.branchTestObjShape(Assembler::NotEqual, objReg, scratch, &failureUnstow);

    Register holderReg = regs.takeAny();
    masm.loadPtr(Address(BaselineStubReg, ICSetProp_CallScripted::offsetOfHolder()), holderReg);
    masm.loadPtr(Address(BaselineStubReg, ICSetProp_CallScripted::offsetOfHolderShape()), scratch);
    masm.branchTestObjShape(Assembler::NotEqual, holderReg, scratch, &failureUnstow);
    regs.add(holderReg);

    
    enterStubFrame(masm, scratch);

    
    
    Register callee;
    if (regs.has(ArgumentsRectifierReg)) {
        callee = ArgumentsRectifierReg;
        regs.take(callee);
    } else {
        callee = regs.takeAny();
    }
    Register code = regs.takeAny();
    masm.loadPtr(Address(BaselineStubReg, ICSetProp_CallScripted::offsetOfSetter()), callee);
    masm.loadPtr(Address(callee, JSFunction::offsetOfNativeOrScript()), code);
    masm.loadBaselineOrIonRaw(code, code, SequentialExecution, &failureLeaveStubFrame);

    
    
    

    
    
    masm.movePtr(BaselineStackReg, scratch);
    masm.PushValue(Address(scratch, STUB_FRAME_SIZE));
    masm.Push(R0);
    EmitCreateStubFrameDescriptor(masm, scratch);
    masm.Push(Imm32(1));  
    masm.Push(callee);
    masm.Push(scratch);

    
    Label noUnderflow;
    masm.load16ZeroExtend(Address(callee, offsetof(JSFunction, nargs)), scratch);
    masm.branch32(Assembler::BelowOrEqual, scratch, Imm32(1), &noUnderflow);
    {
        
        JS_ASSERT(ArgumentsRectifierReg != code);

        IonCode *argumentsRectifier =
            cx->compartment->ionCompartment()->getArgumentsRectifier(SequentialExecution);

        masm.movePtr(ImmGCPtr(argumentsRectifier), code);
        masm.loadPtr(Address(code, IonCode::offsetOfCode()), code);
        masm.mov(Imm32(1), ArgumentsRectifierReg);
    }

    masm.bind(&noUnderflow);

    
    
    {
        Label skipProfilerUpdate;

        
        GeneralRegisterSet availRegs = availableGeneralRegs(0);
        availRegs.take(ArgumentsRectifierReg);
        availRegs.take(code);
        Register scratch = availRegs.takeAny();
        Register pcIdx = availRegs.takeAny();

        
        guardProfilingEnabled(masm, scratch, &skipProfilerUpdate);

        
        masm.load32(Address(BaselineStubReg, ICSetProp_CallScripted::offsetOfPCOffset()), pcIdx);
        masm.spsUpdatePCIdx(&cx->runtime->spsProfiler, pcIdx, scratch);

        masm.bind(&skipProfilerUpdate);
    }
    masm.callIon(code);

    leaveStubFrame(masm, true);
    
    
    EmitUnstowICValues(masm, 2);
    masm.moveValue(R1, R0);
    EmitReturnFromIC(masm);

    
    masm.bind(&failureLeaveStubFrame);
    leaveStubFrame(masm, false);

    
    masm.bind(&failureUnstow);
    EmitUnstowICValues(masm, 2);

    
    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}

static bool
DoCallNativeSetter(JSContext *cx, HandleFunction callee, HandleObject obj, HandleValue val)
{
    JS_ASSERT(callee->isNative());
    JSNative natfun = callee->native();

    Value vp[3] = { ObjectValue(*callee.get()), ObjectValue(*obj.get()), val };
    AutoValueArray rootVp(cx, vp, 3);

    return natfun(cx, 1, vp);
}

typedef bool (*DoCallNativeSetterFn)(JSContext *, HandleFunction, HandleObject, HandleValue);
static const VMFunction DoCallNativeSetterInfo =
    FunctionInfo<DoCallNativeSetterFn>(DoCallNativeSetter);

bool
ICSetProp_CallNative::Compiler::generateStubCode(MacroAssembler &masm)
{
    Label failure;
    Label failureUnstow;

    
    masm.branchTestObject(Assembler::NotEqual, R0, &failure);

    
    EmitStowICValues(masm, 2);

    GeneralRegisterSet regs(availableGeneralRegs(1));
    Register scratch = regs.takeAnyExcluding(BaselineTailCallReg);

    
    Register objReg = masm.extractObject(R0, ExtractTemp0);
    masm.loadPtr(Address(BaselineStubReg, ICSetProp_CallNative::offsetOfShape()), scratch);
    masm.branchTestObjShape(Assembler::NotEqual, objReg, scratch, &failureUnstow);

    Register holderReg = regs.takeAny();
    masm.loadPtr(Address(BaselineStubReg, ICSetProp_CallNative::offsetOfHolder()), holderReg);
    masm.loadPtr(Address(BaselineStubReg, ICSetProp_CallNative::offsetOfHolderShape()), scratch);
    masm.branchTestObjShape(Assembler::NotEqual, holderReg, scratch, &failureUnstow);
    regs.add(holderReg);

    
    enterStubFrame(masm, scratch);

    
    
    Register callee = regs.takeAny();
    masm.loadPtr(Address(BaselineStubReg, ICSetProp_CallNative::offsetOfSetter()), callee);

    
    
    masm.movePtr(BaselineStackReg, scratch);
    masm.pushValue(Address(scratch, STUB_FRAME_SIZE));
    masm.push(objReg);
    masm.push(callee);

    
    regs.add(R0);

    
    {
        Label skipProfilerUpdate;
        Register scratch = regs.takeAny();
        Register pcIdx = regs.takeAny();

        
        guardProfilingEnabled(masm, scratch, &skipProfilerUpdate);

        
        masm.load32(Address(BaselineStubReg, ICSetProp_CallNative::offsetOfPCOffset()), pcIdx);
        masm.spsUpdatePCIdx(&cx->runtime->spsProfiler, pcIdx, scratch);

        masm.bind(&skipProfilerUpdate);
        regs.add(scratch);
        regs.add(pcIdx);
    }
    if (!callVM(DoCallNativeSetterInfo, masm))
        return false;
    leaveStubFrame(masm);

    
    
    EmitUnstowICValues(masm, 2);
    masm.moveValue(R1, R0);
    EmitReturnFromIC(masm);

    
    masm.bind(&failureUnstow);
    EmitUnstowICValues(masm, 2);

    
    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}





static bool
TryAttachFunApplyStub(JSContext *cx, ICCall_Fallback *stub, HandleScript script, jsbytecode *pc,
                      HandleValue thisv, uint32_t argc, Value *argv)
{
    if (argc != 2)
        return true;

    if (!thisv.isObject() || !thisv.toObject().isFunction())
        return true;
    RootedFunction target(cx, thisv.toObject().toFunction());

    
    if (argv[1].isMagic(JS_OPTIMIZED_ARGUMENTS) && !script->needsArgsObj()) {
        if (target->hasScript() &&
            (target->nonLazyScript()->hasBaselineScript() ||
             target->nonLazyScript()->hasIonScript()) &&
            !stub->hasStub(ICStub::Call_ScriptedApplyArguments))
        {
            IonSpew(IonSpew_BaselineIC, "  Generating Call_ScriptedApplyArguments stub");

            ICCall_ScriptedApplyArguments::Compiler compiler(
                cx, stub->fallbackMonitorStub()->firstMonitorStub(), pc - script->code);
            ICStub *newStub = compiler.getStub(compiler.getStubSpace(script));
            if (!newStub)
                return false;

            stub->addNewStub(newStub);
            return true;
        }

        
    }
    return true;
}

static bool
TryAttachCallStub(JSContext *cx, ICCall_Fallback *stub, HandleScript script, jsbytecode *pc,
                  JSOp op, uint32_t argc, Value *vp, bool constructing, bool useNewType)
{
    if (useNewType || op == JSOP_EVAL)
        return true;

    if (stub->numOptimizedStubs() >= ICCall_Fallback::MAX_OPTIMIZED_STUBS) {
        
        
        return true;
    }

    RootedValue callee(cx, vp[0]);
    RootedValue thisv(cx, vp[1]);

    if (!callee.isObject())
        return true;

    RootedObject obj(cx, &callee.toObject());
    if (!obj->isFunction())
        return true;

    RootedFunction fun(cx, obj->toFunction());
    if (fun->hasScript()) {
        RootedScript calleeScript(cx, fun->nonLazyScript());
        if (!calleeScript->hasBaselineScript() && !calleeScript->hasIonScript())
            return true;

        if (calleeScript->shouldCloneAtCallsite)
            return true;

        
        if (stub->scriptedStubsAreGeneralized()) {
            IonSpew(IonSpew_BaselineIC, "  Chain already has generalized scripted call stub!");
            return true;
        }

        if (stub->scriptedStubCount() >= ICCall_Fallback::MAX_SCRIPTED_STUBS) {
            
            IonSpew(IonSpew_BaselineIC, "  Generating Call_AnyScripted stub (cons=%s)",
                    constructing ? "yes" : "no");

            ICCallScriptedCompiler compiler(cx, stub->fallbackMonitorStub()->firstMonitorStub(),
                                            constructing, pc - script->code);
            ICStub *newStub = compiler.getStub(compiler.getStubSpace(script));
            if (!newStub)
                return false;

            
            stub->unlinkStubsWithKind(cx, ICStub::Call_Scripted);

            
            stub->addNewStub(newStub);
            return true;
        }

        IonSpew(IonSpew_BaselineIC,
                "  Generating Call_Scripted stub (fun=%p, %s:%d, cons=%s)",
                fun.get(), fun->nonLazyScript()->filename(), fun->nonLazyScript()->lineno,
                constructing ? "yes" : "no");
        ICCallScriptedCompiler compiler(cx, stub->fallbackMonitorStub()->firstMonitorStub(),
                                        calleeScript, constructing, pc - script->code);
        ICStub *newStub = compiler.getStub(compiler.getStubSpace(script));
        if (!newStub)
            return false;

        stub->addNewStub(newStub);
        return true;
    }

    if (fun->isNative() && (!constructing || (constructing && fun->isNativeConstructor()))) {
        
        JS_ASSERT(!stub->nativeStubsAreGeneralized());

        
        if (op == JSOP_FUNAPPLY && fun->maybeNative() == js_fun_apply) {
            if (!TryAttachFunApplyStub(cx, stub, script, pc, thisv, argc, vp + 2))
                return false;
        } else {
            if (stub->nativeStubCount() >= ICCall_Fallback::MAX_NATIVE_STUBS) {
                IonSpew(IonSpew_BaselineIC,
                        "  Too many Call_Native stubs. TODO: add Call_AnyNative!");
                return true;
            }

            IonSpew(IonSpew_BaselineIC, "  Generating Call_Native stub (fun=%p, cons=%s)",
                    fun.get(), constructing ? "yes" : "no");
            ICCall_Native::Compiler compiler(cx, stub->fallbackMonitorStub()->firstMonitorStub(),
                                             fun, constructing, pc - script->code);
            ICStub *newStub = compiler.getStub(compiler.getStubSpace(script));
            if (!newStub)
                return false;

            stub->addNewStub(newStub);
            return true;
        }
    }

    return true;
}

static bool
MaybeCloneFunctionAtCallsite(JSContext *cx, MutableHandleValue callee, HandleScript script,
                             jsbytecode *pc)
{
    RootedFunction fun(cx);
    if (!IsFunctionObject(callee, fun.address()))
        return true;

    if (!fun->hasScript() || !fun->nonLazyScript()->shouldCloneAtCallsite)
        return true;

    if (!cx->typeInferenceEnabled())
        return true;

    fun = CloneFunctionAtCallsite(cx, fun, script, pc);
    if (!fun)
        return false;

    callee.setObject(*fun);
    return true;
}

static bool
DoCallFallback(JSContext *cx, BaselineFrame *frame, ICCall_Fallback *stub, uint32_t argc,
               Value *vp, MutableHandleValue res)
{
    
    AutoArrayRooter vpRoot(cx, argc + 2, vp);

    RootedScript script(cx, frame->script());
    jsbytecode *pc = stub->icEntry()->pc(script);
    JSOp op = JSOp(*pc);
    FallbackICSpew(cx, stub, "Call(%s)", js_CodeName[op]);

    JS_ASSERT(argc == GET_ARGC(pc));

    RootedValue callee(cx, vp[0]);
    RootedValue thisv(cx, vp[1]);

    Value *args = vp + 2;

    
    if (op == JSOP_FUNAPPLY && argc == 2 && args[1].isMagic(JS_OPTIMIZED_ARGUMENTS)) {
        if (!GuardFunApplyArgumentsOptimization(cx, frame, callee, args, argc))
            return false;
    }

    
    bool constructing = (op == JSOP_NEW);
    bool newType = false;
    if (cx->typeInferenceEnabled())
        newType = types::UseNewType(cx, script, pc);

    
    if (!TryAttachCallStub(cx, stub, script, pc, op, argc, vp, constructing, newType))
        return false;

    
    if (cx->runtime->spsProfiler.enabled() && frame->hasPushedSPSFrame())
        cx->runtime->spsProfiler.updatePC(script, pc);

    if (!MaybeCloneFunctionAtCallsite(cx, &callee, script, pc))
        return false;

    if (op == JSOP_NEW) {
        if (!InvokeConstructor(cx, callee, argc, args, res.address()))
            return false;
    } else if (op == JSOP_EVAL && IsBuiltinEvalForScope(frame->scopeChain(), callee)) {
        if (!DirectEval(cx, CallArgsFromVp(argc, vp)))
            return false;
        res.set(vp[0]);
    } else {
        JS_ASSERT(op == JSOP_CALL || op == JSOP_FUNCALL || op == JSOP_FUNAPPLY || op == JSOP_EVAL);
        if (!Invoke(cx, thisv, callee, argc, args, res.address()))
            return false;
    }

    types::TypeScript::Monitor(cx, script, pc, res);

    
    ICTypeMonitor_Fallback *typeMonFbStub = stub->fallbackMonitorStub();
    if (!typeMonFbStub->addMonitorStubForValue(cx, script, res))
        return false;
    
    if (!stub->addMonitorStubForValue(cx, script, res))
        return false;

    return true;
}

void
ICCallStubCompiler::pushCallArguments(MacroAssembler &masm, GeneralRegisterSet regs, Register argcReg)
{
    JS_ASSERT(!regs.has(argcReg));

    
    Register count = regs.takeAny();
    masm.mov(argcReg, count);
    masm.add32(Imm32(2), count);

    
    Register argPtr = regs.takeAny();
    masm.mov(BaselineStackReg, argPtr);

    
    
    masm.addPtr(Imm32(STUB_FRAME_SIZE), argPtr);

    
    Label loop, done;
    masm.bind(&loop);
    masm.branchTest32(Assembler::Zero, count, count, &done);
    {
        masm.pushValue(Address(argPtr, 0));
        masm.addPtr(Imm32(sizeof(Value)), argPtr);

        masm.sub32(Imm32(1), count);
        masm.jump(&loop);
    }
    masm.bind(&done);
}

Register
ICCallStubCompiler::guardFunApply(MacroAssembler &masm, GeneralRegisterSet regs, Register argcReg,
                                  bool checkNative, Label *failure)
{
    
    masm.branch32(Assembler::NotEqual, argcReg, Imm32(2), failure);

    
    

    
    Address secondArgSlot(BaselineStackReg, ICStackValueOffset);
    masm.branchTestMagic(Assembler::NotEqual, secondArgSlot, failure);

    
    masm.branchTest32(Assembler::NonZero,
                      Address(BaselineFrameReg, BaselineFrame::reverseOffsetOfFlags()),
                      Imm32(BaselineFrame::HAS_ARGS_OBJ),
                      failure);

    
    

    
    ValueOperand val = regs.takeAnyValue();
    Address calleeSlot(BaselineStackReg, ICStackValueOffset + (3 * sizeof(Value)));
    masm.loadValue(calleeSlot, val);

    masm.branchTestObject(Assembler::NotEqual, val, failure);
    Register callee = masm.extractObject(val, ExtractTemp1);

    masm.branchTestObjClass(Assembler::NotEqual, callee, regs.getAny(), &FunctionClass, failure);
    masm.loadPtr(Address(callee, JSFunction::offsetOfNativeOrScript()), callee);

    masm.branchPtr(Assembler::NotEqual, callee, ImmWord((void*) js_fun_apply), failure);

    
    
    Address thisSlot(BaselineStackReg, ICStackValueOffset + (2 * sizeof(Value)));
    masm.loadValue(thisSlot, val);

    masm.branchTestObject(Assembler::NotEqual, val, failure);
    Register target = masm.extractObject(val, ExtractTemp1);
    regs.add(val);
    regs.takeUnchecked(target);

    masm.branchTestObjClass(Assembler::NotEqual, target, regs.getAny(), &FunctionClass, failure);

    if (checkNative) {
        masm.branchIfInterpreted(target, failure);
    } else {
        masm.branchIfFunctionHasNoScript(target, failure);
        Register temp = regs.takeAny();
        masm.loadPtr(Address(target, JSFunction::offsetOfNativeOrScript()), temp);
        masm.loadBaselineOrIonRaw(temp, temp, SequentialExecution, failure);
        regs.add(temp);
    }
    return target;
}

void
ICCallStubCompiler::pushCallerArguments(MacroAssembler &masm, GeneralRegisterSet regs)
{
    
    
    Register startReg = regs.takeAny();
    Register endReg = regs.takeAny();
    masm.loadPtr(Address(BaselineFrameReg, 0), startReg);
    masm.loadPtr(Address(startReg, BaselineFrame::offsetOfNumActualArgs()), endReg);
    masm.addPtr(Imm32(BaselineFrame::offsetOfArg(0)), startReg);
    JS_ASSERT(sizeof(Value) == 8);
    masm.lshiftPtr(Imm32(3), endReg);
    masm.addPtr(startReg, endReg);

    
    Label copyDone;
    Label copyStart;
    masm.bind(&copyStart);
    masm.branchPtr(Assembler::Equal, endReg, startReg, &copyDone);
    masm.subPtr(Imm32(sizeof(Value)), endReg);
    masm.pushValue(Address(endReg, 0));
    masm.jump(&copyStart);
    masm.bind(&copyDone);
}

typedef bool (*DoCallFallbackFn)(JSContext *, BaselineFrame *, ICCall_Fallback *,
                                 uint32_t, Value *, MutableHandleValue);
static const VMFunction DoCallFallbackInfo = FunctionInfo<DoCallFallbackFn>(DoCallFallback);

bool
ICCall_Fallback::Compiler::generateStubCode(MacroAssembler &masm)
{
    JS_ASSERT(R0 == JSReturnOperand);

    
    enterStubFrame(masm, R1.scratchReg());

    
    
    

    GeneralRegisterSet regs(availableGeneralRegs(0));
    regs.take(R0.scratchReg()); 

    pushCallArguments(masm, regs, R0.scratchReg());

    masm.push(BaselineStackReg);
    masm.push(R0.scratchReg());
    masm.push(BaselineStubReg);

    
    masm.loadPtr(Address(BaselineFrameReg, 0), R0.scratchReg());
    masm.pushBaselineFramePtr(R0.scratchReg(), R0.scratchReg());

    if (!callVM(DoCallFallbackInfo, masm))
        return false;

    leaveStubFrame(masm);
    EmitReturnFromIC(masm);

    
    
    
    returnOffset_ = masm.currentOffset();

    
    
    
    masm.loadValue(Address(BaselineStackReg, 3 * sizeof(size_t)), R1);

    leaveStubFrame(masm, true);

    
    regs = availableGeneralRegs(2);
    Register scratch = regs.takeAny();

    
    
    JS_ASSERT(JSReturnOperand == R0);
    Label skipThisReplace;
    masm.load16ZeroExtend(Address(BaselineStubReg, ICStub::offsetOfExtra()), scratch);
    masm.branchTest32(Assembler::Zero, scratch, Imm32(ICCall_Fallback::CONSTRUCTING_FLAG),
                      &skipThisReplace);
    masm.branchTestObject(Assembler::Equal, JSReturnOperand, &skipThisReplace);
    masm.moveValue(R1, R0);
#ifdef DEBUG
    masm.branchTestObject(Assembler::Equal, JSReturnOperand, &skipThisReplace);
    masm.breakpoint();
#endif
    masm.bind(&skipThisReplace);

    
    
    
    
    masm.loadPtr(Address(BaselineStubReg, ICMonitoredFallbackStub::offsetOfFallbackMonitorStub()),
                 BaselineStubReg);
    EmitEnterTypeMonitorIC(masm, ICTypeMonitor_Fallback::offsetOfFirstMonitorStub());

    return true;
}

bool
ICCall_Fallback::Compiler::postGenerateStubCode(MacroAssembler &masm, Handle<IonCode *> code)
{
    CodeOffsetLabel offset(returnOffset_);
    offset.fixup(&masm);
    cx->compartment->ionCompartment()->initBaselineCallReturnAddr(code->raw() + offset.offset());
    return true;
}

typedef bool (*CreateThisFn)(JSContext *cx, HandleObject callee, MutableHandleValue rval);
static const VMFunction CreateThisInfo = FunctionInfo<CreateThisFn>(CreateThis);

bool
ICCallScriptedCompiler::generateStubCode(MacroAssembler &masm)
{
    Label failure;
    GeneralRegisterSet regs(availableGeneralRegs(0));
    bool canUseTailCallReg = regs.has(BaselineTailCallReg);

    Register argcReg = R0.scratchReg();
    JS_ASSERT(argcReg != ArgumentsRectifierReg);

    regs.take(argcReg);
    regs.take(ArgumentsRectifierReg);
    if (regs.has(BaselineTailCallReg))
        regs.take(BaselineTailCallReg);

    
    
    BaseIndex calleeSlot(BaselineStackReg, argcReg, TimesEight, ICStackValueOffset + sizeof(Value));
    masm.loadValue(calleeSlot, R1);
    regs.take(R1);

    
    masm.branchTestObject(Assembler::NotEqual, R1, &failure);

    
    Register callee = masm.extractObject(R1, ExtractTemp0);
    masm.branchTestObjClass(Assembler::NotEqual, callee, regs.getAny(), &FunctionClass, &failure);

    
    
    if (calleeScript_) {
        JS_ASSERT(kind == ICStub::Call_Scripted);

        
        masm.loadPtr(Address(callee, JSFunction::offsetOfNativeOrScript()), callee);
        Address expectedScript(BaselineStubReg, ICCall_Scripted::offsetOfCalleeScript());
        masm.branchPtr(Assembler::NotEqual, expectedScript, callee, &failure);
    } else {
        masm.branchIfFunctionHasNoScript(callee, &failure);
        masm.loadPtr(Address(callee, JSFunction::offsetOfNativeOrScript()), callee);
    }

    
    Register code;
    if (!isConstructing_) {
        code = regs.takeAny();
        masm.loadBaselineOrIonRaw(callee, code, SequentialExecution, &failure);
    } else {
        Address scriptCode(callee, JSScript::offsetOfBaselineOrIonRaw());
        masm.branchPtr(Assembler::Equal, scriptCode, ImmWord((void *)NULL), &failure);
    }

    
    regs.add(R1);

    
    enterStubFrame(masm, regs.getAny());
    if (canUseTailCallReg)
        regs.add(BaselineTailCallReg);

    Label failureLeaveStubFrame;

    if (isConstructing_) {
        
        masm.push(argcReg);

        
        
        BaseIndex calleeSlot2(BaselineStackReg, argcReg, TimesEight,
                               sizeof(Value) + STUB_FRAME_SIZE + sizeof(size_t));
        masm.loadValue(calleeSlot2, R1);
        masm.push(masm.extractObject(R1, ExtractTemp0));
        if (!callVM(CreateThisInfo, masm))
            return false;

        
#ifdef DEBUG
        Label createdThisIsObject;
        masm.branchTestObject(Assembler::Equal, JSReturnOperand, &createdThisIsObject);
        masm.breakpoint();
        masm.bind(&createdThisIsObject);
#endif

        
        JS_ASSERT(JSReturnOperand == R0);
        regs = availableGeneralRegs(0);
        regs.take(R0);
        regs.take(ArgumentsRectifierReg);
        argcReg = regs.takeAny();

        
        
        masm.pop(argcReg);

        
        
        
        BaseIndex thisSlot(BaselineStackReg, argcReg, TimesEight, STUB_FRAME_SIZE);
        masm.storeValue(R0, thisSlot);

        
        masm.loadPtr(Address(BaselineStackReg, STUB_FRAME_SAVED_STUB_OFFSET), BaselineStubReg);

        
        
        
        

        
        BaseIndex calleeSlot3(BaselineStackReg, argcReg, TimesEight,
                               sizeof(Value) + STUB_FRAME_SIZE);
        masm.loadValue(calleeSlot3, R0);
        callee = masm.extractObject(R0, ExtractTemp0);
        regs.add(R0);
        regs.takeUnchecked(callee);
        masm.loadPtr(Address(callee, JSFunction::offsetOfNativeOrScript()), callee);

        code = regs.takeAny();
        masm.loadBaselineOrIonRaw(callee, code, SequentialExecution, &failureLeaveStubFrame);

        
        
        
        if (callee != ExtractTemp0)
            regs.add(callee);

        if (canUseTailCallReg)
            regs.addUnchecked(BaselineTailCallReg);
    }
    Register scratch = regs.takeAny();

    
    
    
    pushCallArguments(masm, regs, argcReg);

    
    ValueOperand val = regs.takeAnyValue();
    masm.popValue(val);
    callee = masm.extractObject(val, ExtractTemp0);

    EmitCreateStubFrameDescriptor(masm, scratch);

    
    
    masm.Push(argcReg);
    masm.Push(callee);
    masm.Push(scratch);

    
    Label noUnderflow;
    masm.load16ZeroExtend(Address(callee, offsetof(JSFunction, nargs)), callee);
    masm.branch32(Assembler::AboveOrEqual, argcReg, callee, &noUnderflow);
    {
        
        JS_ASSERT(ArgumentsRectifierReg != code);
        JS_ASSERT(ArgumentsRectifierReg != argcReg);

        IonCode *argumentsRectifier =
            cx->compartment->ionCompartment()->getArgumentsRectifier(SequentialExecution);

        masm.movePtr(ImmGCPtr(argumentsRectifier), code);
        masm.loadPtr(Address(code, IonCode::offsetOfCode()), code);
        masm.mov(argcReg, ArgumentsRectifierReg);
    }

    masm.bind(&noUnderflow);

    
    {
        Label skipProfilerUpdate;

        
        GeneralRegisterSet availRegs = availableGeneralRegs(0);
        availRegs.take(ArgumentsRectifierReg);
        availRegs.take(code);
        Register scratch = availRegs.takeAny();
        Register pcIdx = availRegs.takeAny();

        
        guardProfilingEnabled(masm, scratch, &skipProfilerUpdate);

        
        JS_ASSERT(kind == ICStub::Call_Scripted || kind == ICStub::Call_AnyScripted);
        if (kind == ICStub::Call_Scripted)
            masm.load32(Address(BaselineStubReg, ICCall_Scripted::offsetOfPCOffset()), pcIdx);
        else
            masm.load32(Address(BaselineStubReg, ICCall_AnyScripted::offsetOfPCOffset()), pcIdx);
        masm.spsUpdatePCIdx(&cx->runtime->spsProfiler, pcIdx, scratch);

        masm.bind(&skipProfilerUpdate);
    }

    masm.callIon(code);

    
    
    if (isConstructing_) {
        Label skipThisReplace;
        masm.branchTestObject(Assembler::Equal, JSReturnOperand, &skipThisReplace);

        Register scratchReg = JSReturnOperand.scratchReg();

        
        
        
        
        
        masm.loadPtr(Address(BaselineStackReg, 2*sizeof(size_t)), scratchReg);

        
        
        
        
        masm.lshiftPtr(Imm32(1), scratchReg);
        BaseIndex reloadThisSlot(BaselineStackReg, scratchReg, TimesEight,
                                 STUB_FRAME_SIZE + sizeof(Value) + 3*sizeof(size_t));
        masm.loadValue(reloadThisSlot, JSReturnOperand);
#ifdef DEBUG
        masm.branchTestObject(Assembler::Equal, JSReturnOperand, &skipThisReplace);
        masm.breakpoint();
#endif
        masm.bind(&skipThisReplace);
    }

    leaveStubFrame(masm, true);

    
    EmitEnterTypeMonitorIC(masm);

    
    masm.bind(&failureLeaveStubFrame);
    leaveStubFrame(masm, false);
    if (argcReg != R0.scratchReg())
        masm.mov(argcReg, R0.scratchReg());

    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}

bool
ICCall_Native::Compiler::generateStubCode(MacroAssembler &masm)
{
    Label failure;
    GeneralRegisterSet regs(availableGeneralRegs(0));

    Register argcReg = R0.scratchReg();
    regs.take(argcReg);
    regs.takeUnchecked(BaselineTailCallReg);

    
    BaseIndex calleeSlot(BaselineStackReg, argcReg, TimesEight, ICStackValueOffset + sizeof(Value));
    masm.loadValue(calleeSlot, R1);
    regs.take(R1);

    masm.branchTestObject(Assembler::NotEqual, R1, &failure);

    
    Register callee = masm.extractObject(R1, ExtractTemp0);
    Address expectedCallee(BaselineStubReg, ICCall_Native::offsetOfCallee());
    masm.branchPtr(Assembler::NotEqual, expectedCallee, callee, &failure);

    regs.add(R1);
    regs.takeUnchecked(callee);

    
    
    enterStubFrame(masm, regs.getAny());

    
    
    
    pushCallArguments(masm, regs, argcReg);

    if (isConstructing_) {
        
        
        masm.storeValue(MagicValue(JS_IS_CONSTRUCTING), Address(BaselineStackReg, sizeof(Value)));
    }

    masm.checkStackAlignment();

    
    
    
    
    
    

    
    Register vpReg = regs.takeAny();
    masm.movePtr(StackPointer, vpReg);

    
    masm.push(argcReg);

    Register scratch = regs.takeAny();
    EmitCreateStubFrameDescriptor(masm, scratch);
    masm.push(scratch);
    masm.push(BaselineTailCallReg);
    masm.enterFakeExitFrame();

    
    
    {
        Label skipProfilerUpdate;
        Register pcIdx = BaselineTailCallReg;
        guardProfilingEnabled(masm, scratch, &skipProfilerUpdate);

        masm.load32(Address(BaselineStubReg, ICCall_Native::offsetOfPCOffset()), pcIdx);
        masm.spsUpdatePCIdx(&cx->runtime->spsProfiler, pcIdx, scratch);

        masm.bind(&skipProfilerUpdate);
    }
    
    masm.setupUnalignedABICall(3, scratch);
    masm.loadJSContext(scratch);
    masm.passABIArg(scratch);
    masm.passABIArg(argcReg);
    masm.passABIArg(vpReg);
    masm.callWithABI(Address(callee, JSFunction::offsetOfNativeOrScript()));

    
    Label success, exception;
    masm.branchTest32(Assembler::Zero, ReturnReg, ReturnReg, &exception);

    
    masm.loadValue(Address(StackPointer, IonNativeExitFrameLayout::offsetOfResult()), R0);

    leaveStubFrame(masm);

    
    EmitEnterTypeMonitorIC(masm);

    
    masm.bind(&exception);
    masm.handleException();

    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}

bool
ICCall_ScriptedApplyArguments::Compiler::generateStubCode(MacroAssembler &masm)
{
    Label failure;
    GeneralRegisterSet regs(availableGeneralRegs(0));

    Register argcReg = R0.scratchReg();
    regs.take(argcReg);
    regs.takeUnchecked(BaselineTailCallReg);
    regs.takeUnchecked(ArgumentsRectifierReg);

    
    
    

    Register target = guardFunApply(masm, regs, argcReg, false, &failure);
    if (regs.has(target)) {
        regs.take(target);
    } else {
        
        
        Register targetTemp = regs.takeAny();
        masm.movePtr(target, targetTemp);
        target = targetTemp;
    }

    
    enterStubFrame(masm, regs.getAny());

    
    
    

    
    

    
    pushCallerArguments(masm, regs);

    
    
    
    
    
    

    
    masm.pushValue(Address(BaselineFrameReg, STUB_FRAME_SIZE + sizeof(Value)));

    
    
    Register scratch = regs.takeAny();
    EmitCreateStubFrameDescriptor(masm, scratch);

    masm.loadPtr(Address(BaselineFrameReg, 0), argcReg);
    masm.loadPtr(Address(argcReg, BaselineFrame::offsetOfNumActualArgs()), argcReg);
    masm.Push(argcReg);
    masm.Push(target);
    masm.Push(scratch);

    
    masm.load16ZeroExtend(Address(target, offsetof(JSFunction, nargs)), scratch);
    masm.loadPtr(Address(target, JSFunction::offsetOfNativeOrScript()), target);
    masm.loadBaselineOrIonRaw(target, target, SequentialExecution, NULL);

    
    Label noUnderflow;
    masm.branch32(Assembler::AboveOrEqual, argcReg, scratch, &noUnderflow);
    {
        
        JS_ASSERT(ArgumentsRectifierReg != target);
        JS_ASSERT(ArgumentsRectifierReg != argcReg);

        IonCode *argumentsRectifier =
            cx->compartment->ionCompartment()->getArgumentsRectifier(SequentialExecution);

        masm.movePtr(ImmGCPtr(argumentsRectifier), target);
        masm.loadPtr(Address(target, IonCode::offsetOfCode()), target);
        masm.mov(argcReg, ArgumentsRectifierReg);
    }
    masm.bind(&noUnderflow);
    regs.add(argcReg);

    
    
    {
        Label skipProfilerUpdate;
        Register pcIdx = regs.getAny();
        JS_ASSERT(pcIdx != ArgumentsRectifierReg);
        JS_ASSERT(pcIdx != target);
        guardProfilingEnabled(masm, scratch, &skipProfilerUpdate);

        masm.load32(Address(BaselineStubReg, ICCall_ScriptedApplyArguments::offsetOfPCOffset()),
                    pcIdx);
        masm.spsUpdatePCIdx(&cx->runtime->spsProfiler, pcIdx, scratch);

        masm.bind(&skipProfilerUpdate);
    }
    
    masm.callIon(target);
    leaveStubFrame(masm, true);

    
    EmitEnterTypeMonitorIC(masm);

    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}

static JSBool
DoubleValueToInt32ForSwitch(Value *v)
{
    double d = v->toDouble();
    int32_t truncated = int32_t(d);
    if (d != double(truncated))
        return false;

    v->setInt32(truncated);
    return true;
}

bool
ICTableSwitch::Compiler::generateStubCode(MacroAssembler &masm)
{
    Label isInt32, notInt32, outOfRange;
    Register scratch = R1.scratchReg();

    masm.branchTestInt32(Assembler::NotEqual, R0, &notInt32);

    Register key = masm.extractInt32(R0, ExtractTemp0);

    masm.bind(&isInt32);

    masm.load32(Address(BaselineStubReg, offsetof(ICTableSwitch, min_)), scratch);
    masm.sub32(scratch, key);
    masm.branch32(Assembler::BelowOrEqual,
                  Address(BaselineStubReg, offsetof(ICTableSwitch, length_)), key, &outOfRange);

    masm.loadPtr(Address(BaselineStubReg, offsetof(ICTableSwitch, table_)), scratch);
    masm.loadPtr(BaseIndex(scratch, key, ScalePointer), scratch);

    EmitChangeICReturnAddress(masm, scratch);
    EmitReturnFromIC(masm);

    masm.bind(&notInt32);

    masm.branchTestDouble(Assembler::NotEqual, R0, &outOfRange);
    if (cx->runtime->jitSupportsFloatingPoint) {
        masm.unboxDouble(R0, FloatReg0);

        
        masm.convertDoubleToInt32(FloatReg0, key, &outOfRange,  false);
    } else {
        
        masm.pushValue(R0);
        masm.movePtr(StackPointer, R0.scratchReg());

        masm.setupUnalignedABICall(1, scratch);
        masm.passABIArg(R0.scratchReg());
        masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, DoubleValueToInt32ForSwitch));

        
        
        masm.mov(ReturnReg, scratch);
        masm.popValue(R0);
        masm.branchTest32(Assembler::Zero, scratch, scratch, &outOfRange);
        masm.unboxInt32(R0, key);
    }
    masm.jump(&isInt32);

    masm.bind(&outOfRange);

    masm.loadPtr(Address(BaselineStubReg, offsetof(ICTableSwitch, defaultTarget_)), scratch);

    EmitChangeICReturnAddress(masm, scratch);
    EmitReturnFromIC(masm);
    return true;
}

ICStub *
ICTableSwitch::Compiler::getStub(ICStubSpace *space)
{
    IonCode *code = getStubCode();
    if (!code)
        return NULL;

    jsbytecode *pc = pc_;
    pc += JUMP_OFFSET_LEN;
    int32_t low = GET_JUMP_OFFSET(pc);
    pc += JUMP_OFFSET_LEN;
    int32_t high = GET_JUMP_OFFSET(pc);
    int32_t length = high - low + 1;
    pc += JUMP_OFFSET_LEN;

    void **table = (void**) space->alloc(sizeof(void*) * length);
    if (!table)
        return NULL;

    jsbytecode *defaultpc = pc_ + GET_JUMP_OFFSET(pc_);

    for (int32_t i = 0; i < length; i++) {
        int32_t off = GET_JUMP_OFFSET(pc);
        if (off)
            table[i] = pc_ + off;
        else
            table[i] = defaultpc;
        pc += JUMP_OFFSET_LEN;
    }

    return ICTableSwitch::New(space, code, table, low, length, defaultpc);
}

void
ICTableSwitch::fixupJumpTable(HandleScript script, BaselineScript *baseline)
{
    defaultTarget_ = baseline->nativeCodeForPC(script, (jsbytecode *) defaultTarget_);

    for (int32_t i = 0; i < length_; i++)
        table_[i] = baseline->nativeCodeForPC(script, (jsbytecode *) table_[i]);
}





static bool
DoIteratorNewFallback(JSContext *cx, BaselineFrame *frame, ICIteratorNew_Fallback *stub,
                      HandleValue value, MutableHandleValue res)
{
    jsbytecode *pc = stub->icEntry()->pc(frame->script());
    FallbackICSpew(cx, stub, "IteratorNew");

    uint8_t flags = GET_UINT8(pc);
    res.set(value);
    return ValueToIterator(cx, flags, res);
}

typedef bool (*DoIteratorNewFallbackFn)(JSContext *, BaselineFrame *, ICIteratorNew_Fallback *,
                                        HandleValue, MutableHandleValue);
static const VMFunction DoIteratorNewFallbackInfo =
    FunctionInfo<DoIteratorNewFallbackFn>(DoIteratorNewFallback, PopValues(1));

bool
ICIteratorNew_Fallback::Compiler::generateStubCode(MacroAssembler &masm)
{
    EmitRestoreTailCallReg(masm);

    
    masm.pushValue(R0);

    masm.pushValue(R0);
    masm.push(BaselineStubReg);
    masm.pushBaselineFramePtr(BaselineFrameReg, R0.scratchReg());

    return tailCallVM(DoIteratorNewFallbackInfo, masm);
}





static bool
DoIteratorMoreFallback(JSContext *cx, BaselineFrame *frame, ICIteratorMore_Fallback *stub,
                       HandleValue iterValue, MutableHandleValue res)
{
    FallbackICSpew(cx, stub, "IteratorMore");

    bool cond;
    if (!IteratorMore(cx, &iterValue.toObject(), &cond, res))
        return false;
    res.setBoolean(cond);

    if (iterValue.toObject().isPropertyIterator() && !stub->hasStub(ICStub::IteratorMore_Native)) {
        ICIteratorMore_Native::Compiler compiler(cx);
        ICStub *newStub = compiler.getStub(compiler.getStubSpace(frame->script()));
        if (!newStub)
            return false;
        stub->addNewStub(newStub);
    }

    return true;
}

typedef bool (*DoIteratorMoreFallbackFn)(JSContext *, BaselineFrame *, ICIteratorMore_Fallback *,
                                         HandleValue, MutableHandleValue);
static const VMFunction DoIteratorMoreFallbackInfo =
    FunctionInfo<DoIteratorMoreFallbackFn>(DoIteratorMoreFallback);

bool
ICIteratorMore_Fallback::Compiler::generateStubCode(MacroAssembler &masm)
{
    EmitRestoreTailCallReg(masm);

    masm.pushValue(R0);
    masm.push(BaselineStubReg);
    masm.pushBaselineFramePtr(BaselineFrameReg, R0.scratchReg());

    return tailCallVM(DoIteratorMoreFallbackInfo, masm);
}





bool
ICIteratorMore_Native::Compiler::generateStubCode(MacroAssembler &masm)
{
    Label failure;

    Register obj = masm.extractObject(R0, ExtractTemp0);

    GeneralRegisterSet regs(availableGeneralRegs(1));
    Register nativeIterator = regs.takeAny();
    Register scratch = regs.takeAny();

    masm.branchTestObjClass(Assembler::NotEqual, obj, scratch,
                            &PropertyIteratorObject::class_, &failure);
    masm.loadObjPrivate(obj, JSObject::ITER_CLASS_NFIXED_SLOTS, nativeIterator);

    masm.branchTest32(Assembler::NonZero, Address(nativeIterator, offsetof(NativeIterator, flags)),
                      Imm32(JSITER_FOREACH), &failure);

    
    masm.loadPtr(Address(nativeIterator, offsetof(NativeIterator, props_end)), scratch);
    masm.cmpPtr(Address(nativeIterator, offsetof(NativeIterator, props_cursor)), scratch);
    masm.emitSet(Assembler::LessThan, scratch);

    masm.tagValue(JSVAL_TYPE_BOOLEAN, scratch, R0);
    EmitReturnFromIC(masm);

    
    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}





static bool
DoIteratorNextFallback(JSContext *cx, BaselineFrame *frame, ICIteratorNext_Fallback *stub,
                       HandleValue iterValue, MutableHandleValue res)
{
    FallbackICSpew(cx, stub, "IteratorNext");

    RootedObject iteratorObject(cx, &iterValue.toObject());
    if (!IteratorNext(cx, iteratorObject, res))
        return false;

    if (iteratorObject->isPropertyIterator() && !stub->hasStub(ICStub::IteratorNext_Native)) {
        ICIteratorNext_Native::Compiler compiler(cx);
        ICStub *newStub = compiler.getStub(compiler.getStubSpace(frame->script()));
        if (!newStub)
            return false;
        stub->addNewStub(newStub);
    }

    return true;
}

typedef bool (*DoIteratorNextFallbackFn)(JSContext *, BaselineFrame *, ICIteratorNext_Fallback *,
                                         HandleValue, MutableHandleValue);
static const VMFunction DoIteratorNextFallbackInfo =
    FunctionInfo<DoIteratorNextFallbackFn>(DoIteratorNextFallback);

bool
ICIteratorNext_Fallback::Compiler::generateStubCode(MacroAssembler &masm)
{
    EmitRestoreTailCallReg(masm);

    masm.pushValue(R0);
    masm.push(BaselineStubReg);
    masm.pushBaselineFramePtr(BaselineFrameReg, R0.scratchReg());

    return tailCallVM(DoIteratorNextFallbackInfo, masm);
}





bool
ICIteratorNext_Native::Compiler::generateStubCode(MacroAssembler &masm)
{
    Label failure;

    Register obj = masm.extractObject(R0, ExtractTemp0);

    GeneralRegisterSet regs(availableGeneralRegs(1));
    Register nativeIterator = regs.takeAny();
    Register scratch = regs.takeAny();

    masm.branchTestObjClass(Assembler::NotEqual, obj, scratch,
                            &PropertyIteratorObject::class_, &failure);
    masm.loadObjPrivate(obj, JSObject::ITER_CLASS_NFIXED_SLOTS, nativeIterator);

    masm.branchTest32(Assembler::NonZero, Address(nativeIterator, offsetof(NativeIterator, flags)),
                      Imm32(JSITER_FOREACH), &failure);

    
    masm.loadPtr(Address(nativeIterator, offsetof(NativeIterator, props_cursor)), scratch);
    masm.loadPtr(Address(scratch, 0), scratch);

    
    masm.addPtr(Imm32(sizeof(JSString *)),
                Address(nativeIterator, offsetof(NativeIterator, props_cursor)));

    masm.tagValue(JSVAL_TYPE_STRING, scratch, R0);
    EmitReturnFromIC(masm);

    
    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}





static bool
DoIteratorCloseFallback(JSContext *cx, ICIteratorClose_Fallback *stub, HandleValue iterValue)
{
    FallbackICSpew(cx, stub, "IteratorClose");

    RootedObject iteratorObject(cx, &iterValue.toObject());
    return CloseIterator(cx, iteratorObject);
}

typedef bool (*DoIteratorCloseFallbackFn)(JSContext *, ICIteratorClose_Fallback *, HandleValue);
static const VMFunction DoIteratorCloseFallbackInfo =
    FunctionInfo<DoIteratorCloseFallbackFn>(DoIteratorCloseFallback);

bool
ICIteratorClose_Fallback::Compiler::generateStubCode(MacroAssembler &masm)
{
    EmitRestoreTailCallReg(masm);

    masm.pushValue(R0);
    masm.push(BaselineStubReg);

    return tailCallVM(DoIteratorCloseFallbackInfo, masm);
}





static bool
DoInstanceOfFallback(JSContext *cx, ICInstanceOf_Fallback *stub,
                     HandleValue lhs, HandleValue rhs,
                     MutableHandleValue res)
{
    FallbackICSpew(cx, stub, "InstanceOf");

    if (!rhs.isObject()) {
        js_ReportValueError(cx, JSMSG_BAD_INSTANCEOF_RHS, -1, rhs, NullPtr());
        return false;
    }

    RootedObject obj(cx, &rhs.toObject());

    JSBool cond = false;
    if (!HasInstance(cx, obj, lhs, &cond))
        return false;

    res.setBoolean(cond);
    return true;
}

typedef bool (*DoInstanceOfFallbackFn)(JSContext *, ICInstanceOf_Fallback *, HandleValue, HandleValue,
                                       MutableHandleValue);
static const VMFunction DoInstanceOfFallbackInfo =
    FunctionInfo<DoInstanceOfFallbackFn>(DoInstanceOfFallback, PopValues(2));

bool
ICInstanceOf_Fallback::Compiler::generateStubCode(MacroAssembler &masm)
{
    EmitRestoreTailCallReg(masm);

    
    masm.pushValue(R0);
    masm.pushValue(R1);

    masm.pushValue(R1);
    masm.pushValue(R0);
    masm.push(BaselineStubReg);

    return tailCallVM(DoInstanceOfFallbackInfo, masm);
}





static bool
DoTypeOfFallback(JSContext *cx, BaselineFrame *frame, ICTypeOf_Fallback *stub, HandleValue val,
                 MutableHandleValue res)
{
    FallbackICSpew(cx, stub, "TypeOf");
    JSType type = JS_TypeOfValue(cx, val);
    RootedString string(cx, TypeName(type, cx));

    res.setString(string);

    JS_ASSERT(type != JSTYPE_NULL);
    if (type != JSTYPE_OBJECT && type != JSTYPE_FUNCTION) {
        
        IonSpew(IonSpew_BaselineIC, "  Generating TypeOf stub for JSType (%d)", (int) type);
        ICTypeOf_Typed::Compiler compiler(cx, type, string);
        ICStub *typeOfStub = compiler.getStub(compiler.getStubSpace(frame->script()));
        if (!typeOfStub)
            return false;
        stub->addNewStub(typeOfStub);
    }

    return true;
}

typedef bool (*DoTypeOfFallbackFn)(JSContext *, BaselineFrame *frame, ICTypeOf_Fallback *,
                                   HandleValue, MutableHandleValue);
static const VMFunction DoTypeOfFallbackInfo =
    FunctionInfo<DoTypeOfFallbackFn>(DoTypeOfFallback);

bool
ICTypeOf_Fallback::Compiler::generateStubCode(MacroAssembler &masm)
{
    EmitRestoreTailCallReg(masm);

    masm.pushValue(R0);
    masm.push(BaselineStubReg);
    masm.pushBaselineFramePtr(BaselineFrameReg, R0.scratchReg());

    return tailCallVM(DoTypeOfFallbackInfo, masm);
}

bool
ICTypeOf_Typed::Compiler::generateStubCode(MacroAssembler &masm)
{
    JS_ASSERT(type_ != JSTYPE_NULL);
    JS_ASSERT(type_ != JSTYPE_FUNCTION);
    JS_ASSERT(type_ != JSTYPE_OBJECT);

    Label failure;
    switch(type_) {
      case JSTYPE_VOID:
        masm.branchTestUndefined(Assembler::NotEqual, R0, &failure);
        break;

      case JSTYPE_STRING:
        masm.branchTestString(Assembler::NotEqual, R0, &failure);
        break;

      case JSTYPE_NUMBER:
        masm.branchTestNumber(Assembler::NotEqual, R0, &failure);
        break;

      case JSTYPE_BOOLEAN:
        masm.branchTestBoolean(Assembler::NotEqual, R0, &failure);
        break;

      default:
        JS_NOT_REACHED("Unexpected type");
    }

    masm.movePtr(ImmGCPtr(typeString_), R0.scratchReg());
    masm.tagValue(JSVAL_TYPE_STRING, R0.scratchReg(), R0);
    EmitReturnFromIC(masm);

    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}





static bool
DoCreateRestParameter(JSContext *cx, BaselineFrame *frame, ICRest_Fallback *stub,
                      HandleTypeObject type, MutableHandleValue res)
{
    FallbackICSpew(cx, stub, "Rest");

    unsigned numFormals = frame->numFormalArgs() - 1;
    unsigned numActuals = frame->numActualArgs();
    unsigned numRest = numActuals > numFormals ? numActuals - numFormals : 0;

    JSObject *obj = NewDenseCopiedArray(cx, numRest, frame->argv() + numFormals, NULL);
    if (!obj)
        return false;
    obj->setType(type);

    res.setObject(*obj);
    return true;
}

typedef bool(*DoCreateRestParameterFn)(JSContext *cx, BaselineFrame *, ICRest_Fallback *,
                                       HandleTypeObject, MutableHandleValue);
static const VMFunction DoCreateRestParameterInfo =
    FunctionInfo<DoCreateRestParameterFn>(DoCreateRestParameter);

bool
ICRest_Fallback::Compiler::generateStubCode(MacroAssembler &masm)
{
    EmitRestoreTailCallReg(masm);

    masm.push(R0.scratchReg()); 
    masm.push(BaselineStubReg); 
    masm.pushBaselineFramePtr(BaselineFrameReg, R0.scratchReg()); 

    return tailCallVM(DoCreateRestParameterInfo, masm);
}

ICProfiler_PushFunction::ICProfiler_PushFunction(IonCode *stubCode, const char *str,
                                                 HandleScript script)
  : ICStub(ICStub::Profiler_PushFunction, stubCode),
    str_(str),
    script_(script)
{ }

ICTypeMonitor_SingleObject::ICTypeMonitor_SingleObject(IonCode *stubCode, HandleObject obj)
  : ICStub(TypeMonitor_SingleObject, stubCode),
    obj_(obj)
{ }

ICTypeMonitor_TypeObject::ICTypeMonitor_TypeObject(IonCode *stubCode, HandleTypeObject type)
  : ICStub(TypeMonitor_TypeObject, stubCode),
    type_(type)
{ }

ICTypeUpdate_SingleObject::ICTypeUpdate_SingleObject(IonCode *stubCode, HandleObject obj)
  : ICStub(TypeUpdate_SingleObject, stubCode),
    obj_(obj)
{ }

ICTypeUpdate_TypeObject::ICTypeUpdate_TypeObject(IonCode *stubCode, HandleTypeObject type)
  : ICStub(TypeUpdate_TypeObject, stubCode),
    type_(type)
{ }

ICGetElemNativeStub::ICGetElemNativeStub(ICStub::Kind kind, IonCode *stubCode,
                                         ICStub *firstMonitorStub,
                                         HandleShape shape, HandleValue idval,
                                         bool isFixedSlot, uint32_t offset)
  : ICMonitoredStub(kind, stubCode, firstMonitorStub),
    shape_(shape),
    idval_(idval),
    offset_(offset)
{
    extra_ = isFixedSlot;
}

ICGetElemNativeStub::~ICGetElemNativeStub()
{ }

ICGetElem_NativePrototype::ICGetElem_NativePrototype(IonCode *stubCode, ICStub *firstMonitorStub,
                                                     HandleShape shape, HandleValue idval,
                                                     bool isFixedSlot, uint32_t offset,
                                                     HandleObject holder, HandleShape holderShape)
  : ICGetElemNativeStub(ICStub::GetElem_NativePrototype, stubCode, firstMonitorStub, shape,
                        idval, isFixedSlot, offset),
    holder_(holder),
    holderShape_(holderShape)
{ }

ICGetElem_Dense::ICGetElem_Dense(IonCode *stubCode, ICStub *firstMonitorStub, HandleShape shape)
    : ICMonitoredStub(GetElem_Dense, stubCode, firstMonitorStub),
      shape_(shape)
{ }

ICGetElem_TypedArray::ICGetElem_TypedArray(IonCode *stubCode, HandleShape shape, uint32_t type)
  : ICStub(GetElem_TypedArray, stubCode),
    shape_(shape)
{
    extra_ = uint16_t(type);
    JS_ASSERT(extra_ == type);
}

ICSetElem_Dense::ICSetElem_Dense(IonCode *stubCode, HandleShape shape, HandleTypeObject type)
  : ICUpdatedStub(SetElem_Dense, stubCode),
    shape_(shape),
    type_(type)
{ }

ICSetElem_DenseAdd::ICSetElem_DenseAdd(IonCode *stubCode, types::TypeObject *type,
                                       size_t protoChainDepth)
  : ICUpdatedStub(SetElem_DenseAdd, stubCode),
    type_(type)
{
    JS_ASSERT(protoChainDepth <= MAX_PROTO_CHAIN_DEPTH);
    extra_ = protoChainDepth;
}

template <size_t ProtoChainDepth>
ICUpdatedStub *
ICSetElemDenseAddCompiler::getStubSpecific(ICStubSpace *space, const AutoShapeVector *shapes)
{
    RootedTypeObject objType(cx, obj_->getType(cx));
    Rooted<IonCode *> stubCode(cx, getStubCode());
    return ICSetElem_DenseAddImpl<ProtoChainDepth>::New(space, stubCode, objType, shapes);
}

ICSetElem_TypedArray::ICSetElem_TypedArray(IonCode *stubCode, HandleShape shape, uint32_t type,
                                           bool expectOutOfBounds)
  : ICStub(SetElem_TypedArray, stubCode),
    shape_(shape)
{
    extra_ = uint8_t(type);
    JS_ASSERT(extra_ == type);
    extra_ |= (static_cast<uint16_t>(expectOutOfBounds) << 8);
}

ICGetName_Global::ICGetName_Global(IonCode *stubCode, ICStub *firstMonitorStub, HandleShape shape,
                                   uint32_t slot)
  : ICMonitoredStub(GetName_Global, stubCode, firstMonitorStub),
    shape_(shape),
    slot_(slot)
{ }

template <size_t NumHops>
ICGetName_Scope<NumHops>::ICGetName_Scope(IonCode *stubCode, ICStub *firstMonitorStub,
                                          AutoShapeVector *shapes, uint32_t offset)
  : ICMonitoredStub(GetStubKind(), stubCode, firstMonitorStub),
    offset_(offset)
{
    JS_STATIC_ASSERT(NumHops <= MAX_HOPS);
    JS_ASSERT(shapes->length() == NumHops + 1);
    for (size_t i = 0; i < NumHops + 1; i++)
        shapes_[i].init((*shapes)[i]);
}

ICGetIntrinsic_Constant::ICGetIntrinsic_Constant(IonCode *stubCode, HandleValue value)
  : ICStub(GetIntrinsic_Constant, stubCode),
    value_(value)
{ }

ICGetIntrinsic_Constant::~ICGetIntrinsic_Constant()
{ }

ICGetProp_String::ICGetProp_String(IonCode *stubCode, ICStub *firstMonitorStub,
                                   HandleShape stringProtoShape, uint32_t offset)
  : ICMonitoredStub(GetProp_String, stubCode, firstMonitorStub),
    stringProtoShape_(stringProtoShape),
    offset_(offset)
{ }

ICGetPropNativeStub::ICGetPropNativeStub(ICStub::Kind kind, IonCode *stubCode,
                                         ICStub *firstMonitorStub,
                                         HandleShape shape, uint32_t offset)
  : ICMonitoredStub(kind, stubCode, firstMonitorStub),
    shape_(shape),
    offset_(offset)
{ }

ICGetProp_NativePrototype::ICGetProp_NativePrototype(IonCode *stubCode, ICStub *firstMonitorStub,
                                                     HandleShape shape, uint32_t offset,
                                                     HandleObject holder, HandleShape holderShape)
  : ICGetPropNativeStub(GetProp_NativePrototype, stubCode, firstMonitorStub, shape, offset),
    holder_(holder),
    holderShape_(holderShape)
{ }

ICGetPropCallGetter::ICGetPropCallGetter(Kind kind, IonCode *stubCode, ICStub *firstMonitorStub,
                                         HandleShape shape, HandleObject holder,
                                         HandleShape holderShape,
                                         HandleFunction getter, uint32_t pcOffset)
  : ICMonitoredStub(kind, stubCode, firstMonitorStub),
    shape_(shape),
    holder_(holder),
    holderShape_(holderShape),
    getter_(getter),
    pcOffset_(pcOffset)
{
    JS_ASSERT(kind == ICStub::GetProp_CallScripted || kind == ICStub::GetProp_CallNative);
}

ICSetProp_Native::ICSetProp_Native(IonCode *stubCode, HandleTypeObject type, HandleShape shape,
                                   uint32_t offset)
  : ICUpdatedStub(SetProp_Native, stubCode),
    type_(type),
    shape_(shape),
    offset_(offset)
{ }

ICUpdatedStub *
ICSetProp_Native::Compiler::getStub(ICStubSpace *space)
{
    RootedTypeObject type(cx, obj_->getType(cx));
    RootedShape shape(cx, obj_->lastProperty());
    ICUpdatedStub *stub = ICSetProp_Native::New(space, getStubCode(), type, shape, offset_);
    if (!stub || !stub->initUpdatingChain(cx, space))
        return NULL;
    return stub;
}

ICSetProp_NativeAdd::ICSetProp_NativeAdd(IonCode *stubCode, HandleTypeObject type,
                                         size_t protoChainDepth,
                                         HandleShape newShape, uint32_t offset)
  : ICUpdatedStub(SetProp_NativeAdd, stubCode),
    type_(type),
    newShape_(newShape),
    offset_(offset)
{
    JS_ASSERT(protoChainDepth <= MAX_PROTO_CHAIN_DEPTH);
    extra_ = protoChainDepth;
}

template <size_t ProtoChainDepth>
ICSetProp_NativeAddImpl<ProtoChainDepth>::ICSetProp_NativeAddImpl(IonCode *stubCode,
                                                                  HandleTypeObject type,
                                                                  const AutoShapeVector *shapes,
                                                                  HandleShape newShape,
                                                                  uint32_t offset)
  : ICSetProp_NativeAdd(stubCode, type, ProtoChainDepth, newShape, offset)
{
    JS_ASSERT(shapes->length() == NumShapes);
    for (size_t i = 0; i < NumShapes; i++)
        shapes_[i].init((*shapes)[i]);
}

ICSetPropNativeAddCompiler::ICSetPropNativeAddCompiler(JSContext *cx, HandleObject obj,
                                                       HandleShape oldShape,
                                                       size_t protoChainDepth, bool isFixedSlot,
                                                       uint32_t offset)
  : ICStubCompiler(cx, ICStub::SetProp_NativeAdd),
    obj_(cx, obj),
    oldShape_(cx, oldShape),
    protoChainDepth_(protoChainDepth),
    isFixedSlot_(isFixedSlot),
    offset_(offset)
{
    JS_ASSERT(protoChainDepth_ <= ICSetProp_NativeAdd::MAX_PROTO_CHAIN_DEPTH);
}

ICSetPropCallSetter::ICSetPropCallSetter(Kind kind, IonCode *stubCode, HandleShape shape,
                                         HandleObject holder, HandleShape holderShape,
                                         HandleFunction setter, uint32_t pcOffset)
  : ICStub(kind, stubCode),
    shape_(shape),
    holder_(holder),
    holderShape_(holderShape),
    setter_(setter),
    pcOffset_(pcOffset)
{
    JS_ASSERT(kind == ICStub::SetProp_CallScripted || kind == ICStub::SetProp_CallNative);
}

ICCall_Scripted::ICCall_Scripted(IonCode *stubCode, ICStub *firstMonitorStub,
                                 HandleScript calleeScript, uint32_t pcOffset)
  : ICMonitoredStub(ICStub::Call_Scripted, stubCode, firstMonitorStub),
    calleeScript_(calleeScript),
    pcOffset_(pcOffset)
{ }

ICCall_Native::ICCall_Native(IonCode *stubCode, ICStub *firstMonitorStub, HandleFunction callee,
                             uint32_t pcOffset)
  : ICMonitoredStub(ICStub::Call_Native, stubCode, firstMonitorStub),
    callee_(callee),
    pcOffset_(pcOffset)
{ }

ICGetPropCallListBaseNativeStub::ICGetPropCallListBaseNativeStub(Kind kind, IonCode *stubCode,
                                                                 ICStub *firstMonitorStub,
                                                                 HandleShape shape,
                                                                 BaseProxyHandler *proxyHandler,
                                                                 HandleShape expandoShape,
                                                                 HandleObject holder,
                                                                 HandleShape holderShape,
                                                                 HandleFunction getter,
                                                                 uint32_t pcOffset)
  : ICMonitoredStub(kind, stubCode, firstMonitorStub),
    shape_(shape),
    proxyHandler_(proxyHandler),
    expandoShape_(expandoShape),
    holder_(holder),
    holderShape_(holderShape),
    getter_(getter),
    pcOffset_(pcOffset)
{ }

ICGetPropCallListBaseNativeCompiler::ICGetPropCallListBaseNativeCompiler(JSContext *cx,
                                                                         ICStub::Kind kind,
                                                                         ICStub *firstMonitorStub,
                                                                         HandleObject obj,
                                                                         HandleObject holder,
                                                                         HandleFunction getter,
                                                                         uint32_t pcOffset)
  : ICStubCompiler(cx, kind),
    firstMonitorStub_(firstMonitorStub),
    obj_(cx, obj),
    holder_(cx, holder),
    getter_(cx, getter),
    pcOffset_(pcOffset)
{
    JS_ASSERT(kind == ICStub::GetProp_CallListBaseNative ||
              kind == ICStub::GetProp_CallListBaseWithGenerationNative);
    JS_ASSERT(obj_->isProxy());
    JS_ASSERT(GetProxyHandler(obj_)->family() == GetListBaseHandlerFamily());
}

ICGetProp_ListBaseShadowed::ICGetProp_ListBaseShadowed(IonCode *stubCode,
                                                       ICStub *firstMonitorStub,
                                                       HandleShape shape,
                                                       BaseProxyHandler *proxyHandler,
                                                       HandlePropertyName name,
                                                       uint32_t pcOffset)
  : ICMonitoredStub(ICStub::GetProp_ListBaseShadowed, stubCode, firstMonitorStub),
    shape_(shape),
    proxyHandler_(proxyHandler),
    name_(name),
    pcOffset_(pcOffset)
{ }

} 
} 
