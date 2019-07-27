





#include "gc/Marking.h"
#include "jit/Disassembler.h"
#include "jit/JitCompartment.h"
#if defined(JS_CODEGEN_X86)
# include "jit/x86/MacroAssembler-x86.h"
#elif defined(JS_CODEGEN_X64)
# include "jit/x64/MacroAssembler-x64.h"
#else
# error "Wrong architecture. Only x86 and x64 should build this file!"
#endif

#ifdef _MSC_VER
# include <intrin.h> 
# if defined(_M_X64) && (_MSC_FULL_VER >= 160040219)
#  include <immintrin.h> 
# endif
#endif

using namespace js;
using namespace js::jit;

void
AssemblerX86Shared::copyJumpRelocationTable(uint8_t* dest)
{
    if (jumpRelocations_.length())
        memcpy(dest, jumpRelocations_.buffer(), jumpRelocations_.length());
}

void
AssemblerX86Shared::copyDataRelocationTable(uint8_t* dest)
{
    if (dataRelocations_.length())
        memcpy(dest, dataRelocations_.buffer(), dataRelocations_.length());
}

void
AssemblerX86Shared::copyPreBarrierTable(uint8_t* dest)
{
    if (preBarriers_.length())
        memcpy(dest, preBarriers_.buffer(), preBarriers_.length());
}

static void
TraceDataRelocations(JSTracer* trc, uint8_t* buffer, CompactBufferReader& reader)
{
    while (reader.more()) {
        size_t offset = reader.readUnsigned();
        void** ptr = X86Encoding::GetPointerRef(buffer + offset);

#ifdef JS_PUNBOX64
        
        
        uintptr_t* word = reinterpret_cast<uintptr_t*>(ptr);
        if (*word >> JSVAL_TAG_SHIFT) {
            jsval_layout layout;
            layout.asBits = *word;
            Value v = IMPL_TO_JSVAL(layout);
            TraceManuallyBarrieredEdge(trc, &v, "ion-masm-value");
            *word = JSVAL_TO_IMPL(v).asBits;
            continue;
        }
#endif

        
        
        
        
        MOZ_ASSERT(!(*reinterpret_cast<uintptr_t*>(ptr) & 0x1));

        
        gc::MarkGCThingUnbarriered(trc, ptr, "ion-masm-ptr");
    }
}


void
AssemblerX86Shared::TraceDataRelocations(JSTracer* trc, JitCode* code, CompactBufferReader& reader)
{
    ::TraceDataRelocations(trc, code->raw(), reader);
}

void
AssemblerX86Shared::FixupNurseryObjects(JSContext* cx, JitCode* code, CompactBufferReader& reader,
                                        const ObjectVector& nurseryObjects)
{
    MOZ_ASSERT(!nurseryObjects.empty());

    uint8_t* buffer = code->raw();
    bool hasNurseryPointers = false;

    while (reader.more()) {
        size_t offset = reader.readUnsigned();
        void** ptr = X86Encoding::GetPointerRef(buffer + offset);

        uintptr_t* word = reinterpret_cast<uintptr_t*>(ptr);

#ifdef JS_PUNBOX64
        if (*word >> JSVAL_TAG_SHIFT)
            continue; 
#endif

        if (!(*word & 0x1))
            continue;

        uint32_t index = *word >> 1;
        JSObject* obj = nurseryObjects[index];
        *word = uintptr_t(obj);

        
        
        MOZ_ASSERT_IF(hasNurseryPointers, IsInsideNursery(obj));

        if (!hasNurseryPointers && IsInsideNursery(obj))
            hasNurseryPointers = true;
    }

    if (hasNurseryPointers)
        cx->runtime()->gc.storeBuffer.putWholeCellFromMainThread(code);
}

void
AssemblerX86Shared::trace(JSTracer* trc)
{
    for (size_t i = 0; i < jumps_.length(); i++) {
        RelativePatch& rp = jumps_[i];
        if (rp.kind == Relocation::JITCODE) {
            JitCode* code = JitCode::FromExecutable((uint8_t*)rp.target);
            MarkJitCodeUnbarriered(trc, &code, "masmrel32");
            MOZ_ASSERT(code == JitCode::FromExecutable((uint8_t*)rp.target));
        }
    }
    if (dataRelocations_.length()) {
        CompactBufferReader reader(dataRelocations_);
        ::TraceDataRelocations(trc, masm.data(), reader);
    }
}

void
AssemblerX86Shared::executableCopy(void* buffer)
{
    masm.executableCopy(buffer);
}

void
AssemblerX86Shared::processCodeLabels(uint8_t* rawCode)
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
        MOZ_CRASH("unexpected condition");
    }
}

void
AssemblerX86Shared::verifyHeapAccessDisassembly(uint32_t begin, uint32_t end,
                                                const Disassembler::HeapAccess& heapAccess)
{
#ifdef DEBUG
    Disassembler::VerifyHeapAccess(masm.data() + begin, masm.data() + end, heapAccess);
#endif
}

CPUInfo::SSEVersion CPUInfo::maxSSEVersion = UnknownSSE;
CPUInfo::SSEVersion CPUInfo::maxEnabledSSEVersion = UnknownSSE;
bool CPUInfo::avxPresent = false;
bool CPUInfo::avxEnabled = false;

static uintptr_t
ReadXGETBV()
{
    
    
    
    
    size_t xcr0EAX = 0;
#if defined(_XCR_XFEATURE_ENABLED_MASK)
    xcr0EAX = _xgetbv(_XCR_XFEATURE_ENABLED_MASK);
#elif defined(__GNUC__)
    
    
    asm(".byte 0x0f, 0x01, 0xd0" : "=a"(xcr0EAX) : "c"(0) : "%edx");
#elif defined(_MSC_VER) && defined(_M_IX86)
    __asm {
        xor ecx, ecx
        _asm _emit 0x0f _asm _emit 0x01 _asm _emit 0xd0
        mov xcr0EAX, eax
    }
#endif
    return xcr0EAX;
}

void
CPUInfo::SetSSEVersion()
{
    int flagsEDX = 0;
    int flagsECX = 0;

#ifdef _MSC_VER
    int cpuinfo[4];
    __cpuid(cpuinfo, 1);
    flagsECX = cpuinfo[2];
    flagsEDX = cpuinfo[3];
#elif defined(__GNUC__)
# ifdef JS_CODEGEN_X64
    asm (
         "movl $0x1, %%eax;"
         "cpuid;"
         : "=c" (flagsECX), "=d" (flagsEDX)
         :
         : "%eax", "%ebx"
         );
# else
    
    
    
    
    asm (
         "xor %%ecx, %%ecx;"
         "movl $0x1, %%eax;"
         "pushl %%ebx;"
         "cpuid;"
         "popl %%ebx;"
         : "=c" (flagsECX), "=d" (flagsEDX)
         :
         : "%eax"
         );
# endif
#else
# error "Unsupported compiler"
#endif

    static const int SSEBit = 1 << 25;
    static const int SSE2Bit = 1 << 26;
    static const int SSE3Bit = 1 << 0;
    static const int SSSE3Bit = 1 << 9;
    static const int SSE41Bit = 1 << 19;
    static const int SSE42Bit = 1 << 20;

    if (flagsECX & SSE42Bit)      maxSSEVersion = SSE4_2;
    else if (flagsECX & SSE41Bit) maxSSEVersion = SSE4_1;
    else if (flagsECX & SSSE3Bit) maxSSEVersion = SSSE3;
    else if (flagsECX & SSE3Bit)  maxSSEVersion = SSE3;
    else if (flagsEDX & SSE2Bit)  maxSSEVersion = SSE2;
    else if (flagsEDX & SSEBit)   maxSSEVersion = SSE;
    else                          maxSSEVersion = NoSSE;

    if (maxEnabledSSEVersion != UnknownSSE)
        maxSSEVersion = Min(maxSSEVersion, maxEnabledSSEVersion);

    static const int AVXBit = 1 << 28;
    static const int XSAVEBit = 1 << 27;
    avxPresent = (flagsECX & AVXBit) && (flagsECX & XSAVEBit) && avxEnabled;

    
    if (avxPresent) {
        size_t xcr0EAX = ReadXGETBV();
        static const int xcr0SSEBit = 1 << 1;
        static const int xcr0AVXBit = 1 << 2;
        avxPresent = (xcr0EAX & xcr0SSEBit) && (xcr0EAX & xcr0AVXBit);
    }
}
