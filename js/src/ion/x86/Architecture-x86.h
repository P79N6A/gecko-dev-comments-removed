








































#ifndef jsion_cpu_x86_regs_h__
#define jsion_cpu_x86_regs_h__

namespace js {
namespace ion {

static const ptrdiff_t STACK_SLOT_SIZE       = 4;

class RegisterCodes {
  public:
    enum Code {
        EAX,
        ECX,
        EDX,
        EBX,
        ESP,
        EBP,
        ESI,
        EDI
    };

    static const char *GetName(Code code) {
        static const char *Names[] = { "eax", "ecx", "edx", "ebx",
                                       "esp", "ebp", "esi", "edi" };
        return Names[code];
    }

    static const uint32 Total = 8;
    static const uint32 Allocatable = 6;

    static const uint32 AllMask = (1 << Total) - 1;

    static const uint32 VolatileMask =
        (1 << EAX) |
        (1 << ECX) |
        (1 << EDX);

    static const uint32 NonVolatileMask =
        (1 << EBX) |
        (1 << ESI) |
        (1 << EDI) |
        (1 << EBP);

    static const uint32 SingleByteRegs =
        (1 << EAX) |
        (1 << ECX) |
        (1 << EDX) |
        (1 << EBX);

    static const uint32 NonAllocatableMask =
        (1 << ESP) |
        (1 << EBP);

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
        XMM7
    };

    static const char *GetName(Code code) {
        static const char *Names[] = { "xmm0", "xmm1", "xmm2", "xmm3",
                                       "xmm4", "xmm5", "xmm6", "xmm7" };
        return Names[code];
    }

    static const uint32 Total = 8;
    static const uint32 Allocatable = 8;

    static const uint32 AllMask = (1 << Total) - 1;

    static const uint32 VolatileMask = AllMask;
    static const uint32 NonVolatileMask = 0;
    static const uint32 AllocatableMask = AllMask;
};

} 
} 

#endif 

