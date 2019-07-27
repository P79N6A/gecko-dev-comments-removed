





#ifndef jit_IonLinker_h
#define jit_IonLinker_h

#include "jscntxt.h"
#include "jscompartment.h"
#include "jsgc.h"

#include "jit/ExecutableAllocator.h"
#include "jit/IonCode.h"
#include "jit/IonMacroAssembler.h"
#include "jit/JitCompartment.h"

namespace js {
namespace jit {

class Linker
{
    MacroAssembler &masm;

    JitCode *fail(JSContext *cx) {
        js_ReportOutOfMemory(cx);
        return nullptr;
    }

    template <AllowGC allowGC>
    JitCode *newCode(JSContext *cx, ExecutableAllocator *execAlloc, CodeKind kind) {
        JS_ASSERT(masm.numAsmJSAbsoluteLinks() == 0);

        gc::AutoSuppressGC suppressGC(cx);
        if (masm.oom())
            return fail(cx);

        ExecutablePool *pool;
        size_t bytesNeeded = masm.bytesNeeded() + sizeof(JitCode *) + CodeAlignment;
        if (bytesNeeded >= MAX_BUFFER_SIZE)
            return fail(cx);

        
        bytesNeeded = AlignBytes(bytesNeeded, sizeof(void *));

        uint8_t *result = (uint8_t *)execAlloc->alloc(bytesNeeded, &pool, kind);
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
#ifdef JSGC_GENERATIONAL
        if (masm.embedsNurseryPointers())
            cx->runtime()->gc.storeBuffer.putWholeCellFromMainThread(code);
#endif
        return code;
    }

  public:
    explicit Linker(MacroAssembler &masm)
      : masm(masm)
    {
        masm.finish();
    }

    template <AllowGC allowGC>
    JitCode *newCode(JSContext *cx, CodeKind kind) {
        return newCode<allowGC>(cx, cx->runtime()->jitRuntime()->execAlloc(), kind);
    }

    JitCode *newCodeForIonScript(JSContext *cx) {
        
        
        JS_ASSERT(cx->runtime()->currentThreadOwnsInterruptLock());

        ExecutableAllocator *alloc = cx->runtime()->jitRuntime()->getIonAlloc(cx);
        if (!alloc)
            return nullptr;

        return newCode<CanGC>(cx, alloc, ION_CODE);
    }
};

} 
} 

#endif 
