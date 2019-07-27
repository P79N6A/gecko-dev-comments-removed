





#ifndef jit_BaselineFrame_h
#define jit_BaselineFrame_h

#include "jit/IonFrames.h"
#include "vm/Stack.h"

namespace js {
namespace jit {

struct BaselineDebugModeOSRInfo;

















class BaselineFrame
{
  public:
    enum Flags {
        
        HAS_RVAL         = 1 << 0,

        
        HAS_CALL_OBJ     = 1 << 2,

        
        HAS_ARGS_OBJ     = 1 << 4,

        
        PREV_UP_TO_DATE  = 1 << 5,

        
        EVAL             = 1 << 6,

        
        HAS_PUSHED_SPS_FRAME = 1 << 8,

        
        OVER_RECURSED    = 1 << 9,

        
        
        HAS_DEBUG_MODE_OSR_INFO = 1 << 10,

        
        
        
        
        
        
        
        HAS_UNWOUND_SCOPE_OVERRIDE_PC = 1 << 11
    };

  protected: 
    
    

    union {
        struct {
            uint32_t loScratchValue_;
            uint32_t hiScratchValue_;
        };
        BaselineDebugModeOSRInfo *debugModeOSRInfo_;
    };
    uint32_t loReturnValue_;              
    uint32_t hiReturnValue_;
    uint32_t frameSize_;
    JSObject *scopeChain_;                
    JSScript *evalScript_;                
    ArgumentsObject *argsObj_;            
    void *unused;                         
    uint32_t unwoundScopeOverrideOffset_; 
    uint32_t flags_;

  public:
    
    
    static const uint32_t FramePointerOffset = sizeof(void *);

    bool initForOsr(InterpreterFrame *fp, uint32_t numStackValues);

    uint32_t frameSize() const {
        return frameSize_;
    }
    void setFrameSize(uint32_t frameSize) {
        frameSize_ = frameSize;
    }
    inline uint32_t *addressOfFrameSize() {
        return &frameSize_;
    }
    JSObject *scopeChain() const {
        return scopeChain_;
    }
    void setScopeChain(JSObject *scopeChain) {
        scopeChain_ = scopeChain;
    }
    inline JSObject **addressOfScopeChain() {
        return &scopeChain_;
    }

    inline Value *addressOfScratchValue() {
        return reinterpret_cast<Value *>(&loScratchValue_);
    }

    inline void pushOnScopeChain(ScopeObject &scope);
    inline void popOffScopeChain();

    inline void popWith(JSContext *cx);

    CalleeToken calleeToken() const {
        uint8_t *pointer = (uint8_t *)this + Size() + offsetOfCalleeToken();
        return *(CalleeToken *)pointer;
    }
    void replaceCalleeToken(CalleeToken token) {
        uint8_t *pointer = (uint8_t *)this + Size() + offsetOfCalleeToken();
        *(CalleeToken *)pointer = token;
    }
    bool isConstructing() const {
        return CalleeTokenIsConstructing(calleeToken());
    }
    JSScript *script() const {
        if (isEvalFrame())
            return evalScript();
        return ScriptFromCalleeToken(calleeToken());
    }
    JSFunction *fun() const {
        return CalleeTokenToFunction(calleeToken());
    }
    JSFunction *maybeFun() const {
        return isFunctionFrame() ? fun() : nullptr;
    }
    JSFunction *callee() const {
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

    Value &unaliasedVar(uint32_t i, MaybeCheckAliasing checkAliasing = CHECK_ALIASING) const {
        JS_ASSERT(i < script()->nfixedvars());
        JS_ASSERT_IF(checkAliasing, !script()->varIsAliased(i));
        return *valueSlot(i);
    }

    Value &unaliasedFormal(unsigned i, MaybeCheckAliasing checkAliasing = CHECK_ALIASING) const {
        JS_ASSERT(i < numFormalArgs());
        JS_ASSERT_IF(checkAliasing, !script()->argsObjAliasesFormals() &&
                                    !script()->formalIsAliased(i));
        return argv()[i];
    }

    Value &unaliasedActual(unsigned i, MaybeCheckAliasing checkAliasing = CHECK_ALIASING) const {
        JS_ASSERT(i < numActualArgs());
        JS_ASSERT_IF(checkAliasing, !script()->argsObjAliasesFormals());
        JS_ASSERT_IF(checkAliasing && i < numFormalArgs(), !script()->formalIsAliased(i));
        return argv()[i];
    }

    Value &unaliasedLocal(uint32_t i, MaybeCheckAliasing checkAliasing = CHECK_ALIASING) const {
        JS_ASSERT(i < script()->nfixed());
#ifdef DEBUG
        CheckLocalUnaliased(checkAliasing, script(), i);
#endif
        return *valueSlot(i);
    }

    unsigned numActualArgs() const {
        return *(size_t *)(reinterpret_cast<const uint8_t *>(this) +
                             BaselineFrame::Size() +
                             offsetOfNumActualArgs());
    }
    unsigned numFormalArgs() const {
        return script()->functionNonDelazifying()->nargs();
    }
    Value &thisValue() const {
        return *(Value *)(reinterpret_cast<const uint8_t *>(this) +
                         BaselineFrame::Size() +
                         offsetOfThis());
    }
    Value *argv() const {
        return (Value *)(reinterpret_cast<const uint8_t *>(this) +
                         BaselineFrame::Size() +
                         offsetOfArg(0));
    }

    bool copyRawFrameSlots(AutoValueVector *vec) const;

    bool hasReturnValue() const {
        return flags_ & HAS_RVAL;
    }
    MutableHandleValue returnValue() {
        if (!hasReturnValue())
            addressOfReturnValue()->setUndefined();
        return MutableHandleValue::fromMarkedLocation(addressOfReturnValue());
    }
    void setReturnValue(const Value &v) {
        returnValue().set(v);
        flags_ |= HAS_RVAL;
    }
    inline Value *addressOfReturnValue() {
        return reinterpret_cast<Value *>(&loReturnValue_);
    }

    bool hasCallObj() const {
        return flags_ & HAS_CALL_OBJ;
    }

    inline CallObject &callObj() const;

    void setFlags(uint32_t flags) {
        flags_ = flags;
    }
    uint32_t *addressOfFlags() {
        return &flags_;
    }

    inline bool pushBlock(JSContext *cx, Handle<StaticBlockObject *> block);
    inline void popBlock(JSContext *cx);

    bool strictEvalPrologue(JSContext *cx);
    bool heavyweightFunPrologue(JSContext *cx);
    bool initFunctionScopeObjects(JSContext *cx);

    void initArgsObjUnchecked(ArgumentsObject &argsobj) {
        flags_ |= HAS_ARGS_OBJ;
        argsObj_ = &argsobj;
    }
    void initArgsObj(ArgumentsObject &argsobj) {
        JS_ASSERT(script()->needsArgsObj());
        initArgsObjUnchecked(argsobj);
    }
    bool hasArgsObj() const {
        return flags_ & HAS_ARGS_OBJ;
    }
    ArgumentsObject &argsObj() const {
        JS_ASSERT(hasArgsObj());
        JS_ASSERT(script()->needsArgsObj());
        return *argsObj_;
    }

    bool prevUpToDate() const {
        return flags_ & PREV_UP_TO_DATE;
    }
    void setPrevUpToDate() {
        flags_ |= PREV_UP_TO_DATE;
    }

    JSScript *evalScript() const {
        JS_ASSERT(isEvalFrame());
        return evalScript_;
    }

    bool hasPushedSPSFrame() const {
        return flags_ & HAS_PUSHED_SPS_FRAME;
    }

    void setPushedSPSFrame() {
        flags_ |= HAS_PUSHED_SPS_FRAME;
    }

    void unsetPushedSPSFrame() {
        flags_ &= ~HAS_PUSHED_SPS_FRAME;
    }

    bool overRecursed() const {
        return flags_ & OVER_RECURSED;
    }

    void setOverRecursed() {
        flags_ |= OVER_RECURSED;
    }

    BaselineDebugModeOSRInfo *debugModeOSRInfo() {
        MOZ_ASSERT(flags_ & HAS_DEBUG_MODE_OSR_INFO);
        return debugModeOSRInfo_;
    }

    BaselineDebugModeOSRInfo *getDebugModeOSRInfo() {
        if (flags_ & HAS_DEBUG_MODE_OSR_INFO)
            return debugModeOSRInfo();
        return nullptr;
    }

    void setDebugModeOSRInfo(BaselineDebugModeOSRInfo *info) {
        flags_ |= HAS_DEBUG_MODE_OSR_INFO;
        debugModeOSRInfo_ = info;
    }

    void deleteDebugModeOSRInfo();

    jsbytecode *unwoundScopeOverridePc() {
        MOZ_ASSERT(flags_ & HAS_UNWOUND_SCOPE_OVERRIDE_PC);
        return script()->offsetToPC(unwoundScopeOverrideOffset_);
    }

    jsbytecode *getUnwoundScopeOverridePc() {
        if (flags_ & HAS_UNWOUND_SCOPE_OVERRIDE_PC)
            return unwoundScopeOverridePc();
        return nullptr;
    }

    void setUnwoundScopeOverridePc(jsbytecode *pc) {
        flags_ |= HAS_UNWOUND_SCOPE_OVERRIDE_PC;
        unwoundScopeOverrideOffset_ = script()->pcToOffset(pc);
    }

    void trace(JSTracer *trc, JitFrameIterator &frame);

    bool isFunctionFrame() const {
        return CalleeTokenIsFunction(calleeToken());
    }
    bool isGlobalFrame() const {
        return !CalleeTokenIsFunction(calleeToken());
    }
     bool isEvalFrame() const {
        return flags_ & EVAL;
    }
    bool isStrictEvalFrame() const {
        return isEvalFrame() && script()->strict();
    }
    bool isNonStrictEvalFrame() const {
        return isEvalFrame() && !script()->strict();
    }
    bool isDirectEvalFrame() const {
        return isEvalFrame() && script()->staticLevel() > 0;
    }
    bool isNonStrictDirectEvalFrame() const {
        return isNonStrictEvalFrame() && isDirectEvalFrame();
    }
    bool isNonEvalFunctionFrame() const {
        return isFunctionFrame() && !isEvalFrame();
    }
    bool isDebuggerFrame() const {
        return false;
    }
    bool isGeneratorFrame() const {
        return false;
    }

    IonJSFrameLayout *framePrefix() const {
        uint8_t *fp = (uint8_t *)this + Size() + FramePointerOffset;
        return (IonJSFrameLayout *)fp;
    }

    
    static size_t offsetOfCalleeToken() {
        return FramePointerOffset + js::jit::IonJSFrameLayout::offsetOfCalleeToken();
    }
    static size_t offsetOfThis() {
        return FramePointerOffset + js::jit::IonJSFrameLayout::offsetOfThis();
    }
    static size_t offsetOfArg(size_t index) {
        return FramePointerOffset + js::jit::IonJSFrameLayout::offsetOfActualArg(index);
    }
    static size_t offsetOfNumActualArgs() {
        return FramePointerOffset + js::jit::IonJSFrameLayout::offsetOfNumActualArgs();
    }
    static size_t Size() {
        return sizeof(BaselineFrame);
    }

    
    
    
    static int reverseOffsetOfFrameSize() {
        return -int(Size()) + offsetof(BaselineFrame, frameSize_);
    }
    static int reverseOffsetOfScratchValue() {
        return -int(Size()) + offsetof(BaselineFrame, loScratchValue_);
    }
    static int reverseOffsetOfScopeChain() {
        return -int(Size()) + offsetof(BaselineFrame, scopeChain_);
    }
    static int reverseOffsetOfArgsObj() {
        return -int(Size()) + offsetof(BaselineFrame, argsObj_);
    }
    static int reverseOffsetOfFlags() {
        return -int(Size()) + offsetof(BaselineFrame, flags_);
    }
    static int reverseOffsetOfEvalScript() {
        return -int(Size()) + offsetof(BaselineFrame, evalScript_);
    }
    static int reverseOffsetOfReturnValue() {
        return -int(Size()) + offsetof(BaselineFrame, loReturnValue_);
    }
    static int reverseOffsetOfLocal(size_t index) {
        return -int(Size()) - (index + 1) * sizeof(Value);
    }
};


JS_STATIC_ASSERT(((sizeof(BaselineFrame) + BaselineFrame::FramePointerOffset) % 8) == 0);

} 
} 

#endif
