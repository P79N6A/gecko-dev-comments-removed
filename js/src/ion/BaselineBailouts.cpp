






#include "BaselineCompiler.h"
#include "BaselineIC.h"
#include "BaselineJIT.h"
#include "CompileInfo.h"
#include "IonSpewer.h"
#include "IonFrames-inl.h"

#include "vm/Stack-inl.h"

#include "jsopcodeinlines.h"

using namespace js;
using namespace js::ion;




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
        buffer_(NULL),
        header_(NULL),
        framePushed_(0)
    {
        JS_ASSERT(bufferTotal_ >= HeaderSize());
    }

    ~BaselineStackBuilder() {
        if (buffer_)
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
        header_->resumeFramePtr = NULL;
        header_->resumeAddr = NULL;
        header_->monitorStub = NULL;
        header_->numFrames = 0;
        return true;
    }

    bool enlarge() {
        JS_ASSERT(buffer_ != NULL);
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
        buffer_ = NULL;
        return header_;
    }

    void resetFramePushed() {
        framePushed_ = 0;
    }

    size_t framePushed() const {
        return framePushed_;
    }

    bool subtract(size_t size, const char *info=NULL) {
        
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
            return NULL;

        
        
        
        
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
            return NULL;

        
        
        
        
        
        size_t extraOffset = IonRectifierFrameLayout::Size() + priorFrame->prevFrameLocalSize() +
                             IonBaselineStubFrameLayout::reverseOffsetOfSavedFramePtr();
        return virtualPointerAtStackOffset(priorOffset + extraOffset);
#else
#  error "Bad architecture!"
#endif
    }
};









































































static bool
InitFromBailout(JSContext *cx, HandleScript caller, jsbytecode *callerPC,
                HandleFunction fun, HandleScript script, SnapshotIterator &iter,
                bool invalidate, BaselineStackBuilder &builder,
                MutableHandleFunction nextCallee, jsbytecode **callPC)
{
    uint32_t exprStackSlots = iter.slots() - (script->nfixed + CountArgSlots(fun));

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

    
    
    
    if (cx->runtime->spsProfiler.enabled()) {
        IonSpew(IonSpew_BaselineBailouts, "      Setting SPS flag on frame!");
        flags |= BaselineFrame::HAS_PUSHED_SPS_FRAME;
    }

    
    JSObject *scopeChain = NULL;
    BailoutKind bailoutKind = iter.bailoutKind();
    if (bailoutKind == Bailout_ArgumentCheck) {
        
        
        
        IonSpew(IonSpew_BaselineBailouts, "      Bailout_ArgumentCheck! (no valid scopeChain)");
        iter.skip();
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
    }
    IonSpew(IonSpew_BaselineBailouts, "      ScopeChain=%p", scopeChain);
    blFrame->setScopeChain(scopeChain);
    

    
    blFrame->setFlags(flags);

    
    
    blFrame->setBlockChainNull();

    if (fun) {
        
        
        Value thisv = iter.read();
        IonSpew(IonSpew_BaselineBailouts, "      Is function!");
        IonSpew(IonSpew_BaselineBailouts, "      thisv=%016llx", *((uint64_t *) &thisv));

        size_t thisvOffset = builder.framePushed() + IonJSFrameLayout::offsetOfThis();
        *builder.valuePointerAtStackOffset(thisvOffset) = thisv;

        JS_ASSERT(iter.slots() >= CountArgSlots(fun));
        IonSpew(IonSpew_BaselineBailouts, "      frame slots %u, nargs %u, nfixed %u",
                iter.slots(), fun->nargs, script->nfixed);

        for (uint32_t i = 0; i < fun->nargs; i++) {
            Value arg = iter.read();
            IonSpew(IonSpew_BaselineBailouts, "      arg %d = %016llx",
                        (int) i, *((uint64_t *) &arg));
            size_t argOffset = builder.framePushed() + IonJSFrameLayout::offsetOfActualArg(i);
            *builder.valuePointerAtStackOffset(argOffset) = arg;
        }
    }

    for (uint32_t i = 0; i < script->nfixed; i++) {
        Value slot = iter.read();
        if (!builder.writeValue(slot, "FixedValue"))
            return false;
    }

    IonSpew(IonSpew_BaselineBailouts, "      pushing %d expression stack slots",
                                      (int) exprStackSlots);
    for (uint32_t i = 0; i < exprStackSlots; i++) {
        Value v;

        
        
        
        if (!iter.moreFrames() && i == exprStackSlots - 1 && cx->runtime->hasIonReturnOverride()) {
            JS_ASSERT(invalidate);
            iter.skip();
            IonSpew(IonSpew_BaselineBailouts, "      [Return Override]");
            v = cx->runtime->takeIonReturnOverride();
        } else {
            v = iter.read();
        }
        if (!builder.writeValue(v, "StackValue"))
            return false;
    }

    size_t endOfBaselineJSFrameStack = builder.framePushed();

    
    jsbytecode *pc = script->code + iter.pcOffset();
    JSOp op = JSOp(*pc);
    bool resumeAfter = iter.resumeAfter();

    
    
    
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
    bool isCall = js_CodeSpec[op].format & JOF_INVOKE;
    BaselineScript *baselineScript = script->baselineScript();

    
    
    
#ifdef DEBUG
    uint32_t expectedDepth = js_ReconstructStackDepth(cx, script,
                                                      resumeAfter ? GetNextPc(pc) : pc);
    JS_ASSERT_IF(op != JSOP_FUNAPPLY || !iter.moreFrames() || resumeAfter,
                 exprStackSlots == expectedDepth);

    IonSpew(IonSpew_BaselineBailouts, "      Resuming %s pc offset %d (op %s) (line %d) of %s:%d",
                resumeAfter ? "after" : "at", (int) pcOff, js_CodeName[op],
                PCToLineNumber(script, pc), script->filename(), (int) script->lineno);
    IonSpew(IonSpew_BaselineBailouts, "      Bailout kind: %s",
            BailoutKindString(bailoutKind));
#endif

    
    if (!iter.moreFrames()) {
        
        *callPC = NULL;

        
        
        
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
            if (scopeChain == NULL) {
                
                
                JS_ASSERT(fun);
                JS_ASSERT(numUnsynced == 0);
                opReturnAddr = baselineScript->prologueEntryAddr();
                IonSpew(IonSpew_BaselineBailouts, "      Resuming into prologue.");

                
                blFrame->unsetPushedSPSFrame();

                
                
                
                
                
                
                
                
                
                
                
                
                
                if (cx->runtime->spsProfiler.enabled()) {
                    if (caller && bailoutKind == Bailout_ArgumentCheck) {
                        IonSpew(IonSpew_BaselineBailouts, "      Setting PCidx on innermost "
                                "inlined frame's parent's SPS entry (%s:%d) (pcIdx=%d)!",
                                caller->filename(), caller->lineno, callerPC - caller->code);
                        cx->runtime->spsProfiler.updatePC(caller, callerPC);
                    } else if (bailoutKind != Bailout_ArgumentCheck) {
                        IonSpew(IonSpew_BaselineBailouts,
                                "      Popping SPS entry for innermost inlined frame's SPS entry");
                        cx->runtime->spsProfiler.exit(cx, script, fun);
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
    JS_ASSERT(icEntry.firstStub()->getChainFallback()->isCall_Fallback());
    if (!builder.writePtr(baselineScript->returnAddressForIC(icEntry), "ReturnAddr"))
        return false;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    IonSpew(IonSpew_BaselineBailouts, "      [BASELINE-STUB FRAME]");

    size_t startOfBaselineStubFrame = builder.framePushed();

    
    JS_ASSERT(icEntry.fallbackStub()->isCall_Fallback());
    if (!builder.writePtr(icEntry.fallbackStub(), "StubPtr"))
        return false;

    
    if (!builder.writePtr(prevFramePtr, "PrevFramePtr"))
        return false;
    prevFramePtr = builder.virtualPointerAtStackOffset(0);

    
    
    JS_ASSERT(isCall);
    unsigned actualArgc = GET_ARGC(pc);
    if (op == JSOP_FUNAPPLY)
        actualArgc = blFrame->numActualArgs();

    JS_ASSERT(actualArgc + 2 <= exprStackSlots);
    for (unsigned i = 0; i < actualArgc + 1; i++) {
        size_t argSlot = (script->nfixed + exprStackSlots) - (i + 1);
        if (!builder.writeValue(*blFrame->valueSlot(argSlot), "ArgVal"))
            return false;
    }
    
    
    size_t endOfBaselineStubArgs = builder.framePushed();

    
    size_t baselineStubFrameSize = builder.framePushed() - startOfBaselineStubFrame;
    size_t baselineStubFrameDescr = MakeFrameDescriptor((uint32_t) baselineStubFrameSize,
                                                        IonFrame_BaselineStub);

    
    if (!builder.writeWord(actualArgc, "ActualArgc"))
        return false;

    
    uint32_t calleeStackSlot = exprStackSlots - uint32_t(actualArgc + 2);
    size_t calleeOffset = (builder.framePushed() - endOfBaselineJSFrameStack)
                            + ((exprStackSlots - (calleeStackSlot + 1)) * sizeof(Value));
    Value callee = *builder.valuePointerAtStackOffset(calleeOffset);
    IonSpew(IonSpew_BaselineBailouts, "      CalleeStackSlot=%d", (int) calleeStackSlot);
    IonSpew(IonSpew_BaselineBailouts, "      Callee = %016llx", *((uint64_t *) &callee));
    JS_ASSERT(callee.isObject() && callee.toObject().isFunction());
    JSFunction *calleeFun = callee.toObject().toFunction();
    if (!builder.writePtr(CalleeToToken(calleeFun), "CalleeToken"))
        return false;
    nextCallee.set(calleeFun);

    
    if (!builder.writeWord(baselineStubFrameDescr, "Descriptor"))
        return false;

    
    void *baselineCallReturnAddr = cx->compartment->ionCompartment()->baselineCallReturnAddr();
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

    
    
    void *rectReturnAddr = cx->compartment->ionCompartment()->getArgumentsRectifierReturnAddr();
    JS_ASSERT(rectReturnAddr);
    if (!builder.writePtr(rectReturnAddr, "ReturnAddr"))
        return false;

    return true;
}

uint32_t
ion::BailoutIonToBaseline(JSContext *cx, IonActivation *activation, IonBailoutIterator &iter,
                          bool invalidate, BaselineBailoutInfo **bailoutInfo)
{
    JS_ASSERT(bailoutInfo != NULL);
    JS_ASSERT(*bailoutInfo == NULL);

    
    
    
    
    
    JS_ASSERT(iter.isOptimizedJS());
    FrameType prevFrameType = iter.prevType();
    JS_ASSERT(prevFrameType == IonFrame_OptimizedJS ||
              prevFrameType == IonFrame_BaselineStub ||
              prevFrameType == IonFrame_Entry ||
              prevFrameType == IonFrame_Rectifier);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    IonSpew(IonSpew_BaselineBailouts, "Bailing to baseline %s:%u (IonScript=%p) (FrameType=%d)",
            iter.script()->filename(), iter.script()->lineno, (void *) iter.ionScript(),
            (int) prevFrameType);
    IonSpew(IonSpew_BaselineBailouts, "  Reading from snapshot offset %u size %u",
            iter.snapshotOffset(), iter.ionScript()->snapshotsSize());
    iter.ionScript()->setBailoutExpected();

    
    BaselineStackBuilder builder(iter, 1024);
    if (!builder.init())
        return BAILOUT_RETURN_FATAL_ERROR;
    IonSpew(IonSpew_BaselineBailouts, "  Incoming frame ptr = %p", builder.startFrame());

    SnapshotIterator snapIter(iter);

    RootedFunction callee(cx, iter.maybeCallee());
    if (callee) {
        IonSpew(IonSpew_BaselineBailouts, "  Callee function (%s:%u)",
                callee->nonLazyScript()->filename(), callee->nonLazyScript()->lineno);
    } else {
        IonSpew(IonSpew_BaselineBailouts, "  No callee!");
    }

    if (iter.isConstructing())
        IonSpew(IonSpew_BaselineBailouts, "  Constructing!");
    else
        IonSpew(IonSpew_BaselineBailouts, "  Not constructing!");

    IonSpew(IonSpew_BaselineBailouts, "  Restoring frames:");
    int frameNo = 0;

    
    RootedScript caller(cx);
    jsbytecode *callerPC = NULL;
    RootedFunction fun(cx, callee);
    RootedScript scr(cx, iter.script());
    while (true) {
        IonSpew(IonSpew_BaselineBailouts, "    FrameNo %d", frameNo);
        jsbytecode *callPC = NULL;
        RootedFunction nextCallee(cx, NULL);
        if (!InitFromBailout(cx, caller, callerPC, fun, scr, snapIter, invalidate, builder,
                             &nextCallee, &callPC))
        {
            return BAILOUT_RETURN_FATAL_ERROR;
        }

        if (!snapIter.moreFrames()) {
            JS_ASSERT(!callPC);
            break;
        }

        JS_ASSERT(nextCallee);
        JS_ASSERT(callPC);
        caller = scr;
        callerPC = callPC;
        fun = nextCallee;
        scr = fun->nonLazyScript();
        snapIter.nextFrame();

        frameNo++;
    }
    IonSpew(IonSpew_BaselineBailouts, "  Done restoring frames");
    BailoutKind bailoutKind = snapIter.bailoutKind();

    
    BaselineBailoutInfo *info = builder.takeBuffer();
    info->numFrames = frameNo + 1;

    
    bool overRecursed = false;
    JS_CHECK_RECURSION_WITH_EXTRA_DONT_REPORT(cx, info->copyStackTop - info->copyStackBottom,
                                              overRecursed = true);
    if (overRecursed)
        return BAILOUT_RETURN_OVERRECURSED;

    info->bailoutKind = bailoutKind;
    *bailoutInfo = info;
    return BAILOUT_RETURN_BASELINE;
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
HandleCachedShapeGuardFailure(JSContext *cx, HandleScript outerScript, HandleScript innerScript)
{
    IonSpew(IonSpew_Bailouts, "Cached shape guard failure %s:%d, inlined into %s:%d",
            innerScript->filename(), innerScript->lineno,
            outerScript->filename(), outerScript->lineno);

    JS_ASSERT(!outerScript->ionScript()->invalidated());

    outerScript->failedShapeGuard = true;

    
    
    

    IonSpew(IonSpew_BaselineBailouts, "Invalidating due to cached shape guard failure");

    return Invalidate(cx, outerScript);
}

uint32_t
ion::FinishBailoutToBaseline(BaselineBailoutInfo *bailoutInfo)
{
    
    
    JSContext *cx = GetIonContext()->cx;
    js::gc::AutoSuppressGC suppressGC(cx);

    IonSpew(IonSpew_BaselineBailouts, "  Done restoring frames");

    uint32_t numFrames = bailoutInfo->numFrames;
    JS_ASSERT(numFrames > 0);
    BailoutKind bailoutKind = bailoutInfo->bailoutKind;

    
    js_free(bailoutInfo);
    bailoutInfo = NULL;

    
    
    
    BaselineFrame *topFrame = GetTopBaselineFrame(cx);
    if (topFrame->scopeChain() && !EnsureHasScopeObjects(cx, topFrame))
        return false;

    
    
    RootedScript innerScript(cx, NULL);
    RootedScript outerScript(cx, NULL);
    IonFrameIterator iter(cx);
    uint32_t frameno = 0;
    while (frameno < numFrames) {
        JS_ASSERT(!iter.isOptimizedJS());

        if (iter.isBaselineJS()) {
            BaselineFrame *frame = iter.baselineFrame();
            JS_ASSERT(!frame->hasArgsObj());

            if (frame->script()->needsArgsObj()) {
                ArgumentsObject *argsobj = ArgumentsObject::createExpected(cx, frame);
                if (!argsobj)
                    return false;

                
                
                
                
                RootedScript script(cx, frame->script());
                SetFrameArgumentsObject(cx, frame, script, argsobj);
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
      case Bailout_TypeBarrier:
      case Bailout_Monitor:
        
        
        
        break;
      case Bailout_BoundsCheck:
        if (!HandleBoundsCheckFailure(cx, outerScript, innerScript))
            return false;
        break;
      case Bailout_ShapeGuard:
        if (!HandleShapeGuardFailure(cx, outerScript, innerScript))
            return false;
        break;
      case Bailout_CachedShapeGuard:
        if (!HandleCachedShapeGuardFailure(cx, outerScript, innerScript))
            return false;
        break;
      default:
        JS_NOT_REACHED("Unknown bailout kind!");
    }

    return true;
}
