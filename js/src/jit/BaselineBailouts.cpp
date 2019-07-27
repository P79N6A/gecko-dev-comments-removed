





#include "mozilla/SizePrintfMacros.h"

#include "jsprf.h"
#include "jsutil.h"
#include "jit/arm/Simulator-arm.h"
#include "jit/BaselineIC.h"
#include "jit/BaselineJIT.h"
#include "jit/CompileInfo.h"
#include "jit/JitSpewer.h"
#include "jit/mips/Simulator-mips.h"
#include "jit/Recover.h"
#include "jit/RematerializedFrame.h"

#include "vm/ArgumentsObject.h"
#include "vm/Debugger.h"
#include "vm/TraceLogging.h"

#include "jsscriptinlines.h"

#include "jit/JitFrames-inl.h"

using namespace js;
using namespace js::jit;




template <typename T>
class BufferPointer
{
    BaselineBailoutInfo** header_;
    size_t offset_;
    bool heap_;

  public:
    BufferPointer(BaselineBailoutInfo** header, size_t offset, bool heap)
      : header_(header), offset_(offset), heap_(heap)
    { }

    T* get() const {
        BaselineBailoutInfo* header = *header_;
        if (!heap_)
            return (T*)(header->incomingStack + offset_);

        uint8_t* p = header->copyStackTop - offset_;
        MOZ_ASSERT(p >= header->copyStackBottom && p < header->copyStackTop);
        return (T*)p;
    }

    T& operator*() const { return *get(); }
    T* operator->() const { return get(); }
};



















struct BaselineStackBuilder
{
    JitFrameIterator& iter_;
    JitFrameLayout* frame_;

    static size_t HeaderSize() {
        return AlignBytes(sizeof(BaselineBailoutInfo), sizeof(void*));
    }
    size_t bufferTotal_;
    size_t bufferAvail_;
    size_t bufferUsed_;
    uint8_t* buffer_;
    BaselineBailoutInfo* header_;

    size_t framePushed_;

    BaselineStackBuilder(JitFrameIterator& iter, size_t initialSize)
      : iter_(iter),
        frame_(static_cast<JitFrameLayout*>(iter.current())),
        bufferTotal_(initialSize),
        bufferAvail_(0),
        bufferUsed_(0),
        buffer_(nullptr),
        header_(nullptr),
        framePushed_(0)
    {
        MOZ_ASSERT(bufferTotal_ >= HeaderSize());
        MOZ_ASSERT(iter.isBailoutJS());
    }

    ~BaselineStackBuilder() {
        js_free(buffer_);
    }

    bool init() {
        MOZ_ASSERT(!buffer_);
        MOZ_ASSERT(bufferUsed_ == 0);
        buffer_ = reinterpret_cast<uint8_t*>(js_calloc(bufferTotal_));
        if (!buffer_)
            return false;
        bufferAvail_ = bufferTotal_ - HeaderSize();
        bufferUsed_ = 0;

        header_ = reinterpret_cast<BaselineBailoutInfo*>(buffer_);
        header_->incomingStack = reinterpret_cast<uint8_t*>(frame_);
        header_->copyStackTop = buffer_ + bufferTotal_;
        header_->copyStackBottom = header_->copyStackTop;
        header_->setR0 = 0;
        header_->valueR0 = UndefinedValue();
        header_->setR1 = 0;
        header_->valueR1 = UndefinedValue();
        header_->resumeFramePtr = nullptr;
        header_->resumeAddr = nullptr;
        header_->resumePC = nullptr;
        header_->monitorStub = nullptr;
        header_->numFrames = 0;
        return true;
    }

    bool enlarge() {
        MOZ_ASSERT(buffer_ != nullptr);
        if (bufferTotal_ & mozilla::tl::MulOverflowMask<2>::value)
            return false;
        size_t newSize = bufferTotal_ * 2;
        uint8_t* newBuffer = reinterpret_cast<uint8_t*>(js_calloc(newSize));
        if (!newBuffer)
            return false;
        memcpy((newBuffer + newSize) - bufferUsed_, header_->copyStackBottom, bufferUsed_);
        memcpy(newBuffer, header_, sizeof(BaselineBailoutInfo));
        js_free(buffer_);
        buffer_ = newBuffer;
        bufferTotal_ = newSize;
        bufferAvail_ = newSize - (HeaderSize() + bufferUsed_);

        header_ = reinterpret_cast<BaselineBailoutInfo*>(buffer_);
        header_->copyStackTop = buffer_ + bufferTotal_;
        header_->copyStackBottom = header_->copyStackTop - bufferUsed_;
        return true;
    }

    BaselineBailoutInfo* info() {
        MOZ_ASSERT(header_ == reinterpret_cast<BaselineBailoutInfo*>(buffer_));
        return header_;
    }

    BaselineBailoutInfo* takeBuffer() {
        MOZ_ASSERT(header_ == reinterpret_cast<BaselineBailoutInfo*>(buffer_));
        buffer_ = nullptr;
        return header_;
    }

    void resetFramePushed() {
        framePushed_ = 0;
    }

    size_t framePushed() const {
        return framePushed_;
    }

    bool subtract(size_t size, const char* info = nullptr) {
        
        while (size > bufferAvail_) {
            if (!enlarge())
                return false;
        }

        
        header_->copyStackBottom -= size;
        bufferAvail_ -= size;
        bufferUsed_ += size;
        framePushed_ += size;
        if (info) {
            JitSpew(JitSpew_BaselineBailouts,
                    "      SUB_%03d   %p/%p %-15s",
                    (int) size, header_->copyStackBottom, virtualPointerAtStackOffset(0), info);
        }
        return true;
    }

    template <typename T>
    bool write(const T& t) {
        if (!subtract(sizeof(T)))
            return false;
        memcpy(header_->copyStackBottom, &t, sizeof(T));
        return true;
    }

    template <typename T>
    bool writePtr(T* t, const char* info) {
        if (!write<T*>(t))
            return false;
        if (info)
            JitSpew(JitSpew_BaselineBailouts,
                    "      WRITE_PTR %p/%p %-15s %p",
                    header_->copyStackBottom, virtualPointerAtStackOffset(0), info, t);
        return true;
    }

    bool writeWord(size_t w, const char* info) {
        if (!write<size_t>(w))
            return false;
        if (info) {
            if (sizeof(size_t) == 4) {
                JitSpew(JitSpew_BaselineBailouts,
                        "      WRITE_WRD %p/%p %-15s %08x",
                        header_->copyStackBottom, virtualPointerAtStackOffset(0), info, w);
            } else {
                JitSpew(JitSpew_BaselineBailouts,
                        "      WRITE_WRD %p/%p %-15s %016llx",
                        header_->copyStackBottom, virtualPointerAtStackOffset(0), info, w);
            }
        }
        return true;
    }

    bool writeValue(Value val, const char* info) {
        if (!write<Value>(val))
            return false;
        if (info) {
            JitSpew(JitSpew_BaselineBailouts,
                    "      WRITE_VAL %p/%p %-15s %016llx",
                    header_->copyStackBottom, virtualPointerAtStackOffset(0), info,
                    *((uint64_t*) &val));
        }
        return true;
    }

    bool maybeWritePadding(size_t alignment, size_t after, const char* info) {
        MOZ_ASSERT(framePushed_ % sizeof(Value) == 0);
        MOZ_ASSERT(after % sizeof(Value) == 0);
        size_t offset = ComputeByteAlignment(after, alignment);
        while (framePushed_ % alignment != offset) {
            if (!writeValue(MagicValue(JS_ARG_POISON), info))
                return false;
        }

        return true;
    }

    Value popValue() {
        MOZ_ASSERT(bufferUsed_ >= sizeof(Value));
        MOZ_ASSERT(framePushed_ >= sizeof(Value));
        bufferAvail_ += sizeof(Value);
        bufferUsed_ -= sizeof(Value);
        framePushed_ -= sizeof(Value);
        Value result = *((Value*) header_->copyStackBottom);
        header_->copyStackBottom += sizeof(Value);
        return result;
    }

    void popValueInto(PCMappingSlotInfo::SlotLocation loc) {
        MOZ_ASSERT(PCMappingSlotInfo::ValidSlotLocation(loc));
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
            MOZ_ASSERT(loc == PCMappingSlotInfo::SlotIgnore);
            popValue();
            break;
        }
    }

    void setResumeFramePtr(void* resumeFramePtr) {
        header_->resumeFramePtr = resumeFramePtr;
    }

    void setResumeAddr(void* resumeAddr) {
        header_->resumeAddr = resumeAddr;
    }

    void setResumePC(jsbytecode* pc) {
        header_->resumePC = pc;
    }

    void setMonitorStub(ICStub* stub) {
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

    inline uint8_t* virtualPointerAtStackOffset(size_t offset) {
        if (offset < bufferUsed_)
            return reinterpret_cast<uint8_t*>(frame_) - (bufferUsed_ - offset);
        return reinterpret_cast<uint8_t*>(frame_) + (offset - bufferUsed_);
    }

    inline JitFrameLayout* startFrame() {
        return frame_;
    }

    BufferPointer<JitFrameLayout> topFrameAddress() {
        return pointerAtStackOffset<JitFrameLayout>(0);
    }

    
    
    
    
    
    
    
    
    
    
    
    void* calculatePrevFramePtr() {
        
        BufferPointer<JitFrameLayout> topFrame = topFrameAddress();
        FrameType type = topFrame->prevType();

        
        
        
        
        if (type == JitFrame_IonJS || type == JitFrame_Entry || type == JitFrame_IonAccessorIC)
            return nullptr;

        
        
        
        
        if (type == JitFrame_BaselineStub) {
            size_t offset = JitFrameLayout::Size() + topFrame->prevFrameLocalSize() +
                            BaselineStubFrameLayout::reverseOffsetOfSavedFramePtr();
            return virtualPointerAtStackOffset(offset);
        }

        MOZ_ASSERT(type == JitFrame_Rectifier);
        
        
        
        
        size_t priorOffset = JitFrameLayout::Size() + topFrame->prevFrameLocalSize();
#if defined(JS_CODEGEN_X86)
        
        MOZ_ASSERT(BaselineFrameReg == FramePointer);
        priorOffset -= sizeof(void*);
        return virtualPointerAtStackOffset(priorOffset);
#elif defined(JS_CODEGEN_X64) || defined(JS_CODEGEN_ARM) || defined(JS_CODEGEN_MIPS)
        
        
        BufferPointer<RectifierFrameLayout> priorFrame =
            pointerAtStackOffset<RectifierFrameLayout>(priorOffset);
        FrameType priorType = priorFrame->prevType();
        MOZ_ASSERT(priorType == JitFrame_IonJS || priorType == JitFrame_BaselineStub);

        
        
        if (priorType == JitFrame_IonJS)
            return nullptr;

        
        
        
        
        
        size_t extraOffset = RectifierFrameLayout::Size() + priorFrame->prevFrameLocalSize() +
                             BaselineStubFrameLayout::reverseOffsetOfSavedFramePtr();
        return virtualPointerAtStackOffset(priorOffset + extraOffset);
#elif defined(JS_CODEGEN_NONE)
        MOZ_CRASH();
#else
#  error "Bad architecture!"
#endif
    }
};




class SnapshotIteratorForBailout : public SnapshotIterator
{
    JitActivation* activation_;
    JitFrameIterator& iter_;

  public:
    SnapshotIteratorForBailout(JitActivation* activation, JitFrameIterator& iter)
      : SnapshotIterator(iter),
        activation_(activation),
        iter_(iter)
    {
        MOZ_ASSERT(iter.isBailoutJS());
    }

    ~SnapshotIteratorForBailout() {
        
        
        activation_->removeIonFrameRecovery(fp_);
    }

    
    
    bool init(JSContext* cx) {

        
        
        
        MaybeReadFallback recoverBailout(cx, activation_, &iter_, MaybeReadFallback::Fallback_DoNothing);
        return initInstructionResults(recoverBailout);
    }
};

#ifdef DEBUG
static inline bool
IsInlinableFallback(ICFallbackStub* icEntry)
{
    return icEntry->isCall_Fallback() || icEntry->isGetProp_Fallback() ||
           icEntry->isSetProp_Fallback();
}
#endif

static inline void*
GetStubReturnAddress(JSContext* cx, jsbytecode* pc)
{
    if (IsGetPropPC(pc))
        return cx->compartment()->jitCompartment()->baselineGetPropReturnAddr();
    if (IsSetPropPC(pc))
        return cx->compartment()->jitCompartment()->baselineSetPropReturnAddr();
    
    MOZ_ASSERT(IsCallPC(pc));
    return cx->compartment()->jitCompartment()->baselineCallReturnAddr();
}

static inline jsbytecode*
GetNextNonLoopEntryPc(jsbytecode* pc)
{
    JSOp op = JSOp(*pc);
    if (op == JSOP_GOTO)
        return pc + GET_JUMP_OFFSET(pc);
    if (op == JSOP_LOOPENTRY || op == JSOP_NOP || op == JSOP_LOOPHEAD)
        return GetNextPc(pc);
    return pc;
}

static bool
HasLiveIteratorAtStackDepth(JSScript* script, jsbytecode* pc, uint32_t stackDepth)
{
    if (!script->hasTrynotes())
        return false;

    JSTryNote* tn = script->trynotes()->vector;
    JSTryNote* tnEnd = tn + script->trynotes()->length;
    uint32_t pcOffset = uint32_t(pc - script->main());
    for (; tn != tnEnd; ++tn) {
        if (pcOffset < tn->start)
            continue;
        if (pcOffset >= tn->start + tn->length)
            continue;

        
        if (tn->kind == JSTRY_FOR_IN && stackDepth == tn->stackDepth)
            return true;

        
        
        if (tn->kind == JSTRY_FOR_OF && stackDepth == tn->stackDepth - 1)
            return true;
    }

    return false;
}













































































static bool
InitFromBailout(JSContext* cx, HandleScript caller, jsbytecode* callerPC,
                HandleFunction fun, HandleScript script, IonScript* ionScript,
                SnapshotIterator& iter, bool invalidate, BaselineStackBuilder& builder,
                AutoValueVector& startFrameFormals, MutableHandleFunction nextCallee,
                jsbytecode** callPC, const ExceptionBailoutInfo* excInfo)
{
    
    
    MOZ_ASSERT(cx->mainThread().suppressGC);

    MOZ_ASSERT(script->hasBaselineScript());

    
    bool catchingException = excInfo && excInfo->catchingException();

    
    
    
    
    uint32_t exprStackSlots;
    if (catchingException)
        exprStackSlots = excInfo->numExprSlots();
    else
        exprStackSlots = iter.numAllocations() - (script->nfixed() + CountArgSlots(script, fun));

    builder.resetFramePushed();

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    JitSpew(JitSpew_BaselineBailouts, "      Unpacking %s:%d", script->filename(), script->lineno());
    JitSpew(JitSpew_BaselineBailouts, "      [BASELINE-JS FRAME]");

    
    
    
    
    void* prevFramePtr = builder.calculatePrevFramePtr();
    if (!builder.writePtr(prevFramePtr, "PrevFramePtr"))
        return false;
    prevFramePtr = builder.virtualPointerAtStackOffset(0);

    
    if (!builder.subtract(BaselineFrame::Size(), "BaselineFrame"))
        return false;
    BufferPointer<BaselineFrame> blFrame = builder.pointerAtStackOffset<BaselineFrame>(0);

    
    uint32_t frameSize = BaselineFrame::Size() + BaselineFrame::FramePointerOffset +
                         (sizeof(Value) * (script->nfixed() + exprStackSlots));
    JitSpew(JitSpew_BaselineBailouts, "      FrameSize=%d", (int) frameSize);
    blFrame->setFrameSize(frameSize);

    uint32_t flags = 0;

    
    
    
    if (script->isDebuggee())
        flags |= BaselineFrame::DEBUGGEE;

    
    JSObject* scopeChain = nullptr;
    Value returnValue;
    ArgumentsObject* argsObj = nullptr;
    BailoutKind bailoutKind = iter.bailoutKind();
    if (bailoutKind == Bailout_ArgumentCheck) {
        
        
        
        
        JitSpew(JitSpew_BaselineBailouts, "      Bailout_ArgumentCheck! (no valid scopeChain)");
        iter.skip();

        
        iter.skip();
        returnValue = UndefinedValue();

        
        if (script->argumentsHasVarBinding()) {
            JitSpew(JitSpew_BaselineBailouts,
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
            MOZ_ASSERT(v.isUndefined() || v.isMagic(JS_OPTIMIZED_OUT));

            
            if (fun) {
                
                
                
                
                
                
                
                if (iter.pcOffset() != 0 || iter.resumeAfter() ||
                    (excInfo && excInfo->propagatingIonExceptionForDebugMode()))
                {
                    scopeChain = fun->environment();
                }
            } else {
                
                
                
                
                
                MOZ_ASSERT(!script->isForEval());
                MOZ_ASSERT(!script->hasPollutedGlobalScope());
                scopeChain = &(script->global());
            }
        }

        
        
        returnValue = iter.read();
        flags |= BaselineFrame::HAS_RVAL;

        
        if (script->argumentsHasVarBinding()) {
            v = iter.read();
            MOZ_ASSERT(v.isObject() || v.isUndefined() || v.isMagic(JS_OPTIMIZED_OUT));
            if (v.isObject())
                argsObj = &v.toObject().as<ArgumentsObject>();
        }
    }
    JitSpew(JitSpew_BaselineBailouts, "      ScopeChain=%p", scopeChain);
    blFrame->setScopeChain(scopeChain);
    JitSpew(JitSpew_BaselineBailouts, "      ReturnValue=%016llx", *((uint64_t*) &returnValue));
    blFrame->setReturnValue(returnValue);

    
    blFrame->setFlags(flags);

    
    if (argsObj)
        blFrame->initArgsObjUnchecked(*argsObj);

    if (fun) {
        
        
        Value thisv = iter.read();
        JitSpew(JitSpew_BaselineBailouts, "      Is function!");
        JitSpew(JitSpew_BaselineBailouts, "      thisv=%016llx", *((uint64_t*) &thisv));

        size_t thisvOffset = builder.framePushed() + JitFrameLayout::offsetOfThis();
        *builder.valuePointerAtStackOffset(thisvOffset) = thisv;

        MOZ_ASSERT(iter.numAllocations() >= CountArgSlots(script, fun));
        JitSpew(JitSpew_BaselineBailouts, "      frame slots %u, nargs %u, nfixed %u",
                iter.numAllocations(), fun->nargs(), script->nfixed());

        if (!callerPC) {
            
            
            
            
            
            MOZ_ASSERT(startFrameFormals.empty());
            if (!startFrameFormals.resize(fun->nargs()))
                return false;
        }

        for (uint32_t i = 0; i < fun->nargs(); i++) {
            Value arg = iter.read();
            JitSpew(JitSpew_BaselineBailouts, "      arg %d = %016llx",
                        (int) i, *((uint64_t*) &arg));
            if (callerPC) {
                size_t argOffset = builder.framePushed() + JitFrameLayout::offsetOfActualArg(i);
                *builder.valuePointerAtStackOffset(argOffset) = arg;
            } else {
                startFrameFormals[i].set(arg);
            }
        }
    }

    for (uint32_t i = 0; i < script->nfixed(); i++) {
        Value slot = iter.read();
        if (!builder.writeValue(slot, "FixedValue"))
            return false;
    }

    
    
    jsbytecode* pc = catchingException ? excInfo->resumePC() : script->offsetToPC(iter.pcOffset());
    bool resumeAfter = catchingException ? false : iter.resumeAfter();

    JSOp op = JSOp(*pc);

    
    
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

        MOZ_ASSERT(exprStackSlots >= inlined_args);
        pushedSlots = exprStackSlots - inlined_args;

        JitSpew(JitSpew_BaselineBailouts,
                "      pushing %u expression stack slots before fixup",
                pushedSlots);
        for (uint32_t i = 0; i < pushedSlots; i++) {
            Value v = iter.read();
            if (!builder.writeValue(v, "StackValue"))
                return false;
        }

        if (op == JSOP_FUNCALL) {
            
            
            
            
            
            JitSpew(JitSpew_BaselineBailouts, "      pushing undefined to fixup funcall");
            if (!builder.writeValue(UndefinedValue(), "StackValue"))
                return false;
        }

        if (needToSaveArgs) {
            
            
            

            
            
            
            
            
            
            
            if (op == JSOP_FUNAPPLY) {
                JitSpew(JitSpew_BaselineBailouts, "      pushing 4x undefined to fixup funapply");
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
                savedCallerArgs[i].set(iter.read());

            if (IsSetPropPC(pc)) {
                
                
                
                
                
                
                Value initialArg = savedCallerArgs[inlined_args - 1];
                JitSpew(JitSpew_BaselineBailouts, "     pushing setter's initial argument");
                if (!builder.writeValue(initialArg, "StackValue"))
                    return false;
            }
            pushedSlots = exprStackSlots;
        }
    }

    JitSpew(JitSpew_BaselineBailouts, "      pushing %u expression stack slots",
                                      exprStackSlots - pushedSlots);
    for (uint32_t i = pushedSlots; i < exprStackSlots; i++) {
        Value v;

        if (!iter.moreFrames() && i == exprStackSlots - 1 &&
            cx->runtime()->jitRuntime()->hasIonReturnOverride())
        {
            
            
            
            MOZ_ASSERT(invalidate);
            iter.skip();
            JitSpew(JitSpew_BaselineBailouts, "      [Return Override]");
            v = cx->runtime()->jitRuntime()->takeIonReturnOverride();
        } else if (excInfo && excInfo->propagatingIonExceptionForDebugMode()) {
            
            
            
            
            
            
            
            
            
            MOZ_ASSERT(cx->compartment()->isDebuggee());
            if (iter.moreFrames() || HasLiveIteratorAtStackDepth(script, pc, i + 1)) {
                v = iter.read();
            } else {
                iter.skip();
                v = MagicValue(JS_OPTIMIZED_OUT);
            }
        } else {
            v = iter.read();
        }
        if (!builder.writeValue(v, "StackValue"))
            return false;
    }

    size_t endOfBaselineJSFrameStack = builder.framePushed();

    
    
    
    
    
    
    if (!resumeAfter) {
        jsbytecode* fasterPc = pc;
        while (true) {
            pc = GetNextNonLoopEntryPc(pc);
            fasterPc = GetNextNonLoopEntryPc(GetNextNonLoopEntryPc(fasterPc));
            if (fasterPc == pc)
                break;
        }
        op = JSOp(*pc);
    }

    uint32_t pcOff = script->pcToOffset(pc);
    bool isCall = IsCallPC(pc);
    BaselineScript* baselineScript = script->baselineScript();

#ifdef DEBUG
    uint32_t expectedDepth;
    bool reachablePC;
    if (!ReconstructStackDepth(cx, script, resumeAfter ? GetNextPc(pc) : pc, &expectedDepth, &reachablePC))
        return false;

    if (reachablePC) {
        if (op != JSOP_FUNAPPLY || !iter.moreFrames() || resumeAfter) {
            if (op == JSOP_FUNCALL) {
                
                
                
                MOZ_ASSERT(expectedDepth - exprStackSlots <= 1);
            } else if (iter.moreFrames() && (IsGetPropPC(pc) || IsSetPropPC(pc))) {
                
                
                
                
                
                
                
                
                MOZ_ASSERT(exprStackSlots - expectedDepth == 1);
            } else {
                
                
                
                
                MOZ_ASSERT(exprStackSlots == expectedDepth);
            }
        }
    }

    JitSpew(JitSpew_BaselineBailouts, "      Resuming %s pc offset %d (op %s) (line %d) of %s:%" PRIuSIZE,
                resumeAfter ? "after" : "at", (int) pcOff, js_CodeName[op],
                PCToLineNumber(script, pc), script->filename(), script->lineno());
    JitSpew(JitSpew_BaselineBailouts, "      Bailout kind: %s",
            BailoutKindString(bailoutKind));
#endif

    
    
    if (!iter.moreFrames() || catchingException) {
        
        *callPC = nullptr;

        
        
        
        bool enterMonitorChain = false;
        if (resumeAfter && (js_CodeSpec[op].format & JOF_TYPESET)) {
            
            
            
            ICEntry& icEntry = baselineScript->icEntryFromPCOffset(pcOff);
            ICFallbackStub* fallbackStub = icEntry.firstStub()->getChainFallback();
            if (fallbackStub->isMonitoredFallback())
                enterMonitorChain = true;
        }

        uint32_t numCallArgs = isCall ? GET_ARGC(pc) : 0;

        if (resumeAfter && !enterMonitorChain)
            pc = GetNextPc(pc);

        builder.setResumePC(pc);
        builder.setResumeFramePtr(prevFramePtr);

        if (enterMonitorChain) {
            ICEntry& icEntry = baselineScript->icEntryFromPCOffset(pcOff);
            ICFallbackStub* fallbackStub = icEntry.firstStub()->getChainFallback();
            MOZ_ASSERT(fallbackStub->isMonitoredFallback());
            JitSpew(JitSpew_BaselineBailouts, "      [TYPE-MONITOR CHAIN]");
            ICMonitoredFallbackStub* monFallbackStub = fallbackStub->toMonitoredFallbackStub();
            ICStub* firstMonStub = monFallbackStub->fallbackMonitorStub()->firstMonitorStub();

            
            JitSpew(JitSpew_BaselineBailouts, "      Popping top stack value into R0.");
            builder.popValueInto(PCMappingSlotInfo::SlotInR0);

            
            
            frameSize -= sizeof(Value);
            blFrame->setFrameSize(frameSize);
            JitSpew(JitSpew_BaselineBailouts, "      Adjusted framesize -= %d: %d",
                            (int) sizeof(Value), (int) frameSize);

            
            
            
            
            if (isCall) {
                builder.writeValue(UndefinedValue(), "CallOp FillerCallee");
                builder.writeValue(UndefinedValue(), "CallOp FillerThis");
                for (uint32_t i = 0; i < numCallArgs; i++)
                    builder.writeValue(UndefinedValue(), "CallOp FillerArg");

                frameSize += (numCallArgs + 2) * sizeof(Value);
                blFrame->setFrameSize(frameSize);
                JitSpew(JitSpew_BaselineBailouts, "      Adjusted framesize += %d: %d",
                                (int) ((numCallArgs + 2) * sizeof(Value)), (int) frameSize);
            }

            
            
            builder.setResumeAddr(baselineScript->returnAddressForIC(icEntry));
            builder.setMonitorStub(firstMonStub);
            JitSpew(JitSpew_BaselineBailouts, "      Set resumeAddr=%p monitorStub=%p",
                    baselineScript->returnAddressForIC(icEntry), firstMonStub);

        } else {
            
            
            
            
            
            PCMappingSlotInfo slotInfo;
            uint8_t* nativeCodeForPC;

            if (excInfo && excInfo->propagatingIonExceptionForDebugMode()) {
                
                
                
                
                
                
                
                
                
                
                
                jsbytecode* throwPC = script->offsetToPC(iter.pcOffset());
                builder.setResumePC(throwPC);
                nativeCodeForPC = nullptr;
            } else {
                nativeCodeForPC = baselineScript->nativeCodeForPC(script, pc, &slotInfo);
                MOZ_ASSERT(nativeCodeForPC);
            }

            unsigned numUnsynced = slotInfo.numUnsynced();

            MOZ_ASSERT(numUnsynced <= 2);
            PCMappingSlotInfo::SlotLocation loc1, loc2;
            if (numUnsynced > 0) {
                loc1 = slotInfo.topSlotLocation();
                JitSpew(JitSpew_BaselineBailouts, "      Popping top stack value into %d.",
                        (int) loc1);
                builder.popValueInto(loc1);
            }
            if (numUnsynced > 1) {
                loc2 = slotInfo.nextSlotLocation();
                JitSpew(JitSpew_BaselineBailouts, "      Popping next stack value into %d.",
                        (int) loc2);
                MOZ_ASSERT_IF(loc1 != PCMappingSlotInfo::SlotIgnore, loc1 != loc2);
                builder.popValueInto(loc2);
            }

            
            
            frameSize -= sizeof(Value) * numUnsynced;
            blFrame->setFrameSize(frameSize);
            JitSpew(JitSpew_BaselineBailouts, "      Adjusted framesize -= %d: %d",
                    int(sizeof(Value) * numUnsynced), int(frameSize));

            
            
            uint8_t* opReturnAddr;
            if (scopeChain == nullptr) {
                
                
                MOZ_ASSERT(fun);
                MOZ_ASSERT(numUnsynced == 0);
                opReturnAddr = baselineScript->prologueEntryAddr();
                JitSpew(JitSpew_BaselineBailouts, "      Resuming into prologue.");

            } else {
                opReturnAddr = nativeCodeForPC;
            }
            builder.setResumeAddr(opReturnAddr);
            JitSpew(JitSpew_BaselineBailouts, "      Set resumeAddr=%p", opReturnAddr);
        }

        if (cx->runtime()->spsProfiler.enabled()) {
            
            const char* filename = script->filename();
            if (filename == nullptr)
                filename = "<unknown>";
            unsigned len = strlen(filename) + 200;
            char* buf = js_pod_malloc<char>(len);
            if (buf == nullptr)
                return false;
            JS_snprintf(buf, len, "%s %s %s on line %u of %s:%" PRIuSIZE,
                                  BailoutKindString(bailoutKind),
                                  resumeAfter ? "after" : "at",
                                  js_CodeName[op],
                                  PCToLineNumber(script, pc),
                                  filename,
                                  script->lineno());
            cx->runtime()->spsProfiler.markEvent(buf);
            js_free(buf);
        }

        return true;
    }

    *callPC = pc;

    
    size_t baselineFrameDescr = MakeFrameDescriptor((uint32_t) builder.framePushed(),
                                                    JitFrame_BaselineJS);
    if (!builder.writeWord(baselineFrameDescr, "Descriptor"))
        return false;

    
    
    ICEntry& icEntry = baselineScript->icEntryFromPCOffset(pcOff);
    MOZ_ASSERT(IsInlinableFallback(icEntry.firstStub()->getChainFallback()));
    if (!builder.writePtr(baselineScript->returnAddressForIC(icEntry), "ReturnAddr"))
        return false;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    JitSpew(JitSpew_BaselineBailouts, "      [BASELINE-STUB FRAME]");

    size_t startOfBaselineStubFrame = builder.framePushed();

    
    MOZ_ASSERT(IsInlinableFallback(icEntry.fallbackStub()));
    if (!builder.writePtr(icEntry.fallbackStub(), "StubPtr"))
        return false;

    
    if (!builder.writePtr(prevFramePtr, "PrevFramePtr"))
        return false;
    prevFramePtr = builder.virtualPointerAtStackOffset(0);

    
    
    MOZ_ASSERT(IsIonInlinablePC(pc));
    unsigned actualArgc;
    if (needToSaveArgs) {
        
        
        if (op == JSOP_FUNAPPLY)
            actualArgc = blFrame->numActualArgs();
        else
            actualArgc = IsSetPropPC(pc);

        
        size_t afterFrameSize = (actualArgc + 1) * sizeof(Value) + JitFrameLayout::Size();
        if (!builder.maybeWritePadding(JitStackAlignment, afterFrameSize, "Padding"))
            return false;

        
        MOZ_ASSERT(actualArgc + 2 <= exprStackSlots);
        MOZ_ASSERT(savedCallerArgs.length() == actualArgc + 2);
        for (unsigned i = 0; i < actualArgc + 1; i++) {
            size_t arg = savedCallerArgs.length() - (i + 1);
            if (!builder.writeValue(savedCallerArgs[arg], "ArgVal"))
                return false;
        }
    } else {
        actualArgc = GET_ARGC(pc);
        if (op == JSOP_FUNCALL) {
            MOZ_ASSERT(actualArgc > 0);
            actualArgc--;
        }

        
        size_t afterFrameSize = (actualArgc + 1) * sizeof(Value) + JitFrameLayout::Size();
        if (!builder.maybeWritePadding(JitStackAlignment, afterFrameSize, "Padding"))
            return false;

        MOZ_ASSERT(actualArgc + 2 <= exprStackSlots);
        for (unsigned i = 0; i < actualArgc + 1; i++) {
            size_t argSlot = (script->nfixed() + exprStackSlots) - (i + 1);
            if (!builder.writeValue(*blFrame->valueSlot(argSlot), "ArgVal"))
                return false;
        }
    }

    
    
    size_t endOfBaselineStubArgs = builder.framePushed();

    
    size_t baselineStubFrameSize = builder.framePushed() - startOfBaselineStubFrame;
    size_t baselineStubFrameDescr = MakeFrameDescriptor((uint32_t) baselineStubFrameSize,
                                                        JitFrame_BaselineStub);

    
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
        JitSpew(JitSpew_BaselineBailouts, "      CalleeStackSlot=%d", (int) calleeStackSlot);
    }
    JitSpew(JitSpew_BaselineBailouts, "      Callee = %016llx", *((uint64_t*) &callee));
    MOZ_ASSERT(callee.isObject() && callee.toObject().is<JSFunction>());
    JSFunction* calleeFun = &callee.toObject().as<JSFunction>();
    if (!builder.writePtr(CalleeToToken(calleeFun, JSOp(*pc) == JSOP_NEW), "CalleeToken"))
        return false;
    nextCallee.set(calleeFun);

    
    if (!builder.writeWord(baselineStubFrameDescr, "Descriptor"))
        return false;

    
    void* baselineCallReturnAddr = GetStubReturnAddress(cx, pc);
    MOZ_ASSERT(baselineCallReturnAddr);
    if (!builder.writePtr(baselineCallReturnAddr, "ReturnAddr"))
        return false;
    MOZ_ASSERT(builder.framePushed() % JitStackAlignment == 0);

    
    
    if (actualArgc >= calleeFun->nargs())
        return true;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    JitSpew(JitSpew_BaselineBailouts, "      [RECTIFIER FRAME]");

    size_t startOfRectifierFrame = builder.framePushed();

    
#if defined(JS_CODEGEN_X86)
    if (!builder.writePtr(prevFramePtr, "PrevFramePtr-X86Only"))
        return false;
    
    prevFramePtr = builder.virtualPointerAtStackOffset(0);
    if (!builder.writePtr(prevFramePtr, "Padding-X86Only"))
        return false;
#endif

    
    size_t afterFrameSize = (calleeFun->nargs() + 1) * sizeof(Value) + RectifierFrameLayout::Size();
    if (!builder.maybeWritePadding(JitStackAlignment, afterFrameSize, "Padding"))
        return false;

    
    for (unsigned i = 0; i < (calleeFun->nargs() - actualArgc); i++) {
        if (!builder.writeValue(UndefinedValue(), "FillerVal"))
            return false;
    }

    
    if (!builder.subtract((actualArgc + 1) * sizeof(Value), "CopiedArgs"))
        return false;
    BufferPointer<uint8_t> stubArgsEnd =
        builder.pointerAtStackOffset<uint8_t>(builder.framePushed() - endOfBaselineStubArgs);
    JitSpew(JitSpew_BaselineBailouts, "      MemCpy from %p", stubArgsEnd.get());
    memcpy(builder.pointerAtStackOffset<uint8_t>(0).get(), stubArgsEnd.get(),
           (actualArgc + 1) * sizeof(Value));

    
    size_t rectifierFrameSize = builder.framePushed() - startOfRectifierFrame;
    size_t rectifierFrameDescr = MakeFrameDescriptor((uint32_t) rectifierFrameSize,
                                                     JitFrame_Rectifier);

    
    if (!builder.writeWord(actualArgc, "ActualArgc"))
        return false;

    
    if (!builder.writePtr(CalleeToToken(calleeFun, JSOp(*pc) == JSOP_NEW), "CalleeToken"))
        return false;

    
    if (!builder.writeWord(rectifierFrameDescr, "Descriptor"))
        return false;

    
    
    void* rectReturnAddr = cx->runtime()->jitRuntime()->getArgumentsRectifierReturnAddr();
    MOZ_ASSERT(rectReturnAddr);
    if (!builder.writePtr(rectReturnAddr, "ReturnAddr"))
        return false;
    MOZ_ASSERT(builder.framePushed() % JitStackAlignment == 0);

    return true;
}

uint32_t
jit::BailoutIonToBaseline(JSContext* cx, JitActivation* activation, JitFrameIterator& iter,
                          bool invalidate, BaselineBailoutInfo** bailoutInfo,
                          const ExceptionBailoutInfo* excInfo)
{
    MOZ_ASSERT(bailoutInfo != nullptr);
    MOZ_ASSERT(*bailoutInfo == nullptr);

    TraceLoggerThread* logger = TraceLoggerForMainThread(cx->runtime());
    TraceLogStopEvent(logger, TraceLogger_IonMonkey);
    TraceLogStartEvent(logger, TraceLogger_Baseline);

    
    
    
    
    
    MOZ_ASSERT(iter.isBailoutJS());
    FrameType prevFrameType = iter.prevType();
    MOZ_ASSERT(prevFrameType == JitFrame_IonJS ||
               prevFrameType == JitFrame_BaselineStub ||
               prevFrameType == JitFrame_Entry ||
               prevFrameType == JitFrame_Rectifier ||
               prevFrameType == JitFrame_IonAccessorIC);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    JitSpew(JitSpew_BaselineBailouts, "Bailing to baseline %s:%u (IonScript=%p) (FrameType=%d)",
            iter.script()->filename(), iter.script()->lineno(), (void*) iter.ionScript(),
            (int) prevFrameType);

    bool catchingException;
    bool propagatingExceptionForDebugMode;
    if (excInfo) {
        catchingException = excInfo->catchingException();
        propagatingExceptionForDebugMode = excInfo->propagatingIonExceptionForDebugMode();

        if (catchingException)
            JitSpew(JitSpew_BaselineBailouts, "Resuming in catch or finally block");

        if (propagatingExceptionForDebugMode)
            JitSpew(JitSpew_BaselineBailouts, "Resuming in-place for debug mode");
    } else {
        catchingException = false;
        propagatingExceptionForDebugMode = false;
    }

    JitSpew(JitSpew_BaselineBailouts, "  Reading from snapshot offset %u size %u",
            iter.snapshotOffset(), iter.ionScript()->snapshotsListSize());

    if (!excInfo)
        iter.ionScript()->incNumBailouts();
    iter.script()->updateBaselineOrIonRaw(cx);

    
    BaselineStackBuilder builder(iter, 1024);
    if (!builder.init())
        return BAILOUT_RETURN_FATAL_ERROR;
    JitSpew(JitSpew_BaselineBailouts, "  Incoming frame ptr = %p", builder.startFrame());

    SnapshotIteratorForBailout snapIter(activation, iter);
    if (!snapIter.init(cx))
        return BAILOUT_RETURN_FATAL_ERROR;

#ifdef TRACK_SNAPSHOTS
    snapIter.spewBailingFrom();
#endif

    RootedFunction callee(cx, iter.maybeCallee());
    RootedScript scr(cx, iter.script());
    if (callee) {
        JitSpew(JitSpew_BaselineBailouts, "  Callee function (%s:%u)",
                scr->filename(), scr->lineno());
    } else {
        JitSpew(JitSpew_BaselineBailouts, "  No callee!");
    }

    if (iter.isConstructing())
        JitSpew(JitSpew_BaselineBailouts, "  Constructing!");
    else
        JitSpew(JitSpew_BaselineBailouts, "  Not constructing!");

    JitSpew(JitSpew_BaselineBailouts, "  Restoring frames:");
    size_t frameNo = 0;

    
    RootedScript caller(cx);
    jsbytecode* callerPC = nullptr;
    RootedFunction fun(cx, callee);
    AutoValueVector startFrameFormals(cx);

    gc::AutoSuppressGC suppress(cx);

    while (true) {
        
        snapIter.settleOnFrame();

        if (frameNo > 0) {
            
            
            
            TraceLoggerEvent scriptEvent(logger, TraceLogger_Scripts, scr);
            TraceLogStartEvent(logger, scriptEvent);
            TraceLogStartEvent(logger, TraceLogger_Baseline);
        }

        JitSpew(JitSpew_BaselineBailouts, "    FrameNo %d", frameNo);

        
        
        bool handleException = (catchingException && excInfo->frameNo() == frameNo);

        
        
        bool passExcInfo = handleException || propagatingExceptionForDebugMode;

        jsbytecode* callPC = nullptr;
        RootedFunction nextCallee(cx, nullptr);
        if (!InitFromBailout(cx, caller, callerPC, fun, scr, iter.ionScript(),
                             snapIter, invalidate, builder, startFrameFormals,
                             &nextCallee, &callPC, passExcInfo ? excInfo : nullptr))
        {
            return BAILOUT_RETURN_FATAL_ERROR;
        }

        if (!snapIter.moreFrames()) {
            MOZ_ASSERT(!callPC);
            break;
        }

        if (handleException)
            break;

        MOZ_ASSERT(nextCallee);
        MOZ_ASSERT(callPC);
        caller = scr;
        callerPC = callPC;
        fun = nextCallee;
        scr = fun->existingScriptForInlinedFunction();

        frameNo++;

        snapIter.nextInstruction();
    }
    JitSpew(JitSpew_BaselineBailouts, "  Done restoring frames");

    BailoutKind bailoutKind = snapIter.bailoutKind();

    if (!startFrameFormals.empty()) {
        
        Value* argv = builder.startFrame()->argv() + 1; 
        mozilla::PodCopy(argv, startFrameFormals.begin(), startFrameFormals.length());
    }

    
    bool overRecursed = false;
    BaselineBailoutInfo* info = builder.info();
    uint8_t* newsp = info->incomingStack - (info->copyStackTop - info->copyStackBottom);
#if defined(JS_ARM_SIMULATOR) || defined(JS_MIPS_SIMULATOR)
    if (Simulator::Current()->overRecursed(uintptr_t(newsp)))
        overRecursed = true;
#else
    JS_CHECK_RECURSION_WITH_SP_DONT_REPORT(cx, newsp, overRecursed = true);
#endif
    if (overRecursed) {
        JitSpew(JitSpew_BaselineBailouts, "  Overrecursion check failed!");
        return BAILOUT_RETURN_OVERRECURSED;
    }

    
    info = builder.takeBuffer();
    info->numFrames = frameNo + 1;
    info->bailoutKind = bailoutKind;
    *bailoutInfo = info;
    return BAILOUT_RETURN_OK;
}

static bool
HandleBoundsCheckFailure(JSContext* cx, HandleScript outerScript, HandleScript innerScript)
{
    JitSpew(JitSpew_IonBailouts, "Bounds check failure %s:%d, inlined into %s:%d",
            innerScript->filename(), innerScript->lineno(),
            outerScript->filename(), outerScript->lineno());

    MOZ_ASSERT(!outerScript->ionScript()->invalidated());

    if (!innerScript->failedBoundsCheck())
        innerScript->setFailedBoundsCheck();

    JitSpew(JitSpew_BaselineBailouts, "Invalidating due to bounds check failure");
    if (!Invalidate(cx, outerScript))
        return false;

    if (innerScript->hasIonScript() && !Invalidate(cx, innerScript))
        return false;

    return true;
}

static bool
HandleShapeGuardFailure(JSContext* cx, HandleScript outerScript, HandleScript innerScript)
{
    JitSpew(JitSpew_IonBailouts, "Shape guard failure %s:%d, inlined into %s:%d",
            innerScript->filename(), innerScript->lineno(),
            outerScript->filename(), outerScript->lineno());

    MOZ_ASSERT(!outerScript->ionScript()->invalidated());

    
    
    
    outerScript->setFailedShapeGuard();
    JitSpew(JitSpew_BaselineBailouts, "Invalidating due to shape guard failure");
    return Invalidate(cx, outerScript);
}

static bool
HandleBaselineInfoBailout(JSContext* cx, JSScript* outerScript, JSScript* innerScript)
{
    JitSpew(JitSpew_IonBailouts, "Baseline info failure %s:%d, inlined into %s:%d",
            innerScript->filename(), innerScript->lineno(),
            outerScript->filename(), outerScript->lineno());

    MOZ_ASSERT(!outerScript->ionScript()->invalidated());

    JitSpew(JitSpew_BaselineBailouts, "Invalidating due to invalid baseline info");
    return Invalidate(cx, outerScript);
}

static bool
HandleLexicalCheckFailure(JSContext* cx, HandleScript outerScript, HandleScript innerScript)
{
    JitSpew(JitSpew_IonBailouts, "Lexical check failure %s:%d, inlined into %s:%d",
            innerScript->filename(), innerScript->lineno(),
            outerScript->filename(), outerScript->lineno());

    MOZ_ASSERT(!outerScript->ionScript()->invalidated());

    if (!innerScript->failedLexicalCheck())
        innerScript->setFailedLexicalCheck();

    JitSpew(JitSpew_BaselineBailouts, "Invalidating due to lexical check failure");
    if (!Invalidate(cx, outerScript))
        return false;

    if (innerScript->hasIonScript() && !Invalidate(cx, innerScript))
        return false;

    return true;
}

static bool
CopyFromRematerializedFrame(JSContext* cx, JitActivation* act, uint8_t* fp, size_t inlineDepth,
                            BaselineFrame* frame)
{
    RematerializedFrame* rematFrame = act->lookupRematerializedFrame(fp, inlineDepth);

    
    
    if (!rematFrame)
        return true;

    MOZ_ASSERT(rematFrame->script() == frame->script());
    MOZ_ASSERT(rematFrame->numActualArgs() == frame->numActualArgs());

    frame->setScopeChain(rematFrame->scopeChain());
    frame->thisValue() = rematFrame->thisValue();

    for (unsigned i = 0; i < frame->numActualArgs(); i++)
        frame->argv()[i] = rematFrame->argv()[i];

    for (size_t i = 0; i < frame->script()->nfixed(); i++)
        *frame->valueSlot(i) = rematFrame->locals()[i];

    JitSpew(JitSpew_BaselineBailouts,
            "  Copied from rematerialized frame at (%p,%u)",
            fp, inlineDepth);

    
    
    
    
    if (rematFrame->isDebuggee()) {
        frame->setIsDebuggee();
        return Debugger::handleIonBailout(cx, rematFrame, frame);
    }

    return true;
}

uint32_t
jit::FinishBailoutToBaseline(BaselineBailoutInfo* bailoutInfo)
{
    
    
    JSContext* cx = GetJSContextFromJitCode();
    js::gc::AutoSuppressGC suppressGC(cx);

    JitSpew(JitSpew_BaselineBailouts, "  Done restoring frames");

    
    
    
    
    BaselineFrame* topFrame = GetTopBaselineFrame(cx);
    topFrame->setOverridePc(bailoutInfo->resumePC);

    uint32_t numFrames = bailoutInfo->numFrames;
    MOZ_ASSERT(numFrames > 0);
    BailoutKind bailoutKind = bailoutInfo->bailoutKind;

    
    js_free(bailoutInfo);
    bailoutInfo = nullptr;

    
    
    
    if (topFrame->scopeChain() && !EnsureHasScopeObjects(cx, topFrame))
        return false;

    
    
    RootedScript innerScript(cx, nullptr);
    RootedScript outerScript(cx, nullptr);

    MOZ_ASSERT(cx->currentlyRunningInJit());
    JitFrameIterator iter(cx);
    uint8_t* outerFp = nullptr;

    
    
    
    if (cx->runtime()->jitRuntime()->isProfilerInstrumentationEnabled(cx->runtime()))
        cx->runtime()->jitActivation->setLastProfilingFrame(iter.prevFp());

    uint32_t frameno = 0;
    while (frameno < numFrames) {
        MOZ_ASSERT(!iter.isIonJS());

        if (iter.isBaselineJS()) {
            BaselineFrame* frame = iter.baselineFrame();
            MOZ_ASSERT(frame->script()->hasBaselineScript());

            
            
            
            if (frame->scopeChain() && frame->script()->needsArgsObj()) {
                ArgumentsObject* argsObj;
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

            if (frameno == numFrames - 1) {
                outerScript = frame->script();
                outerFp = iter.fp();
            }

            frameno++;
        }

        ++iter;
    }

    MOZ_ASSERT(innerScript);
    MOZ_ASSERT(outerScript);
    MOZ_ASSERT(outerFp);

    
    
    
    
    JitActivation* act = cx->runtime()->activation()->asJit();
    if (act->hasRematerializedFrame(outerFp)) {
        JitFrameIterator iter(cx);
        size_t inlineDepth = numFrames;
        while (inlineDepth > 0) {
            if (iter.isBaselineJS() &&
                !CopyFromRematerializedFrame(cx, act, outerFp, --inlineDepth,
                                             iter.baselineFrame()))
            {
                return false;
            }
            ++iter;
        }

        
        
        act->removeRematerializedFrame(outerFp);
    }

    JitSpew(JitSpew_BaselineBailouts,
            "  Restored outerScript=(%s:%u,%u) innerScript=(%s:%u,%u) (bailoutKind=%u)",
            outerScript->filename(), outerScript->lineno(), outerScript->getWarmUpCount(),
            innerScript->filename(), innerScript->lineno(), innerScript->getWarmUpCount(),
            (unsigned) bailoutKind);

    switch (bailoutKind) {
      
      case Bailout_Inevitable:
      case Bailout_DuringVMCall:
      case Bailout_NonJSFunctionCallee:
      case Bailout_DynamicNameNotFound:
      case Bailout_StringArgumentsEval:
      case Bailout_Overflow:
      case Bailout_Round:
      case Bailout_NonPrimitiveInput:
      case Bailout_PrecisionLoss:
      case Bailout_TypeBarrierO:
      case Bailout_TypeBarrierV:
      case Bailout_MonitorTypes:
      case Bailout_Hole:
      case Bailout_NegativeIndex:
      case Bailout_NonInt32Input:
      case Bailout_NonNumericInput:
      case Bailout_NonBooleanInput:
      case Bailout_NonObjectInput:
      case Bailout_NonStringInput:
      case Bailout_NonSymbolInput:
      case Bailout_NonSimdInt32x4Input:
      case Bailout_NonSimdFloat32x4Input:
      case Bailout_InitialState:
      case Bailout_Debugger:
        
        break;

      
      case Bailout_OverflowInvalidate:
      case Bailout_NonStringInputInvalidate:
      case Bailout_DoubleOutput:
      case Bailout_ObjectIdentityOrTypeGuard:
        if (!HandleBaselineInfoBailout(cx, outerScript, innerScript))
            return false;
        break;

      case Bailout_ArgumentCheck:
        
        break;
      case Bailout_BoundsCheck:
      case Bailout_Neutered:
        if (!HandleBoundsCheckFailure(cx, outerScript, innerScript))
            return false;
        break;
      case Bailout_ShapeGuard:
        if (!HandleShapeGuardFailure(cx, outerScript, innerScript))
            return false;
        break;
      case Bailout_UninitializedLexical:
        if (!HandleLexicalCheckFailure(cx, outerScript, innerScript))
            return false;
        break;
      case Bailout_IonExceptionDebugMode:
        
        
        return false;
      default:
        MOZ_CRASH("Unknown bailout kind!");
    }

    if (!CheckFrequentBailouts(cx, outerScript))
        return false;

    
    topFrame->clearOverridePc();
    return true;
}
