





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
    typedef uint8_t Code;
    typedef RegisterID Encoding;

    
    union RegisterContent {
        uintptr_t r;
    };

    static const char* GetName(Code code) {
        static const char * const Names[] = { "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
                                              "r8", "r9", "r10", "r11", "r12", "sp", "r14", "pc"};
        return Names[code];
    }
    static const char* GetName(uint32_t i) {
        MOZ_ASSERT(i < Total);
        return GetName(Code(i));
    }

    static Code FromName(const char* name);

    static const Encoding StackPointer = sp;
    static const Encoding Invalid = invalid_reg;

    static const uint32_t Total = 16;
    static const uint32_t Allocatable = 13;

    typedef uint32_t SetType;

    static const SetType AllMask = (1 << Total) - 1;
    static const SetType ArgRegMask = (1 << r0) | (1 << r1) | (1 << r2) | (1 << r3);

    static const SetType VolatileMask =
        (1 << r0) |
        (1 << r1) |
        (1 << Registers::r2) |
        (1 << Registers::r3);

    static const SetType NonVolatileMask =
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

    static const SetType WrapperMask =
        VolatileMask |         
        (1 << Registers::r4) | 
        (1 << Registers::r5);  

    static const SetType SingleByteRegs =
        VolatileMask | NonVolatileMask;

    static const SetType NonAllocatableMask =
        (1 << Registers::sp) |
        (1 << Registers::r12) | 
        (1 << Registers::lr) |
        (1 << Registers::pc);

    
    static const SetType TempMask = VolatileMask & ~NonAllocatableMask;

    
    static const SetType JSCallMask =
        (1 << Registers::r2) |
        (1 << Registers::r3);

    
    static const SetType CallMask =
        (1 << Registers::r0) |
        (1 << Registers::r1);  

    static const SetType AllocatableMask = AllMask & ~NonAllocatableMask;

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
};


typedef uint16_t PackedRegisterMask;
typedef uint16_t PackedRegisterMask;

class FloatRegisters
{
  public:
    enum FPRegisterID {
        s0,
        s1,
        s2,
        s3,
        s4,
        s5,
        s6,
        s7,
        s8,
        s9,
        s10,
        s11,
        s12,
        s13,
        s14,
        s15,
        s16,
        s17,
        s18,
        s19,
        s20,
        s21,
        s22,
        s23,
        s24,
        s25,
        s26,
        s27,
        s28,
        s29,
        s30,
        s31,
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
        d31,
        invalid_freg
    };

    typedef FPRegisterID Code;
    typedef FPRegisterID Encoding;

    
    union RegisterContent {
        double d;
    };

    static const char* GetDoubleName(Code code) {
        static const char * const Names[] = { "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7",
                                              "d8", "d9", "d10", "d11", "d12", "d13", "d14", "d15",
                                              "d16", "d17", "d18", "d19", "d20", "d21", "d22", "d23",
                                              "d24", "d25", "d26", "d27", "d28", "d29", "d30", "d31"};
        return Names[code];
    }
    static const char* GetSingleName(Code code) {
        static const char * const Names[] = { "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
                                              "s8", "s9", "s10", "s11", "s12", "s13", "s14", "s15",
                                              "s16", "s17", "s18", "s19", "s20", "s21", "s22", "s23",
                                              "s24", "s25", "s26", "s27", "s28", "s29", "s30", "s31"};
        return Names[code];
    }

    static const char* GetName(uint32_t i) {
        MOZ_ASSERT(i < Total);
        return GetName(Code(i));
    }

    static Code FromName(const char* name);

    static const Code Invalid = invalid_freg;
    static const uint32_t Total = 48;
    static const uint32_t TotalDouble = 16;
    static const uint32_t TotalSingle = 32;
    static const uint32_t Allocatable = 45;
    
    static const uint32_t TotalPhys = 32;
    static uint32_t ActualTotalPhys();

    typedef uint64_t SetType;
    static const SetType AllDoubleMask = ((1ull << 16) - 1) << 32;
    static const SetType AllMask = ((1ull << 48) - 1);

    
    static const SetType NonVolatileDoubleMask =
         ((1ULL << d8) |
          (1ULL << d9) |
          (1ULL << d10) |
          (1ULL << d11) |
          (1ULL << d12) |
          (1ULL << d13) |
          (1ULL << d14));
    
    static const SetType NonVolatileMask =
        (NonVolatileDoubleMask |
         ((1 << s16) |
          (1 << s17) |
          (1 << s18) |
          (1 << s19) |
          (1 << s20) |
          (1 << s21) |
          (1 << s22) |
          (1 << s23) |
          (1 << s24) |
          (1 << s25) |
          (1 << s26) |
          (1 << s27) |
          (1 << s28) |
          (1 << s29) |
          (1 << s30)));

    static const SetType VolatileMask = AllMask & ~NonVolatileMask;
    static const SetType VolatileDoubleMask = AllDoubleMask & ~NonVolatileDoubleMask;

    static const SetType WrapperMask = VolatileMask;

    
    
    static const SetType NonAllocatableMask = ((1ULL << d15)) |
                                               (1ULL << s30) |
                                               (1ULL << s31);

    
    static const SetType TempMask = VolatileMask & ~NonAllocatableMask;

    static const SetType AllocatableMask = AllMask & ~NonAllocatableMask;
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
    typedef Codes::Encoding Encoding;

  protected:
    RegType kind : 2;
    
    
    
    
    
  public:
    uint32_t code_ : 5;
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
    bool operator==(const VFPRegister& other) const {
        MOZ_ASSERT(!isInvalid());
        MOZ_ASSERT(!other.isInvalid());
        return kind == other.kind && code_ == other.code_;
    }

    bool isSingle() const { return kind == Single; }
    bool isDouble() const { return kind == Double; }
    bool isInt32x4() const { return false; }
    bool isFloat32x4() const { return false; }
    bool isFloat() const { return (kind == Double) || (kind == Single); }
    bool isInt() const { return (kind == UInt) || (kind == Int); }
    bool isSInt() const { return kind == Int; }
    bool isUInt() const { return kind == UInt; }
    bool equiv(const VFPRegister& other) const { return other.kind == kind; }
    size_t size() const { return (kind == Double) ? 8 : 4; }
    bool isInvalid() const;
    bool isMissing() const;

    VFPRegister doubleOverlay(unsigned int which = 0) const;
    VFPRegister singleOverlay(unsigned int which = 0) const;
    VFPRegister sintOverlay(unsigned int which = 0) const;
    VFPRegister uintOverlay(unsigned int which = 0) const;

    VFPRegister asSingle() const { return singleOverlay(); }
    VFPRegister asDouble() const { return doubleOverlay(); }
    VFPRegister asInt32x4() const { MOZ_CRASH("NYI"); }
    VFPRegister asFloat32x4() const { MOZ_CRASH("NYI"); }

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
            MOZ_ASSERT(block == block_);
            MOZ_ASSERT(bit == bit_);
        }
    };

    Code code() const {
        MOZ_ASSERT(!_isInvalid && !_isMissing);
        
        
        MOZ_ASSERT(isFloat());
        return Code(code_ | (kind << 5));
    }
    Encoding encoding() const {
        MOZ_ASSERT(!_isInvalid && !_isMissing);
        return Code(code_ | (kind << 5));
    }
    uint32_t id() const {
        return code_;
    }
    static VFPRegister FromCode(uint32_t i) {
        uint32_t code = i & 31;
        uint32_t kind = i >> 5;
        return VFPRegister(code, RegType(kind));
    }
    bool volatile_() const {
        if (isDouble())
            return !!((1 << (code_ >> 1)) & FloatRegisters::VolatileMask);
        return !!((1 << code_) & FloatRegisters::VolatileMask);
    }
    const char* name() const {
        if (isDouble())
            return FloatRegisters::GetDoubleName(Code(code_));
        return FloatRegisters::GetSingleName(Code(code_));
    }
    bool operator != (const VFPRegister& other) const {
        return other.kind != kind || code_ != other.code_;
    }
    bool aliases(const VFPRegister& other) {
        if (kind == other.kind)
            return code_ == other.code_;
        return doubleOverlay() == other.doubleOverlay();
    }
    static const int NumAliasedDoubles = 16;
    uint32_t numAliased() const {
        if (isDouble()) {
            if (code_ < NumAliasedDoubles)
                return 3;
            return 1;
        }
        return 2;
    }

    
    
    void aliased(uint32_t aliasIdx, VFPRegister* ret) {
        if (aliasIdx == 0) {
            *ret = *this;
            return;
        }
        if (isDouble()) {
            MOZ_ASSERT(code_ < NumAliasedDoubles);
            MOZ_ASSERT(aliasIdx <= 2);
            *ret = singleOverlay(aliasIdx - 1);
            return;
        }
        MOZ_ASSERT(aliasIdx == 1);
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
    
    
    
    
    
    void alignedAliased(uint32_t aliasIdx, VFPRegister* ret) {
        if (aliasIdx == 0) {
            *ret = *this;
            return;
        }
        MOZ_ASSERT(aliasIdx == 1);
        if (isDouble()) {
            MOZ_ASSERT(code_ < NumAliasedDoubles);
            *ret = singleOverlay(aliasIdx - 1);
            return;
        }
        MOZ_ASSERT((code_ & 1) == 0);
        *ret = doubleOverlay(aliasIdx - 1);
        return;
    }

    typedef FloatRegisters::SetType SetType;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    SetType alignedOrDominatedAliasedSet() const {
        if (isSingle()) {
            if (code_ % 2 != 0)
                return SetType(1) << code_;
            return (SetType(1) << code_) | (SetType(1) << (32 + code_ / 2));
        }

        MOZ_ASSERT(isDouble());
        return (SetType(0b11) << (code_ * 2)) | (SetType(1) << (32 + code_));
    }

    static uint32_t SetSize(SetType x) {
        static_assert(sizeof(SetType) == 8, "SetType must be 64 bits");
        return mozilla::CountPopulation32(x);
    }
    static Code FromName(const char* name) {
        return FloatRegisters::FromName(name);
    }
    static TypedRegisterSet<VFPRegister> ReduceSetForPush(const TypedRegisterSet<VFPRegister>& s);
    static uint32_t GetPushSizeInBytes(const TypedRegisterSet<VFPRegister>& s);
    uint32_t getRegisterDumpOffsetInBytes();
    static uint32_t FirstBit(SetType x) {
        return mozilla::CountTrailingZeroes64(x);
    }
    static uint32_t LastBit(SetType x) {
        return 63 - mozilla::CountLeadingZeroes64(x);
    }

};


typedef VFPRegister FloatRegister;

uint32_t GetARMFlags();
bool HasMOVWT();
bool HasLDSTREXBHD();           
bool HasDMBDSBISB();            
bool HasVFPv3();
bool HasVFP();
bool Has32DP();
bool HasIDIV();

extern volatile uint32_t armHwCapFlags;





#define HWCAP_UNINITIALIZED (1 << 25)


#define HWCAP_ALIGNMENT_FAULT (1 << 26)


#define HWCAP_USE_HARDFP_ABI (1 << 27)


#define HWCAP_ARMv7 (1 << 28)



inline bool HasAlignmentFault()
{
    MOZ_ASSERT(armHwCapFlags != HWCAP_UNINITIALIZED);
    return armHwCapFlags & HWCAP_ALIGNMENT_FAULT;
}



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

bool ParseARMHwCapFlags(const char* armHwCap);
void InitARMFlags();
uint32_t GetARMFlags();




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




static const size_t AsmJSCheckedImmediateRange = 0;
static const size_t AsmJSImmediateRange = 0;

} 
} 

#endif
