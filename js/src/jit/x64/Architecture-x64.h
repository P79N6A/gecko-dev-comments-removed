





#ifndef jit_x64_Architecture_x64_h
#define jit_x64_Architecture_x64_h

#include "jit/shared/Constants-x86-shared.h"

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
    typedef X86Encoding::RegisterID Code;
    typedef X86Encoding::RegisterID Encoding;
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

    static const Code StackPointer = X86Encoding::rsp;
    static const Code Invalid = X86Encoding::invalid_reg;

    static const uint32_t Total = 16;
    static const uint32_t TotalPhys = 16;
    static const uint32_t Allocatable = 14;

    static const SetType AllMask = (1 << Total) - 1;

    static const SetType ArgRegMask =
# if !defined(_WIN64)
        (1 << X86Encoding::rdi) |
        (1 << X86Encoding::rsi) |
# endif
        (1 << X86Encoding::rdx) |
        (1 << X86Encoding::rcx) |
        (1 << X86Encoding::r8) |
        (1 << X86Encoding::r9);

    static const SetType VolatileMask =
        (1 << X86Encoding::rax) |
        (1 << X86Encoding::rcx) |
        (1 << X86Encoding::rdx) |
# if !defined(_WIN64)
        (1 << X86Encoding::rsi) |
        (1 << X86Encoding::rdi) |
# endif
        (1 << X86Encoding::r8) |
        (1 << X86Encoding::r9) |
        (1 << X86Encoding::r10) |
        (1 << X86Encoding::r11);

    static const SetType NonVolatileMask =
        (1 << X86Encoding::rbx) |
#if defined(_WIN64)
        (1 << X86Encoding::rsi) |
        (1 << X86Encoding::rdi) |
#endif
        (1 << X86Encoding::rbp) |
        (1 << X86Encoding::r12) |
        (1 << X86Encoding::r13) |
        (1 << X86Encoding::r14) |
        (1 << X86Encoding::r15);

    static const SetType WrapperMask = VolatileMask;

    static const SetType SingleByteRegs = VolatileMask | NonVolatileMask;

    static const SetType NonAllocatableMask =
        (1 << X86Encoding::rsp) |
        (1 << X86Encoding::r11);      

    static const SetType AllocatableMask = AllMask & ~NonAllocatableMask;

    
    static const SetType TempMask = VolatileMask & ~NonAllocatableMask;

    
    static const SetType JSCallMask =
        (1 << X86Encoding::rcx);

    
    static const SetType CallMask =
        (1 << X86Encoding::rax);
};


typedef uint16_t PackedRegisterMask;

class FloatRegisters {
  public:
    typedef X86Encoding::XMMRegisterID Encoding;
    typedef uint32_t SetType;
    static const char *GetName(Encoding code) {
        return X86Encoding::XMMRegName(code);
    }

    static Encoding FromName(const char *name) {
        for (size_t i = 0; i < Total; i++) {
            if (strcmp(GetName(Encoding(i)), name) == 0)
                return Encoding(i);
        }
        return Invalid;
    }

    static const Encoding Invalid = X86Encoding::invalid_xmm;

    static const uint32_t Total = 16;
    static const uint32_t TotalPhys = 16;

    static const uint32_t Allocatable = 15;

    static const SetType AllMask = (1 << Total) - 1;
    static const SetType AllDoubleMask = AllMask;
    static const SetType VolatileMask =
#if defined(_WIN64)
        (1 << X86Encoding::xmm0) |
        (1 << X86Encoding::xmm1) |
        (1 << X86Encoding::xmm2) |
        (1 << X86Encoding::xmm3) |
        (1 << X86Encoding::xmm4) |
        (1 << X86Encoding::xmm5);
#else
        AllMask;
#endif

    static const SetType NonVolatileMask = AllMask & ~VolatileMask;

    static const SetType WrapperMask = VolatileMask;

    static const SetType NonAllocatableMask =
        (1 << X86Encoding::xmm15);    

    static const SetType AllocatableMask = AllMask & ~NonAllocatableMask;
};

template <typename T>
class TypedRegisterSet;

struct FloatRegister {
    typedef FloatRegisters Codes;
    typedef size_t Code;
    typedef Codes::Encoding Encoding;
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
        MOZ_ASSERT(i < FloatRegisters::Total);
        FloatRegister r = { Code(i) };
        return r;
    }

    bool isSingle() const { return true; }
    bool isDouble() const { return true; }
    bool isInt32x4() const { return true; }
    bool isFloat32x4() const { return true; }

    FloatRegister asSingle() const { return *this; }
    FloatRegister asDouble() const { return *this; }
    FloatRegister asInt32x4() const { return *this; }
    FloatRegister asFloat32x4() const { return *this; }

    Code code() const {
        MOZ_ASSERT(uint32_t(code_) < FloatRegisters::Total);
        return code_;
    }
    Encoding encoding() const {
        MOZ_ASSERT(uint32_t(code_) < FloatRegisters::Total);
        return Encoding(code_);
    }
    const char *name() const {
        return FloatRegisters::GetName(encoding());
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
        MOZ_ASSERT(aliasIdx == 0);
        *ret = *this;
    }
    
    
    
    
    
    bool equiv(FloatRegister other) const {
        return true;
    }
    uint32_t size() const {
        return sizeof(double);
    }
    uint32_t numAlignedAliased() const {
        return 1;
    }
    void alignedAliased(uint32_t aliasIdx, FloatRegister *ret) {
        MOZ_ASSERT(aliasIdx == 0);
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



static const size_t AsmJSCheckedImmediateRange = 4096;
static const size_t AsmJSImmediateRange = UINT32_C(0x80000000);

} 
} 

#endif 
