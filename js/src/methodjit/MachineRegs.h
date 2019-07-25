






































#if !defined jsjaeger_regstate_h__ && defined JS_METHODJIT
#define jsjaeger_regstate_h__

#include "jsbit.h"
#include "assembler/assembler/MacroAssembler.h"

namespace js {

namespace mjit {



struct AnyRegisterID {
    unsigned reg_;

    AnyRegisterID()
        : reg_((unsigned)-1)
    { pin(); }

    AnyRegisterID(const AnyRegisterID &o)
        : reg_(o.reg_)
    { pin(); }

    AnyRegisterID(JSC::MacroAssembler::RegisterID reg)
        : reg_((unsigned)reg)
    { pin(); }

    AnyRegisterID(JSC::MacroAssembler::FPRegisterID reg)
        : reg_(JSC::MacroAssembler::TotalRegisters + (unsigned)reg)
    { pin(); }

    static inline AnyRegisterID fromRaw(unsigned reg);

    inline JSC::MacroAssembler::RegisterID reg();
    inline JSC::MacroAssembler::FPRegisterID fpreg();

    bool isSet() { return reg_ != unsigned(-1); }
    bool isReg() { return reg_ < JSC::MacroAssembler::TotalRegisters; }
    bool isFPReg() { return isSet() && !isReg(); }

    inline const char * name();

  private:
    unsigned * pin() {
        



        static unsigned *v;
        v = &reg_;
        return v;
    }
};

struct Registers {

    

    static const uint32 TotalRegisters = JSC::MacroAssembler::TotalRegisters;

    enum CallConvention {
        NormalCall,
        FastCall
    };

    typedef JSC::MacroAssembler::RegisterID RegisterID;

    
#if defined(JS_CPU_X64)
    static const RegisterID TypeMaskReg = JSC::X86Registers::r13;
    static const RegisterID PayloadMaskReg = JSC::X86Registers::r14;
    static const RegisterID ValueReg = JSC::X86Registers::r10;
    static const RegisterID ScratchReg = JSC::X86Registers::r11;
#endif

    
#if defined(JS_CPU_X86)
    static const RegisterID JSFrameReg = JSC::X86Registers::ebp;
#elif defined(JS_CPU_X64)
    static const RegisterID JSFrameReg = JSC::X86Registers::ebx;
#elif defined(JS_CPU_ARM)
    static const RegisterID JSFrameReg = JSC::ARMRegisters::r11;
#elif defined(JS_CPU_SPARC)
    static const RegisterID JSFrameReg = JSC::SparcRegisters::l0;
#endif

#if defined(JS_CPU_X86) || defined(JS_CPU_X64)
    static const RegisterID ReturnReg = JSC::X86Registers::eax;
# if defined(JS_CPU_X86) || defined(_WIN64)
    static const RegisterID ArgReg0 = JSC::X86Registers::ecx;
    static const RegisterID ArgReg1 = JSC::X86Registers::edx;
#  if defined(JS_CPU_X64)
    static const RegisterID ArgReg2 = JSC::X86Registers::r8;
#  endif
# else
    static const RegisterID ArgReg0 = JSC::X86Registers::edi;
    static const RegisterID ArgReg1 = JSC::X86Registers::esi;
    static const RegisterID ArgReg2 = JSC::X86Registers::edx;
# endif
#elif JS_CPU_ARM
    static const RegisterID ReturnReg = JSC::ARMRegisters::r0;
    static const RegisterID ArgReg0 = JSC::ARMRegisters::r0;
    static const RegisterID ArgReg1 = JSC::ARMRegisters::r1;
    static const RegisterID ArgReg2 = JSC::ARMRegisters::r2;
#elif JS_CPU_SPARC
    static const RegisterID ReturnReg = JSC::SparcRegisters::o0;
    static const RegisterID ArgReg0 = JSC::SparcRegisters::o0;
    static const RegisterID ArgReg1 = JSC::SparcRegisters::o1;
    static const RegisterID ArgReg2 = JSC::SparcRegisters::o2;
    static const RegisterID ArgReg3 = JSC::SparcRegisters::o3;
    static const RegisterID ArgReg4 = JSC::SparcRegisters::o4;
    static const RegisterID ArgReg5 = JSC::SparcRegisters::o5;
#endif

    static const RegisterID StackPointer = JSC::MacroAssembler::stackPointerRegister;

    static inline uint32 maskReg(RegisterID reg) {
        return (1 << reg);
    }

    static inline uint32 mask2Regs(RegisterID reg1, RegisterID reg2) {
        return maskReg(reg1) | maskReg(reg2);
    }

    static inline uint32 mask3Regs(RegisterID reg1, RegisterID reg2, RegisterID reg3) {
        return maskReg(reg1) | maskReg(reg2) | maskReg(reg3);
    }

#if defined(JS_CPU_X86) || defined(JS_CPU_X64)
    static const uint32 TempRegs =
          (1 << JSC::X86Registers::eax)
# if defined(JS_CPU_X86)
        | (1 << JSC::X86Registers::ebx)
# endif
        | (1 << JSC::X86Registers::ecx)
        | (1 << JSC::X86Registers::edx)
# if defined(JS_CPU_X64)
        | (1 << JSC::X86Registers::r8)
        | (1 << JSC::X86Registers::r9)
#  if !defined(_WIN64)
        | (1 << JSC::X86Registers::esi)
        | (1 << JSC::X86Registers::edi)
#  endif
# endif
        ;

# if defined(JS_CPU_X64)
    static const uint32 SavedRegs =
        
          (1 << JSC::X86Registers::r12)
    
    
        | (1 << JSC::X86Registers::r15)
#  if defined(_WIN64)
        | (1 << JSC::X86Registers::esi)
        | (1 << JSC::X86Registers::edi)
#  endif
# else
    static const uint32 SavedRegs =
          (1 << JSC::X86Registers::esi)
        | (1 << JSC::X86Registers::edi)
# endif
        ;

# if defined(JS_CPU_X86)
    static const uint32 SingleByteRegs = (TempRegs | SavedRegs) &
        ~((1 << JSC::X86Registers::esi) |
          (1 << JSC::X86Registers::edi) |
          (1 << JSC::X86Registers::ebp) |
          (1 << JSC::X86Registers::esp));
# elif defined(JS_CPU_X64)
    static const uint32 SingleByteRegs = TempRegs | SavedRegs;
# endif

#elif defined(JS_CPU_ARM)
    static const uint32 TempRegs =
          (1 << JSC::ARMRegisters::r0)
        | (1 << JSC::ARMRegisters::r1)
        | (1 << JSC::ARMRegisters::r2);
    
    

    static const uint32 SavedRegs =
          (1 << JSC::ARMRegisters::r4)
        | (1 << JSC::ARMRegisters::r5)
        | (1 << JSC::ARMRegisters::r6)
        | (1 << JSC::ARMRegisters::r7)
    
        | (1 << JSC::ARMRegisters::r9)
        | (1 << JSC::ARMRegisters::r10);
    
    
    
    

    static const uint32 SingleByteRegs = TempRegs | SavedRegs;
#elif defined(JS_CPU_SPARC)
    static const uint32 TempRegs =
          (1 << JSC::SparcRegisters::o0)
        | (1 << JSC::SparcRegisters::o1)
        | (1 << JSC::SparcRegisters::o2)
        | (1 << JSC::SparcRegisters::o3)
        | (1 << JSC::SparcRegisters::o4)
        | (1 << JSC::SparcRegisters::o5);

    static const uint32 SavedRegs =
          (1 << JSC::SparcRegisters::l2)
        | (1 << JSC::SparcRegisters::l3)
        | (1 << JSC::SparcRegisters::l4)
        | (1 << JSC::SparcRegisters::l5)
        | (1 << JSC::SparcRegisters::l6)
        | (1 << JSC::SparcRegisters::l7);

    static const uint32 SingleByteRegs = TempRegs | SavedRegs;
#else
# error "Unsupported platform"
#endif

    static const uint32 AvailRegs = SavedRegs | TempRegs;

    static bool isAvail(RegisterID reg) {
        uint32 mask = maskReg(reg);
        return bool(mask & AvailRegs);
    }

    static bool isSaved(RegisterID reg) {
        uint32 mask = maskReg(reg);
        JS_ASSERT(mask & AvailRegs);
        return bool(mask & SavedRegs);
    }

    static inline uint32 numArgRegs(CallConvention convention) {
#if defined(JS_CPU_X86)
# if defined(JS_NO_FASTCALL)
        return 0;
# else
        return (convention == FastCall) ? 2 : 0;
# endif
#elif defined(JS_CPU_X64)
# ifdef _WIN64
        return 4;
# else
        return 6;
# endif
#elif defined(JS_CPU_ARM)
        return 4;
#elif defined(JS_CPU_SPARC)
        return 6;
#endif
    }

    static inline bool regForArg(CallConvention conv, uint32 i, RegisterID *reg) {
#if defined(JS_CPU_X86)
        static const RegisterID regs[] = {
            JSC::X86Registers::ecx,
            JSC::X86Registers::edx
        };

# if defined(JS_NO_FASTCALL)
        return false;
# else
        if (conv == NormalCall)
            return false;
# endif
#elif defined(JS_CPU_X64)
# ifdef _WIN64
        static const RegisterID regs[] = {
            JSC::X86Registers::ecx,
            JSC::X86Registers::edx,
            JSC::X86Registers::r8,
            JSC::X86Registers::r9
        };
# else
        static const RegisterID regs[] = {
            JSC::X86Registers::edi,
            JSC::X86Registers::esi,
            JSC::X86Registers::edx,
            JSC::X86Registers::ecx,
            JSC::X86Registers::r8,
            JSC::X86Registers::r9
        };
# endif
#elif defined(JS_CPU_ARM)
        static const RegisterID regs[] = {
            JSC::ARMRegisters::r0,
            JSC::ARMRegisters::r1,
            JSC::ARMRegisters::r2,
            JSC::ARMRegisters::r3
        };
#elif defined(JS_CPU_SPARC)
        static const RegisterID regs[] = {
            JSC::SparcRegisters::o0,
            JSC::SparcRegisters::o1,
            JSC::SparcRegisters::o2,
            JSC::SparcRegisters::o3,
            JSC::SparcRegisters::o4,
            JSC::SparcRegisters::o5
        };
#endif
        JS_ASSERT(numArgRegs(conv) == JS_ARRAY_LENGTH(regs));
        if (i > JS_ARRAY_LENGTH(regs))
            return false;
        *reg = regs[i];
        return true;
    }

    

    typedef JSC::MacroAssembler::FPRegisterID FPRegisterID;

#if defined(JS_CPU_X86) || defined(JS_CPU_X64)
#ifdef _WIN64
    
    static const uint32 TotalFPRegisters = 5;
    static const FPRegisterID FPConversionTemp = JSC::X86Registers::xmm5;
#else
    static const uint32 TotalFPRegisters = 7;
    static const FPRegisterID FPConversionTemp = JSC::X86Registers::xmm7;
#endif
    static const uint32 TempFPRegs = (
          (1 << JSC::X86Registers::xmm0)
        | (1 << JSC::X86Registers::xmm1)
        | (1 << JSC::X86Registers::xmm2)
        | (1 << JSC::X86Registers::xmm3)
        | (1 << JSC::X86Registers::xmm4)
#ifndef _WIN64
        | (1 << JSC::X86Registers::xmm5)
        | (1 << JSC::X86Registers::xmm6)
#endif
        ) << TotalRegisters;
#elif defined(JS_CPU_ARM)
    static const uint32 TotalFPRegisters = 3;
    static const uint32 TempFPRegs = (
          (1 << JSC::ARMRegisters::d0)
        | (1 << JSC::ARMRegisters::d1)
        | (1 << JSC::ARMRegisters::d2)
        ) << TotalRegisters;
    static const FPRegisterID FPConversionTemp = JSC::ARMRegisters::d3;
#elif defined(JS_CPU_SPARC)
    static const uint32 TotalFPRegisters = 8;
    static const uint32 TempFPRegs = (uint32)(
          (1 << JSC::SparcRegisters::f0)
        | (1 << JSC::SparcRegisters::f2)
        | (1 << JSC::SparcRegisters::f4)
        | (1 << JSC::SparcRegisters::f6)
        ) << TotalRegisters;
    static const FPRegisterID FPConversionTemp = JSC::SparcRegisters::f8;
#else
# error "Unsupported platform"
#endif

    
#if defined(JS_CPU_X86) || defined(JS_CPU_X64)
    static const RegisterID ClobberInCall = JSC::X86Registers::ecx;
#elif defined(JS_CPU_ARM)
    static const RegisterID ClobberInCall = JSC::ARMRegisters::r2;
#elif defined(JS_CPU_SPARC)
    static const RegisterID ClobberInCall = JSC::SparcRegisters::l1;
#endif

    static const uint32 AvailFPRegs = TempFPRegs;

    static inline uint32 maskReg(FPRegisterID reg) {
        return (1 << reg) << TotalRegisters;
    }

    

    static const uint32 TotalAnyRegisters = TotalRegisters + TotalFPRegisters;
    static const uint32 TempAnyRegs = TempRegs | TempFPRegs;
    static const uint32 AvailAnyRegs = AvailRegs | AvailFPRegs;

    static inline uint32 maskReg(AnyRegisterID reg) {
        return (1 << reg.reg_);
    }

    
    static inline RegisterID tempCallReg() {
        Registers regs(AvailRegs);
        regs.takeReg(Registers::ArgReg0);
        regs.takeReg(Registers::ArgReg1);
        return regs.takeAnyReg().reg();
    }

    
    static inline Registers tempCallRegMask() {
        Registers regs(AvailRegs);
#ifndef JS_CPU_X86
        regs.takeReg(ArgReg0);
        regs.takeReg(ArgReg1);
        regs.takeReg(ArgReg2);
#ifdef JS_CPU_SPARC
        regs.takeReg(ArgReg3);
#endif
#endif
        return regs;
    }

    Registers(uint32 freeMask)
      : freeMask(freeMask)
    { }

    Registers(const Registers &other)
      : freeMask(other.freeMask)
    { }

    Registers & operator =(const Registers &other)
    {
        freeMask = other.freeMask;
        return *this;
    }

    bool empty(uint32 mask) const {
        return !(freeMask & mask);
    }

    bool empty() const {
        return !freeMask;
    }

    AnyRegisterID peekReg(uint32 mask) {
        JS_ASSERT(!empty(mask));
        unsigned ireg;
        JS_FLOOR_LOG2(ireg, freeMask & mask);
        return AnyRegisterID::fromRaw(ireg);
    }

    AnyRegisterID peekReg() {
        return peekReg(freeMask);
    }

    AnyRegisterID takeAnyReg(uint32 mask) {
        AnyRegisterID reg = peekReg(mask);
        takeReg(reg);
        return reg;
    }

    AnyRegisterID takeAnyReg() {
        return takeAnyReg(freeMask);
    }

    bool hasReg(AnyRegisterID reg) const {
        return !!(freeMask & (1 << reg.reg_));
    }

    bool hasRegInMask(uint32 mask) const {
        return !!(freeMask & mask);
    }

    bool hasAllRegs(uint32 mask) const {
        return (freeMask & mask) == mask;
    }

    void putRegUnchecked(AnyRegisterID reg) {
        freeMask |= (1 << reg.reg_);
    }

    void putReg(AnyRegisterID reg) {
        JS_ASSERT(!hasReg(reg));
        putRegUnchecked(reg);
    }

    void takeReg(AnyRegisterID reg) {
        JS_ASSERT(hasReg(reg));
        takeRegUnchecked(reg);
    }

    void takeRegUnchecked(AnyRegisterID reg) {
        freeMask &= ~(1 << reg.reg_);
    }

    bool operator ==(const Registers &other) {
        return freeMask == other.freeMask;
    }

    uint32 freeMask;
};

static const JSC::MacroAssembler::RegisterID JSFrameReg = Registers::JSFrameReg;

AnyRegisterID
AnyRegisterID::fromRaw(unsigned reg_)
{
    JS_ASSERT(reg_ < Registers::TotalAnyRegisters);
    AnyRegisterID reg;
    reg.reg_ = reg_;
    return reg;
}

JSC::MacroAssembler::RegisterID
AnyRegisterID::reg()
{
    JS_ASSERT(reg_ < Registers::TotalRegisters);
    return (JSC::MacroAssembler::RegisterID) reg_;
}

JSC::MacroAssembler::FPRegisterID
AnyRegisterID::fpreg()
{
    JS_ASSERT(reg_ >= Registers::TotalRegisters &&
              reg_ < Registers::TotalAnyRegisters);
    return (JSC::MacroAssembler::FPRegisterID) (reg_ - Registers::TotalRegisters);
}

const char *
AnyRegisterID::name()
{
#if defined(JS_CPU_X86) || defined(JS_CPU_X64)
    return isReg() ? JSC::X86Registers::nameIReg(reg()) : JSC::X86Registers::nameFPReg(fpreg());
#elif defined(JS_CPU_ARM)
    return isReg() ? JSC::ARMAssembler::nameGpReg(reg()) : JSC::ARMAssembler::nameFpRegD(fpreg());
#else
    return "???";
#endif
}

} 

} 

#endif 

