





#ifndef jit_x86_Architecture_x86_h
#define jit_x86_Architecture_x86_h

#include "assembler/assembler/MacroAssembler.h"

namespace js {
namespace jit {





static const uint32_t ION_FRAME_SLACK_SIZE    = 20;


static const uint32_t ShadowStackSpace = 0;



static const int32_t NUNBOX32_TYPE_OFFSET         = 4;
static const int32_t NUNBOX32_PAYLOAD_OFFSET      = 0;






static const uint32_t BAILOUT_TABLE_ENTRY_SIZE    = 5;

class Registers {
  public:
    typedef JSC::X86Registers::RegisterID Code;

    static const char *GetName(Code code) {
        static const char * const Names[] = { "eax", "ecx", "edx", "ebx",
                                              "esp", "ebp", "esi", "edi" };
        return Names[code];
    }

    static Code FromName(const char *name) {
        for (size_t i = 0; i < Total; i++) {
            if (strcmp(GetName(Code(i)), name) == 0)
                return Code(i);
        }
        return Invalid;
    }

    static const Code StackPointer = JSC::X86Registers::esp;
    static const Code Invalid = JSC::X86Registers::invalid_reg;

    static const uint32_t Total = 8;
    static const uint32_t Allocatable = 7;

    static const uint32_t AllMask = (1 << Total) - 1;

    static const uint32_t ArgRegMask = 0;

    static const uint32_t VolatileMask =
        (1 << JSC::X86Registers::eax) |
        (1 << JSC::X86Registers::ecx) |
        (1 << JSC::X86Registers::edx);

    static const uint32_t NonVolatileMask =
        (1 << JSC::X86Registers::ebx) |
        (1 << JSC::X86Registers::esi) |
        (1 << JSC::X86Registers::edi) |
        (1 << JSC::X86Registers::ebp);

    static const uint32_t WrapperMask =
        VolatileMask |
        (1 << JSC::X86Registers::ebx);

    static const uint32_t SingleByteRegs =
        (1 << JSC::X86Registers::eax) |
        (1 << JSC::X86Registers::ecx) |
        (1 << JSC::X86Registers::edx) |
        (1 << JSC::X86Registers::ebx);

    static const uint32_t NonAllocatableMask =
        (1 << JSC::X86Registers::esp);

    static const uint32_t AllocatableMask = AllMask & ~NonAllocatableMask;

    
    static const uint32_t TempMask = VolatileMask & ~NonAllocatableMask;

    
    static const uint32_t JSCallMask =
        (1 << JSC::X86Registers::ecx) |
        (1 << JSC::X86Registers::edx);

    
    static const uint32_t CallMask =
        (1 << JSC::X86Registers::eax);
};


typedef uint8_t PackedRegisterMask;

class FloatRegisters {
  public:
    typedef JSC::X86Registers::XMMRegisterID Code;

    static const char *GetName(Code code) {
        static const char * const Names[] = { "xmm0", "xmm1", "xmm2", "xmm3",
                                              "xmm4", "xmm5", "xmm6", "xmm7" };
        return Names[code];
    }

    static Code FromName(const char *name) {
        for (size_t i = 0; i < Total; i++) {
            if (strcmp(GetName(Code(i)), name) == 0)
                return Code(i);
        }
        return Invalid;
    }

    static const Code Invalid = JSC::X86Registers::invalid_xmm;

    static const uint32_t Total = 8;
    static const uint32_t Allocatable = 7;

    static const uint32_t AllMask = (1 << Total) - 1;

    static const uint32_t VolatileMask = AllMask;
    static const uint32_t NonVolatileMask = 0;

    static const uint32_t WrapperMask = VolatileMask;

    static const uint32_t NonAllocatableMask =
        (1 << JSC::X86Registers::xmm7);

    static const uint32_t AllocatableMask = AllMask & ~NonAllocatableMask;
};

struct FloatRegister {
    typedef FloatRegisters Codes;
    typedef Codes::Code Code;

    Code code_;

    static FloatRegister FromCode(uint32_t i) {
        JS_ASSERT(i < FloatRegisters::Total);
        FloatRegister r = { (FloatRegisters::Code)i };
        return r;
    }
    Code code() const {
        JS_ASSERT((uint32_t)code_ < FloatRegisters::Total);
        return code_;
    }
    const char *name() const {
        return FloatRegisters::GetName(code());
    }
    bool volatile_() const {
        return !!((1 << code()) & FloatRegisters::VolatileMask);
    }
    bool operator != (const FloatRegister &other) const {
        return other.code_ != code_;
    }
    bool operator == (const FloatRegister &other) const {
        return other.code_ == code_;
    }
    bool aliases(FloatRegister const &other) const;
};

} 
} 

#endif 
