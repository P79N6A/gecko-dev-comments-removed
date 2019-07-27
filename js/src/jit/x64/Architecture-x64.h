





#ifndef jit_x64_Architecture_x64_h
#define jit_x64_Architecture_x64_h

#include "jit/shared/BaseAssembler-x86-shared.h"

namespace js {
namespace jit {





static const uint32_t ION_FRAME_SLACK_SIZE     = 24;

#ifdef _WIN64
static const uint32_t ShadowStackSpace = 32;
#else
static const uint32_t ShadowStackSpace = 0;
#endif

class Registers {
  public:
    typedef X86Registers::RegisterID Code;
    typedef uint32_t SetType;
    static uint32_t SetSize(SetType x) {
        static_assert(sizeof(SetType) == 4, "SetType must be 32 bits");
        return mozilla::CountPopulation32(x);
    }
    static uint32_t FirstBit(SetType x) {
        return mozilla::CountTrailingZeroes32(x);
    }
    static uint32_t LastBit(SetType x) {
        return 31 - mozilla::CountLeadingZeroes32(x);
    }
    static const char *GetName(Code code) {
        static const char * const Names[] = { "rax", "rcx", "rdx", "rbx",
                                              "rsp", "rbp", "rsi", "rdi",
                                              "r8",  "r9",  "r10", "r11",
                                              "r12", "r13", "r14", "r15" };
        return Names[code];
    }

    static Code FromName(const char *name) {
        for (size_t i = 0; i < Total; i++) {
            if (strcmp(GetName(Code(i)), name) == 0)
                return Code(i);
        }
        return Invalid;
    }

    static const Code StackPointer = X86Registers::esp;
    static const Code Invalid = X86Registers::invalid_reg;

    static const uint32_t Total = 16;
    static const uint32_t TotalPhys = 16;
    static const uint32_t Allocatable = 14;

    static const uint32_t AllMask = (1 << Total) - 1;

    static const uint32_t ArgRegMask =
# if !defined(_WIN64)
        (1 << X86Registers::edi) |
        (1 << X86Registers::esi) |
# endif
        (1 << X86Registers::edx) |
        (1 << X86Registers::ecx) |
        (1 << X86Registers::r8) |
        (1 << X86Registers::r9);

    static const uint32_t VolatileMask =
        (1 << X86Registers::eax) |
        (1 << X86Registers::ecx) |
        (1 << X86Registers::edx) |
# if !defined(_WIN64)
        (1 << X86Registers::esi) |
        (1 << X86Registers::edi) |
# endif
        (1 << X86Registers::r8) |
        (1 << X86Registers::r9) |
        (1 << X86Registers::r10) |
        (1 << X86Registers::r11);

    static const uint32_t NonVolatileMask =
        (1 << X86Registers::ebx) |
#if defined(_WIN64)
        (1 << X86Registers::esi) |
        (1 << X86Registers::edi) |
#endif
        (1 << X86Registers::ebp) |
        (1 << X86Registers::r12) |
        (1 << X86Registers::r13) |
        (1 << X86Registers::r14) |
        (1 << X86Registers::r15);

    static const uint32_t WrapperMask = VolatileMask;

    static const uint32_t SingleByteRegs = VolatileMask | NonVolatileMask;

    static const uint32_t NonAllocatableMask =
        (1 << X86Registers::esp) |
        (1 << X86Registers::r11);      

    
    static const uint32_t TempMask = VolatileMask & ~NonAllocatableMask;

    static const uint32_t AllocatableMask = AllMask & ~NonAllocatableMask;

    
    static const uint32_t JSCallMask =
        (1 << X86Registers::ecx);

    
    static const uint32_t CallMask =
        (1 << X86Registers::eax);
};


typedef uint16_t PackedRegisterMask;

class FloatRegisters {
  public:
    typedef X86Registers::XMMRegisterID Code;
    typedef uint32_t SetType;
    static const char *GetName(Code code) {
        static const char * const Names[] = { "xmm0",  "xmm1",  "xmm2",  "xmm3",
                                              "xmm4",  "xmm5",  "xmm6",  "xmm7",
                                              "xmm8",  "xmm9",  "xmm10", "xmm11",
                                              "xmm12", "xmm13", "xmm14", "xmm15" };
        return Names[code];
    }

    static Code FromName(const char *name) {
        for (size_t i = 0; i < Total; i++) {
            if (strcmp(GetName(Code(i)), name) == 0)
                return Code(i);
        }
        return Invalid;
    }

    static const Code Invalid = X86Registers::invalid_xmm;

    static const uint32_t Total = 16;
    static const uint32_t TotalPhys = 16;

    static const uint32_t Allocatable = 15;

    static const uint32_t AllMask = (1 << Total) - 1;
    static const uint32_t AllDoubleMask = AllMask;
    static const uint32_t VolatileMask =
#if defined(_WIN64)
        (1 << X86Registers::xmm0) |
        (1 << X86Registers::xmm1) |
        (1 << X86Registers::xmm2) |
        (1 << X86Registers::xmm3) |
        (1 << X86Registers::xmm4) |
        (1 << X86Registers::xmm5);
#else
        AllMask;
#endif

    static const uint32_t NonVolatileMask = AllMask & ~VolatileMask;

    static const uint32_t WrapperMask = VolatileMask;

    static const uint32_t NonAllocatableMask =
        (1 << X86Registers::xmm15);    

    static const uint32_t AllocatableMask = AllMask & ~NonAllocatableMask;

};

template <typename T>
class TypedRegisterSet;

struct FloatRegister {
    typedef FloatRegisters Codes;
    typedef Codes::Code Code;
    typedef Codes::SetType SetType;
    static uint32_t SetSize(SetType x) {
        static_assert(sizeof(SetType) == 4, "SetType must be 32 bits");
        return mozilla::CountPopulation32(x);
    }
    static uint32_t FirstBit(SetType x) {
        return mozilla::CountTrailingZeroes32(x);
    }
    static uint32_t LastBit(SetType x) {
        return 31 - mozilla::CountLeadingZeroes32(x);
    }
    Code code_;

    static FloatRegister FromCode(uint32_t i) {
        JS_ASSERT(i < FloatRegisters::Total);
        FloatRegister r = { (FloatRegisters::Code)i };
        return r;
    }
    Code code() const {
        JS_ASSERT((uint32_t)code_ < FloatRegisters::Total);
        return code_;
    }
    const char *name() const {
        return FloatRegisters::GetName(code());
    }
    bool volatile_() const {
        return !!((1 << code()) & FloatRegisters::VolatileMask);
    }
    bool operator !=(FloatRegister other) const {
        return other.code_ != code_;
    }
    bool operator ==(FloatRegister other) const {
        return other.code_ == code_;
    }
    bool aliases(FloatRegister other) const {
        return other.code_ == code_;
    }
    uint32_t numAliased() const {
        return 1;
    }

    
    
    void aliased(uint32_t aliasIdx, FloatRegister *ret) {
        JS_ASSERT(aliasIdx == 0);
        *ret = *this;
    }
    
    
    
    
    
    bool equiv(FloatRegister other) const {
        return true;
    }
    uint32_t size() const {
        return sizeof(double);
    }
    uint32_t numAlignedAliased() {
        return 1;
    }
    void alignedAliased(uint32_t aliasIdx, FloatRegister *ret) {
        JS_ASSERT(aliasIdx == 0);
        *ret = *this;
    }
    static TypedRegisterSet<FloatRegister> ReduceSetForPush(const TypedRegisterSet<FloatRegister> &s);
    static uint32_t GetSizeInBytes(const TypedRegisterSet<FloatRegister> &s);
    static uint32_t GetPushSizeInBytes(const TypedRegisterSet<FloatRegister> &s);
    uint32_t getRegisterDumpOffsetInBytes();

};



inline bool
hasUnaliasedDouble()
{
    return false;
}



inline bool
hasMultiAlias()
{
    return false;
}

} 
} 

#endif 
