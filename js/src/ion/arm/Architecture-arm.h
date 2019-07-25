








































#ifndef jsion_architecture_arm_h__
#define jsion_architecture_arm_h__

#include "assembler/assembler/ARMAssembler.h"

namespace js {
namespace ion {

static const ptrdiff_t STACK_SLOT_SIZE       = 4;
static const uint32 DOUBLE_STACK_ALIGNMENT   = 2;





static const uint32 ION_FRAME_SLACK_SIZE    = 20;


static const int32 INVALID_STACK_SLOT       = -1;






static const uint32 BAILOUT_TABLE_ENTRY_SIZE    = 8;

class Registers
{
  public:
    typedef JSC::ARMRegisters::RegisterID Code;

    static const char *GetName(Code code) {
        static const char *Names[] = { "r0", "r1", "r2", "r3",
                                       "r4", "r5", "r6", "r7",
                                       "r8", "r9", "r10", "r11",
                                       "r12", "sp", "r14", "pc"};
        return Names[code];
    }

    static const Code StackPointer = JSC::ARMRegisters::sp;
    static const Code Invalid = JSC::ARMRegisters::invalid_reg;

    static const uint32 Total = 16;
    static const uint32 Allocatable = 13;

    static const uint32 AllMask = (1 << Total) - 1;

    static const uint32 VolatileMask =
        (1 << JSC::ARMRegisters::r0) |
        (1 << JSC::ARMRegisters::r1) |
        (1 << JSC::ARMRegisters::r2) |
        (1 << JSC::ARMRegisters::r3);

    static const uint32 NonVolatileMask =
        (1 << JSC::ARMRegisters::r4) |
        (1 << JSC::ARMRegisters::r5) |
        (1 << JSC::ARMRegisters::r6) |
        (1 << JSC::ARMRegisters::r7) |
        (1 << JSC::ARMRegisters::r8) |
        (1 << JSC::ARMRegisters::r9) |
        (1 << JSC::ARMRegisters::r10) |
        (1 << JSC::ARMRegisters::r11) |
        (1 << JSC::ARMRegisters::r12) |
        (1 << JSC::ARMRegisters::r14);

    static const uint32 SingleByteRegs =
        VolatileMask | NonVolatileMask;
    
    
    static const uint32 NonAllocatableMask =
        (1 << JSC::ARMRegisters::sp) |
        (1 << JSC::ARMRegisters::r12) | 
        (1 << JSC::ARMRegisters::pc);

    
    static const uint32 TempMask = VolatileMask & ~NonAllocatableMask;

    static const uint32 JSCallClobberMask =
              (1 << JSC::ARMRegisters::r0) |
              (1 << JSC::ARMRegisters::r1) |
              (1 << JSC::ARMRegisters::r2) |
              (1 << JSC::ARMRegisters::r3);
    static const uint32 AllocatableMask = AllMask & ~NonAllocatableMask;
};

class FloatRegisters
{
  public:
    typedef JSC::ARMRegisters::FPRegisterID Code;

    static const char *GetName(Code code) {
        static const char *Names[] = { "d0", "d1", "d2", "d3",
                                       "d4", "d5", "d6", "d7",
                                       "d8", "d9", "d10", "d11",
                                       "d12", "d13", "d14", "d15"};
        return Names[code];
    }

    static const Code Invalid = JSC::ARMRegisters::invalid_freg;

    static const uint32 Total = 16;
    static const uint32 Allocatable = 15;

    static const uint32 AllMask = (1 << Total) - 1;

    static const uint32 VolatileMask = AllMask;
    static const uint32 NonVolatileMask = 0;

    static const uint32 NonAllocatableMask =
        
        (1 << JSC::ARMRegisters::SD0);

    
    static const uint32 TempMask = VolatileMask & ~NonAllocatableMask;

    static const uint32 AllocatableMask = AllMask & ~NonAllocatableMask;
    static const uint32 JSCallClobberMask = AllocatableMask;

};

bool hasMOVWT();
bool hasVFPv3();

} 
} 

#endif 
