








































#ifndef jsion_linker_h__
#define jsion_linker_h__

#include "jscntxt.h"
#include "jscompartment.h"
#include "ion/IonCode.h"
#include "ion/IonCompartment.h"
#include "assembler/jit/ExecutableAllocator.h"
#include "ion/IonMacroAssembler.h"

namespace js {
namespace ion {

class Linker
{
    MacroAssembler &masm;

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
    Linker(MacroAssembler &masm)
      : masm(masm)
    { }

    IonCode *newCode(JSContext *cx) {
        return newCode(cx, cx->compartment->ionCompartment());
    }
};

} 
} 

#endif 

