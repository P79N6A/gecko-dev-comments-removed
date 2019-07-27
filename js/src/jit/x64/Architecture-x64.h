





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

    
    union RegisterContent {
        uintptr_t r;
    };

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

    enum ContentType {
        Single,
        Double,
        Int32x4,
        Float32x4,
        NumTypes
    };

    
    union RegisterContent {
        float s;
        double d;
        int32_t i4[4];
        float s4[4];
    };

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

    static const uint32_t Total = 16 * NumTypes;
    static const uint32_t TotalPhys = 16;
    static const uint32_t Allocatable = 15;

    typedef uint64_t SetType;
    static_assert(sizeof(SetType) * 8 >= Total,
                  "SetType should be large enough to enumerate all registers.");

    
    
    
    static const SetType SpreadSingle = SetType(1) << (uint32_t(Single) * TotalPhys);
    static const SetType SpreadDouble = SetType(1) << (uint32_t(Double) * TotalPhys);
    static const SetType SpreadInt32x4 = SetType(1) << (uint32_t(Int32x4) * TotalPhys);
    static const SetType SpreadFloat32x4 = SetType(1) << (uint32_t(Float32x4) * TotalPhys);
    static const SetType SpreadScalar = SpreadSingle | SpreadDouble;
    static const SetType SpreadVector = SpreadInt32x4 | SpreadFloat32x4;
    static const SetType Spread = SpreadScalar | SpreadVector;

    static const SetType AllPhysMask = ((1 << TotalPhys) - 1);
    static const SetType AllMask = AllPhysMask * Spread;
    static const SetType AllDoubleMask = AllPhysMask * SpreadDouble;
    static const SetType VolatileMask =
#if defined(_WIN64)
        ( (1 << X86Encoding::xmm0) |
          (1 << X86Encoding::xmm1) |
          (1 << X86Encoding::xmm2) |
          (1 << X86Encoding::xmm3) |
          (1 << X86Encoding::xmm4) |
          (1 << X86Encoding::xmm5)
        ) * SpreadScalar
        | AllPhysMask * SpreadVector;
#else
        AllMask;
#endif

    static const SetType NonVolatileMask = AllMask & ~VolatileMask;

    static const SetType WrapperMask = VolatileMask;

    static const SetType NonAllocatableMask =
        Spread * (1 << X86Encoding::xmm15);    

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
        static_assert(sizeof(SetType) == 8, "SetType must be 64 bits");
        
        
        
        
        
        
        x |= x >> (2 * Codes::TotalPhys);
        x |= x >> Codes::TotalPhys;
        x &= Codes::AllPhysMask;
        static_assert(Codes::AllPhysMask <= 0xffff, "We can safely use CountPopulation32");
        return mozilla::CountPopulation32(x);
    }
    static uint32_t FirstBit(SetType x) {
        return mozilla::CountTrailingZeroes64(x);
    }
    static uint32_t LastBit(SetType x) {
        return 63 - mozilla::CountLeadingZeroes64(x);
    }

  private:
    
    
    Codes::Encoding reg_ : 5;
    Codes::ContentType type_ : 3;
    bool isInvalid_ : 1;

    
    static const size_t RegSize = 4;
    static const size_t RegMask = (1 << RegSize) - 1;

  public:
    MOZ_CONSTEXPR FloatRegister()
        : reg_(Codes::Encoding(0)), type_(Codes::Single), isInvalid_(true)
    { }
    MOZ_CONSTEXPR FloatRegister(uint32_t r, Codes::ContentType k)
        : reg_(Codes::Encoding(r)), type_(k), isInvalid_(false)
    { }
    MOZ_CONSTEXPR FloatRegister(Codes::Encoding r, Codes::ContentType k)
        : reg_(r), type_(k), isInvalid_(false)
    { }

    static FloatRegister FromCode(uint32_t i) {
        MOZ_ASSERT(i < Codes::Total);
        return FloatRegister(i & RegMask, Codes::ContentType(i >> RegSize));
    }

    bool isSingle() const { MOZ_ASSERT(!isInvalid()); return type_ == Codes::Single; }
    bool isDouble() const { MOZ_ASSERT(!isInvalid()); return type_ == Codes::Double; }
    bool isInt32x4() const { MOZ_ASSERT(!isInvalid()); return type_ == Codes::Int32x4; }
    bool isFloat32x4() const { MOZ_ASSERT(!isInvalid()); return type_ == Codes::Float32x4; }
    bool isInvalid() const { return isInvalid_; }

    FloatRegister asSingle() const { MOZ_ASSERT(!isInvalid()); return FloatRegister(reg_, Codes::Single); }
    FloatRegister asDouble() const { MOZ_ASSERT(!isInvalid()); return FloatRegister(reg_, Codes::Double); }
    FloatRegister asInt32x4() const { MOZ_ASSERT(!isInvalid()); return FloatRegister(reg_, Codes::Int32x4); }
    FloatRegister asFloat32x4() const { MOZ_ASSERT(!isInvalid()); return FloatRegister(reg_, Codes::Float32x4); }

    uint32_t size() const {
        MOZ_ASSERT(!isInvalid());
        if (isSingle())
            return sizeof(float);
        if (isDouble())
            return sizeof(double);
        MOZ_ASSERT(isInt32x4() || isFloat32x4());
        return 4 * sizeof(int32_t);
    }

    Code code() const {
        MOZ_ASSERT(!isInvalid());
        MOZ_ASSERT(uint32_t(reg_) < Codes::TotalPhys);
        
        
        return Code(reg_ | (type_ << RegSize));
    }
    Encoding encoding() const {
        MOZ_ASSERT(!isInvalid());
        MOZ_ASSERT(uint32_t(reg_) < Codes::TotalPhys);
        return reg_;
    }
    const char *name() const {
        
        return FloatRegisters::GetName(encoding());
    }
    bool volatile_() const {
        return !!((SetType(1) << code()) & FloatRegisters::VolatileMask);
    }
    bool operator !=(FloatRegister other) const {
        return other.reg_ != reg_ || other.type_ != type_;
    }
    bool operator ==(FloatRegister other) const {
        return other.reg_ == reg_ && other.type_ == type_;
    }
    bool aliases(FloatRegister other) const {
        return other.reg_ == reg_;
    }
    
    bool equiv(FloatRegister other) const {
        return other.type_ == type_;
    }

    uint32_t numAliased() const {
        return Codes::NumTypes;
    }
    uint32_t numAlignedAliased() const {
        return numAliased();
    }

    
    
    void aliased(uint32_t aliasIdx, FloatRegister *ret) const {
        MOZ_ASSERT(aliasIdx < Codes::NumTypes);
        *ret = FloatRegister(reg_, Codes::ContentType((aliasIdx + type_) % Codes::NumTypes));
    }
    void alignedAliased(uint32_t aliasIdx, FloatRegister *ret) const {
        aliased(aliasIdx, ret);
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
