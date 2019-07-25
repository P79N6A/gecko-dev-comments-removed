








































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
        masm.flush();
        if (masm.oom())
            return fail(cx);

        JSC::ExecutablePool *pool;
        size_t bytesNeeded = masm.bytesNeeded() + sizeof(IonCode *);
        if (bytesNeeded >= MAX_BUFFER_SIZE)
            return fail(cx);

        uint8 *result = (uint8 *)comp->execAlloc()->alloc(bytesNeeded, &pool, JSC::METHOD_CODE);
        if (!result)
            return fail(cx);

        IonCode *code = IonCode::New(cx, result + sizeof(IonCode *),
                                     bytesNeeded - sizeof(IonCode *), pool);
        if (!code)
            return NULL;
        code->copyFrom(masm);
        return code;
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

