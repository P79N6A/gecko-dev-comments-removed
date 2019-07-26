






#include "BaselineJIT.h"
#include "BaselineCompiler.h"
#include "BaselineHelpers.h"
#include "BaselineIC.h"
#include "IonLinker.h"
#include "IonSpewer.h"
#include "VMFunctions.h"
#include "IonFrames-inl.h"

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
                script->filename,
                script->lineno,
                (int) (pc - script->code),
                PCToLineNumber(script, pc),
                script->getUseCount(),
                (int) stub->numOptimizedStubs(),
                fmtbuf);
    }
}
#else
#define FallbackICSpew(...)
#endif

ICFallbackStub *
ICEntry::fallbackStub() const
{
    return firstStub()->getChainFallback();
}

void
ICStub::markCode(JSTracer *trc, const char *name)
{
    IonCode *stubIonCode = ionCode();
    MarkIonCodeUnbarriered(trc, &stubIonCode, name);
}

 void
ICStub::trace(JSTracer *trc)
{
    markCode(trc, "baseline-stub-ioncode");

    
    
    
    
    if (isMonitoredFallback()) {
        ICTypeMonitor_Fallback *lastMonStub = toMonitoredFallbackStub()->fallbackMonitorStub();
        for (ICStub *monStub = lastMonStub->firstMonitorStub();
             monStub != NULL;
             monStub = monStub->next())
        {
            JS_ASSERT_IF(monStub->next() == NULL, monStub == lastMonStub);
            monStub->markCode(trc, "baseline-monitor-stub-ioncode");
        }
    }

    if (isUpdated()) {
        for (ICStub *updateStub = toUpdatedStub()->firstUpdateStub();
             updateStub != NULL;
             updateStub = updateStub->next())
        {
            JS_ASSERT_IF(updateStub->next() == NULL, updateStub->isTypeUpdate_Fallback());
            updateStub->markCode(trc, "baseline-update-stub-ioncode");
        }
    }

    switch (kind()) {
      case ICStub::Call_Scripted: {
        ICCall_Scripted *callStub = toCall_Scripted();
        MarkObject(trc, &callStub->callee(), "baseline-callstub-callee");
        break;
      }
      case ICStub::Call_Native: {
        ICCall_Native *callStub = toCall_Native();
        MarkObject(trc, &callStub->callee(), "baseline-callstub-callee");
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
        MarkShape(trc, &setElemStub->shape(), "baseline-getelem-denseadd-shape");
        MarkTypeObject(trc, &setElemStub->type(), "baseline-setelem-denseadd-type");
        MarkObject(trc, &setElemStub->lastProto(), "baseline-setelem-denseadd-lastproto");
        MarkShape(trc, &setElemStub->lastProtoShape(), "baseline-setelem-denseadd-lastprotoshape");
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
      case ICStub::GetName_Global: {
        ICGetName_Global *globalStub = toGetName_Global();
        MarkShape(trc, &globalStub->shape(), "baseline-global-stub-shape");
        break;
      }
      case ICStub::GetProp_Native: {
        ICGetProp_Native *propStub = toGetProp_Native();
        MarkShape(trc, &propStub->shape(), "baseline-getprop-stub-shape");
        break;
      }
      case ICStub::GetProp_NativePrototype: {
        ICGetProp_NativePrototype *propStub = toGetProp_NativePrototype();
        MarkShape(trc, &propStub->shape(), "baseline-getprop-stub-shape");
        MarkObject(trc, &propStub->holder(), "baseline-getprop-stub-holder");
        MarkShape(trc, &propStub->holderShape(), "baseline-getprop-stub-holdershape");
        break;
      }
      case ICStub::SetProp_Native: {
        ICSetProp_Native *propStub = toSetProp_Native();
        MarkShape(trc, &propStub->shape(), "baseline-setprop-stub-shape");
        MarkTypeObject(trc, &propStub->type(), "baseline-setprop-stub-type");
        break;
      }
      default:
        break;
    }
}

void
ICFallbackStub::unlinkStubsWithKind(ICStub::Kind kind)
{
    ICStub *stub = icEntry_->firstStub();
    ICStub *last = NULL;
    do {
        if (stub->kind() == kind) {
            JS_ASSERT(stub->next());

            
            if (stub->next() == this) {
                JS_ASSERT(lastStubPtrAddr_ == stub->addressOfNext());
                if (last)
                    lastStubPtrAddr_ = last->addressOfNext();
                else
                    lastStubPtrAddr_ = icEntry()->addressOfFirstStub();
                *lastStubPtrAddr_ = this;
            }

            JS_ASSERT(numOptimizedStubs_ > 0);
            numOptimizedStubs_--;

            stub = stub->next();
            continue;
        }

        last = stub;
        stub = stub->next();
    } while (stub);
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

    AutoFlushCache afc("ICStubCompiler::getStubCode", ion);
    if (!generateStubCode(masm))
        return NULL;
    Linker linker(masm);
    Rooted<IonCode *> newStubCode(cx, linker.newCode(cx));
    if (!newStubCode)
        return NULL;

    
    if (!postGenerateStubCode(masm, newStubCode))
        return NULL;

    
    if (cx->zone()->needsBarrier())
        newStubCode->togglePreBarriers(true);

    
    if (!ion->putStubCode(stubKey, newStubCode))
        return NULL;

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





static bool
DoStackCheckFallback(JSContext *cx, ICStackCheck_Fallback *stub)
{
    FallbackICSpew(cx, stub, "StackCheck");
    JS_CHECK_RECURSION(cx, return false);
    return true;
}

typedef bool (*DoStackCheckFallbackFn)(JSContext *, ICStackCheck_Fallback *);
static const VMFunction DoStackCheckFallbackInfo =
    FunctionInfo<DoStackCheckFallbackFn>(DoStackCheckFallback);

bool
ICStackCheck_Fallback::Compiler::generateStubCode(MacroAssembler &masm)
{
    JS_ASSERT(R0 == JSReturnOperand);

    
    EmitRestoreTailCallReg(masm);

    masm.push(BaselineStubReg);

    return tailCallVM(DoStackCheckFallbackInfo, masm);
}





static bool
EnsureCanEnterIon(JSContext *cx, ICUseCount_Fallback *stub, IonJSFrameLayout *frame,
                  HandleScript script, jsbytecode *pc, void **jitcodePtr)
{
    JS_ASSERT(jitcodePtr);
    JS_ASSERT(!*jitcodePtr);

    
    
    if (JSOp(*pc) != JSOP_LOOPENTRY) {
        IonSpew(IonSpew_BaselineICFallback, "  Not at LOOPENTRY.");
        
        JS_ASSERT(pc == script->code);
        return true;
    }

    
    if (script->isIonCompilingOffThread()) {
        IonSpew(IonSpew_BaselineOSR, "  IonScript exists, and is compiling off thread!");
        
        
        
        script->resetUseCount();
        return true;
    }

    AbstractFramePtr abstractFp(BaselineFrame::FromIonJSFrame(frame));
    bool isConstructing = ScriptFrameIter(cx).isConstructing();
    MethodStatus stat = CanEnterAtBranch(cx, script, abstractFp, pc, isConstructing);
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
        
        
        script->resetUseCount();
        return true;
    }

    IonSpew(IonSpew_BaselineOSR, "  IonScript exists, and OSR possible!");
    IonScript *ion = script->ionScript();
    void *osrcode = ion->method()->raw() + ion->osrEntryOffset();

    *jitcodePtr = osrcode;

    script->resetUseCount();
    return true;
}

struct IonOsrTempData
{
    
    void *jitcode;

    
    uint8_t *stackFrameStart;
    uint8_t *stackFrameEnd;
    uint8_t *stackFrame;
    size_t stackFrameSize;

    
    uint8_t *actualArgsStart;
    uint8_t *actualArgsEnd;
    size_t actualArgsSize;
};











































static IonOsrTempData *
PrepareOsrTempData(JSContext *cx, ICUseCount_Fallback *stub, IonJSFrameLayout *frame,
                   HandleScript script, jsbytecode *pc, void *jitcode)
{
    size_t blFrameHeaderSize = BaselineFrame::FramePointerOffset + BaselineFrame::Size();

    
    CalleeToken calleeTok = frame->calleeToken();
    BaselineFrame *baselineFrame = BaselineFrame::FromIonJSFrame(frame);

    
    
    size_t numActualArgs = frame->numActualArgs();
    size_t numLocalsAndStackVals = (baselineFrame->frameSize() - blFrameHeaderSize) / sizeof(Value);
    size_t numFormalArgs = CalleeTokenIsFunction(calleeTok) ?
                                    CalleeTokenToFunction(calleeTok)->nargs : 0;
    size_t numPushedArgs = (numActualArgs > numFormalArgs) ? numActualArgs : numFormalArgs;

    
    
    
    
    
    
    
    
    
    
    
    

    size_t actualArgsSpace = (sizeof(Value) * (numPushedArgs + 1)) + (sizeof(void *) * 4);
    size_t stackFrameSpace = (sizeof(Value) * numLocalsAndStackVals) + sizeof(StackFrame)
                                + (sizeof(Value) * (numFormalArgs + 1));
    size_t ionOsrTempDataSpace = sizeof(IonOsrTempData);

    size_t totalSpace = AlignBytes(actualArgsSpace, sizeof(Value)) +
                        AlignBytes(stackFrameSpace, sizeof(Value)) +
                        AlignBytes(ionOsrTempDataSpace, sizeof(Value));

    uint8_t *tempInfo = (uint8_t *) js_calloc(totalSpace);
    if (!tempInfo)
        return NULL;

    IonOsrTempData *info = (IonOsrTempData *) tempInfo;
    info->jitcode = jitcode;

    info->stackFrameStart = tempInfo + AlignBytes(ionOsrTempDataSpace, sizeof(Value));
    info->stackFrameEnd = info->stackFrameStart + stackFrameSpace;
    info->stackFrame = info->stackFrameStart + (numFormalArgs * sizeof(Value)) + sizeof(Value);
    info->stackFrameSize = stackFrameSpace;

    info->actualArgsStart = info->stackFrameStart + AlignBytes(stackFrameSpace, sizeof(Value));
    info->actualArgsEnd = info->actualArgsStart + actualArgsSpace;
    info->actualArgsSize = actualArgsSpace;

    
    
    

    
    memcpy(info->stackFrameStart, frame->argv(), (numFormalArgs + 1) * sizeof(Value));

    
    uint8_t *stackFrame = info->stackFrame;
    *((JSObject **) (stackFrame + StackFrame::offsetOfScopeChain())) = baselineFrame->scopeChain();
    if (CalleeTokenIsFunction(calleeTok)) {
        
        *((JSFunction **) (stackFrame + StackFrame::offsetOfExec())) =
            CalleeTokenToFunction(calleeTok);
        *((uint32_t *) (stackFrame + StackFrame::offsetOfFlags())) = StackFrame::FUNCTION;
    } else {
        *((RawScript *) (stackFrame + StackFrame::offsetOfExec())) =
            CalleeTokenToScript(calleeTok);
        *((uint32_t *) (stackFrame + StackFrame::offsetOfFlags())) = 0;
    }

    
    
    
    Value *stackFrameLocalsStart = (Value *) (stackFrame + sizeof(StackFrame));
    for (size_t i = 0; i < numLocalsAndStackVals; i++)
        stackFrameLocalsStart[i] = *(baselineFrame->valueSlot(i));

    
    
    
    memcpy(info->actualArgsStart, frame, actualArgsSpace);

    IonSpew(IonSpew_BaselineOSR, "Allocated IonOsrTempData at %p", (void *) info);
    IonSpew(IonSpew_BaselineOSR, "Jitcode is %p", info->jitcode);

    
    return info;
}

static bool
DoUseCountFallback(JSContext *cx, ICUseCount_Fallback *stub, IonJSFrameLayout *frame,
                   IonOsrTempData **infoPtr)
{
    
    JS_ASSERT(ion::IsEnabled(cx));
    JS_ASSERT(infoPtr);
    *infoPtr = NULL;

    RootedScript script(cx, GetTopIonJSScript(cx));
    jsbytecode *pc = stub->icEntry()->pc(script);

    
    IonSpew(IonSpew_BaselineOSR,
            "UseCount for %s:%d reached %d at pc %p, trying to switch to Ion!",
            script->filename, script->lineno, (int) script->getUseCount(), (void *) pc);
    void *jitcode = NULL;
    if (!EnsureCanEnterIon(cx, stub, frame, script, pc, &jitcode))
        return false;

    if (!jitcode)
        return true;

    
    IonSpew(IonSpew_BaselineOSR, "Got jitcode.  Preparing for OSR into ion.");
    IonOsrTempData *info = PrepareOsrTempData(cx, stub, frame, script, pc, jitcode);
    if (!info)
        return false;
    *infoPtr = info;

    return true;
}

typedef bool (*DoUseCountFallbackFn)(JSContext *, ICUseCount_Fallback *, IonJSFrameLayout *frame,
                                     IonOsrTempData **infoPtr);
static const VMFunction DoUseCountFallbackInfo =
    FunctionInfo<DoUseCountFallbackFn>(DoUseCountFallback);

static bool
GenerateReplaceStack(MacroAssembler &masm, GeneralRegisterSet regs, Register osrDataReg)
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    Register reg0 = regs.takeAny();
    Register reg1 = regs.takeAny();
    Register reg2 = regs.takeAny();
    Register reg3 = regs.takeAny();

    Register copyFrom = reg0;
    Register copyEnd = reg1;
    Register copyTo = reg2;
    Register tempReg = reg3;

    
    
    masm.movePtr(BaselineFrameReg, BaselineStackReg);
    masm.loadPtr(Address(BaselineFrameReg, 0), BaselineFrameReg);

    
    
    masm.addPtr(Imm32(BaselineFrame::FramePointerOffset), BaselineStackReg);
    masm.addPtr(Address(osrDataReg, offsetof(IonOsrTempData, actualArgsSize)), BaselineStackReg);

    
    masm.subPtr(Address(osrDataReg, offsetof(IonOsrTempData, stackFrameSize)), BaselineStackReg);

    masm.loadPtr(Address(osrDataReg, offsetof(IonOsrTempData, stackFrameStart)), copyFrom);
    masm.loadPtr(Address(osrDataReg, offsetof(IonOsrTempData, stackFrameEnd)), copyEnd);
    masm.movePtr(BaselineStackReg, copyTo);
    masm.copyMem(copyFrom, copyEnd, copyTo, tempReg);

    
    masm.subPtr(Address(osrDataReg, offsetof(IonOsrTempData, actualArgsSize)), BaselineStackReg);

    masm.loadPtr(Address(osrDataReg, offsetof(IonOsrTempData, actualArgsStart)), copyFrom);
    masm.loadPtr(Address(osrDataReg, offsetof(IonOsrTempData, actualArgsEnd)), copyEnd);
    masm.movePtr(BaselineStackReg, copyTo);
    masm.copyMem(copyFrom, copyEnd, copyTo, tempReg);

    
    
    
    
    
    
    Address descriptorAddr(BaselineStackReg, sizeof(void *));
    masm.loadPtr(descriptorAddr, reg0);
    masm.movePtr(reg0, reg1);
    masm.rshiftPtr(Imm32(FRAMESIZE_SHIFT), reg1);
    masm.addPtr(Address(osrDataReg, offsetof(IonOsrTempData, stackFrameSize)), reg1);
    masm.lshiftPtr(Imm32(FRAMESIZE_SHIFT), reg1);
    masm.andPtr(Imm32(FRAMETYPE_MASK), reg0);
    masm.orPtr(reg1, reg0);
    masm.storePtr(reg0, descriptorAddr);

    return true;
}

bool
ICUseCount_Fallback::Compiler::generateStubCode(MacroAssembler &masm)
{
    
    
    masm.movePtr(BaselineFrameReg, R0.scratchReg());

    
    EmitEnterStubFrame(masm, R1.scratchReg());

    Label noCompiledCode;
    
    {
        
        masm.subPtr(Imm32(sizeof(void *)), BaselineStackReg);
        masm.push(BaselineStackReg);

        
        masm.addPtr(Imm32(BaselineFrame::FramePointerOffset), R0.scratchReg());
        masm.push(R0.scratchReg());

        
        masm.push(BaselineStubReg);

        if (!callVM(DoUseCountFallbackInfo, masm))
            return false;

        
        masm.pop(R0.scratchReg());

        
        masm.branchPtr(Assembler::Equal, R0.scratchReg(), ImmWord((void*) NULL), &noCompiledCode);
    }

    
    
    GeneralRegisterSet regs(availableGeneralRegs(0));
    regs.add(BaselineStubReg);
    regs.addUnchecked(BaselineTailCallReg);
    Register osrDataReg = R0.scratchReg();
    regs.take(osrDataReg);

    
    masm.loadPtr(Address(BaselineFrameReg, 0), BaselineFrameReg);

    
    if (!GenerateReplaceStack(masm, regs, osrDataReg))
        return false;

    regs.takeUnchecked(OsrFrameReg);
    Register scratchReg = regs.takeAny();
    Register scratchReg2 = regs.takeAny();
    
    
    

    
    
    
    masm.movePtr(BaselineStackReg, scratchReg);
    masm.addPtr(Address(osrDataReg, offsetof(IonOsrTempData, actualArgsSize)), scratchReg);

    masm.loadPtr(Address(osrDataReg, offsetof(IonOsrTempData, stackFrame)), scratchReg2);
    masm.subPtr(Address(osrDataReg, offsetof(IonOsrTempData, stackFrameStart)), scratchReg2);
    masm.addPtr(scratchReg2, scratchReg);
    masm.push(scratchReg);

    masm.loadPtr(Address(osrDataReg, offsetof(IonOsrTempData, jitcode)), scratchReg);
    masm.push(scratchReg);

    
    masm.setupUnalignedABICall(1, scratchReg);
    masm.passABIArg(osrDataReg);
    masm.callWithABI(JS_FUNC_TO_DATA_PTR(void *, js_free));

    
    
    
    
    
    
    
    
    
    
    
    
    masm.pop(scratchReg);
    masm.pop(OsrFrameReg);
    masm.jump(scratchReg);

    
    masm.bind(&noCompiledCode);

    EmitLeaveStubFrame(masm);
    EmitReturnFromIC(masm);

    return true;
}





bool
ICTypeMonitor_Fallback::addMonitorStubForValue(JSContext *cx, ICStubSpace *space, HandleValue val)
{
    bool wasEmptyMonitorChain = (numOptimizedMonitorStubs_ == 0);

    if (numOptimizedMonitorStubs_ >= MAX_OPTIMIZED_STUBS) {
        
        
        return true;
    }

    if (val.isPrimitive()) {
        JSValueType type = val.isDouble() ? JSVAL_TYPE_DOUBLE : val.extractNonDoubleType();
        ICTypeMonitor_Type::Compiler compiler(cx, type);
        ICStub *stub = compiler.getStub(space);
        if (!stub)
            return false;
        addOptimizedMonitorStub(stub);
    } else if (val.toObject().hasSingletonType()) {
        RootedObject obj(cx, &val.toObject());
        ICTypeMonitor_SingleObject::Compiler compiler(cx, obj);
        ICStub *stub = compiler.getStub(space);
        if (!stub)
            return false;
        addOptimizedMonitorStub(stub);
    } else {
        RootedTypeObject type(cx, val.toObject().type());
        ICTypeMonitor_TypeObject::Compiler compiler(cx, type);
        ICStub *stub = compiler.getStub(space);
        if (!stub)
            return false;
        addOptimizedMonitorStub(stub);
    }

    bool firstMonitorStubAdded = wasEmptyMonitorChain && (numOptimizedMonitorStubs_ > 0);

    if (firstMonitorStubAdded) {
        
        
        
        ICEntry *ent = mainFallbackStub_->icEntry();
        for (ICStub *mainStub = ent->firstStub();
             mainStub != mainFallbackStub_;
             mainStub = mainStub->next())
        {
            
            JS_ASSERT(mainStub->next() != NULL);

            
            
            

            
            
            if (!mainStub->isMonitored())
                continue;

            JS_ASSERT(mainStub->toMonitoredStub()->firstMonitorStub() == this);
            mainStub->toMonitoredStub()->updateFirstMonitorStub(firstMonitorStub_);
        }
    }

    return true;
}

static bool
DoTypeMonitorFallback(JSContext *cx, ICTypeMonitor_Fallback *stub, HandleValue value,
                      MutableHandleValue res)
{
    RootedScript script(cx, GetTopIonJSScript(cx));
    jsbytecode *pc = stub->mainFallbackStub()->icEntry()->pc(script);
    FallbackICSpew(cx, stub->mainFallbackStub(), "TypeMonitor");

    types::TypeScript::Monitor(cx, script, pc, value);

    if (!stub->addMonitorStubForValue(cx, ICStubSpace::StubSpaceFor(script), value))
        return false;

    
    res.set(value);
    return true;
}

typedef bool (*DoTypeMonitorFallbackFn)(JSContext *, ICTypeMonitor_Fallback *, HandleValue,
                                        MutableHandleValue);
static const VMFunction DoTypeMonitorFallbackInfo =
    FunctionInfo<DoTypeMonitorFallbackFn>(DoTypeMonitorFallback);

bool
ICTypeMonitor_Fallback::Compiler::generateStubCode(MacroAssembler &masm)
{
    JS_ASSERT(R0 == JSReturnOperand);

    
    EmitRestoreTailCallReg(masm);

    masm.pushValue(R0);
    masm.push(BaselineStubReg);

    return tailCallVM(DoTypeMonitorFallbackInfo, masm);
}

bool
ICTypeMonitor_Type::Compiler::generateStubCode(MacroAssembler &masm)
{
    Label failure;
    switch (type_) {
      case JSVAL_TYPE_INT32:
        masm.branchTestInt32(Assembler::NotEqual, R0, &failure);
        break;
      case JSVAL_TYPE_DOUBLE:
        
        masm.branchTestNumber(Assembler::NotEqual, R0, &failure);
        break;
      case JSVAL_TYPE_BOOLEAN:
        masm.branchTestBoolean(Assembler::NotEqual, R0, &failure);
        break;
      case JSVAL_TYPE_STRING:
        masm.branchTestString(Assembler::NotEqual, R0, &failure);
        break;
      case JSVAL_TYPE_NULL:
        masm.branchTestNull(Assembler::NotEqual, R0, &failure);
        break;
      case JSVAL_TYPE_UNDEFINED:
        masm.branchTestUndefined(Assembler::NotEqual, R0, &failure);
        break;
      default:
        JS_NOT_REACHED("Unexpected type");
    }

    EmitReturnFromIC(masm);

    masm.bind(&failure);
    EmitStubGuardFailure(masm);
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
ICUpdatedStub::addUpdateStubForValue(JSContext *cx, ICStubSpace *space, HandleValue val)
{
    if (numOptimizedStubs_ >= MAX_OPTIMIZED_STUBS) {
        
        
        return true;
    }

    if (val.isPrimitive()) {
        JSValueType type = val.isDouble() ? JSVAL_TYPE_DOUBLE : val.extractNonDoubleType();
        ICTypeUpdate_Type::Compiler compiler(cx, type);
        ICStub *stub = compiler.getStub(space);
        if (!stub)
            return false;
        addOptimizedUpdateStub(stub);
    } else if (val.toObject().hasSingletonType()) {
        RootedObject obj(cx, &val.toObject());
        ICTypeUpdate_SingleObject::Compiler compiler(cx, obj);
        ICStub *stub = compiler.getStub(space);
        if (!stub)
            return false;
        addOptimizedUpdateStub(stub);
    } else {
        RootedTypeObject type(cx, val.toObject().type());
        ICTypeUpdate_TypeObject::Compiler compiler(cx, type);
        ICStub *stub = compiler.getStub(space);
        if (!stub)
            return false;
        addOptimizedUpdateStub(stub);
    }

    return true;
}




static bool
DoTypeUpdateFallback(JSContext *cx, ICUpdatedStub *stub, HandleValue objval, HandleValue value)
{
    FallbackICSpew(cx, stub->getChainFallback(), "TypeUpdate(%s)",
                   ICStub::KindString(stub->kind()));

    RootedScript script(cx, GetTopIonJSScript(cx));
    RootedObject obj(cx, &objval.toObject());

    switch(stub->kind()) {
      case ICStub::SetElem_Dense:
      case ICStub::SetElem_DenseAdd: {
        JS_ASSERT(obj->isNative());
        types::AddTypePropertyId(cx, obj, JSID_VOID, value);
        break;
      }
      case ICStub::SetProp_Native: {
        JS_ASSERT(obj->isNative());
        jsbytecode *pc = stub->getChainFallback()->icEntry()->pc(script);
        RootedPropertyName name(cx, script->getName(pc));
        types::AddTypePropertyId(cx, obj, NameToId(name), value);
        break;
      }
      default:
        JS_NOT_REACHED("Invalid stub");
        return false;
    }

    return stub->addUpdateStubForValue(cx, ICStubSpace::StubSpaceFor(script), value);
}

typedef bool (*DoTypeUpdateFallbackFn)(JSContext *, ICUpdatedStub *, HandleValue, HandleValue);
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
ICTypeUpdate_Type::Compiler::generateStubCode(MacroAssembler &masm)
{
    Label failure;
    switch (type_) {
      case JSVAL_TYPE_INT32:
        masm.branchTestInt32(Assembler::NotEqual, R0, &failure);
        break;
      case JSVAL_TYPE_DOUBLE:
        
        masm.branchTestNumber(Assembler::NotEqual, R0, &failure);
        break;
      case JSVAL_TYPE_BOOLEAN:
        masm.branchTestBoolean(Assembler::NotEqual, R0, &failure);
        break;
      case JSVAL_TYPE_STRING:
        masm.branchTestString(Assembler::NotEqual, R0, &failure);
        break;
      case JSVAL_TYPE_NULL:
        masm.branchTestNull(Assembler::NotEqual, R0, &failure);
        break;
      case JSVAL_TYPE_UNDEFINED:
        masm.branchTestUndefined(Assembler::NotEqual, R0, &failure);
        break;
      default:
        JS_NOT_REACHED("Unexpected type");
    }

    
    masm.mov(Imm32(1), R1.scratchReg());
    EmitReturnFromIC(masm);

    masm.bind(&failure);
    EmitStubGuardFailure(masm);
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

    RawObject obj = NewInitArray(cx, length, type);
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
DoCompareFallback(JSContext *cx, ICCompare_Fallback *stub, HandleValue lhs, HandleValue rhs,
                  MutableHandleValue ret)
{
    RootedScript script(cx, GetTopIonJSScript(cx));
    jsbytecode *pc = stub->icEntry()->pc(script);
    JSOp op = JSOp(*pc);

    FallbackICSpew(cx, stub, "Compare(%s)", js_CodeName[op]);

    
    
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
      default:
        JS_ASSERT(!"Unhandled baseline compare op");
        return false;
    }

    ret.setBoolean(out);

    
    if (stub->numOptimizedStubs() >= ICCompare_Fallback::MAX_OPTIMIZED_STUBS) {
        
        
        return true;
    }

    
    if (lhs.isInt32() && rhs.isInt32()) {
        ICCompare_Int32::Compiler compiler(cx, op);
        ICStub *int32Stub = compiler.getStub(ICStubSpace::StubSpaceFor(script));
        if (!int32Stub)
            return false;

        stub->addNewStub(int32Stub);
        return true;
    }

    if (lhs.isNumber() && rhs.isNumber()) {
        
        stub->unlinkStubsWithKind(ICStub::Compare_Int32);

        ICCompare_Double::Compiler compiler(cx, op);
        ICStub *doubleStub = compiler.getStub(ICStubSpace::StubSpaceFor(script));
        if (!doubleStub)
            return false;

        stub->addNewStub(doubleStub);
        return true;
    }

    return true;
}

typedef bool (*DoCompareFallbackFn)(JSContext *, ICCompare_Fallback *, HandleValue, HandleValue,
                                    MutableHandleValue);
static const VMFunction DoCompareFallbackInfo = FunctionInfo<DoCompareFallbackFn>(DoCompareFallback);

bool
ICCompare_Fallback::Compiler::generateStubCode(MacroAssembler &masm)
{
    JS_ASSERT(R0 == JSReturnOperand);

    
    EmitRestoreTailCallReg(masm);

    masm.pushValue(R1);
    masm.pushValue(R0);
    masm.push(BaselineStubReg);

    return tailCallVM(DoCompareFallbackInfo, masm);
}





static bool
DoToBoolFallback(JSContext *cx, ICToBool_Fallback *stub, HandleValue arg, MutableHandleValue ret)
{
    RootedScript script(cx, GetTopIonJSScript(cx));
    FallbackICSpew(cx, stub, "ToBool");

    bool cond = ToBoolean(arg);
    ret.setBoolean(cond);

    
    if (stub->numOptimizedStubs() >= ICToBool_Fallback::MAX_OPTIMIZED_STUBS) {
        
        
        return true;
    }

    JS_ASSERT(!arg.isBoolean());

    
    if (arg.isInt32()) {
        ICToBool_Int32::Compiler compiler(cx);
        ICStub *int32Stub = compiler.getStub(ICStubSpace::StubSpaceFor(script));
        if (!int32Stub)
            return false;

        stub->addNewStub(int32Stub);
        return true;
    }

    return true;
}

typedef bool (*pf)(JSContext *, ICToBool_Fallback *, HandleValue, MutableHandleValue);
static const VMFunction fun = FunctionInfo<pf>(DoToBoolFallback);

bool
ICToBool_Fallback::Compiler::generateStubCode(MacroAssembler &masm)
{
    JS_ASSERT(R0 == JSReturnOperand);

    
    EmitRestoreTailCallReg(masm);

    
    masm.pushValue(R0);
    masm.push(BaselineStubReg);

    
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





static bool
DoToNumberFallback(JSContext *cx, ICToNumber_Fallback *stub, HandleValue arg, MutableHandleValue ret)
{
    FallbackICSpew(cx, stub, "ToNumber");
    ret.set(arg);
    return ToNumber(cx, ret.address());
}

typedef bool (*DoToNumberFallbackFn)(JSContext *, ICToNumber_Fallback *, HandleValue, MutableHandleValue);
static const VMFunction DoToNumberFallbackInfo = FunctionInfo<DoToNumberFallbackFn>(DoToNumberFallback);

bool
ICToNumber_Fallback::Compiler::generateStubCode(MacroAssembler &masm)
{
    JS_ASSERT(R0 == JSReturnOperand);

    
    EmitRestoreTailCallReg(masm);

    masm.pushValue(R0);
    masm.push(BaselineStubReg);

    return tailCallVM(DoToNumberFallbackInfo, masm);
}





static bool
DoBinaryArithFallback(JSContext *cx, ICBinaryArith_Fallback *stub, HandleValue lhs,
                      HandleValue rhs, MutableHandleValue ret)
{
    RootedScript script(cx, GetTopIonJSScript(cx));
    jsbytecode *pc = stub->icEntry()->pc(script);
    JSOp op = JSOp(*pc);
    FallbackICSpew(cx, stub, "BinaryArith(%s)", js_CodeName[op]);

    
    
    RootedValue lhsCopy(cx, lhs);
    RootedValue rhsCopy(cx, rhs);

    
    switch(op) {
      case JSOP_ADD:
        
        if (!AddValues(cx, script, stub->icEntry()->pc(script), &lhsCopy, &rhsCopy, ret.address()))
            return false;
        break;
      case JSOP_SUB:
        if (!SubValues(cx, script, stub->icEntry()->pc(script), &lhsCopy, &rhsCopy, ret.address()))
            return false;
        break;
      case JSOP_MUL:
        if (!MulValues(cx, script, stub->icEntry()->pc(script), &lhsCopy, &rhsCopy, ret.address()))
            return false;
        break;
      case JSOP_DIV:
        if (!DivValues(cx, script, stub->icEntry()->pc(script), &lhsCopy, &rhsCopy, ret.address()))
            return false;
        break;
      case JSOP_MOD:
        if (!ModValues(cx, script, stub->icEntry()->pc(script), &lhsCopy, &rhsCopy, ret.address()))
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
        if (!UrshOperation(cx, script, stub->icEntry()->pc(script), lhs, rhs, ret.address()))
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

    
    if (!lhs.isNumber() || !rhs.isNumber())
        return true;

    JS_ASSERT(ret.isNumber());

    if (lhs.isDouble() || rhs.isDouble() || ret.isDouble()) {
        switch (op) {
          case JSOP_ADD:
          case JSOP_SUB:
          case JSOP_MUL:
          case JSOP_DIV:
          case JSOP_MOD: {
            
            stub->unlinkStubsWithKind(ICStub::BinaryArith_Int32);

            ICBinaryArith_Double::Compiler compiler(cx, op);
            ICStub *doubleStub = compiler.getStub(ICStubSpace::StubSpaceFor(script));
            if (!doubleStub)
                return false;
            stub->addNewStub(doubleStub);
            return true;
          }
          case JSOP_URSH:
            
            break;
          default:
            
            return true;
        }
    }

    
    if (lhs.isInt32() && rhs.isInt32()) {
        bool allowDouble = ret.isDouble();
        ICBinaryArith_Int32::Compiler compilerInt32(cx, op, allowDouble);
        ICStub *int32Stub = compilerInt32.getStub(ICStubSpace::StubSpaceFor(script));
        if (!int32Stub)
            return false;
        stub->addNewStub(int32Stub);
    }

    return true;
}

typedef bool (*DoBinaryArithFallbackFn)(JSContext *, ICBinaryArith_Fallback *, HandleValue,
                                        HandleValue, MutableHandleValue);
static const VMFunction DoBinaryArithFallbackInfo =
    FunctionInfo<DoBinaryArithFallbackFn>(DoBinaryArithFallback);

bool
ICBinaryArith_Fallback::Compiler::generateStubCode(MacroAssembler &masm)
{
    JS_ASSERT(R0 == JSReturnOperand);

    
    EmitRestoreTailCallReg(masm);

    masm.pushValue(R1);
    masm.pushValue(R0);
    masm.push(BaselineStubReg);

    return tailCallVM(DoBinaryArithFallbackInfo, masm);
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





static bool
DoUnaryArithFallback(JSContext *cx, ICUnaryArith_Fallback *stub, HandleValue val,
                     MutableHandleValue res)
{
    RootedScript script(cx, GetTopIonJSScript(cx));
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
        ICUnaryArith_Int32::Compiler compiler(cx, op);
        ICStub *int32Stub = compiler.getStub(ICStubSpace::StubSpaceFor(script));
        if (!int32Stub)
            return false;
        stub->addNewStub(int32Stub);
        return true;
    }

    if (val.isNumber() && res.isNumber() && op == JSOP_NEG) {
        
        stub->unlinkStubsWithKind(ICStub::UnaryArith_Int32);

        ICUnaryArith_Double::Compiler compiler(cx, op);
        ICStub *doubleStub = compiler.getStub(ICStubSpace::StubSpaceFor(script));
        if (!doubleStub)
            return false;
        stub->addNewStub(doubleStub);
        return true;
    }

    return true;
}

typedef bool (*DoUnaryArithFallbackFn)(JSContext *, ICUnaryArith_Fallback *, HandleValue,
                                       MutableHandleValue);
static const VMFunction DoUnaryArithFallbackInfo =
    FunctionInfo<DoUnaryArithFallbackFn>(DoUnaryArithFallback);

bool
ICUnaryArith_Fallback::Compiler::generateStubCode(MacroAssembler &masm)
{
    JS_ASSERT(R0 == JSReturnOperand);

    
    EmitRestoreTailCallReg(masm);

    masm.pushValue(R0);
    masm.push(BaselineStubReg);

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





static bool
DoGetElemFallback(JSContext *cx, ICGetElem_Fallback *stub, HandleValue lhs, HandleValue rhs, MutableHandleValue res)
{
    RootedScript script(cx, GetTopIonJSScript(cx));
    FallbackICSpew(cx, stub, "GetElem");

    
    RootedValue lhsCopy(cx, lhs);

    bool isOptimizedArgs = false;
    if (lhs.isMagic(JS_OPTIMIZED_ARGUMENTS)) {
        
        BaselineFrame *frame = GetTopBaselineFrame(cx);
        if (!GetElemOptimizedArguments(cx, frame, &lhsCopy, rhs, res, &isOptimizedArgs))
            return false;
        if (isOptimizedArgs)
            types::TypeScript::Monitor(cx, res);
    }

    if (!isOptimizedArgs && !GetElementMonitored(cx, &lhsCopy, rhs, res))
        return false;

    if (stub->numOptimizedStubs() >= ICGetElem_Fallback::MAX_OPTIMIZED_STUBS) {
        
        
        return true;
    }

    if (lhs.isString() && rhs.isInt32() && res.isString() && !stub->hasStub(ICStub::GetElem_String)) {
        ICGetElem_String::Compiler compiler(cx);
        ICStub *stringStub = compiler.getStub(ICStubSpace::StubSpaceFor(script));
        if (!stringStub)
            return false;

        stub->addNewStub(stringStub);
        return true;
    }

    
    if (!lhs.isObject())
        return true;

    RootedObject obj(cx, &lhs.toObject());
    if (obj->isNative() && rhs.isInt32()) {
        ICGetElem_Dense::Compiler compiler(cx, stub->fallbackMonitorStub()->firstMonitorStub(),
                                           obj->lastProperty());
        ICStub *denseStub = compiler.getStub(ICStubSpace::StubSpaceFor(script));
        if (!denseStub)
            return false;

        stub->addNewStub(denseStub);
        return true;
    }

    if (obj->isTypedArray() && rhs.isInt32() && res.isNumber()) {
        ICGetElem_TypedArray::Compiler compiler(cx, obj->lastProperty(), TypedArray::type(obj));
        ICStub *typedArrayStub = compiler.getStub(ICStubSpace::StubSpaceFor(script));
        if (!typedArrayStub)
            return false;

        stub->addNewStub(typedArrayStub);
        return true;
    }

    return true;
}

typedef bool (*DoGetElemFallbackFn)(JSContext *, ICGetElem_Fallback *, HandleValue, HandleValue,
                                    MutableHandleValue);
static const VMFunction DoGetElemFallbackInfo = FunctionInfo<DoGetElemFallbackFn>(DoGetElemFallback);

bool
ICGetElem_Fallback::Compiler::generateStubCode(MacroAssembler &masm)
{
    JS_ASSERT(R0 == JSReturnOperand);

    
    EmitRestoreTailCallReg(masm);

    masm.pushValue(R1);
    masm.pushValue(R0);
    masm.push(BaselineStubReg);

    return tailCallVM(DoGetElemFallbackInfo, masm);
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





static bool
DenseSetElemStubExists(JSContext *cx, ICStub::Kind kind, ICSetElem_Fallback *stub, HandleObject obj)
{
    for (ICStub *cur = stub->icEntry()->firstStub(); cur != stub; cur = cur->next()) {
        if (kind == ICStub::SetElem_Dense) {
            if (!cur->isSetElem_Dense())
                continue;
            ICSetElem_Dense *dense = cur->toSetElem_Dense();
            if (obj->lastProperty() == dense->shape() && obj->getType(cx) == dense->type())
                return true;
        } else if (kind == ICStub::SetElem_DenseAdd) {
            if (!cur->isSetElem_DenseAdd())
                continue;
            ICSetElem_DenseAdd *dense = cur->toSetElem_DenseAdd();
            if (obj->lastProperty() == dense->shape() && obj->getType(cx) == dense->type())
                return true;
        } else {
            JS_NOT_REACHED("Invalid SetElem_Dense kind!");
        }
    }
    return false;
}

static bool
CanOptimizeDenseSetElem(JSContext *cx, HandleObject obj, uint32_t index,
                        HandleShape oldShape, uint32_t oldCapacity, uint32_t oldInitLength,
                        bool *isAddingCaseOut, MutableHandleObject lastProtoOut)
{
    uint32_t initLength = obj->getDenseInitializedLength();
    uint32_t capacity = obj->getDenseCapacity();

    *isAddingCaseOut = false;
    lastProtoOut.set(NULL);

    
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
    RootedObject lastObj(cx);
    while (curObj) {
        
        if (!curObj->isNative())
            return false;

        
        if (curObj->isIndexed())
            return false;

        if (!curObj->getProto())
            lastObj = curObj;

        curObj = curObj->getProto();
    }

    *isAddingCaseOut = true;
    lastProtoOut.set(lastObj);

    return true;
}

static bool
DoSetElemFallback(JSContext *cx, ICSetElem_Fallback *stub, HandleValue rhs, HandleValue objv,
                  HandleValue index)
{
    RootedScript script(cx, GetTopIonJSScript(cx));
    FallbackICSpew(cx, stub, "SetElem");

    RootedObject obj(cx, ToObject(cx, objv));
    if (!obj)
        return false;

    RootedShape oldShape(cx, obj->lastProperty());

    
    uint32_t oldCapacity = 0;
    uint32_t oldInitLength = 0;
    if (obj->isNative() && index.isInt32() && index.toInt32() >= 0) {
        oldCapacity = obj->getDenseCapacity();
        oldInitLength = obj->getDenseInitializedLength();
    }

    if (!SetObjectElement(cx, obj, index, rhs, script->strict))
        return false;

    if (stub->numOptimizedStubs() >= ICSetElem_Fallback::MAX_OPTIMIZED_STUBS) {
        
        
        return true;
    }

    
    if (obj->isNative() && index.isInt32() && index.toInt32() >= 0) {
        bool addingCase;
        RootedObject lastProto(cx);

        if (CanOptimizeDenseSetElem(cx, obj, index.toInt32(), oldShape, oldCapacity, oldInitLength,
                                    &addingCase, &lastProto))
        {
            RootedTypeObject type(cx, obj->getType(cx));
            RootedShape shape(cx, obj->lastProperty());

            if (addingCase && !DenseSetElemStubExists(cx, ICStub::SetElem_DenseAdd, stub, obj)) {
                RootedShape lastProtoShape(cx, lastProto->lastProperty());
                ICSetElem_DenseAdd::Compiler compiler(cx, shape, type, lastProto, lastProtoShape);
                IonSpew(IonSpew_BaselineIC,
                        "  Generating SetElem_DenseAdd stub "
                        "(shape=%p, type=%p, lastProto=%p, lastProtoShape=%p)",
                        obj->lastProperty(), type.get(), lastProto.get(), lastProtoShape.get());
                ICStub *denseStub = compiler.getStub(ICStubSpace::StubSpaceFor(script));
                if (!denseStub)
                    return false;

                stub->addNewStub(denseStub);
            } else if (!addingCase &&
                       !DenseSetElemStubExists(cx, ICStub::SetElem_Dense, stub, obj))
            {
                ICSetElem_Dense::Compiler compiler(cx, shape, type);
                IonSpew(IonSpew_BaselineIC,
                        "  Generating SetElem_Dense stub (shape=%p, type=%p)",
                        obj->lastProperty(), type.get());
                ICStub *denseStub = compiler.getStub(ICStubSpace::StubSpaceFor(script));
                if (!denseStub)
                    return false;

                stub->addNewStub(denseStub);
            }
        }
    }

    return true;
}

typedef bool (*DoSetElemFallbackFn)(JSContext *, ICSetElem_Fallback *, HandleValue, HandleValue,
                                    HandleValue);
static const VMFunction DoSetElemFallbackInfo = FunctionInfo<DoSetElemFallbackFn>(DoSetElemFallback);

bool
ICSetElem_Fallback::Compiler::generateStubCode(MacroAssembler &masm)
{
    JS_ASSERT(R0 == JSReturnOperand);

    EmitRestoreTailCallReg(masm);

    
    masm.pushValue(R1);
    masm.pushValue(R0);

    
    
    masm.mov(BaselineStackReg, R0.scratchReg());
    masm.pushValue(Address(R0.scratchReg(), 2 * sizeof(Value)));

    masm.push(BaselineStubReg);

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
    Address convertDoublesAddr(scratchReg, ObjectElements::offsetOfConvertDoubleElements());
    masm.branch32(Assembler::NotEqual, convertDoublesAddr, Imm32(0), &convertDoubles);
    masm.bind(&convertDoublesDone);

    
    Address valueAddr(BaselineStackReg, ICStackValueOffset);
    masm.loadValue(valueAddr, R0);
    masm.patchableCallPreBarrier(element, MIRType_Value);
    masm.storeValue(R0, element);
    EmitReturnFromIC(masm);

    
    masm.bind(&convertDoubles);
    masm.convertInt32ValueToDouble(valueAddr, R0.scratchReg(), &convertDoublesDone);
    masm.jump(&convertDoublesDone);

    
    masm.bind(&failureUnstow);
    EmitUnstowICValues(masm, 2);

    
    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}





bool
ICSetElem_DenseAdd::Compiler::generateStubCode(MacroAssembler &masm)
{
    
    
    
    Label failure;
    Label failureUnstow;
    masm.branchTestObject(Assembler::NotEqual, R0, &failure);
    masm.branchTestInt32(Assembler::NotEqual, R1, &failure);

    GeneralRegisterSet regs(availableGeneralRegs(2));
    Register scratchReg = regs.takeAny();

    
    masm.loadPtr(Address(BaselineStubReg, ICSetElem_DenseAdd::offsetOfLastProto()), scratchReg);
    masm.loadPtr(Address(scratchReg, JSObject::offsetOfShape()), scratchReg);
    Address lastProtoShape(BaselineStubReg, ICSetElem_DenseAdd::offsetOfLastProtoShape());
    masm.branchPtr(Assembler::NotEqual, lastProtoShape, scratchReg, &failure);

    
    Register obj = masm.extractObject(R0, ExtractTemp0);
    masm.loadPtr(Address(BaselineStubReg, ICSetElem_DenseAdd::offsetOfShape()), scratchReg);
    masm.branchTestObjShape(Assembler::NotEqual, obj, scratchReg, &failure);

    
    
    EmitStowICValues(masm, 2);

    
    regs = availableGeneralRegs(0);
    regs.take(R0);

    
    Register typeReg = regs.takeAny();
    masm.loadPtr(Address(BaselineStubReg, ICSetElem_DenseAdd::offsetOfType()), typeReg);
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
    Address convertDoublesAddr(scratchReg, ObjectElements::offsetOfConvertDoubleElements());
    masm.branch32(Assembler::NotEqual, convertDoublesAddr, Imm32(0), &convertDoubles);
    masm.bind(&convertDoublesDone);

    
    
    BaseIndex element(scratchReg, key, TimesEight);
    Address valueAddr(BaselineStackReg, ICStackValueOffset);
    masm.loadValue(valueAddr, R0);
    masm.storeValue(R0, element);
    EmitReturnFromIC(masm);

    
    masm.bind(&convertDoubles);
    masm.convertInt32ValueToDouble(valueAddr, R0.scratchReg(), &convertDoublesDone);
    masm.jump(&convertDoublesDone);

    
    masm.bind(&failureUnstow);
    EmitUnstowICValues(masm, 2);

    
    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
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
    ICGetName_Global::Compiler compiler(cx, monitorStub, global->lastProperty(), slot);
    ICStub *newStub = compiler.getStub(ICStubSpace::StubSpaceFor(script));
    if (!newStub)
        return false;

    stub->addNewStub(newStub);
    return true;
}

static bool
DoGetNameFallback(JSContext *cx, ICGetName_Fallback *stub, HandleObject scopeChain, MutableHandleValue res)
{
    RootedScript script(cx, GetTopIonJSScript(cx));
    jsbytecode *pc = stub->icEntry()->pc(script);
    mozilla::DebugOnly<JSOp> op = JSOp(*pc);
    FallbackICSpew(cx, stub, "GetName(%s)", js_CodeName[JSOp(*pc)]);

    JS_ASSERT(op == JSOP_GETGNAME || op == JSOP_CALLGNAME);

    RootedPropertyName name(cx, script->getName(pc));

    if (JSOp(pc[JSOP_GETGNAME_LENGTH]) == JSOP_TYPEOF) {
        if (!GetScopeNameForTypeOf(cx, scopeChain, name, res))
            return false;
    } else {
        if (!GetScopeName(cx, scopeChain, name, res))
            return false;
    }

    types::TypeScript::Monitor(cx, script, pc, res);

    
    if (stub->numOptimizedStubs() >= ICGetName_Fallback::MAX_OPTIMIZED_STUBS) {
        
        return true;
    }

    if (js_CodeSpec[*pc].format & JOF_GNAME) {
        if (!TryAttachGlobalNameStub(cx, script, stub, scopeChain, name))
            return false;
    }

    return true;
}

typedef bool (*DoGetNameFallbackFn)(JSContext *, ICGetName_Fallback *, HandleObject, MutableHandleValue);
static const VMFunction DoGetNameFallbackInfo = FunctionInfo<DoGetNameFallbackFn>(DoGetNameFallback);

bool
ICGetName_Fallback::Compiler::generateStubCode(MacroAssembler &masm)
{
    JS_ASSERT(R0 == JSReturnOperand);

    EmitRestoreTailCallReg(masm);

    masm.push(R0.scratchReg());
    masm.push(BaselineStubReg);

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





static bool
TryAttachLengthStub(JSContext *cx, HandleScript script, ICGetProp_Fallback *stub, HandleValue val,
                    HandleValue res, bool *attached)
{
    JS_ASSERT(!*attached);

    if (val.isString()) {
        JS_ASSERT(res.isInt32());
        ICGetProp_StringLength::Compiler compiler(cx);
        ICStub *newStub = compiler.getStub(ICStubSpace::StubSpaceFor(script));
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
        ICGetProp_ArrayLength::Compiler compiler(cx);
        ICStub *newStub = compiler.getStub(ICStubSpace::StubSpaceFor(script));
        if (!newStub)
            return false;

        *attached = true;
        stub->addNewStub(newStub);
        return true;
    }
    if (obj->isTypedArray()) {
        JS_ASSERT(res.isInt32());
        ICGetProp_TypedArrayLength::Compiler compiler(cx);
        ICStub *newStub = compiler.getStub(ICStubSpace::StubSpaceFor(script));
        if (!newStub)
            return false;

        *attached = true;
        stub->addNewStub(newStub);
        return true;
    }

    return true;
}

static bool
IsCacheableProtoChain(JSObject *obj, JSObject *holder)
{
    while (obj != holder) {
        
        
        
        JSObject *proto = obj->getProto();
        if (!proto || !proto->isNative())
            return false;

        
        
        if (proto->hasUncacheableProto())
            return false;

        obj = proto;
    }
    return true;
}

static bool
IsCacheableGetPropReadSlot(JSObject *obj, JSObject *holder, UnrootedShape shape)
{
    if (!shape || !IsCacheableProtoChain(obj, holder))
        return false;

    if (!shape->hasSlot() || !shape->hasDefaultGetter())
        return false;

    return true;
}

static bool
TryAttachNativeGetPropStub(JSContext *cx, HandleScript script, ICGetProp_Fallback *stub,
                           HandlePropertyName name, HandleValue val, HandleValue res,
                           bool *attached)
{
    JS_ASSERT(!*attached);

    if (!val.isObject())
        return true;

    RootedObject obj(cx, &val.toObject());
    if (!obj->isNative())
        return true;

    RootedShape shape(cx);
    RootedObject holder(cx);
    if (!JSObject::lookupProperty(cx, obj, name, &holder, &shape))
        return false;

    if (!IsCacheableGetPropReadSlot(obj, holder, shape))
        return true;

    bool isFixedSlot = holder->isFixedSlot(shape->slot());
    uint32_t offset = isFixedSlot
                      ? JSObject::getFixedSlotOffset(shape->slot())
                      : holder->dynamicSlotIndex(shape->slot()) * sizeof(Value);

    ICStub *monitorStub = stub->fallbackMonitorStub()->firstMonitorStub();
    ICStub::Kind kind = (obj == holder) ? ICStub::GetProp_Native : ICStub::GetProp_NativePrototype;

    ICGetPropNativeCompiler compiler(cx, kind, monitorStub, obj, holder, isFixedSlot, offset);
    ICStub *newStub = compiler.getStub(ICStubSpace::StubSpaceFor(script));
    if (!newStub)
        return false;

    stub->addNewStub(newStub);
    *attached = true;
    return true;
}

static bool
DoGetPropFallback(JSContext *cx, ICGetProp_Fallback *stub, MutableHandleValue val,
                  MutableHandleValue res)
{
    RootedScript script(cx, GetTopIonJSScript(cx));
    jsbytecode *pc = stub->icEntry()->pc(script);
    JSOp op = JSOp(*pc);
    FallbackICSpew(cx, stub, "GetProp(%s)", js_CodeName[op]);

    JS_ASSERT(op == JSOP_GETPROP || op == JSOP_CALLPROP || op == JSOP_LENGTH);

    RootedPropertyName name(cx, script->getName(pc));
    RootedId id(cx, NameToId(name));

    if (op == JSOP_LENGTH && val.isMagic(JS_OPTIMIZED_ARGUMENTS)) {
        
        BaselineFrame *frame = GetTopBaselineFrame(cx);
        if (IsOptimizedArguments(frame, val.address())) {
            
            res.setInt32(frame->numActualArgs());
            types::TypeScript::Monitor(cx, script, pc, res);
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
    
    if (op == JSOP_CALLPROP && JS_UNLIKELY(res.isPrimitive())) {
        if (!OnUnknownMethod(cx, obj, IdToValue(id), res))
            return false;
    }
#endif

    types::TypeScript::Monitor(cx, script, pc, res);

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

    if (!TryAttachNativeGetPropStub(cx, script, stub, name, val, res, &attached))
        return false;
    if (attached)
        return true;

    return true;
}

typedef bool (*DoGetPropFallbackFn)(JSContext *, ICGetProp_Fallback *, MutableHandleValue,
                                    MutableHandleValue);
static const VMFunction DoGetPropFallbackInfo = FunctionInfo<DoGetPropFallbackFn>(DoGetPropFallback);

bool
ICGetProp_Fallback::Compiler::generateStubCode(MacroAssembler &masm)
{
    JS_ASSERT(R0 == JSReturnOperand);

    EmitRestoreTailCallReg(masm);

    masm.pushValue(R0);
    masm.push(BaselineStubReg);

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


static bool
TryAttachSetPropStub(JSContext *cx, HandleScript script, ICSetProp_Fallback *stub,
                     HandleObject obj, HandleId id, bool *attached)
{
    JS_ASSERT(!*attached);

    if (!obj->isNative() || obj->watched())
        return true;

    
    RootedShape shape(cx, obj->nativeLookup(cx, id));
    if (!shape || !shape->hasSlot() || !shape->hasDefaultSetter() || !shape->writable())
        return true;

    bool isFixedSlot = obj->isFixedSlot(shape->slot());
    uint32_t offset = isFixedSlot
                      ? JSObject::getFixedSlotOffset(shape->slot())
                      : obj->dynamicSlotIndex(shape->slot()) * sizeof(Value);


    RootedTypeObject type(cx, obj->getType(cx));
    ICSetProp_Native::Compiler compiler(cx, type, obj->lastProperty(), isFixedSlot, offset);
    ICStub *newStub = compiler.getStub(ICStubSpace::StubSpaceFor(script));
    if (!newStub)
        return false;

    stub->addNewStub(newStub);
    *attached = true;
    return true;
}





static bool
DoSetPropFallback(JSContext *cx, ICSetProp_Fallback *stub, HandleValue lhs, HandleValue rhs,
                  MutableHandleValue res)
{
    RootedScript script(cx, GetTopIonJSScript(cx));
    jsbytecode *pc = stub->icEntry()->pc(script);
    mozilla::DebugOnly<JSOp> op = JSOp(*pc);
    FallbackICSpew(cx, stub, "SetProp(%s)", js_CodeName[op]);

    JS_ASSERT(op == JSOP_SETPROP || op == JSOP_SETNAME || op == JSOP_SETGNAME);

    RootedPropertyName name(cx, script->getName(pc));
    RootedId id(cx, NameToId(name));

    RootedObject obj(cx, ToObjectFromStack(cx, lhs));
    if (!obj)
        return false;

    if (script->strict) {
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
    if (!TryAttachSetPropStub(cx, script, stub, obj, id, &attached))
        return false;
    if (attached)
        return true;

    return true;
}

typedef bool (*DoSetPropFallbackFn)(JSContext *, ICSetProp_Fallback *, HandleValue, HandleValue,
                                    MutableHandleValue);
static const VMFunction DoSetPropFallbackInfo =
    FunctionInfo<DoSetPropFallbackFn>(DoSetPropFallback);

bool
ICSetProp_Fallback::Compiler::generateStubCode(MacroAssembler &masm)
{
    JS_ASSERT(R0 == JSReturnOperand);

    EmitRestoreTailCallReg(masm);

    masm.pushValue(R1);
    masm.pushValue(R0);
    masm.push(BaselineStubReg);

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
    masm.patchableCallPreBarrier(BaseIndex(objReg, scratch, TimesOne), MIRType_Value);
    masm.storeValue(R1, BaseIndex(objReg, scratch, TimesOne));

    
    masm.moveValue(R1, R0);
    EmitReturnFromIC(masm);

    
    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}





static bool
TryAttachCallStub(JSContext *cx, ICCall_Fallback *stub, HandleScript script, JSOp op,
                  uint32_t argc, Value *vp, MutableHandleValue res, bool *attachedStub)
{
    *attachedStub = false;

    RootedValue callee(cx, vp[0]);
    RootedValue thisv(cx, vp[1]);

    if (stub->numOptimizedStubs() >= ICCall_Fallback::MAX_OPTIMIZED_STUBS) {
        
        return true;
    }

    if (op == JSOP_NEW || !callee.isObject())
        return true;

    RootedObject obj(cx, &callee.toObject());
    if (!obj->isFunction())
        return true;

    RootedFunction fun(cx, obj->toFunction());
    if (fun->hasScript()) {
        RootedScript calleeScript(cx, fun->nonLazyScript());
        if (!calleeScript->hasBaselineScript() && !calleeScript->hasIonScript())
            return true;

        ICCall_Scripted::Compiler compiler(cx, stub->fallbackMonitorStub()->firstMonitorStub(), fun);
        ICStub *newStub = compiler.getStub(ICStubSpace::StubSpaceFor(script));
        if (!newStub)
            return false;

        stub->addNewStub(newStub);
        *attachedStub = true;
        return true;
    }

    if (fun->isNative()) {
        ICCall_Native::Compiler compiler(cx, stub->fallbackMonitorStub()->firstMonitorStub(), fun);
        ICStub *newStub = compiler.getStub(ICStubSpace::StubSpaceFor(script));
        if (!newStub)
            return false;

        stub->addNewStub(newStub);
        *attachedStub = true;
        return true;
    }

    return true;
}

static bool
DoCallFallback(JSContext *cx, ICCall_Fallback *stub, uint32_t argc, Value *vp, MutableHandleValue res)
{
    RootedScript script(cx, GetTopIonJSScript(cx));
    jsbytecode *pc = stub->icEntry()->pc(script);
    JSOp op = JSOp(*pc);
    FallbackICSpew(cx, stub, "Call(%s)", js_CodeName[op]);

    RootedValue callee(cx, vp[0]);
    RootedValue thisv(cx, vp[1]);

    Value *args = vp + 2;

    if (op == JSOP_FUNAPPLY && argc == 2 && args[1].isMagic(JS_OPTIMIZED_ARGUMENTS)) {
        BaselineFrame *frame = GetTopBaselineFrame(cx);
        GuardFunApplyArgumentsOptimization(cx, frame, callee, args, argc);
    }

    bool attachedStub;
    if (!TryAttachCallStub(cx, stub, script, op, argc, vp, res, &attachedStub))
        return false;
    

    if (op == JSOP_NEW) {
        if (!InvokeConstructor(cx, callee, argc, args, res.address()))
            return false;
    } else {
        JS_ASSERT(op == JSOP_CALL || op == JSOP_FUNCALL || op == JSOP_FUNAPPLY);
        if (!Invoke(cx, thisv, callee, argc, args, res.address()))
            return false;
    }

    types::TypeScript::Monitor(cx, script, pc, res);

    
    ICTypeMonitor_Fallback *typeMonFbStub = stub->fallbackMonitorStub();
    if (!typeMonFbStub->addMonitorStubForValue(cx, ICStubSpace::StubSpaceFor(script), res))
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

    
    
    masm.addPtr(Imm32(sizeof(void *) * 4), argPtr);

    
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

typedef bool (*DoCallFallbackFn)(JSContext *, ICCall_Fallback *, uint32_t, Value *, MutableHandleValue);
static const VMFunction DoCallFallbackInfo = FunctionInfo<DoCallFallbackFn>(DoCallFallback);

bool
ICCall_Fallback::Compiler::generateStubCode(MacroAssembler &masm)
{
    JS_ASSERT(R0 == JSReturnOperand);

    
    EmitEnterStubFrame(masm, R1.scratchReg());

    
    
    

    GeneralRegisterSet regs(availableGeneralRegs(0));
    regs.take(R0.scratchReg()); 

    pushCallArguments(masm, regs, R0.scratchReg());

    masm.push(BaselineStackReg);
    masm.push(R0.scratchReg());
    masm.push(BaselineStubReg);

    if (!callVM(DoCallFallbackInfo, masm))
        return false;

    EmitLeaveStubFrame(masm);
    EmitReturnFromIC(masm);

    
    
    
    returnOffset_ = masm.currentOffset();

    EmitLeaveStubFrame(masm, true);

    
    
    
    
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

bool
ICCall_Scripted::Compiler::generateStubCode(MacroAssembler &masm)
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
    Address expectedCallee(BaselineStubReg, ICCall_Scripted::offsetOfCallee());
    masm.branchPtr(Assembler::NotEqual, expectedCallee, callee, &failure);

    
    
    masm.loadPtr(Address(callee, offsetof(JSFunction, u.i.script_)), callee);

    
    Register loadScratch = regs.takeAny();
    masm.loadBaselineOrIonCode(callee, loadScratch, &failure);
    regs.add(loadScratch);

    
    Register code = regs.takeAny();
    masm.loadPtr(Address(callee, IonCode::offsetOfCode()), code);

    
    regs.add(R1);

    
    Register scratch = regs.takeAny();
    EmitEnterStubFrame(masm, scratch);
    if (canUseTailCallReg)
        regs.add(BaselineTailCallReg);

    
    
    
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

        IonCode *argumentsRectifier = cx->compartment->ionCompartment()->getArgumentsRectifier();

        masm.movePtr(ImmGCPtr(argumentsRectifier), code);
        masm.loadPtr(Address(code, IonCode::offsetOfCode()), code);
        masm.mov(argcReg, ArgumentsRectifierReg);
    }

    masm.bind(&noUnderflow);
    masm.callIon(code);

    EmitLeaveStubFrame(masm, true);

    
    EmitEnterTypeMonitorIC(masm);

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

    
    
    EmitEnterStubFrame(masm, regs.getAny());

    
    
    
    pushCallArguments(masm, regs, argcReg);

    masm.checkStackAlignment();

    
    
    
    
    
    

    
    Register vpReg = regs.takeAny();
    masm.movePtr(StackPointer, vpReg);

    
    masm.push(argcReg);

    Register scratch = regs.takeAny();
    EmitCreateStubFrameDescriptor(masm, scratch);
    masm.push(scratch);
    masm.push(BaselineTailCallReg);
    masm.enterFakeExitFrame();

    
    masm.setupUnalignedABICall(3, scratch);
    masm.loadJSContext(scratch);
    masm.passABIArg(scratch);
    masm.passABIArg(argcReg);
    masm.passABIArg(vpReg);
    masm.callWithABI(Address(callee, JSFunction::offsetOfNativeOrScript()));

    
    Label success, exception;
    masm.branchTest32(Assembler::Zero, ReturnReg, ReturnReg, &exception);

    
    masm.loadValue(Address(StackPointer, IonNativeExitFrameLayout::offsetOfResult()), R0);

    EmitLeaveStubFrame(masm);

    
    EmitEnterTypeMonitorIC(masm);

    
    masm.bind(&exception);
    masm.handleException();

    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}

} 
} 
