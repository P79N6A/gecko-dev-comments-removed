








































#ifndef jsion_cpu_x64_regs_h__
#define jsion_cpu_x64_regs_h__

namespace js {
namespace ion {

static const ptrdiff_t STACK_SLOT_SIZE       = 8;

class RegisterCodes {
  public:
    enum Code {
        RAX,
        RCX,
        RDX,
        RBX,
        RSP,
        RBP,
        RSI,
        RDI,
        R8,
        R9,
        R10,
        R11,
        R12,
        R13,
        R14,
        R15
    };

    static const uint32 Total = 16;
    static const uint32 Allocatable = 13;

    static const uint32 AllMask = (1 << Total) - 1;

    static const uint32 VolatileMask =
        (1 << RAX) |
        (1 << RCX) |
        (1 << RDX) |
# if !defined(_WIN64)
        (1 << RSI) |
        (1 << RDI) |
# endif
        (1 << R8) |
        (1 << R9) |
        (1 << R10) |
        (1 << R11);

    static const uint32 NonVolatileMask =
        (1 << RBX) |
#if defined(_WIN64)
        (1 << RSI) |
        (1 << RDI) |
#endif
        (1 << RBP) |
        (1 << R12) |
        (1 << R13) |
        (1 << R14) |
        (1 << R15);

    static const uint32 SingleByteRegs = VolatileMask | NonVolatileMask;

    static const uint32 NonAllocatableMask =
        (1 << RSP) |
        (1 << RBP) |
        (1 << R11);         

    static const uint32 AllocatableMask = AllMask & ~NonAllocatableMask;
};

class FloatRegisterCodes {
  public:
    enum Code {
        XMM0,
        XMM1,
        XMM2,
        XMM3,
        XMM4,
        XMM5,
        XMM6,
        XMM7,
        XMM8,
        XMM9,
        XMM10,
        XMM11,
        XMM12,
        XMM13,
        XMM14,
        XMM15
    };

    static const uint32 Total = 16;
    static const uint32 Allocatable = 16;

    static const uint32 AllMask = (1 << Total) - 1;

    static const uint32 VolatileMask = 
#if defined(_WIN64)
        (1 << XMM0) |
        (1 << XMM1) |
        (1 << XMM2) |
        (1 << XMM3) |
        (1 << XMM4) |
        (1 << XMM5);
#else
        AllMask;
#endif


    static const uint32 NonVolatileMask = AllMask & ~VolatileMask;
    static const uint32 AllocatableMask = AllMask;
};

} 
} 

#endif

