





#ifndef jit_x64_Architecture_x64_h
#define jit_x64_Architecture_x64_h

#include "assembler/assembler/MacroAssembler.h"

namespace js {
namespace jit {

static const ptrdiff_t STACK_SLOT_SIZE       = 8;
static const uint32_t DOUBLE_STACK_ALIGNMENT   = 1;





static const uint32_t ION_FRAME_SLACK_SIZE     = 24;

#ifdef _WIN64
static const uint32_t ShadowStackSpace = 32;
#else
static const uint32_t ShadowStackSpace = 0;
#endif


static const int32_t INVALID_STACK_SLOT       = -1;

class Registers {
  public:
    typedef JSC::X86Registers::RegisterID Code;

    static const char *GetName(Code code) {
        static const char * const Names[] = { "rax", "rcx", "rdx", "rbx",
                                              "rsp", "rbp", "rsi", "rdi",
                                              "r8",  "r9",  "r10", "r11",
                                              "r12", "r13", "r14", "r15" };
        return Names[code];
    }

    static const Code StackPointer = JSC::X86Registers::esp;
    static const Code Invalid = JSC::X86Registers::invalid_reg;

    static const uint32_t Total = 16;
    static const uint32_t Allocatable = 14;

    static const uint32_t AllMask = (1 << Total) - 1;

    static const uint32_t ArgRegMask =
# if !defined(_WIN64)
        (1 << JSC::X86Registers::edi) |
        (1 << JSC::X86Registers::esi) |
# endif
        (1 << JSC::X86Registers::edx) |
        (1 << JSC::X86Registers::ecx) |
        (1 << JSC::X86Registers::r8) |
        (1 << JSC::X86Registers::r9);

    static const uint32_t VolatileMask =
        (1 << JSC::X86Registers::eax) |
        (1 << JSC::X86Registers::ecx) |
        (1 << JSC::X86Registers::edx) |
# if !defined(_WIN64)
        (1 << JSC::X86Registers::esi) |
        (1 << JSC::X86Registers::edi) |
# endif
        (1 << JSC::X86Registers::r8) |
        (1 << JSC::X86Registers::r9) |
        (1 << JSC::X86Registers::r10) |
        (1 << JSC::X86Registers::r11);

    static const uint32_t NonVolatileMask =
        (1 << JSC::X86Registers::ebx) |
#if defined(_WIN64)
        (1 << JSC::X86Registers::esi) |
        (1 << JSC::X86Registers::edi) |
#endif
        (1 << JSC::X86Registers::ebp) |
        (1 << JSC::X86Registers::r12) |
        (1 << JSC::X86Registers::r13) |
        (1 << JSC::X86Registers::r14) |
        (1 << JSC::X86Registers::r15);

    static const uint32_t WrapperMask = VolatileMask;

    static const uint32_t SingleByteRegs = VolatileMask | NonVolatileMask;

    static const uint32_t NonAllocatableMask =
        (1 << JSC::X86Registers::esp) |
        (1 << JSC::X86Registers::r11);      

    
    static const uint32_t TempMask = VolatileMask & ~NonAllocatableMask;

    static const uint32_t AllocatableMask = AllMask & ~NonAllocatableMask;

    
    static const uint32_t JSCallMask =
        (1 << JSC::X86Registers::ecx);

    
    static const uint32_t CallMask =
        (1 << JSC::X86Registers::eax);
};


typedef uint16_t PackedRegisterMask;

class FloatRegisters {
  public:
    typedef JSC::X86Registers::XMMRegisterID Code;

    static const char *GetName(Code code) {
        static const char * const Names[] = { "xmm0",  "xmm1",  "xmm2",  "xmm3",
                                              "xmm4",  "xmm5",  "xmm6",  "xmm7",
                                              "xmm8",  "xmm9",  "xmm10", "xmm11",
                                              "xmm12", "xmm13", "xmm14", "xmm15" };
        return Names[code];
    }

    static const Code Invalid = JSC::X86Registers::invalid_xmm;

    static const uint32_t Total = 16;
    static const uint32_t Allocatable = 15;

    static const uint32_t AllMask = (1 << Total) - 1;

    static const uint32_t VolatileMask = 
#if defined(_WIN64)
        (1 << JSC::X86Registers::xmm0) |
        (1 << JSC::X86Registers::xmm1) |
        (1 << JSC::X86Registers::xmm2) |
        (1 << JSC::X86Registers::xmm3) |
        (1 << JSC::X86Registers::xmm4) |
        (1 << JSC::X86Registers::xmm5);
#else
        AllMask;
#endif

    static const uint32_t NonVolatileMask = AllMask & ~VolatileMask;

    static const uint32_t WrapperMask = VolatileMask;

    static const uint32_t NonAllocatableMask =
        (1 << JSC::X86Registers::xmm15);    

    static const uint32_t AllocatableMask = AllMask & ~NonAllocatableMask;
};

} 
} 

#endif 
