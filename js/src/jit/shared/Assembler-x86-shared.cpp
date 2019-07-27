





#include "gc/Marking.h"
#include "jit/JitCompartment.h"
#if defined(JS_CODEGEN_X86)
# include "jit/x86/MacroAssembler-x86.h"
#elif defined(JS_CODEGEN_X64)
# include "jit/x64/MacroAssembler-x64.h"
#else
# error "Wrong architecture. Only x86 and x64 should build this file!"
#endif

using namespace js;
using namespace js::jit;

void
AssemblerX86Shared::copyJumpRelocationTable(uint8_t *dest)
{
    if (jumpRelocations_.length())
        memcpy(dest, jumpRelocations_.buffer(), jumpRelocations_.length());
}

void
AssemblerX86Shared::copyDataRelocationTable(uint8_t *dest)
{
    if (dataRelocations_.length())
        memcpy(dest, dataRelocations_.buffer(), dataRelocations_.length());
}

void
AssemblerX86Shared::copyPreBarrierTable(uint8_t *dest)
{
    if (preBarriers_.length())
        memcpy(dest, preBarriers_.buffer(), preBarriers_.length());
}

static void
TraceDataRelocations(JSTracer *trc, uint8_t *buffer, CompactBufferReader &reader)
{
    while (reader.more()) {
        size_t offset = reader.readUnsigned();
        void **ptr = X86Assembler::getPointerRef(buffer + offset);

#ifdef JS_PUNBOX64
        
        
        uintptr_t *word = reinterpret_cast<uintptr_t *>(ptr);
        if (*word >> JSVAL_TAG_SHIFT) {
            jsval_layout layout;
            layout.asBits = *word;
            Value v = IMPL_TO_JSVAL(layout);
            gc::MarkValueUnbarriered(trc, &v, "ion-masm-value");
            *word = JSVAL_TO_IMPL(v).asBits;
            continue;
        }
#endif

        
        gc::MarkGCThingUnbarriered(trc, reinterpret_cast<void **>(ptr), "ion-masm-ptr");
    }
}


void
AssemblerX86Shared::TraceDataRelocations(JSTracer *trc, JitCode *code, CompactBufferReader &reader)
{
    ::TraceDataRelocations(trc, code->raw(), reader);
}

void
AssemblerX86Shared::trace(JSTracer *trc)
{
    for (size_t i = 0; i < jumps_.length(); i++) {
        RelativePatch &rp = jumps_[i];
        if (rp.kind == Relocation::JITCODE) {
            JitCode *code = JitCode::FromExecutable((uint8_t *)rp.target);
            MarkJitCodeUnbarriered(trc, &code, "masmrel32");
            JS_ASSERT(code == JitCode::FromExecutable((uint8_t *)rp.target));
        }
    }
    if (dataRelocations_.length()) {
        CompactBufferReader reader(dataRelocations_);
        ::TraceDataRelocations(trc, masm.buffer(), reader);
    }
}

void
AssemblerX86Shared::executableCopy(void *buffer)
{
    masm.executableCopy(buffer);
}

void
AssemblerX86Shared::processCodeLabels(uint8_t *rawCode)
{
    for (size_t i = 0; i < codeLabels_.length(); i++) {
        CodeLabel label = codeLabels_[i];
        Bind(rawCode, label.dest(), rawCode + label.src()->offset());
    }
}

AssemblerX86Shared::Condition
AssemblerX86Shared::InvertCondition(Condition cond)
{
    switch (cond) {
      case Zero:
        return NonZero;
      case NonZero:
        return Zero;
      case LessThan:
        return GreaterThanOrEqual;
      case LessThanOrEqual:
        return GreaterThan;
      case GreaterThan:
        return LessThanOrEqual;
      case GreaterThanOrEqual:
        return LessThan;
      case Above:
        return BelowOrEqual;
      case AboveOrEqual:
        return Below;
      case Below:
        return AboveOrEqual;
      case BelowOrEqual:
        return Above;
      default:
        MOZ_ASSUME_UNREACHABLE("unexpected condition");
    }
}
