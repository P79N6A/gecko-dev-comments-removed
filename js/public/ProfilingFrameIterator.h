





#ifndef js_ProfilingFrameIterator_h
#define js_ProfilingFrameIterator_h

#include "mozilla/Alignment.h"
#include "mozilla/Maybe.h"

#include <stdint.h>

#include "js/Utility.h"

struct JSRuntime;

namespace js {
    class Activation;
    class AsmJSProfilingFrameIterator;
    namespace jit {
        class JitActivation;
        class JitProfilingFrameIterator;
        class JitcodeGlobalEntry;
    }
}

namespace JS {





class JS_PUBLIC_API(ProfilingFrameIterator)
{
    JSRuntime* rt_;
    uint32_t sampleBufferGen_;
    js::Activation* activation_;

    
    
    
    void* savedPrevJitTop_;

    static const unsigned StorageSpace = 6 * sizeof(void*);
    mozilla::AlignedStorage<StorageSpace> storage_;
    js::AsmJSProfilingFrameIterator& asmJSIter() {
        MOZ_ASSERT(!done());
        MOZ_ASSERT(isAsmJS());
        return *reinterpret_cast<js::AsmJSProfilingFrameIterator*>(storage_.addr());
    }
    const js::AsmJSProfilingFrameIterator& asmJSIter() const {
        MOZ_ASSERT(!done());
        MOZ_ASSERT(isAsmJS());
        return *reinterpret_cast<const js::AsmJSProfilingFrameIterator*>(storage_.addr());
    }

    js::jit::JitProfilingFrameIterator& jitIter() {
        MOZ_ASSERT(!done());
        MOZ_ASSERT(isJit());
        return *reinterpret_cast<js::jit::JitProfilingFrameIterator*>(storage_.addr());
    }

    const js::jit::JitProfilingFrameIterator& jitIter() const {
        MOZ_ASSERT(!done());
        MOZ_ASSERT(isJit());
        return *reinterpret_cast<const js::jit::JitProfilingFrameIterator*>(storage_.addr());
    }

    void settle();

    bool hasSampleBufferGen() const {
        return sampleBufferGen_ != UINT32_MAX;
    }

  public:
    struct RegisterState
    {
        RegisterState() : pc(nullptr), sp(nullptr), lr(nullptr) {}
        void* pc;
        void* sp;
        void* lr;
    };

    ProfilingFrameIterator(JSRuntime* rt, const RegisterState& state,
                           uint32_t sampleBufferGen = UINT32_MAX);
    ~ProfilingFrameIterator();
    void operator++();
    bool done() const { return !activation_; }

    
    
    
    
    
    void* stackAddress() const;

    enum FrameKind
    {
      Frame_Baseline,
      Frame_Ion,
      Frame_AsmJS
    };

    struct Frame
    {
        FrameKind kind;
        void* stackAddress;
        void* returnAddress;
        void* activation;
        const char* label;
    };

    bool isAsmJS() const;
    bool isJit() const;

    uint32_t extractStack(Frame* frames, uint32_t offset, uint32_t end) const;

    mozilla::Maybe<Frame> getPhysicalFrameWithoutLabel() const;

  private:
    mozilla::Maybe<Frame> getPhysicalFrameAndEntry(js::jit::JitcodeGlobalEntry* entry) const;

    void iteratorConstruct(const RegisterState& state);
    void iteratorConstruct();
    void iteratorDestroy();
    bool iteratorDone();
};

extern JS_PUBLIC_API(ProfilingFrameIterator::FrameKind)
GetProfilingFrameKindFromNativeAddr(JSRuntime* runtime, void* pc);

JS_FRIEND_API(bool)
IsProfilingEnabledForRuntime(JSRuntime* runtime);









JS_FRIEND_API(void)
UpdateJSRuntimeProfilerSampleBufferGen(JSRuntime* runtime, uint32_t generation,
                                       uint32_t lapCount);

struct ForEachProfiledFrameOp
{
    
    virtual void operator()(const char* label, bool mightHaveTrackedOptimizations) = 0;
};

JS_PUBLIC_API(void)
ForEachProfiledFrame(JSRuntime* rt, void* addr, ForEachProfiledFrameOp& op);

} 

#endif  
