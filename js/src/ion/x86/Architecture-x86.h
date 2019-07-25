








































#ifndef jsion_architecture_x86_h__
#define jsion_architecture_x86_h__

#include "assembler/assembler/MacroAssembler.h"

namespace js {
namespace ion {
static const ptrdiff_t STACK_SLOT_SIZE       = 4;
static const uint32 DOUBLE_STACK_ALIGNMENT   = 2;





static const uint32 ION_FRAME_SLACK_SIZE    = 20;


static const int32 INVALID_STACK_SLOT       = -1;



static const int32 NUNBOX32_TYPE_OFFSET         = 4;
static const int32 NUNBOX32_PAYLOAD_OFFSET      = 0;






static const uint32 BAILOUT_TABLE_ENTRY_SIZE    = 5;

class Registers {
  public:
    typedef JSC::X86Registers::RegisterID Code;

    static const char *GetName(Code code) {
        static const char *Names[] = { "eax", "ecx", "edx", "ebx",
                                       "esp", "ebp", "esi", "edi" };
        return Names[code];
    }

    static const Code StackPointer = JSC::X86Registers::esp;
    static const Code Invalid = JSC::X86Registers::invalid_reg;

    static const uint32 Total = 8;
    static const uint32 Allocatable = 6;

    static const uint32 AllMask = (1 << Total) - 1;

    static const uint32 ArgRegMask = 0;

    static const uint32 VolatileMask =
        (1 << JSC::X86Registers::eax) |
        (1 << JSC::X86Registers::ecx) |
        (1 << JSC::X86Registers::edx);

    static const uint32 NonVolatileMask =
        (1 << JSC::X86Registers::ebx) |
        (1 << JSC::X86Registers::esi) |
        (1 << JSC::X86Registers::edi) |
        (1 << JSC::X86Registers::ebp);

    static const uint32 SingleByteRegs =
        (1 << JSC::X86Registers::eax) |
        (1 << JSC::X86Registers::ecx) |
        (1 << JSC::X86Registers::edx) |
        (1 << JSC::X86Registers::ebx);

    static const uint32 NonAllocatableMask =
        (1 << JSC::X86Registers::esp);

    static const uint32 AllocatableMask = AllMask & ~NonAllocatableMask;

    
    static const uint32 TempMask = VolatileMask & ~NonAllocatableMask;

    static const uint32 JSCallClobberMask =
        AllocatableMask &
        ~(1 << JSC::X86Registers::ecx) &
        ~(1 << JSC::X86Registers::edx);
    static const uint32 JSCCallMask =
        (1 << JSC::X86Registers::eax) |
        (1 << JSC::X86Registers::edx);

    static const uint32 ValueReturnCallMask =
        (1 << JSC::X86Registers::eax) |
        (1 << JSC::X86Registers::edx);
    static const uint32 ObjectReturnCallMask =
        (1 << JSC::X86Registers::eax);
    static const uint32 BoolReturnCallMask =
        (1 << JSC::X86Registers::eax);
    typedef JSC::MacroAssembler::RegisterID RegisterID;

};

class FloatRegisters {
  public:
    typedef JSC::X86Registers::XMMRegisterID Code;

    static const char *GetName(Code code) {
        static const char *Names[] = { "xmm0", "xmm1", "xmm2", "xmm3",
                                       "xmm4", "xmm5", "xmm6", "xmm7" };
        return Names[code];
    }

    static const Code Invalid = JSC::X86Registers::invalid_xmm;

    static const uint32 Total = 8;
    static const uint32 Allocatable = 7;

    static const uint32 AllMask = (1 << Total) - 1;

    static const uint32 VolatileMask = AllMask;
    static const uint32 NonVolatileMask = 0;

    static const uint32 NonAllocatableMask =
        (1 << JSC::X86Registers::xmm7);

    static const uint32 AllocatableMask = AllMask & ~NonAllocatableMask;

    static const uint32 JSCallClobberMask = AllocatableMask;
    static const uint32 JSCCallMask = 0;

    static const uint32 ValueReturnCallMask = 0;
    static const uint32 ObjectReturnCallMask = 0;
    static const uint32 BoolReturnCallMask = 0;
};

} 
} 

#endif 

