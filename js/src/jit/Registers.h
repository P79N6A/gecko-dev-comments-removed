





#ifndef jit_Registers_h
#define jit_Registers_h

#include "mozilla/Array.h"

#include "jit/IonTypes.h"
#if defined(JS_CODEGEN_X86)
# include "jit/x86/Architecture-x86.h"
#elif defined(JS_CODEGEN_X64)
# include "jit/x64/Architecture-x64.h"
#elif defined(JS_CODEGEN_ARM)
# include "jit/arm/Architecture-arm.h"
#elif defined(JS_CODEGEN_MIPS)
# include "jit/mips/Architecture-mips.h"
#elif defined(JS_CODEGEN_NONE)
# include "jit/none/Architecture-none.h"
#else
# error "Unknown architecture!"
#endif

namespace js {
namespace jit {

struct Register {
    typedef Registers Codes;
    typedef Codes::Code Code;
    typedef Codes::SetType SetType;
    Code code_;
    static Register FromCode(uint32_t i) {
        JS_ASSERT(i < Registers::Total);
        Register r = { (Registers::Code)i };
        return r;
    }
    static Register FromName(const char *name) {
        Registers::Code code = Registers::FromName(name);
        Register r = { code };
        return r;
    }
    Code code() const {
        JS_ASSERT((uint32_t)code_ < Registers::Total);
        return code_;
    }
    const char *name() const {
        return Registers::GetName(code());
    }
    bool operator ==(Register other) const {
        return code_ == other.code_;
    }
    bool operator !=(Register other) const {
        return code_ != other.code_;
    }
    bool volatile_() const {
        return !!((1 << code()) & Registers::VolatileMask);
    }
    bool aliases(const Register &other) const {
        return code_ == other.code_;
    }
    uint32_t numAliased() const {
        return 1;
    }

    
    
    
    void aliased(uint32_t aliasIdx, Register *ret) const {
        JS_ASSERT(aliasIdx == 0);
        *ret = *this;
    }
    static uint32_t SetSize(SetType x) {
        return Codes::SetSize(x);
    }
    static uint32_t FirstBit(SetType x) {
        return Codes::FirstBit(x);
    }
    static uint32_t LastBit(SetType x) {
        return Codes::LastBit(x);
    }
};

class RegisterDump
{
  protected: 
    mozilla::Array<uintptr_t, Registers::Total> regs_;
    mozilla::Array<double, FloatRegisters::TotalPhys> fpregs_;

  public:
    static size_t offsetOfRegister(Register reg) {
        return offsetof(RegisterDump, regs_) + reg.code() * sizeof(uintptr_t);
    }
    static size_t offsetOfRegister(FloatRegister reg) {
        return offsetof(RegisterDump, fpregs_) + reg.getRegisterDumpOffsetInBytes();
    }
};


class MachineState
{
    mozilla::Array<uintptr_t *, Registers::Total> regs_;
    mozilla::Array<double *, FloatRegisters::Total> fpregs_;

  public:
    static MachineState FromBailout(mozilla::Array<uintptr_t, Registers::Total> &regs,
                                    mozilla::Array<double, FloatRegisters::TotalPhys> &fpregs);

    void setRegisterLocation(Register reg, uintptr_t *up) {
        regs_[reg.code()] = up;
    }
    void setRegisterLocation(FloatRegister reg, double *dp) {
        fpregs_[reg.code()] = dp;
    }

    bool has(Register reg) const {
        return regs_[reg.code()] != nullptr;
    }
    bool has(FloatRegister reg) const {
        return fpregs_[reg.code()] != nullptr;
    }
    uintptr_t read(Register reg) const {
        return *regs_[reg.code()];
    }
    double read(FloatRegister reg) const {
        return *fpregs_[reg.code()];
    }
    void write(Register reg, uintptr_t value) const {
        *regs_[reg.code()] = value;
    }
};

} 
} 

#endif 
