





#ifndef jit_Linker_h
#define jit_Linker_h

#include "jscntxt.h"
#include "jscompartment.h"
#include "jsgc.h"

#include "jit/ExecutableAllocator.h"
#include "jit/IonCode.h"
#include "jit/JitCompartment.h"
#include "jit/MacroAssembler.h"

namespace js {
namespace jit {

class Linker
{
    MacroAssembler &masm;

    JitCode *fail(JSContext *cx) {
        ReportOutOfMemory(cx);
        return nullptr;
    }

  public:
    explicit Linker(MacroAssembler &masm)
      : masm(masm)
    {
        masm.finish();
    }

    template <AllowGC allowGC>
    JitCode *newCode(JSContext *cx, CodeKind kind) {
        MOZ_ASSERT(masm.numAsmJSAbsoluteLinks() == 0);

        gc::AutoSuppressGC suppressGC(cx);
        if (masm.oom())
            return fail(cx);

        ExecutablePool *pool;
        size_t bytesNeeded = masm.bytesNeeded() + sizeof(JitCode *) + CodeAlignment;
        if (bytesNeeded >= MAX_BUFFER_SIZE)
            return fail(cx);

        
        bytesNeeded = AlignBytes(bytesNeeded, sizeof(void *));

        ExecutableAllocator &execAlloc = cx->runtime()->jitRuntime()->execAlloc();
        uint8_t *result = (uint8_t *)execAlloc.alloc(bytesNeeded, &pool, kind);
        if (!result)
            return fail(cx);

        
        uint8_t *codeStart = result + sizeof(JitCode *);

        
        codeStart = (uint8_t *)AlignBytes((uintptr_t)codeStart, CodeAlignment);
        uint32_t headerSize = codeStart - result;
        JitCode *code = JitCode::New<allowGC>(cx, codeStart, bytesNeeded - headerSize,
                                              headerSize, pool, kind);
        if (!code)
            return nullptr;
        if (masm.oom())
            return fail(cx);
        code->copyFrom(masm);
        masm.link(code);
        if (masm.embedsNurseryPointers())
            cx->runtime()->gc.storeBuffer.putWholeCellFromMainThread(code);
        return code;
    }
};

} 
} 

#endif 
