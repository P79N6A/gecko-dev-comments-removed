





#ifndef jit_arm_Architecture_arm_h
#define jit_arm_Architecture_arm_h

#include "mozilla/MathAlgorithms.h"

#include <limits.h>
#include <stdint.h>

#include "js/Utility.h"


#if defined(__ARM_PCS_VFP)
#define JS_CODEGEN_ARM_HARDFP
#endif

namespace js {
namespace jit {





static const uint32_t ION_FRAME_SLACK_SIZE   = 20;



static const int32_t NUNBOX32_TYPE_OFFSET    = 4;
static const int32_t NUNBOX32_PAYLOAD_OFFSET = 0;

static const uint32_t ShadowStackSpace = 0;









static const uint32_t BAILOUT_TABLE_ENTRY_SIZE    = 4;

class Registers
{
  public:
    enum RegisterID {
        r0 = 0,
        r1,
        r2,
        r3,
        S0 = r3,
        r4,
        r5,
        r6,
        r7,
        r8,
        S1 = r8,
        r9,
        r10,
        r11,
        r12,
        ip = r12,
        r13,
        sp = r13,
        r14,
        lr = r14,
        r15,
        pc = r15,
        invalid_reg
    };
    typedef RegisterID Code;

    static const char *GetName(Code code) {
        static const char * const Names[] = { "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
                                              "r8", "r9", "r10", "r11", "r12", "sp", "r14", "pc"};
        return Names[code];
    }
    static const char *GetName(uint32_t i) {
        MOZ_ASSERT(i < Total);
        return GetName(Code(i));
    }

    static Code FromName(const char *name);

    static const Code StackPointer = sp;
    static const Code Invalid = invalid_reg;

    static const uint32_t Total = 16;
    static const uint32_t Allocatable = 13;

    static const uint32_t AllMask = (1 << Total) - 1;
    static const uint32_t ArgRegMask = (1 << r0) | (1 << r1) | (1 << r2) | (1 << r3);

    static const uint32_t VolatileMask =
        (1 << r0) |
        (1 << r1) |
        (1 << Registers::r2) |
        (1 << Registers::r3);

    static const uint32_t NonVolatileMask =
        (1 << Registers::r4) |
        (1 << Registers::r5) |
        (1 << Registers::r6) |
        (1 << Registers::r7) |
        (1 << Registers::r8) |
        (1 << Registers::r9) |
        (1 << Registers::r10) |
        (1 << Registers::r11) |
        (1 << Registers::r12) |
        (1 << Registers::r14);

    static const uint32_t WrapperMask =
        VolatileMask |         
        (1 << Registers::r4) | 
        (1 << Registers::r5);  

    static const uint32_t SingleByteRegs =
        VolatileMask | NonVolatileMask;

    static const uint32_t NonAllocatableMask =
        (1 << Registers::sp) |
        (1 << Registers::r12) | 
        (1 << Registers::lr) |
        (1 << Registers::pc);

    
    static const uint32_t TempMask = VolatileMask & ~NonAllocatableMask;

    
    static const uint32_t JSCallMask =
        (1 << Registers::r2) |
        (1 << Registers::r3);

    
    static const uint32_t CallMask =
        (1 << Registers::r0) |
        (1 << Registers::r1);  

    static const uint32_t AllocatableMask = AllMask & ~NonAllocatableMask;
    typedef uint32_t SetType;
    static uint32_t SetSize(SetType x) {
        static_assert(sizeof(SetType) == 4, "SetType must be 32 bits");
        return mozilla::CountPopulation32(x);
    }
};


typedef uint16_t PackedRegisterMask;

class FloatRegisters
{
  public:
    enum FPRegisterID {
        d0,
        d1,
        d2,
        d3,
        d4,
        d5,
        d6,
        d7,
        d8,
        d9,
        d10,
        d11,
        d12,
        d13,
        d14,
        d15,
        d16,
        d17,
        d18,
        d19,
        d20,
        d21,
        d22,
        d23,
        d24,
        d25,
        d26,
        d27,
        d28,
        d29,
        d30,
        invalid_freg
    };
    typedef FPRegisterID Code;

    static const char *GetName(Code code) {
        static const char * const Names[] = { "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7",
                                              "d8", "d9", "d10", "d11", "d12", "d13", "d14", "d15"};
        return Names[code];
    }
    static const char *GetName(uint32_t i) {
        JS_ASSERT(i < Total);
        return GetName(Code(i));
    }

    static Code FromName(const char *name);

    static const Code Invalid = invalid_freg;

    static const uint32_t Total = 16;
    static const uint32_t Allocatable = 15;

    static const uint32_t AllMask = (1 << Total) - 1;

    
    static const uint32_t NonVolatileMask =
        (1 << d8) |
        (1 << d9) |
        (1 << d10) |
        (1 << d11) |
        (1 << d12) |
        (1 << d13) |
        (1 << d14);

    static const uint32_t VolatileMask = AllMask & ~NonVolatileMask;

    static const uint32_t WrapperMask = VolatileMask;

    
    static const uint32_t NonAllocatableMask = (1 << d15) | (1 << invalid_freg);

    
    static const uint32_t TempMask = VolatileMask & ~NonAllocatableMask;

    static const uint32_t AllocatableMask = AllMask & ~NonAllocatableMask;
    typedef uint32_t SetType;
};

template <typename T>
class TypedRegisterSet;

class VFPRegister
{
  public:
    
    
    
    enum RegType {
        Single = 0x0,
        Double = 0x1,
        UInt   = 0x2,
        Int    = 0x3
    };

    typedef FloatRegisters Codes;
    typedef Codes::Code Code;

  protected:
    RegType kind : 2;
    
    
    
    
    
    
    
  public:
    Code code_ : 5;
  protected:
    bool _isInvalid : 1;
    bool _isMissing : 1;

  public:
    MOZ_CONSTEXPR VFPRegister(uint32_t r, RegType k)
      : kind(k), code_ (Code(r)), _isInvalid(false), _isMissing(false)
    { }
    MOZ_CONSTEXPR VFPRegister()
      : kind(Double), code_(Code(0)), _isInvalid(true), _isMissing(false)
    { }

    MOZ_CONSTEXPR VFPRegister(RegType k, uint32_t id, bool invalid, bool missing) :
        kind(k), code_(Code(id)), _isInvalid(invalid), _isMissing(missing) {
    }

    explicit MOZ_CONSTEXPR VFPRegister(Code id)
      : kind(Double), code_(id), _isInvalid(false), _isMissing(false)
    { }
    bool operator==(const VFPRegister &other) const {
        JS_ASSERT(!isInvalid());
        JS_ASSERT(!other.isInvalid());
        return kind == other.kind && code_ == other.code_;
    }
    bool isDouble() const { return kind == Double; }
    bool isSingle() const { return kind == Single; }
    bool isFloat() const { return (kind == Double) || (kind == Single); }
    bool isInt() const { return (kind == UInt) || (kind == Int); }
    bool isSInt() const { return kind == Int; }
    bool isUInt() const { return kind == UInt; }
    bool equiv(VFPRegister other) const { return other.kind == kind; }
    size_t size() const { return (kind == Double) ? 8 : 4; }
    bool isInvalid() const;
    bool isMissing() const;

    VFPRegister doubleOverlay(unsigned int which = 0) const;
    VFPRegister singleOverlay(unsigned int which = 0) const;
    VFPRegister sintOverlay(unsigned int which = 0) const;
    VFPRegister uintOverlay(unsigned int which = 0) const;

    struct VFPRegIndexSplit;
    VFPRegIndexSplit encode();

    
    struct VFPRegIndexSplit {
        const uint32_t block : 4;
        const uint32_t bit : 1;

      private:
        friend VFPRegIndexSplit js::jit::VFPRegister::encode();

        VFPRegIndexSplit(uint32_t block_, uint32_t bit_)
          : block(block_), bit(bit_)
        {
            JS_ASSERT(block == block_);
            JS_ASSERT(bit == bit_);
        }
    };

    Code code() const {
        JS_ASSERT(!_isInvalid && !_isMissing);
        
        
        JS_ASSERT(isFloat());
        return Code(code_);
    }
    uint32_t id() const {
        return code_;
    }
    static VFPRegister FromCode(uint32_t i) {
        uint32_t code = i & 31;
        return VFPRegister(code, Double);
    }
    bool volatile_() const {
        if (isDouble())
            return !!((1 << (code_ >> 1)) & FloatRegisters::VolatileMask);
        return !!((1 << code_) & FloatRegisters::VolatileMask);
    }
    const char *name() const {
        return FloatRegisters::GetName(code_);
    }
    bool operator != (const VFPRegister &other) const {
        return other.kind != kind || code_ != other.code_;
    }
    bool aliases(const VFPRegister &other) {
        if (kind == other.kind)
            return code_ == other.code_;
        return doubleOverlay() == other.doubleOverlay();
    }
    static const int NumAliasedDoubles = 16;
    uint32_t numAliased() const {
        return 1;
#ifdef EVERYONE_KNOWS_ABOUT_ALIASING
        if (isDouble()) {
            if (code_ < NumAliasedDoubles)
                return 3;
            return 1;
        }
        return 2;
#endif
    }
    void aliased(uint32_t aliasIdx, VFPRegister *ret) {
        if (aliasIdx == 0) {
            *ret = *this;
            return;
        }
        if (isDouble()) {
            JS_ASSERT(code_ < NumAliasedDoubles);
            JS_ASSERT(aliasIdx <= 2);
            *ret = singleOverlay(aliasIdx - 1);
            return;
        }
        JS_ASSERT(aliasIdx == 1);
        *ret = doubleOverlay(aliasIdx - 1);
    }
    uint32_t numAlignedAliased() const {
        if (isDouble()) {
            if (code_ < NumAliasedDoubles)
                return 2;
            return 1;
        }
        
        
        return 2 - (code_ & 1);
    }
    
    
    
    
    
    void alignedAliased(uint32_t aliasIdx, VFPRegister *ret) {
        if (aliasIdx == 0) {
            *ret = *this;
            return;
        }
        JS_ASSERT(aliasIdx == 1);
        if (isDouble()) {
            JS_ASSERT(code_ < NumAliasedDoubles);
            *ret = singleOverlay(aliasIdx - 1);
            return;
        }
        JS_ASSERT((code_ & 1) == 0);
        *ret = doubleOverlay(aliasIdx - 1);
        return;
    }
    typedef FloatRegisters::SetType SetType;
    static uint32_t SetSize(SetType x) {
        static_assert(sizeof(SetType) == 4, "SetType must be 32 bits");
        return mozilla::CountPopulation32(x);
    }
    static Code FromName(const char *name) {
        return FloatRegisters::FromName(name);
    }
    static TypedRegisterSet<VFPRegister> ReduceSetForPush(const TypedRegisterSet<VFPRegister> &s);
    static uint32_t GetSizeInBytes(const TypedRegisterSet<VFPRegister> &s);
    static uint32_t GetPushSizeInBytes(const TypedRegisterSet<VFPRegister> &s);
    uint32_t getRegisterDumpOffsetInBytes();

};



typedef VFPRegister FloatRegister;

uint32_t GetARMFlags();
bool HasMOVWT();
bool HasVFPv3();
bool HasVFP();
bool Has32DP();
bool HasIDIV();



inline bool
hasUnaliasedDouble()
{
    return Has32DP();
}



inline bool
hasMultiAlias()
{
    return true;
}

bool ParseARMHwCapFlags(const char *armHwCap);



#if defined(JS_ARM_SIMULATOR)
bool UseHardFpABI();
#else
static inline bool UseHardFpABI()
{
#if defined(JS_CODEGEN_ARM_HARDFP)
    return true;
#else
    return false;
#endif
}
#endif

} 
} 

#endif
