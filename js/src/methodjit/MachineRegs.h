






































#if !defined jsjaeger_regstate_h__ && defined JS_METHODJIT
#define jsjaeger_regstate_h__

#include "jsbit.h"
#include "assembler/assembler/MacroAssembler.h"

namespace js {

namespace mjit {

struct Registers {

    typedef JSC::MacroAssembler::RegisterID RegisterID;

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
        | (1 << JSC::X86Registers::ecx)
        | (1 << JSC::X86Registers::edx)
# if defined(JS_CPU_X64)
        | (1 << JSC::X86Registers::r8)
        | (1 << JSC::X86Registers::r9)
        | (1 << JSC::X86Registers::r10)
#  if !defined(_MSC_VER)
        | (1 << JSC::X86Registers::esi)
        | (1 << JSC::X86Registers::edi)
#  endif
# endif
        ;

# if defined(JS_CPU_X64)
    static const uint32 SavedRegs =
          (1 << JSC::X86Registers::r12)
        | (1 << JSC::X86Registers::r13)
        | (1 << JSC::X86Registers::r14)
        | (1 << JSC::X86Registers::r15)
#  if defined(_MSC_VER)
        | (1 << JSC::X86Registers::esi)
        | (1 << JSC::X86Registers::edi)
#  endif
# else
    static const uint32 SavedRegs =
          (1 << JSC::X86Registers::esi)
        | (1 << JSC::X86Registers::edi)
# endif
        ;

    static const uint32 SingleByteRegs = (TempRegs | SavedRegs) &
        ~((1 << JSC::X86Registers::esi) |
          (1 << JSC::X86Registers::edi) |
          (1 << JSC::X86Registers::ebp) |
          (1 << JSC::X86Registers::esp));

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
#else
# error "Unsupported platform"
#endif

    static const uint32 AvailRegs = SavedRegs | TempRegs;

    Registers() : freeMask(AvailRegs)
    {
    }

    inline void reset() {
        freeMask = AvailRegs;
    }

    inline bool anyRegsFree() {
        return !!freeMask;
    }

    inline bool anyRegsFree(uint32 mask) {
        return !!(freeMask & mask);
    }

    inline RegisterID allocReg() {
        JS_ASSERT(anyRegsFree());
        RegisterID reg = (RegisterID)(31 - js_bitscan_clz32(freeMask));
        allocSpecific(reg);
        return reg;
    }

    inline bool isRegFree(RegisterID reg) {
        return !!(freeMask & (1 << reg));
    }

    inline void freeReg(RegisterID reg) {
        JS_ASSERT(!isRegFree(reg));
        freeMask |= (1 << reg);
    }

    inline RegisterID allocFromMask(uint32 mask) {
        mask &= freeMask;
        JS_ASSERT(mask);
        RegisterID reg = (RegisterID)(31 - js_bitscan_clz32(freeMask));
        allocSpecific(reg);
        return reg;
    }

    
    
    bool tryAllocReg(uint32 mask, RegisterID &reg) {
        if (anyRegsFree(mask)) {
            reg = allocFromMask(mask);
            return true;
        } else {
            return false;
        }
    }


    inline void allocSpecific(RegisterID reg) {
        JS_ASSERT(isRegFree(reg));
        freeMask &= ~(1 << reg);
    }

    uint32 freeMask;
};

} 

} 

#endif 

