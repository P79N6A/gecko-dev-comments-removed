





#ifndef js_ProfilingFrameIterator_h
#define js_ProfilingFrameIterator_h

#include "mozilla/Alignment.h"

#include <stdint.h>

#include "js/Utility.h"

class JSAtom;
struct JSRuntime;

namespace js {
    class Activation;
    class AsmJSProfilingFrameIterator;
    namespace jit {
        class JitActivation;
        class JitProfilingFrameIterator;
    }
}

namespace JS {





class JS_PUBLIC_API(ProfilingFrameIterator)
{
    JSRuntime *rt_;
    js::Activation *activation_;

    
    
    
    void *savedPrevJitTop_;

    static const unsigned StorageSpace = 6 * sizeof(void*);
    mozilla::AlignedStorage<StorageSpace> storage_;
    js::AsmJSProfilingFrameIterator &asmJSIter() {
        MOZ_ASSERT(!done());
        MOZ_ASSERT(isAsmJS());
        return *reinterpret_cast<js::AsmJSProfilingFrameIterator*>(storage_.addr());
    }
    const js::AsmJSProfilingFrameIterator &asmJSIter() const {
        MOZ_ASSERT(!done());
        MOZ_ASSERT(isAsmJS());
        return *reinterpret_cast<const js::AsmJSProfilingFrameIterator*>(storage_.addr());
    }

    js::jit::JitProfilingFrameIterator &jitIter() {
        MOZ_ASSERT(!done());
        MOZ_ASSERT(isJit());
        return *reinterpret_cast<js::jit::JitProfilingFrameIterator*>(storage_.addr());
    }

    const js::jit::JitProfilingFrameIterator &jitIter() const {
        MOZ_ASSERT(!done());
        MOZ_ASSERT(isJit());
        return *reinterpret_cast<const js::jit::JitProfilingFrameIterator*>(storage_.addr());
    }

    void settle();

  public:
    struct RegisterState
    {
        RegisterState() : pc(nullptr), sp(nullptr), lr(nullptr) {}
        void *pc;
        void *sp;
        void *lr;
    };

    ProfilingFrameIterator(JSRuntime *rt, const RegisterState &state);
    ~ProfilingFrameIterator();
    void operator++();
    bool done() const { return !activation_; }

    
    
    
    
    
    void *stackAddress() const;

    enum FrameKind
    {
      Frame_Baseline,
      Frame_Ion,
      Frame_AsmJS
    };

    struct Frame
    {
        FrameKind kind;
        void *stackAddress;
        void *returnAddress;
        void *activation;
        const char *label;
    };
    uint32_t extractStack(Frame *frames, uint32_t offset, uint32_t end) const;

  private:
    void iteratorConstruct(const RegisterState &state);
    void iteratorConstruct();
    void iteratorDestroy();
    bool iteratorDone();

    bool isAsmJS() const;
    bool isJit() const;
};

} 

#endif  
