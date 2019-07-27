





#include "jit/x86-shared/Architecture-x86-shared.h"
#if !defined(JS_CODEGEN_X86) && !defined(JS_CODEGEN_X64)
# error "Wrong architecture. Only x86 and x64 should build this file!"
#endif

#include "jit/RegisterSets.h"

const char*
js::jit::FloatRegister::name() const {
    static const char* const names[] = {

#ifdef JS_CODEGEN_X64
#define FLOAT_REGS_(TYPE) \
        "%xmm0" TYPE, "%xmm1" TYPE, "%xmm2" TYPE, "%xmm3" TYPE, \
        "%xmm4" TYPE, "%xmm5" TYPE, "%xmm6" TYPE, "%xmm7" TYPE, \
        "%xmm8" TYPE, "%xmm9" TYPE, "%xmm10" TYPE, "%xmm11" TYPE, \
        "%xmm12" TYPE, "%xmm13" TYPE, "%xmm14" TYPE, "%xmm15" TYPE
#else
#define FLOAT_REGS_(TYPE) \
        "%xmm0" TYPE, "%xmm1" TYPE, "%xmm2" TYPE, "%xmm3" TYPE, \
        "%xmm4" TYPE, "%xmm5" TYPE, "%xmm6" TYPE, "%xmm7" TYPE
#endif

        
        
        FLOAT_REGS_(".s"),
        FLOAT_REGS_(".d"),
        FLOAT_REGS_(".i4"),
        FLOAT_REGS_(".s4")
#undef FLOAT_REGS_

    };
    MOZ_ASSERT(size_t(code()) < mozilla::ArrayLength(names));
    return names[size_t(code())];
}

js::jit::FloatRegisterSet
js::jit::FloatRegister::ReduceSetForPush(const FloatRegisterSet& s)
{
    SetType bits = s.bits();

    
    if (!JitSupportsSimd())
        bits &= Codes::AllPhysMask * Codes::SpreadScalar;

    
    
    
    bits &= ~(bits >> (1 * Codes::TotalPhys));
    bits &= ~(bits >> (2 * Codes::TotalPhys));
    bits &= ~(bits >> (3 * Codes::TotalPhys));

    return FloatRegisterSet(bits);
}

uint32_t
js::jit::FloatRegister::GetPushSizeInBytes(const FloatRegisterSet& s)
{
    SetType all = s.bits();
    SetType float32x4Set =
        (all >> (uint32_t(Codes::Float32x4) * Codes::TotalPhys)) & Codes::AllPhysMask;
    SetType int32x4Set =
        (all >> (uint32_t(Codes::Int32x4) * Codes::TotalPhys)) & Codes::AllPhysMask;
    SetType doubleSet =
        (all >> (uint32_t(Codes::Double) * Codes::TotalPhys)) & Codes::AllPhysMask;
    SetType singleSet =
        (all >> (uint32_t(Codes::Single) * Codes::TotalPhys)) & Codes::AllPhysMask;

    
    
    
    SetType set128b = int32x4Set | float32x4Set;
    SetType set64b = doubleSet & ~set128b;
    SetType set32b = singleSet & ~set64b  & ~set128b;

    static_assert(Codes::AllPhysMask <= 0xffff, "We can safely use CountPopulation32");
    uint32_t count32b = mozilla::CountPopulation32(set32b);

#if defined(JS_CODEGEN_X64)
    
    
    
    count32b += count32b & 1;
#endif

    return mozilla::CountPopulation32(set128b) * (4 * sizeof(int32_t))
        + mozilla::CountPopulation32(set64b) * sizeof(double)
        + count32b * sizeof(float);
}
uint32_t
js::jit::FloatRegister::getRegisterDumpOffsetInBytes()
{
    return uint32_t(encoding()) * sizeof(FloatRegisters::RegisterContent);
}
