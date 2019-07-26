





#ifndef jit_BytecodeAnalysis_h
#define jit_BytecodeAnalysis_h

#include "jsscript.h"
#include "jit/IonAllocPolicy.h"
#include "js/Vector.h"

namespace js {
namespace jit {


struct BytecodeInfo
{
    static const uint16_t MAX_STACK_DEPTH = 0xffffU;
    uint16_t stackDepth;
    bool initialized : 1;
    bool jumpTarget : 1;
    bool jumpFallthrough : 1;
    bool fallthrough : 1;

    
    bool loopEntryInCatchOrFinally : 1;

    void init(unsigned depth) {
        JS_ASSERT(depth <= MAX_STACK_DEPTH);
        JS_ASSERT_IF(initialized, stackDepth == depth);
        initialized = true;
        stackDepth = depth;
    }
};

class BytecodeAnalysis
{
    JSScript *script_;
    Vector<BytecodeInfo, 0, IonAllocPolicy> infos_;

    bool usesScopeChain_;
    bool hasTryFinally_;

  public:
    explicit BytecodeAnalysis(JSScript *script);

    bool init(JSContext *cx);

    BytecodeInfo &info(jsbytecode *pc) {
        JS_ASSERT(infos_[pc - script_->code].initialized);
        return infos_[pc - script_->code];
    }

    BytecodeInfo *maybeInfo(jsbytecode *pc) {
        if (infos_[pc - script_->code].initialized)
            return &infos_[pc - script_->code];
        return nullptr;
    }

    bool usesScopeChain() {
        return usesScopeChain_;
    }

    bool hasTryFinally() {
        return hasTryFinally_;
    }
};


} 
} 

#endif 
