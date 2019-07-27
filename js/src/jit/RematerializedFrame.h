





#ifndef jit_RematerializedFrame_h
#define jit_RematerializedFrame_h

#include "jsfun.h"

#include "jit/JitFrameIterator.h"
#include "jit/JitFrames.h"

#include "vm/Stack.h"

namespace js {
namespace jit {





class RematerializedFrame
{
    
    bool prevUpToDate_;

    
    bool isDebuggee_;

    
    bool hasCallObj_;

    
    bool isConstructing_;

    
    uint8_t* top_;

    
    jsbytecode* pc_;

    size_t frameNo_;
    unsigned numActualArgs_;

    JSScript* script_;
    JSObject* scopeChain_;
    JSFunction* callee_;
    ArgumentsObject* argsObj_;

    Value returnValue_;
    Value thisValue_;
    Value slots_[1];

    RematerializedFrame(JSContext* cx, uint8_t* top, unsigned numActualArgs,
                        InlineFrameIterator& iter, MaybeReadFallback& fallback);

  public:
    static RematerializedFrame* New(JSContext* cx, uint8_t* top, InlineFrameIterator& iter,
                                    MaybeReadFallback& fallback);

    
    
    static bool RematerializeInlineFrames(JSContext* cx, uint8_t* top,
                                          InlineFrameIterator& iter,
                                          MaybeReadFallback& fallback,
                                          Vector<RematerializedFrame*>& frames);

    
    
    static void FreeInVector(Vector<RematerializedFrame*>& frames);

    
    static void MarkInVector(JSTracer* trc, Vector<RematerializedFrame*>& frames);

    bool prevUpToDate() const {
        return prevUpToDate_;
    }
    void setPrevUpToDate() {
        prevUpToDate_ = true;
    }
    void unsetPrevUpToDate() {
        prevUpToDate_ = false;
    }

    bool isDebuggee() const {
        return isDebuggee_;
    }
    void setIsDebuggee() {
        isDebuggee_ = true;
    }
    void unsetIsDebuggee() {
        MOZ_ASSERT(!script()->isDebuggee());
        isDebuggee_ = false;
    }

    uint8_t* top() const {
        return top_;
    }
    JSScript* outerScript() const {
        JitFrameLayout* jsFrame = (JitFrameLayout*)top_;
        return ScriptFromCalleeToken(jsFrame->calleeToken());
    }
    jsbytecode* pc() const {
        return pc_;
    }
    size_t frameNo() const {
        return frameNo_;
    }
    bool inlined() const {
        return frameNo_ > 0;
    }

    JSObject* scopeChain() const {
        return scopeChain_;
    }
    void pushOnScopeChain(ScopeObject& scope);
    bool initFunctionScopeObjects(JSContext* cx);

    bool hasCallObj() const {
        MOZ_ASSERT(fun()->isHeavyweight());
        return hasCallObj_;
    }
    CallObject& callObj() const;

    bool hasArgsObj() const {
        return !!argsObj_;
    }
    ArgumentsObject& argsObj() const {
        MOZ_ASSERT(hasArgsObj());
        MOZ_ASSERT(script()->needsArgsObj());
        return *argsObj_;
    }

    bool isFunctionFrame() const {
        return !!script_->functionNonDelazifying();
    }
    bool isGlobalFrame() const {
        return !isFunctionFrame();
    }
    bool isNonEvalFunctionFrame() const {
        
        return isFunctionFrame();
    }

    JSScript* script() const {
        return script_;
    }
    JSFunction* fun() const {
        MOZ_ASSERT(isFunctionFrame());
        return script_->functionNonDelazifying();
    }
    JSFunction* maybeFun() const {
        return isFunctionFrame() ? fun() : nullptr;
    }
    JSFunction* callee() const {
        MOZ_ASSERT(isFunctionFrame());
        return callee_;
    }
    Value calleev() const {
        return ObjectValue(*callee());
    }
    Value& thisValue() {
        return thisValue_;
    }

    bool isConstructing() const {
        return isConstructing_;
    }

    unsigned numFormalArgs() const {
        return maybeFun() ? fun()->nargs() : 0;
    }
    unsigned numActualArgs() const {
        return numActualArgs_;
    }

    Value* argv() {
        return slots_;
    }
    Value* locals() {
        return slots_ + numActualArgs_ + isConstructing_;
    }

    Value& unaliasedLocal(unsigned i) {
        MOZ_ASSERT(i < script()->nfixed());
        return locals()[i];
    }
    Value& unaliasedFormal(unsigned i, MaybeCheckAliasing checkAliasing = CHECK_ALIASING) {
        MOZ_ASSERT(i < numFormalArgs());
        MOZ_ASSERT_IF(checkAliasing, !script()->argsObjAliasesFormals() &&
                                     !script()->formalIsAliased(i));
        return argv()[i];
    }
    Value& unaliasedActual(unsigned i, MaybeCheckAliasing checkAliasing = CHECK_ALIASING) {
        MOZ_ASSERT(i < numActualArgs());
        MOZ_ASSERT_IF(checkAliasing, !script()->argsObjAliasesFormals());
        MOZ_ASSERT_IF(checkAliasing && i < numFormalArgs(), !script()->formalIsAliased(i));
        return argv()[i];
    }

    Value newTarget() {
        MOZ_ASSERT(isFunctionFrame());
        if (isConstructing())
            return argv()[numActualArgs()];
        return UndefinedValue();
    }

    Value returnValue() const {
        return returnValue_;
    }

    void mark(JSTracer* trc);
    void dump();
};

} 
} 

#endif 
