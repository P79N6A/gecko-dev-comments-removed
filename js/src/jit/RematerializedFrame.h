





#ifndef jit_RematerializedFrame_h
#define jit_RematerializedFrame_h

#ifdef JS_ION

#include "jsfun.h"

#include "jit/JitFrameIterator.h"

#include "vm/Stack.h"

namespace js {
namespace jit {





class RematerializedFrame
{
    
    bool prevUpToDate_;

    
    uint8_t *top_;

    
    jsbytecode *pc_;

    size_t frameNo_;
    unsigned numActualArgs_;

    JSScript *script_;
    JSObject *scopeChain_;
    ArgumentsObject *argsObj_;

    Value returnValue_;
    Value thisValue_;
    Value slots_[1];

    RematerializedFrame(ThreadSafeContext *cx, uint8_t *top, unsigned numActualArgs,
                        InlineFrameIterator &iter);

  public:
    static RematerializedFrame *New(ThreadSafeContext *cx, uint8_t *top,
                                    InlineFrameIterator &iter);

    
    
    static bool RematerializeInlineFrames(ThreadSafeContext *cx, uint8_t *top,
                                          InlineFrameIterator &iter,
                                          Vector<RematerializedFrame *> &frames);

    
    
    static void FreeInVector(Vector<RematerializedFrame *> &frames);

    bool prevUpToDate() const {
        return prevUpToDate_;
    }
    void setPrevUpToDate() {
        prevUpToDate_ = true;
    }

    uint8_t *top() const {
        return top_;
    }
    jsbytecode *pc() const {
        return pc_;
    }
    size_t frameNo() const {
        return frameNo_;
    }
    bool inlined() const {
        return frameNo_ > 0;
    }

    JSObject *scopeChain() const {
        return scopeChain_;
    }
    bool hasCallObj() const {
        return maybeFun() && fun()->isHeavyweight();
    }
    CallObject &callObj() const;

    bool hasArgsObj() const {
        return !!argsObj_;
    }
    ArgumentsObject &argsObj() const {
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

    JSScript *script() const {
        return script_;
    }
    JSFunction *fun() const {
        MOZ_ASSERT(isFunctionFrame());
        return script_->functionNonDelazifying();
    }
    JSFunction *maybeFun() const {
        return isFunctionFrame() ? fun() : nullptr;
    }
    JSFunction *callee() const {
        return fun();
    }
    Value calleev() const {
        return ObjectValue(*fun());
    }
    Value &thisValue() {
        return thisValue_;
    }

    unsigned numFormalArgs() const {
        return maybeFun() ? fun()->nargs() : 0;
    }
    unsigned numActualArgs() const {
        return numActualArgs_;
    }

    Value *argv() {
        return slots_;
    }
    Value *locals() {
        return slots_ + numActualArgs_;
    }

    Value &unaliasedVar(unsigned i, MaybeCheckAliasing checkAliasing = CHECK_ALIASING) {
        JS_ASSERT_IF(checkAliasing, !script()->varIsAliased(i));
        JS_ASSERT(i < script()->nfixed());
        return locals()[i];
    }
    Value &unaliasedLocal(unsigned i, MaybeCheckAliasing checkAliasing = CHECK_ALIASING) {
        JS_ASSERT(i < script()->nfixed());
#ifdef DEBUG
        CheckLocalUnaliased(checkAliasing, script(), i);
#endif
        return locals()[i];
    }
    Value &unaliasedFormal(unsigned i, MaybeCheckAliasing checkAliasing = CHECK_ALIASING) {
        JS_ASSERT(i < numFormalArgs());
        JS_ASSERT_IF(checkAliasing, !script()->argsObjAliasesFormals() &&
                                    !script()->formalIsAliased(i));
        return argv()[i];
    }
    Value &unaliasedActual(unsigned i, MaybeCheckAliasing checkAliasing = CHECK_ALIASING) {
        JS_ASSERT(i < numActualArgs());
        JS_ASSERT_IF(checkAliasing, !script()->argsObjAliasesFormals());
        JS_ASSERT_IF(checkAliasing && i < numFormalArgs(), !script()->formalIsAliased(i));
        return argv()[i];
    }

    Value returnValue() const {
        return returnValue_;
    }

    void mark(JSTracer *trc);
    void dump();
};

} 
} 

#endif 
#endif 
