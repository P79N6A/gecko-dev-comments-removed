








































#ifndef jsion_cpu_assembler_h__
#define jsion_cpu_assembler_h__

#if defined(JS_CPU_X86)
# include "x86/Architecture-x86.h"
#elif defined(JS_CPU_X64)
# include "x64/Architecture-x64.h"
#elif defined(JS_CPU_ARM)
# include "arm/Architecture-ARM.h"
#endif

namespace js {
namespace ion {

struct Register {
    RegisterCodes::Code code_;

    static Register FromCode(uint32 i) {
        JS_ASSERT(i < RegisterCodes::Total);
        Register r = { (RegisterCodes::Code)i };
        return r;
    }

    RegisterCodes::Code code() const {
        return code_;
    }

    const char *name() const {
        return RegisterCodes::GetName(code());
    }
};

struct FloatRegister {
    FloatRegisterCodes::Code code_;

    static FloatRegister FromCode(uint32 i) {
        JS_ASSERT(i < FloatRegisterCodes::Total);
        FloatRegister r = { (FloatRegisterCodes::Code)i };
        return r;
    }

    FloatRegisterCodes::Code code() const {
        return code_;
    }

    const char *name() const {
        return FloatRegisterCodes::GetName(code());
    }
};

} 
} 

#if defined(JS_CPU_X86)
# include "x86/Assembler-x86.h"
#elif defined(JS_CPU_X64)
# include "x64/Assembler-x64.h"
#elif defined(JS_CPU_ARM)
# include "arm/Assembler-ARM.h"
#endif

#endif 

