





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
}

namespace JS {





class JS_PUBLIC_API(ProfilingFrameIterator)
{
    js::Activation *activation_;

    static const unsigned StorageSpace = 6 * sizeof(void*);
    mozilla::AlignedStorage<StorageSpace> storage_;
    js::AsmJSProfilingFrameIterator &asmJSIter() {
        JS_ASSERT(!done());
        return *reinterpret_cast<js::AsmJSProfilingFrameIterator*>(storage_.addr());
    }
    const js::AsmJSProfilingFrameIterator &asmJSIter() const {
        JS_ASSERT(!done());
        return *reinterpret_cast<const js::AsmJSProfilingFrameIterator*>(storage_.addr());
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

    
    
    const char *label() const;

  private:
    void iteratorConstruct(const RegisterState &state);
    void iteratorConstruct();
    void iteratorDestroy();
    bool iteratorDone();
};

} 

#endif  
