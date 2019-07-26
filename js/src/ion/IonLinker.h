






#ifndef jsion_linker_h__
#define jsion_linker_h__

#include "jscntxt.h"
#include "jscompartment.h"
#include "ion/IonCode.h"
#include "ion/IonCompartment.h"
#include "assembler/jit/ExecutableAllocator.h"
#include "ion/IonMacroAssembler.h"
#include "jsgcinlines.h"

namespace js {
namespace ion {

static const int CodeAlignment = 8;
class Linker
{
    MacroAssembler &masm;

    IonCode *fail(JSContext *cx) {
        js_ReportOutOfMemory(cx);
        return NULL;
    }

    IonCode *newCode(JSContext *cx, IonCompartment *comp) {
        gc::AutoSuppressGC suppressGC(cx);
        if (masm.oom())
            return fail(cx);

        JSC::ExecutablePool *pool;
        size_t bytesNeeded = masm.bytesNeeded() + sizeof(IonCode *) + CodeAlignment;
        if (bytesNeeded >= MAX_BUFFER_SIZE)
            return fail(cx);

        uint8_t *result = (uint8_t *)comp->execAlloc()->alloc(bytesNeeded, &pool, JSC::ION_CODE);
        if (!result)
            return fail(cx);

        
        uint8_t *codeStart = result + sizeof(IonCode *);

        
        codeStart = (uint8_t *)AlignBytes((uintptr_t)codeStart, CodeAlignment);
        uint32_t headerSize = codeStart - result;
        IonCode *code = IonCode::New(cx, codeStart,
                                     bytesNeeded - headerSize, pool);
        if (!code)
            return NULL;
        if (masm.oom())
            return fail(cx);
        code->copyFrom(masm);
        masm.link(code);
        return code;
    }

  public:
    Linker(MacroAssembler &masm)
      : masm(masm)
    {
        masm.finish();
    }

    IonCode *newCode(JSContext *cx) {
        return newCode(cx, cx->compartment->ionCompartment());
    }
};

} 
} 

#endif 

