





#include "jit/BaselineIC.h"
#include "jit/BaselineJIT.h"
#include "jit/CompileInfo.h"
#include "jit/IonSpewer.h"
#include "vm/ArgumentsObject.h"

#include "jsscriptinlines.h"

#include "jit/IonFrames-inl.h"

using namespace js;
using namespace js::jit;




template <typename T>
class BufferPointer
{
    BaselineBailoutInfo **header_;
    size_t offset_;
    bool heap_;

  public:
    BufferPointer(BaselineBailoutInfo **header, size_t offset, bool heap)
      : header_(header), offset_(offset), heap_(heap)
    { }

    T *get() const {
        BaselineBailoutInfo *header = *header_;
        if (!heap_)
            return (T*)(header->incomingStack + offset_);

        uint8_t *p = header->copyStackTop - offset_;
        JS_ASSERT(p >= header->copyStackBottom && p < header->copyStackTop);
        return (T*)p;
    }

    T &operator*() const { return *get(); }
    T *operator->() const { return get(); }
};



















struct BaselineStackBuilder
{
    IonBailoutIterator &iter_;
    IonJSFrameLayout *frame_;

    static size_t HeaderSize() {
        return AlignBytes(sizeof(BaselineBailoutInfo), sizeof(void *));
    };
    size_t bufferTotal_;
    size_t bufferAvail_;
    size_t bufferUsed_;
    uint8_t *buffer_;
    BaselineBailoutInfo *header_;

    size_t framePushed_;

    BaselineStackBuilder(IonBailoutIterator &iter, size_t initialSize)
      : iter_(iter),
        frame_(static_cast<IonJSFrameLayout*>(iter.current())),
        bufferTotal_(initialSize),
        bufferAvail_(0),
        bufferUsed_(0),
        buffer_(nullptr),
        header_(nullptr),
        framePushed_(0)
    {
        JS_ASSERT(bufferTotal_ >= HeaderSize());
    }

    ~BaselineStackBuilder() {
        js_free(buffer_);
    }

    bool init() {
        JS_ASSERT(!buffer_);
        JS_ASSERT(bufferUsed_ == 0);
        buffer_ = reinterpret_cast<uint8_t *>(js_calloc(bufferTotal_));
        if (!buffer_)
            return false;
        bufferAvail_ = bufferTotal_ - HeaderSize();
        bufferUsed_ = 0;

        header_ = reinterpret_cast<BaselineBailoutInfo *>(buffer_);
        header_->incomingStack = reinterpret_cast<uint8_t *>(frame_);
        header_->copyStackTop = buffer_ + bufferTotal_;
        header_->copyStackBottom = header_->copyStackTop;
        header_->setR0 = 0;
        header_->valueR0 = UndefinedValue();
        header_->setR1 = 0;
        header_->valueR1 = UndefinedValue();
        header_->resumeFramePtr = nullptr;
        header_->resumeAddr = nullptr;
        header_->monitorStub = nullptr;
        header_->numFrames = 0;
        return true;
    }

    bool enlarge() {
        JS_ASSERT(buffer_ != nullptr);
        if (bufferTotal_ & mozilla::tl::MulOverflowMask<2>::value)
            return false;
        size_t newSize = bufferTotal_ * 2;
        uint8_t *newBuffer = reinterpret_cast<uint8_t *>(js_calloc(newSize));
        if (!newBuffer)
            return false;
        memcpy((newBuffer + newSize) - bufferUsed_, header_->copyStackBottom, bufferUsed_);
        memcpy(newBuffer, header_, sizeof(BaselineBailoutInfo));
        js_free(buffer_);
        buffer_ = newBuffer;
        bufferTotal_ = newSize;
        bufferAvail_ = newSize - (HeaderSize() + bufferUsed_);

        header_ = reinterpret_cast<BaselineBailoutInfo *>(buffer_);
        header_->copyStackTop = buffer_ + bufferTotal_;
        header_->copyStackBottom = header_->copyStackTop - bufferUsed_;
        return true;
    }

    BaselineBailoutInfo *info() {
        JS_ASSERT(header_ == reinterpret_cast<BaselineBailoutInfo *>(buffer_));
        return header_;
    }

    BaselineBailoutInfo *takeBuffer() {
        JS_ASSERT(header_ == reinterpret_cast<BaselineBailoutInfo *>(buffer_));
        buffer_ = nullptr;
        return header_;
    }

    void resetFramePushed() {
        framePushed_ = 0;
    }

    size_t framePushed() const {
        return framePushed_;
    }

    bool subtract(size_t size, const char *info = nullptr) {
        
        while (size > bufferAvail_) {
            if (!enlarge())
                return false;
        }

        
        header_->copyStackBottom -= size;
        bufferAvail_ -= size;
        bufferUsed_ += size;
        framePushed_ += size;
        if (info) {
            IonSpew(IonSpew_BaselineBailouts,
                    "      SUB_%03d   %p/%p %-15s",
                    (int) size, header_->copyStackBottom, virtualPointerAtStackOffset(0), info);
        }
        return true;
    }

    template <typename T>
    bool write(const T &t) {
        if (!subtract(sizeof(T)))
            return false;
        memcpy(header_->copyStackBottom, &t, sizeof(T));
        return true;
    }

    template <typename T>
    bool writePtr(T *t, const char *info) {
        if (!write<T *>(t))
            return false;
        if (info)
            IonSpew(IonSpew_BaselineBailouts,
                    "      WRITE_PTR %p/%p %-15s %p",
                    header_->copyStackBottom, virtualPointerAtStackOffset(0), info, t);
        return true;
    }

    bool writeWord(size_t w, const char *info) {
        if (!write<size_t>(w))
            return false;
        if (info) {
            if (sizeof(size_t) == 4) {
                IonSpew(IonSpew_BaselineBailouts,
                        "      WRITE_WRD %p/%p %-15s %08x",
                        header_->copyStackBottom, virtualPointerAtStackOffset(0), info, w);
            } else {
                IonSpew(IonSpew_BaselineBailouts,
                        "      WRITE_WRD %p/%p %-15s %016llx",
                        header_->copyStackBottom, virtualPointerAtStackOffset(0), info, w);
            }
        }
        return true;
    }

    bool writeValue(Value val, const char *info) {
        if (!write<Value>(val))
            return false;
        if (info) {
            IonSpew(IonSpew_BaselineBailouts,
                    "      WRITE_VAL %p/%p %-15s %016llx",
                    header_->copyStackBottom, virtualPointerAtStackOffset(0), info,
                    *((uint64_t *) &val));
        }
        return true;
    }

    Value popValue() {
        JS_ASSERT(bufferUsed_ >= sizeof(Value));
        JS_ASSERT(framePushed_ >= sizeof(Value));
        bufferAvail_ += sizeof(Value);
        bufferUsed_ -= sizeof(Value);
        framePushed_ -= sizeof(Value);
        Value result = *((Value *) header_->copyStackBottom);
        header_->copyStackBottom += sizeof(Value);
        return result;
    }

    void popValueInto(PCMappingSlotInfo::SlotLocation loc) {
        JS_ASSERT(PCMappingSlotInfo::ValidSlotLocation(loc));
        switch(loc) {
          case PCMappingSlotInfo::SlotInR0:
            header_->setR0 = 1;
            header_->valueR0 = popValue();
            break;
          case PCMappingSlotInfo::SlotInR1:
            header_->setR1 = 1;
            header_->valueR1 = popValue();
            break;
          default:
            JS_ASSERT(loc == PCMappingSlotInfo::SlotIgnore);
            popValue();
            break;
        }
    }

    void setResumeFramePtr(void *resumeFramePtr) {
        header_->resumeFramePtr = resumeFramePtr;
    }

    void setResumeAddr(void *resumeAddr) {
        header_->resumeAddr = resumeAddr;
    }

    void setMonitorStub(ICStub *stub) {
        header_->monitorStub = stub;
    }

    template <typename T>
    BufferPointer<T> pointerAtStackOffset(size_t offset) {
        if (offset < bufferUsed_) {
            
            offset = header_->copyStackTop - (header_->copyStackBottom + offset);
            return BufferPointer<T>(&header_, offset,  true);
        }

        return BufferPointer<T>(&header_, offset - bufferUsed_,  false);
    }

    BufferPointer<Value> valuePointerAtStackOffset(size_t offset) {
        return pointerAtStackOffset<Value>(offset);
    }

    inline uint8_t *virtualPointerAtStackOffset(size_t offset) {
        if (offset < bufferUsed_)
            return reinterpret_cast<uint8_t *>(frame_) - (bufferUsed_ - offset);
        return reinterpret_cast<uint8_t *>(frame_) + (offset - bufferUsed_);
    }

    inline IonJSFrameLayout *startFrame() {
        return frame_;
    }

    BufferPointer<IonJSFrameLayout> topFrameAddress() {
        return pointerAtStackOffset<IonJSFrameLayout>(0);
    }

    
    
    
    
    
    
    
    
    
    
    
    void *calculatePrevFramePtr() {
        
        BufferPointer<IonJSFrameLayout> topFrame = topFrameAddress();
        FrameType type = topFrame->prevType();

        
        
        
        if (type == IonFrame_OptimizedJS || type == IonFrame_Entry)
            return nullptr;

        
        
        
        
        if (type == IonFrame_BaselineStub) {
            size_t offset = IonJSFrameLayout::Size() + topFrame->prevFrameLocalSize() +
                            IonBaselineStubFrameLayout::reverseOffsetOfSavedFramePtr();
            return virtualPointerAtStackOffset(offset);
        }

        JS_ASSERT(type == IonFrame_Rectifier);
        
        
        
        
        size_t priorOffset = IonJSFrameLayout::Size() + topFrame->prevFrameLocalSize();
#if defined(JS_CPU_X86)
        
        JS_ASSERT(BaselineFrameReg == FramePointer);
        priorOffset -= sizeof(void *);
        return virtualPointerAtStackOffset(priorOffset);
#elif defined(JS_CPU_X64) || defined(JS_CPU_ARM)
        
        
        BufferPointer<IonRectifierFrameLayout> priorFrame =
            pointerAtStackOffset<IonRectifierFrameLayout>(priorOffset);
        FrameType priorType = priorFrame->prevType();
        JS_ASSERT(priorType == IonFrame_OptimizedJS || priorType == IonFrame_BaselineStub);

        
        
        if (priorType == IonFrame_OptimizedJS)
            return nullptr;

        
        
        
        
        
        size_t extraOffset = IonRectifierFrameLayout::Size() + priorFrame->prevFrameLocalSize() +
                             IonBaselineStubFrameLayout::reverseOffsetOfSavedFramePtr();
        return virtualPointerAtStackOffset(priorOffset + extraOffset);
#else
#  error "Bad architecture!"
#endif
    }
};

static inline bool
IsInlinableFallback(ICFallbackStub *icEntry)
{
    return icEntry->isCall_Fallback() || icEntry->isGetProp_Fallback() ||
           icEntry->isSetProp_Fallback();
}

static inline void*
GetStubReturnAddress(JSContext *cx, jsbytecode *pc)
{
    if (IsGetPropPC(pc))
        return cx->compartment()->ionCompartment()->baselineGetPropReturnAddr();
    if (IsSetPropPC(pc))
        return cx->compartment()->ionCompartment()->baselineSetPropReturnAddr();
    
    JS_ASSERT(IsCallPC(pc));
    return cx->compartment()->ionCompartment()->baselineCallReturnAddr();
}









































































static bool
InitFromBailout(JSContext *cx, HandleScript caller, jsbytecode *callerPC,
                HandleFunction fun, HandleScript script, IonScript *ionScript,
                SnapshotIterator &iter, bool invalidate, BaselineStackBuilder &builder,
                AutoValueVector &startFrameFormals, MutableHandleFunction nextCallee,
                jsbytecode **callPC, const ExceptionBailoutInfo *excInfo)
{
    
    
    
    uint32_t exprStackSlots;
    if (excInfo)
        exprStackSlots = excInfo->numExprSlots;
    else
        exprStackSlots = iter.slots() - (script->nfixed + CountArgSlots(script, fun));

    builder.resetFramePushed();

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    IonSpew(IonSpew_BaselineBailouts, "      Unpacking %s:%d", script->filename(), script->lineno);
    IonSpew(IonSpew_BaselineBailouts, "      [BASELINE-JS FRAME]");

    
    
    
    
    void *prevFramePtr = builder.calculatePrevFramePtr();
    if (!builder.writePtr(prevFramePtr, "PrevFramePtr"))
        return false;
    prevFramePtr = builder.virtualPointerAtStackOffset(0);

    
    if (!builder.subtract(BaselineFrame::Size(), "BaselineFrame"))
        return false;
    BufferPointer<BaselineFrame> blFrame = builder.pointerAtStackOffset<BaselineFrame>(0);

    
    uint32_t frameSize = BaselineFrame::Size() + BaselineFrame::FramePointerOffset +
                         (sizeof(Value) * (script->nfixed + exprStackSlots));
    IonSpew(IonSpew_BaselineBailouts, "      FrameSize=%d", (int) frameSize);
    blFrame->setFrameSize(frameSize);

    uint32_t flags = 0;

    
    
    
    if (cx->runtime()->spsProfiler.enabled() && ionScript->hasSPSInstrumentation()) {
        IonSpew(IonSpew_BaselineBailouts, "      Setting SPS flag on frame!");
        flags |= BaselineFrame::HAS_PUSHED_SPS_FRAME;
    }

    
    JSObject *scopeChain = nullptr;
    ArgumentsObject *argsObj = nullptr;
    BailoutKind bailoutKind = iter.bailoutKind();
    if (bailoutKind == Bailout_ArgumentCheck) {
        
        
        
        
        IonSpew(IonSpew_BaselineBailouts, "      Bailout_ArgumentCheck! (no valid scopeChain)");
        iter.skip();

        
        if (script->argumentsHasVarBinding()) {
            IonSpew(IonSpew_BaselineBailouts,
                    "      Bailout_ArgumentCheck for script with argumentsHasVarBinding!"
                    "Using empty arguments object");
            iter.skip();
        }
    } else {
        Value v = iter.read();
        if (v.isObject()) {
            scopeChain = &v.toObject();
            if (fun && fun->isHeavyweight())
                flags |= BaselineFrame::HAS_CALL_OBJ;
        } else {
            JS_ASSERT(v.isUndefined());

            
            if (fun) {
                
                
                
                if (iter.pcOffset() != 0 || iter.resumeAfter())
                    scopeChain = fun->environment();
            } else {
                
                
                
                
                
                JS_ASSERT(!script->isForEval());
                JS_ASSERT(script->compileAndGo);
                scopeChain = &(script->global());
            }
        }

        
        if (script->argumentsHasVarBinding()) {
            v = iter.read();
            JS_ASSERT(v.isObject() || v.isUndefined());
            if (v.isObject())
                argsObj = &v.toObject().as<ArgumentsObject>();
        }
    }
    IonSpew(IonSpew_BaselineBailouts, "      ScopeChain=%p", scopeChain);
    blFrame->setScopeChain(scopeChain);

    

    blFrame->setFlags(flags);

    
    if (argsObj)
        blFrame->initArgsObjUnchecked(*argsObj);

    
    
    blFrame->setBlockChainNull();

    if (fun) {
        
        
        Value thisv = iter.read();
        IonSpew(IonSpew_BaselineBailouts, "      Is function!");
        IonSpew(IonSpew_BaselineBailouts, "      thisv=%016llx", *((uint64_t *) &thisv));

        size_t thisvOffset = builder.framePushed() + IonJSFrameLayout::offsetOfThis();
        *builder.valuePointerAtStackOffset(thisvOffset) = thisv;

        JS_ASSERT(iter.slots() >= CountArgSlots(script, fun));
        IonSpew(IonSpew_BaselineBailouts, "      frame slots %u, nargs %u, nfixed %u",
                iter.slots(), fun->nargs, script->nfixed);

        if (!callerPC) {
            
            
            
            
            
            JS_ASSERT(startFrameFormals.empty());
            if (!startFrameFormals.resize(fun->nargs))
                return false;
        }

        for (uint32_t i = 0; i < fun->nargs; i++) {
            Value arg = iter.read();
            IonSpew(IonSpew_BaselineBailouts, "      arg %d = %016llx",
                        (int) i, *((uint64_t *) &arg));
            if (callerPC) {
                size_t argOffset = builder.framePushed() + IonJSFrameLayout::offsetOfActualArg(i);
                *builder.valuePointerAtStackOffset(argOffset) = arg;
            } else {
                startFrameFormals[i] = arg;
            }
        }
    }

    for (uint32_t i = 0; i < script->nfixed; i++) {
        Value slot = iter.read();
        if (!builder.writeValue(slot, "FixedValue"))
            return false;
    }

    
    
    jsbytecode *pc = excInfo ? excInfo->resumePC : script->code + iter.pcOffset();
    bool resumeAfter = excInfo ? false : iter.resumeAfter();

    JSOp op = JSOp(*pc);
    JS_ASSERT_IF(excInfo, op == JSOP_ENTERBLOCK);

    
    
    uint32_t pushedSlots = 0;
    AutoValueVector savedCallerArgs(cx);
    bool needToSaveArgs = op == JSOP_FUNAPPLY || IsGetPropPC(pc) || IsSetPropPC(pc);
    if (iter.moreFrames() && (op == JSOP_FUNCALL || needToSaveArgs))
    {
        uint32_t inlined_args = 0;
        if (op == JSOP_FUNCALL)
            inlined_args = 2 + GET_ARGC(pc) - 1;
        else if (op == JSOP_FUNAPPLY)
            inlined_args = 2 + blFrame->numActualArgs();
        else
            inlined_args = 2 + IsSetPropPC(pc);

        JS_ASSERT(exprStackSlots >= inlined_args);
        pushedSlots = exprStackSlots - inlined_args;

        IonSpew(IonSpew_BaselineBailouts,
                "      pushing %u expression stack slots before fixup",
                pushedSlots);
        for (uint32_t i = 0; i < pushedSlots; i++) {
            Value v = iter.read();
            if (!builder.writeValue(v, "StackValue"))
                return false;
        }

        if (op == JSOP_FUNCALL) {
            
            
            
            
            
            IonSpew(IonSpew_BaselineBailouts, "      pushing undefined to fixup funcall");
            if (!builder.writeValue(UndefinedValue(), "StackValue"))
                return false;
        }

        if (needToSaveArgs) {
            
            
            

            
            
            
            
            
            
            
            if (op == JSOP_FUNAPPLY) {
                IonSpew(IonSpew_BaselineBailouts, "      pushing 4x undefined to fixup funapply");
                if (!builder.writeValue(UndefinedValue(), "StackValue"))
                    return false;
                if (!builder.writeValue(UndefinedValue(), "StackValue"))
                    return false;
                if (!builder.writeValue(UndefinedValue(), "StackValue"))
                    return false;
                if (!builder.writeValue(UndefinedValue(), "StackValue"))
                    return false;
            }
            
            
            if (!savedCallerArgs.resize(inlined_args))
                return false;
            for (uint32_t i = 0; i < inlined_args; i++)
                savedCallerArgs[i] = iter.read();

            if (IsSetPropPC(pc)) {
                
                
                
                
                
                
                Value initialArg = savedCallerArgs[inlined_args - 1];
                IonSpew(IonSpew_BaselineBailouts, "     pushing setter's initial argument");
                if (!builder.writeValue(initialArg, "StackValue"))
                    return false;
            }
            pushedSlots = exprStackSlots;
        }
    }

    IonSpew(IonSpew_BaselineBailouts, "      pushing %u expression stack slots",
                                      exprStackSlots - pushedSlots);
    for (uint32_t i = pushedSlots; i < exprStackSlots; i++) {
        Value v;

        
        
        
        if (!iter.moreFrames() && i == exprStackSlots - 1 &&
            cx->runtime()->hasIonReturnOverride())
        {
            JS_ASSERT(invalidate);
            iter.skip();
            IonSpew(IonSpew_BaselineBailouts, "      [Return Override]");
            v = cx->runtime()->takeIonReturnOverride();
        } else {
            v = iter.read();
        }
        if (!builder.writeValue(v, "StackValue"))
            return false;
    }

    size_t endOfBaselineJSFrameStack = builder.framePushed();

    
    
    
    if (!resumeAfter) {
        while (true) {
            op = JSOp(*pc);
            if (op == JSOP_GOTO)
                pc += GET_JUMP_OFFSET(pc);
            else if (op == JSOP_LOOPENTRY || op == JSOP_NOP || op == JSOP_LOOPHEAD)
                pc = GetNextPc(pc);
            else
                break;
        }
    }

    uint32_t pcOff = pc - script->code;
    bool isCall = IsCallPC(pc);
    BaselineScript *baselineScript = script->baselineScript();

#ifdef DEBUG
    uint32_t expectedDepth = js_ReconstructStackDepth(cx, script,
                                                      resumeAfter ? GetNextPc(pc) : pc);
    if (op != JSOP_FUNAPPLY || !iter.moreFrames() || resumeAfter) {
        if (op == JSOP_FUNCALL) {
            
            
            
            JS_ASSERT(expectedDepth - exprStackSlots <= 1);
        } else if (iter.moreFrames() && (IsGetPropPC(pc) || IsSetPropPC(pc))) {
            
            
            
            
            
            
            
            
            JS_ASSERT(exprStackSlots - expectedDepth == 1);
        } else {
            
            
            
            
            JS_ASSERT(exprStackSlots == expectedDepth);
        }
    }

    IonSpew(IonSpew_BaselineBailouts, "      Resuming %s pc offset %d (op %s) (line %d) of %s:%d",
                resumeAfter ? "after" : "at", (int) pcOff, js_CodeName[op],
                PCToLineNumber(script, pc), script->filename(), (int) script->lineno);
    IonSpew(IonSpew_BaselineBailouts, "      Bailout kind: %s",
            BailoutKindString(bailoutKind));
#endif

    
    
    if (!iter.moreFrames() || excInfo) {
        
        *callPC = nullptr;

        
        
        
        bool enterMonitorChain = false;
        if (resumeAfter && (js_CodeSpec[op].format & JOF_TYPESET)) {
            
            
            
            ICEntry &icEntry = baselineScript->icEntryFromPCOffset(pcOff);
            ICFallbackStub *fallbackStub = icEntry.firstStub()->getChainFallback();
            if (fallbackStub->isMonitoredFallback())
                enterMonitorChain = true;
        }

        uint32_t numCallArgs = isCall ? GET_ARGC(pc) : 0;

        if (resumeAfter && !enterMonitorChain)
            pc = GetNextPc(pc);

        builder.setResumeFramePtr(prevFramePtr);

        if (enterMonitorChain) {
            ICEntry &icEntry = baselineScript->icEntryFromPCOffset(pcOff);
            ICFallbackStub *fallbackStub = icEntry.firstStub()->getChainFallback();
            JS_ASSERT(fallbackStub->isMonitoredFallback());
            IonSpew(IonSpew_BaselineBailouts, "      [TYPE-MONITOR CHAIN]");
            ICMonitoredFallbackStub *monFallbackStub = fallbackStub->toMonitoredFallbackStub();
            ICStub *firstMonStub = monFallbackStub->fallbackMonitorStub()->firstMonitorStub();

            
            IonSpew(IonSpew_BaselineBailouts, "      Popping top stack value into R0.");
            builder.popValueInto(PCMappingSlotInfo::SlotInR0);

            
            
            frameSize -= sizeof(Value);
            blFrame->setFrameSize(frameSize);
            IonSpew(IonSpew_BaselineBailouts, "      Adjusted framesize -= %d: %d",
                            (int) sizeof(Value), (int) frameSize);

            
            
            
            
            if (isCall) {
                builder.writeValue(UndefinedValue(), "CallOp FillerCallee");
                builder.writeValue(UndefinedValue(), "CallOp FillerThis");
                for (uint32_t i = 0; i < numCallArgs; i++)
                    builder.writeValue(UndefinedValue(), "CallOp FillerArg");

                frameSize += (numCallArgs + 2) * sizeof(Value);
                blFrame->setFrameSize(frameSize);
                IonSpew(IonSpew_BaselineBailouts, "      Adjusted framesize += %d: %d",
                                (int) ((numCallArgs + 2) * sizeof(Value)), (int) frameSize);
            }

            
            
            builder.setResumeAddr(baselineScript->returnAddressForIC(icEntry));
            builder.setMonitorStub(firstMonStub);
            IonSpew(IonSpew_BaselineBailouts, "      Set resumeAddr=%p monitorStub=%p",
                    baselineScript->returnAddressForIC(icEntry), firstMonStub);

        } else {
            
            
            PCMappingSlotInfo slotInfo;
            uint8_t *nativeCodeForPC = baselineScript->nativeCodeForPC(script, pc, &slotInfo);
            unsigned numUnsynced = slotInfo.numUnsynced();
            JS_ASSERT(numUnsynced <= 2);
            PCMappingSlotInfo::SlotLocation loc1, loc2;
            if (numUnsynced > 0) {
                loc1 = slotInfo.topSlotLocation();
                IonSpew(IonSpew_BaselineBailouts, "      Popping top stack value into %d.",
                            (int) loc1);
                builder.popValueInto(loc1);
            }
            if (numUnsynced > 1) {
                loc2 = slotInfo.nextSlotLocation();
                IonSpew(IonSpew_BaselineBailouts, "      Popping next stack value into %d.",
                            (int) loc2);
                JS_ASSERT_IF(loc1 != PCMappingSlotInfo::SlotIgnore, loc1 != loc2);
                builder.popValueInto(loc2);
            }

            
            
            frameSize -= sizeof(Value) * numUnsynced;
            blFrame->setFrameSize(frameSize);
            IonSpew(IonSpew_BaselineBailouts, "      Adjusted framesize -= %d: %d",
                            int(sizeof(Value) * numUnsynced), int(frameSize));

            
            
            uint8_t *opReturnAddr;
            if (scopeChain == nullptr) {
                
                
                JS_ASSERT(fun);
                JS_ASSERT(numUnsynced == 0);
                opReturnAddr = baselineScript->prologueEntryAddr();
                IonSpew(IonSpew_BaselineBailouts, "      Resuming into prologue.");

                
                blFrame->unsetPushedSPSFrame();

                
                
                
                
                
                
                
                
                
                
                
                
                
                if (cx->runtime()->spsProfiler.enabled()) {
                    if (caller && bailoutKind == Bailout_ArgumentCheck) {
                        IonSpew(IonSpew_BaselineBailouts, "      Setting PCidx on innermost "
                                "inlined frame's parent's SPS entry (%s:%d) (pcIdx=%d)!",
                                caller->filename(), caller->lineno, callerPC - caller->code);
                        cx->runtime()->spsProfiler.updatePC(caller, callerPC);
                    } else if (bailoutKind != Bailout_ArgumentCheck) {
                        IonSpew(IonSpew_BaselineBailouts,
                                "      Popping SPS entry for innermost inlined frame's SPS entry");
                        cx->runtime()->spsProfiler.exit(cx, script, fun);
                    }
                }
            } else {
                opReturnAddr = nativeCodeForPC;
            }
            builder.setResumeAddr(opReturnAddr);
            IonSpew(IonSpew_BaselineBailouts, "      Set resumeAddr=%p", opReturnAddr);
        }

        return true;
    }

    *callPC = pc;

    
    size_t baselineFrameDescr = MakeFrameDescriptor((uint32_t) builder.framePushed(),
                                                    IonFrame_BaselineJS);
    if (!builder.writeWord(baselineFrameDescr, "Descriptor"))
        return false;

    
    
    ICEntry &icEntry = baselineScript->icEntryFromPCOffset(pcOff);
    JS_ASSERT(IsInlinableFallback(icEntry.firstStub()->getChainFallback()));
    if (!builder.writePtr(baselineScript->returnAddressForIC(icEntry), "ReturnAddr"))
        return false;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    IonSpew(IonSpew_BaselineBailouts, "      [BASELINE-STUB FRAME]");

    size_t startOfBaselineStubFrame = builder.framePushed();

    
    JS_ASSERT(IsInlinableFallback(icEntry.fallbackStub()));
    if (!builder.writePtr(icEntry.fallbackStub(), "StubPtr"))
        return false;

    
    if (!builder.writePtr(prevFramePtr, "PrevFramePtr"))
        return false;
    prevFramePtr = builder.virtualPointerAtStackOffset(0);

    
    
    JS_ASSERT(IsIonInlinablePC(pc));
    unsigned actualArgc;
    if (needToSaveArgs) {
        
        
        if (op == JSOP_FUNAPPLY)
            actualArgc = blFrame->numActualArgs();
        else
            actualArgc = IsSetPropPC(pc);

        JS_ASSERT(actualArgc + 2 <= exprStackSlots);
        JS_ASSERT(savedCallerArgs.length() == actualArgc + 2);
        for (unsigned i = 0; i < actualArgc + 1; i++) {
            size_t arg = savedCallerArgs.length() - (i + 1);
            if (!builder.writeValue(savedCallerArgs[arg], "ArgVal"))
                return false;
        }
    } else {
        actualArgc = GET_ARGC(pc);
        if (op == JSOP_FUNCALL) {
            JS_ASSERT(actualArgc > 0);
            actualArgc--;
        }

        JS_ASSERT(actualArgc + 2 <= exprStackSlots);
        for (unsigned i = 0; i < actualArgc + 1; i++) {
            size_t argSlot = (script->nfixed + exprStackSlots) - (i + 1);
            if (!builder.writeValue(*blFrame->valueSlot(argSlot), "ArgVal"))
                return false;
        }
    }

    
    
    size_t endOfBaselineStubArgs = builder.framePushed();

    
    size_t baselineStubFrameSize = builder.framePushed() - startOfBaselineStubFrame;
    size_t baselineStubFrameDescr = MakeFrameDescriptor((uint32_t) baselineStubFrameSize,
                                                        IonFrame_BaselineStub);

    
    if (!builder.writeWord(actualArgc, "ActualArgc"))
        return false;

    
    Value callee;
    if (needToSaveArgs) {
        
        
        callee = savedCallerArgs[0];
    } else {
        uint32_t calleeStackSlot = exprStackSlots - uint32_t(actualArgc + 2);
        size_t calleeOffset = (builder.framePushed() - endOfBaselineJSFrameStack)
            + ((exprStackSlots - (calleeStackSlot + 1)) * sizeof(Value));
        callee = *builder.valuePointerAtStackOffset(calleeOffset);
        IonSpew(IonSpew_BaselineBailouts, "      CalleeStackSlot=%d", (int) calleeStackSlot);
    }
    IonSpew(IonSpew_BaselineBailouts, "      Callee = %016llx", *((uint64_t *) &callee));
    JS_ASSERT(callee.isObject() && callee.toObject().is<JSFunction>());
    JSFunction *calleeFun = &callee.toObject().as<JSFunction>();
    if (!builder.writePtr(CalleeToToken(calleeFun), "CalleeToken"))
        return false;
    nextCallee.set(calleeFun);

    
    if (!builder.writeWord(baselineStubFrameDescr, "Descriptor"))
        return false;

    
    void *baselineCallReturnAddr = GetStubReturnAddress(cx, pc);
    JS_ASSERT(baselineCallReturnAddr);
    if (!builder.writePtr(baselineCallReturnAddr, "ReturnAddr"))
        return false;

    
    
    if (actualArgc >= calleeFun->nargs)
        return true;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    IonSpew(IonSpew_BaselineBailouts, "      [RECTIFIER FRAME]");

    size_t startOfRectifierFrame = builder.framePushed();

    
#if defined(JS_CPU_X86)
    if (!builder.writePtr(prevFramePtr, "PrevFramePtr-X86Only"))
        return false;
#endif

    
    for (unsigned i = 0; i < (calleeFun->nargs - actualArgc); i++) {
        if (!builder.writeValue(UndefinedValue(), "FillerVal"))
            return false;
    }

    
    if (!builder.subtract((actualArgc + 1) * sizeof(Value), "CopiedArgs"))
        return false;
    BufferPointer<uint8_t> stubArgsEnd =
        builder.pointerAtStackOffset<uint8_t>(builder.framePushed() - endOfBaselineStubArgs);
    IonSpew(IonSpew_BaselineBailouts, "      MemCpy from %p", stubArgsEnd.get());
    memcpy(builder.pointerAtStackOffset<uint8_t>(0).get(), stubArgsEnd.get(),
           (actualArgc + 1) * sizeof(Value));

    
    size_t rectifierFrameSize = builder.framePushed() - startOfRectifierFrame;
    size_t rectifierFrameDescr = MakeFrameDescriptor((uint32_t) rectifierFrameSize,
                                                     IonFrame_Rectifier);

    
    if (!builder.writeWord(actualArgc, "ActualArgc"))
        return false;

    
    if (!builder.writePtr(CalleeToToken(calleeFun), "CalleeToken"))
        return false;

    
    if (!builder.writeWord(rectifierFrameDescr, "Descriptor"))
        return false;

    
    
    void *rectReturnAddr = cx->runtime()->ionRuntime()->getArgumentsRectifierReturnAddr();
    JS_ASSERT(rectReturnAddr);
    if (!builder.writePtr(rectReturnAddr, "ReturnAddr"))
        return false;

    return true;
}

uint32_t
jit::BailoutIonToBaseline(JSContext *cx, JitActivation *activation, IonBailoutIterator &iter,
                          bool invalidate, BaselineBailoutInfo **bailoutInfo,
                          const ExceptionBailoutInfo *excInfo)
{
    JS_ASSERT(bailoutInfo != nullptr);
    JS_ASSERT(*bailoutInfo == nullptr);

#if JS_TRACE_LOGGING
    TraceLogging::defaultLogger()->log(TraceLogging::INFO_ENGINE_BASELINE);
#endif

    
    
    
    
    
    JS_ASSERT(iter.isOptimizedJS());
    FrameType prevFrameType = iter.prevType();
    JS_ASSERT(prevFrameType == IonFrame_OptimizedJS ||
              prevFrameType == IonFrame_BaselineStub ||
              prevFrameType == IonFrame_Entry ||
              prevFrameType == IonFrame_Rectifier);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    IonSpew(IonSpew_BaselineBailouts, "Bailing to baseline %s:%u (IonScript=%p) (FrameType=%d)",
            iter.script()->filename(), iter.script()->lineno, (void *) iter.ionScript(),
            (int) prevFrameType);

    if (excInfo)
        IonSpew(IonSpew_BaselineBailouts, "Resuming in catch or finally block");

    IonSpew(IonSpew_BaselineBailouts, "  Reading from snapshot offset %u size %u",
            iter.snapshotOffset(), iter.ionScript()->snapshotsSize());

    if (excInfo)
        iter.ionScript()->incNumExceptionBailouts();
    else
        iter.ionScript()->incNumBailouts();
    iter.script()->updateBaselineOrIonRaw();

    
    BaselineStackBuilder builder(iter, 1024);
    if (!builder.init())
        return BAILOUT_RETURN_FATAL_ERROR;
    IonSpew(IonSpew_BaselineBailouts, "  Incoming frame ptr = %p", builder.startFrame());

    SnapshotIterator snapIter(iter);

    RootedFunction callee(cx, iter.maybeCallee());
    if (callee) {
        IonSpew(IonSpew_BaselineBailouts, "  Callee function (%s:%u)",
                callee->existingScript()->filename(), callee->existingScript()->lineno);
    } else {
        IonSpew(IonSpew_BaselineBailouts, "  No callee!");
    }

    if (iter.isConstructing())
        IonSpew(IonSpew_BaselineBailouts, "  Constructing!");
    else
        IonSpew(IonSpew_BaselineBailouts, "  Not constructing!");

    IonSpew(IonSpew_BaselineBailouts, "  Restoring frames:");
    size_t frameNo = 0;

    
    RootedScript caller(cx);
    jsbytecode *callerPC = nullptr;
    RootedFunction fun(cx, callee);
    RootedScript scr(cx, iter.script());
    AutoValueVector startFrameFormals(cx);

    while (true) {
#if JS_TRACE_LOGGING
        if (frameNo > 0) {
            TraceLogging::defaultLogger()->log(TraceLogging::SCRIPT_START, scr);
            TraceLogging::defaultLogger()->log(TraceLogging::INFO_ENGINE_BASELINE);
        }
#endif
        IonSpew(IonSpew_BaselineBailouts, "    FrameNo %d", frameNo);

        
        
        bool handleException = (excInfo && excInfo->frameNo == frameNo);

        jsbytecode *callPC = nullptr;
        RootedFunction nextCallee(cx, nullptr);
        if (!InitFromBailout(cx, caller, callerPC, fun, scr, iter.ionScript(),
                             snapIter, invalidate, builder, startFrameFormals,
                             &nextCallee, &callPC, handleException ? excInfo : nullptr))
        {
            return BAILOUT_RETURN_FATAL_ERROR;
        }

        if (!snapIter.moreFrames()) {
            JS_ASSERT(!callPC);
            break;
        }

        if (handleException)
            break;

        JS_ASSERT(nextCallee);
        JS_ASSERT(callPC);
        caller = scr;
        callerPC = callPC;
        fun = nextCallee;
        scr = fun->existingScript();
        snapIter.nextFrame();

        frameNo++;
    }
    IonSpew(IonSpew_BaselineBailouts, "  Done restoring frames");
    BailoutKind bailoutKind = snapIter.bailoutKind();

    if (!startFrameFormals.empty()) {
        
        Value *argv = builder.startFrame()->argv() + 1; 
        mozilla::PodCopy(argv, startFrameFormals.begin(), startFrameFormals.length());
    }

    
    BaselineBailoutInfo *info = builder.takeBuffer();
    info->numFrames = frameNo + 1;

    
    bool overRecursed = false;
    uint8_t *newsp = info->incomingStack - (info->copyStackTop - info->copyStackBottom);
    JS_CHECK_RECURSION_WITH_SP_DONT_REPORT(cx, newsp, overRecursed = true);
    if (overRecursed) {
        IonSpew(IonSpew_BaselineBailouts, "  Overrecursion check failed!");
        return BAILOUT_RETURN_OVERRECURSED;
    }

    info->bailoutKind = bailoutKind;
    *bailoutInfo = info;
    return BAILOUT_RETURN_OK;
}

static bool
HandleBoundsCheckFailure(JSContext *cx, HandleScript outerScript, HandleScript innerScript)
{
    IonSpew(IonSpew_Bailouts, "Bounds check failure %s:%d, inlined into %s:%d",
            innerScript->filename(), innerScript->lineno,
            outerScript->filename(), outerScript->lineno);

    JS_ASSERT(!outerScript->ionScript()->invalidated());

    
    
    
    if (!outerScript->failedBoundsCheck) {
        outerScript->failedBoundsCheck = true;
    }
    IonSpew(IonSpew_BaselineBailouts, "Invalidating due to bounds check failure");
    return Invalidate(cx, outerScript);
}

static bool
HandleShapeGuardFailure(JSContext *cx, HandleScript outerScript, HandleScript innerScript)
{
    IonSpew(IonSpew_Bailouts, "Shape guard failure %s:%d, inlined into %s:%d",
            innerScript->filename(), innerScript->lineno,
            outerScript->filename(), outerScript->lineno);

    JS_ASSERT(!outerScript->ionScript()->invalidated());

    
    
    
    outerScript->failedShapeGuard = true;
    IonSpew(IonSpew_BaselineBailouts, "Invalidating due to shape guard failure");
    return Invalidate(cx, outerScript);
}

static bool
HandleBaselineInfoBailout(JSContext *cx, JSScript *outerScript, JSScript *innerScript)
{
    IonSpew(IonSpew_Bailouts, "Baseline info failure %s:%d, inlined into %s:%d",
            innerScript->filename(), innerScript->lineno,
            outerScript->filename(), outerScript->lineno);

    JS_ASSERT(!outerScript->ionScript()->invalidated());

    IonSpew(IonSpew_BaselineBailouts, "Invalidating due to invalid baseline info");
    return Invalidate(cx, outerScript);
}

uint32_t
jit::FinishBailoutToBaseline(BaselineBailoutInfo *bailoutInfo)
{
    
    
    JSContext *cx = GetIonContext()->cx;
    js::gc::AutoSuppressGC suppressGC(cx);

    IonSpew(IonSpew_BaselineBailouts, "  Done restoring frames");

    
#ifdef DEBUG
    jsbytecode *pc;
    cx->currentScript(&pc);
    IonSpew(IonSpew_BaselineBailouts, "  Got pc=%p", pc);
#endif

    uint32_t numFrames = bailoutInfo->numFrames;
    JS_ASSERT(numFrames > 0);
    BailoutKind bailoutKind = bailoutInfo->bailoutKind;

    
    js_free(bailoutInfo);
    bailoutInfo = nullptr;

    
    
    
    BaselineFrame *topFrame = GetTopBaselineFrame(cx);
    if (topFrame->scopeChain() && !EnsureHasScopeObjects(cx, topFrame))
        return false;

    
    
    RootedScript innerScript(cx, nullptr);
    RootedScript outerScript(cx, nullptr);

    JS_ASSERT(cx->currentlyRunningInJit());
    IonFrameIterator iter(cx->mainThread().ionTop);

    uint32_t frameno = 0;
    while (frameno < numFrames) {
        JS_ASSERT(!iter.isOptimizedJS());

        if (iter.isBaselineJS()) {
            BaselineFrame *frame = iter.baselineFrame();

            
            
            
            if (frame->scopeChain() && frame->script()->needsArgsObj()) {
                ArgumentsObject *argsObj;
                if (frame->hasArgsObj()) {
                    argsObj = &frame->argsObj();
                } else {
                    argsObj = ArgumentsObject::createExpected(cx, frame);
                    if (!argsObj)
                        return false;
                }

                
                
                
                
                RootedScript script(cx, frame->script());
                SetFrameArgumentsObject(cx, frame, script, argsObj);
            }

            if (frameno == 0)
                innerScript = frame->script();

            if (frameno == numFrames - 1)
                outerScript = frame->script();

            frameno++;
        }

        ++iter;
    }

    JS_ASSERT(innerScript);
    JS_ASSERT(outerScript);
    IonSpew(IonSpew_BaselineBailouts,
            "  Restored outerScript=(%s:%u,%u) innerScript=(%s:%u,%u) (bailoutKind=%u)",
            outerScript->filename(), outerScript->lineno, outerScript->getUseCount(),
            innerScript->filename(), innerScript->lineno, innerScript->getUseCount(),
            (unsigned) bailoutKind);

    switch (bailoutKind) {
      case Bailout_Normal:
        
        break;
      case Bailout_ArgumentCheck:
        
        break;
      case Bailout_BoundsCheck:
        if (!HandleBoundsCheckFailure(cx, outerScript, innerScript))
            return false;
        break;
      case Bailout_ShapeGuard:
        if (!HandleShapeGuardFailure(cx, outerScript, innerScript))
            return false;
        break;
      case Bailout_BaselineInfo:
        if (!HandleBaselineInfoBailout(cx, outerScript, innerScript))
            return false;
        break;
      default:
        MOZ_ASSUME_UNREACHABLE("Unknown bailout kind!");
    }

    if (!CheckFrequentBailouts(cx, outerScript))
        return false;

    return true;
}
