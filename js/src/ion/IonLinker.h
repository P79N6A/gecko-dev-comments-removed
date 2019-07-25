








































#ifndef jsion_linker_h__
#define jsion_linker_h__

#include "jscntxt.h"
#include "jscompartment.h"
#include "ion/IonCode.h"
#include "ion/IonCompartment.h"
#include "assembler/jit/ExecutableAllocator.h"

#if defined(JS_CPU_X86)
# include "ion/x86/Assembler-x86.h"
#elif defined(JS_CPU_X64)
# include "ion/x64/Assembler-x64.h"
#endif

namespace js {
namespace ion {

template <typename T>
class LinkerT
{
    T &masm;

    IonCode *fail(JSContext *cx) {
        js_ReportOutOfMemory(cx);
        return NULL;
    }

    IonCode *newCode(JSContext *cx, IonCompartment *comp) {
        if (masm.oom())
            return fail(cx);
        JSC::ExecutablePool *pool;
        void *result = comp->execAlloc()->alloc(masm.size(), &pool);
        if (!result)
            return fail(cx);
        masm.executableCopy(result);
        return IonCode::New(cx, (uint8 *)result, masm.size(), pool);
    }

  public:
    LinkerT(T &masm)
      : masm(masm)
    { }

    IonCode *newCode(JSContext *cx) {
        return newCode(cx, cx->compartment->ionCompartment());
    }
};

typedef LinkerT<Assembler> Linker;

} 
} 

#endif 

