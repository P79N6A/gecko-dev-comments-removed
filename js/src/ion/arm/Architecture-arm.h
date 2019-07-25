








































#ifndef jsion_architecture_arm_h__
#define jsion_architecture_arm_h__
#include <limits.h>
#include "jscntxt.h"

namespace js {
namespace ion {

static const ptrdiff_t STACK_SLOT_SIZE       = 4;
static const uint32 DOUBLE_STACK_ALIGNMENT   = 2;





static const uint32 ION_FRAME_SLACK_SIZE    = 20;


static const int32 INVALID_STACK_SLOT       = -1;



static const int32 NUNBOX32_TYPE_OFFSET         = 4;
static const int32 NUNBOX32_PAYLOAD_OFFSET      = 0;










static const uint32 BAILOUT_TABLE_ENTRY_SIZE    = 4;

class Registers
{
  public:
    typedef enum {
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
    } RegisterID;
    typedef RegisterID Code;


    static const char *GetName(Code code) {
        static const char *Names[] = { "r0", "r1", "r2", "r3",
                                       "r4", "r5", "r6", "r7",
                                       "r8", "r9", "r10", "r11",
                                       "r12", "sp", "r14", "pc"};
        return Names[code];
    }

    static const Code StackPointer = sp;
    static const Code Invalid = invalid_reg;

    static const uint32 Total = 16;
    static const uint32 Allocatable = 13;

    static const uint32 AllMask = (1 << Total) - 1;
    static const uint32 ArgRegMask = (1 << r0) | (1 << r1) | (1 << r2) | (1 << r3);

    
    static const uint32 JSCCallMask = (1 << r0) | (1 << r1) | (1 << r2) | (1 << r3);

    static const uint32 VolatileMask =
        (1 << r0) |
        (1 << r1) |
        (1 << Registers::r2) |
        (1 << Registers::r3);

    static const uint32 NonVolatileMask =
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

    static const uint32 SingleByteRegs =
        VolatileMask | NonVolatileMask;
    
    
    static const uint32 NonAllocatableMask =
        (1 << Registers::sp) |
        (1 << Registers::r12) | 
        (1 << Registers::lr) |
        (1 << Registers::pc);

    
    static const uint32 TempMask = VolatileMask & ~NonAllocatableMask;

    
    static const uint32 JSCallMask =
              (1 << Registers::r0) |
              (1 << Registers::r1) |
              (1 << Registers::r2) |
              (1 << Registers::r3);

    static const uint32 AllocatableMask = AllMask & ~NonAllocatableMask;
};

class FloatRegisters
{
  public:
    typedef enum {
        d0,
        d1,
        d2,
        d3,
        SD0 = d3,
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
    } FPRegisterID;
    typedef FPRegisterID Code;

    static const char *GetName(Code code) {
        static const char *Names[] = { "d0", "d1", "d2", "d3",
                                       "d4", "d5", "d6", "d7",
                                       "d8", "d9", "d10", "d11",
                                       "d12", "d13", "d14", "d15"};
        return Names[code];
    }

    static const Code Invalid = invalid_freg;

    static const uint32 Total = 16;
    static const uint32 Allocatable = 15;

    static const uint32 AllMask = (1 << Total) - 1;

    static const uint32 VolatileMask = AllMask;
    static const uint32 NonVolatileMask = 0;

    static const uint32 NonAllocatableMask =
        
                                        (1 << d0) | (1 << invalid_freg);

    
    static const uint32 TempMask = VolatileMask & ~NonAllocatableMask;

    static const uint32 AllocatableMask = AllMask & ~NonAllocatableMask;
};

bool hasMOVWT();
bool hasVFPv3();
bool has16DP();

} 
} 


#endif 
