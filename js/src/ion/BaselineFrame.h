






#if !defined(jsion_baseline_frame_h__) && defined(JS_ION)
#define jsion_baseline_frame_h__

#include "jscntxt.h"
#include "jscompartment.h"

#include "IonFrames.h"

namespace js {
namespace ion {









class BaselineFrame
{
  public:
    enum Flags {
        
        HAS_RVAL         = 1 << 0,

        
        HAS_BLOCKCHAIN   = 1 << 1,

        
        PREV_UP_TO_DATE  = 1 << 2,
    };

  protected: 
    
    
    uint32_t loScratchValue_;
    uint32_t hiScratchValue_;
    uint32_t loReturnValue_;
    uint32_t hiReturnValue_;
    size_t frameSize_;
    JSObject *scopeChain_;
    StaticBlockObject *blockChain_;
    uint32_t flags_;

#if JS_BITS_PER_WORD == 32
    
    uint32_t padding_;
#endif

  public:
    
    
    static const uint32_t FramePointerOffset = sizeof(void *);

    size_t frameSize() const {
        return frameSize_;
    }
    UnrootedObject scopeChain() const {
        return scopeChain_;
    }

    inline void pushOnScopeChain(ScopeObject &scope);
    inline void popOffScopeChain();

    CalleeToken calleeToken() const {
        uint8_t *pointer = (uint8_t *)this + Size() + offsetOfCalleeToken();
        return *(CalleeToken *)pointer;
    }
    UnrootedScript script() const {
        return ScriptFromCalleeToken(calleeToken());
    }
    UnrootedFunction fun() const {
        return CalleeTokenToFunction(calleeToken());
    }
    UnrootedFunction callee() const {
        return CalleeTokenToFunction(calleeToken());
    }
    Value calleev() const {
        return ObjectValue(*callee());
    }
    size_t numValueSlots() const {
        size_t size = frameSize();

        JS_ASSERT(size >= BaselineFrame::FramePointerOffset + BaselineFrame::Size());
        size -= BaselineFrame::FramePointerOffset + BaselineFrame::Size();

        JS_ASSERT((size % sizeof(Value)) == 0);
        return size / sizeof(Value);
    }
    Value *valueSlot(size_t slot) const {
        JS_ASSERT(slot < numValueSlots());
        return (Value *)this - (slot + 1);
    }

    Value &unaliasedVar(unsigned i, MaybeCheckAliasing checkAliasing) const {
        JS_ASSERT_IF(checkAliasing, !script()->varIsAliased(i));
        JS_ASSERT(i < script()->nfixed);
        return *valueSlot(i);
    }

    Value &unaliasedFormal(unsigned i, MaybeCheckAliasing checkAliasing) const {
        JS_ASSERT(i < numFormalArgs());
        JS_ASSERT_IF(checkAliasing, !script()->argsObjAliasesFormals());
        JS_ASSERT_IF(checkAliasing, !script()->formalIsAliased(i));
        return formals()[i];
    }

    Value &unaliasedLocal(unsigned i, MaybeCheckAliasing checkAliasing) const {
#ifdef DEBUG
        CheckLocalUnaliased(checkAliasing, script(), maybeBlockChain(), i);
#endif
        return *valueSlot(i);
    }

    unsigned numActualArgs() const {
        return *(size_t *)(reinterpret_cast<const uint8_t *>(this) +
                             BaselineFrame::Size() +
                             offsetOfNumActualArgs());
    }
    unsigned numFormalArgs() const {
        return script()->function()->nargs;
    }
    Value &thisValue() const {
        return *(Value *)(reinterpret_cast<const uint8_t *>(this) +
                         BaselineFrame::Size() +
                         offsetOfThis());
    }
    Value *formals() const {
        return (Value *)(reinterpret_cast<const uint8_t *>(this) +
                         BaselineFrame::Size() +
                         offsetOfArg(0));
    }
    Value *actuals() const {
        return formals();
    }

    bool copyRawFrameSlots(AutoValueVector *vec) const;

    bool hasReturnValue() const {
        return flags_ & HAS_RVAL;
    }
    Value *returnValue() {
        return reinterpret_cast<Value *>(&loReturnValue_);
    }
    void setReturnValue(const Value &v) {
        flags_ |= HAS_RVAL;
        *returnValue() = v;
    }

    bool hasBlockChain() const {
        return (flags_ & HAS_BLOCKCHAIN) && blockChain_;
    }

    StaticBlockObject &blockChain() const {
        JS_ASSERT(hasBlockChain());
        return *blockChain_;
    }

    StaticBlockObject *maybeBlockChain() const {
        return hasBlockChain() ? blockChain_ : NULL;
    }

    void setBlockChain(StaticBlockObject &block) {
        flags_ |= HAS_BLOCKCHAIN;
        blockChain_ = &block;
    }

    inline bool pushBlock(JSContext *cx, Handle<StaticBlockObject *> block);
    inline void popBlock(JSContext *cx);

    bool prevUpToDate() const {
        return flags_ & PREV_UP_TO_DATE;
    }
    void setPrevUpToDate() {
        flags_ |= PREV_UP_TO_DATE;
    }

    void *maybeHookData() const {
        return NULL;
    }

    void trace(JSTracer *trc);

    bool isGlobalFrame() const {
        return !script()->function();
    }
    bool isEvalFrame() const {
        return false;
    }
    bool isDebuggerFrame() const {
        return false;
    }
    bool isNonStrictDirectEvalFrame() const {
        return false;
    }
    bool isStrictEvalFrame() const {
        return false;
    }
    bool isNonEvalFunctionFrame() const {
        return !!script()->function();
    }
    bool isFunctionFrame() const {
        return !!script()->function();
    }

    
    static size_t offsetOfCalleeToken() {
        return FramePointerOffset + js::ion::IonJSFrameLayout::offsetOfCalleeToken();
    }
    static size_t offsetOfThis() {
        return FramePointerOffset + js::ion::IonJSFrameLayout::offsetOfThis();
    }
    static size_t offsetOfArg(size_t index) {
        return FramePointerOffset + js::ion::IonJSFrameLayout::offsetOfActualArg(index);
    }
    static size_t offsetOfNumActualArgs() {
        return FramePointerOffset + js::ion::IonJSFrameLayout::offsetOfNumActualArgs();
    }
    static size_t Size() {
        return sizeof(BaselineFrame);
    }

    
    static BaselineFrame *FromIonJSFrame(IonJSFrameLayout *frame) {
        size_t adjust = FramePointerOffset + BaselineFrame::Size();
        return reinterpret_cast<BaselineFrame *>(reinterpret_cast<uint8_t *>(frame) - adjust);
    }

    
    
    
    static size_t reverseOffsetOfFrameSize() {
        return -BaselineFrame::Size() + offsetof(BaselineFrame, frameSize_);
    }
    static size_t reverseOffsetOfScratchValue() {
        return -BaselineFrame::Size() + offsetof(BaselineFrame, loScratchValue_);
    }
    static size_t reverseOffsetOfScopeChain() {
        return -BaselineFrame::Size() + offsetof(BaselineFrame, scopeChain_);
    }
    static size_t reverseOffsetOfBlockChain() {
        return -BaselineFrame::Size() + offsetof(BaselineFrame, blockChain_);
    }
    static size_t reverseOffsetOfFlags() {
        return -BaselineFrame::Size() + offsetof(BaselineFrame, flags_);
    }
    static size_t reverseOffsetOfReturnValue() {
        return -BaselineFrame::Size() + offsetof(BaselineFrame, loReturnValue_);
    }
    static size_t reverseOffsetOfLocal(size_t index) {
        return -BaselineFrame::Size() - (index + 1) * sizeof(Value);
    }
};


JS_STATIC_ASSERT(((sizeof(BaselineFrame) + BaselineFrame::FramePointerOffset) % 8) == 0);

} 
} 

#endif

