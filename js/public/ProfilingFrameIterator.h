





#ifndef js_ProfilingFrameIterator_h
#define js_ProfilingFrameIterator_h

#include "mozilla/Alignment.h"

#include <stdint.h>

#include "js/Utility.h"

class JSAtom;
struct JSRuntime;
namespace js { class AsmJSActivation; class AsmJSProfilingFrameIterator; }

namespace JS {





class JS_PUBLIC_API(ProfilingFrameIterator)
{
    js::AsmJSActivation *activation_;

    static const unsigned StorageSpace = 5 * sizeof(void*);
    mozilla::AlignedStorage<StorageSpace> storage_;
    js::AsmJSProfilingFrameIterator &iter() {
        JS_ASSERT(!done());
        return *reinterpret_cast<js::AsmJSProfilingFrameIterator*>(storage_.addr());
    }
    const js::AsmJSProfilingFrameIterator &iter() const {
        JS_ASSERT(!done());
        return *reinterpret_cast<const js::AsmJSProfilingFrameIterator*>(storage_.addr());
    }

    void settle();

  public:
    struct RegisterState
    {
        void *pc;
        void *sp;
#if defined(JS_CODEGEN_ARM)
        void *lr;
#endif
    };

    ProfilingFrameIterator(JSRuntime *rt, const RegisterState &state);
    ~ProfilingFrameIterator();
    void operator++();
    bool done() const { return !activation_; }

    enum Kind {
        Function,
        AsmJSTrampoline,
        CppFunction
    };
    Kind kind() const;

    
    JSAtom *functionDisplayAtom() const;
    const char *functionFilename() const;

    
    const char *nonFunctionDescription() const;
};

} 

#endif  
