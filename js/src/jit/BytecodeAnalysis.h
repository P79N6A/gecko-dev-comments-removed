





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
    bool hasSetArg_;

  public:
    explicit BytecodeAnalysis(TempAllocator &alloc, JSScript *script);

    bool init(TempAllocator &alloc, GSNCache &gsn);

    BytecodeInfo &info(jsbytecode *pc) {
        JS_ASSERT(infos_[script_->pcToOffset(pc)].initialized);
        return infos_[script_->pcToOffset(pc)];
    }

    BytecodeInfo *maybeInfo(jsbytecode *pc) {
        if (infos_[script_->pcToOffset(pc)].initialized)
            return &infos_[script_->pcToOffset(pc)];
        return nullptr;
    }

    bool usesScopeChain() const {
        return usesScopeChain_;
    }

    bool hasTryFinally() const {
        return hasTryFinally_;
    }

    bool hasSetArg() const {
        return hasSetArg_;
    }
};


} 
} 

#endif 
