





#ifndef jit_x86_Architecture_x86_h
#define jit_x86_Architecture_x86_h

#include "jit/shared/Constants-x86-shared.h"

namespace js {
namespace jit {





static const uint32_t ION_FRAME_SLACK_SIZE    = 20;


static const uint32_t ShadowStackSpace = 0;



static const int32_t NUNBOX32_TYPE_OFFSET         = 4;
static const int32_t NUNBOX32_PAYLOAD_OFFSET      = 0;






static const uint32_t BAILOUT_TABLE_ENTRY_SIZE    = 5;

class Registers {
  public:
    typedef X86Encoding::RegisterID Code;
    typedef X86Encoding::RegisterID Encoding;
    typedef uint8_t SetType;
    static uint32_t SetSize(SetType x) {
        static_assert(sizeof(SetType) == 1, "SetType must be 8 bits");
        return mozilla::CountPopulation32(x);
    }
    static uint32_t FirstBit(SetType x) {
        return mozilla::CountTrailingZeroes32(x);
    }
    static uint32_t LastBit(SetType x) {
        return 31 - mozilla::CountLeadingZeroes32(x);
    }
    static const char *GetName(Code code) {
        return X86Encoding::GPRegName(code);
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

    static const uint32_t Total = 8;
    static const uint32_t TotalPhys = 8;
    static const uint32_t Allocatable = 7;

    static const uint32_t AllMask = (1 << Total) - 1;

    static const uint32_t ArgRegMask = 0;

    static const uint32_t VolatileMask =
        (1 << X86Encoding::rax) |
        (1 << X86Encoding::rcx) |
        (1 << X86Encoding::rdx);

    static const uint32_t NonVolatileMask =
        (1 << X86Encoding::rbx) |
        (1 << X86Encoding::rsi) |
        (1 << X86Encoding::rdi) |
        (1 << X86Encoding::rbp);

    static const uint32_t WrapperMask =
        VolatileMask |
        (1 << X86Encoding::rbx);

    static const uint32_t SingleByteRegs =
        (1 << X86Encoding::rax) |
        (1 << X86Encoding::rcx) |
        (1 << X86Encoding::rdx) |
        (1 << X86Encoding::rbx);

    static const uint32_t NonAllocatableMask =
        (1 << X86Encoding::rsp);

    static const uint32_t AllocatableMask = AllMask & ~NonAllocatableMask;

    
    static const uint32_t TempMask = VolatileMask & ~NonAllocatableMask;

    
    static const uint32_t JSCallMask =
        (1 << X86Encoding::rcx) |
        (1 << X86Encoding::rdx);

    
    static const uint32_t CallMask =
        (1 << X86Encoding::rax);
};


typedef uint8_t PackedRegisterMask;

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

    static const uint32_t Total = 8;
    static const uint32_t TotalPhys = 8;
    static const uint32_t Allocatable = 7;

    static const uint32_t AllMask = (1 << Total) - 1;
    static const uint32_t AllDoubleMask = AllMask;
    static const uint32_t VolatileMask = AllMask;
    static const uint32_t NonVolatileMask = 0;

    static const uint32_t WrapperMask = VolatileMask;

    static const uint32_t NonAllocatableMask =
        (1 << X86Encoding::xmm7);     

    static const uint32_t AllocatableMask = AllMask & ~NonAllocatableMask;
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
