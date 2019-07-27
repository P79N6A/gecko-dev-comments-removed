





#ifndef jit_Registers_h
#define jit_Registers_h

#include "mozilla/Array.h"

#include "jit/IonTypes.h"
#if defined(JS_CODEGEN_X86) || defined(JS_CODEGEN_X64)
# include "jit/x86-shared/Architecture-x86-shared.h"
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
    typedef Codes::Encoding Encoding;
    typedef Codes::Code Code;
    typedef Codes::SetType SetType;

    Codes::Encoding reg_;
    static Register FromCode(Code i) {
        MOZ_ASSERT(i < Registers::Total);
        Register r = { Encoding(i) };
        return r;
    }
    static Register FromName(const char* name) {
        Code code = Registers::FromName(name);
        Register r = { Encoding(code) };
        return r;
    }
    Code code() const {
        MOZ_ASSERT(Code(reg_) < Registers::Total);
        return Code(reg_);
    }
    Encoding encoding() const {
        MOZ_ASSERT(Code(reg_) < Registers::Total);
        return reg_;
    }
    const char* name() const {
        return Registers::GetName(code());
    }
    bool operator ==(Register other) const {
        return reg_ == other.reg_;
    }
    bool operator !=(Register other) const {
        return reg_ != other.reg_;
    }
    bool volatile_() const {
        return !!((SetType(1) << code()) & Registers::VolatileMask);
    }
    bool aliases(const Register& other) const {
        return reg_ == other.reg_;
    }
    uint32_t numAliased() const {
        return 1;
    }

    
    
    
    void aliased(uint32_t aliasIdx, Register* ret) const {
        MOZ_ASSERT(aliasIdx == 0);
        *ret = *this;
    }

    SetType alignedOrDominatedAliasedSet() const {
        return SetType(1) << code();
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
  public:
    typedef mozilla::Array<Registers::RegisterContent, Registers::Total> GPRArray;
    typedef mozilla::Array<FloatRegisters::RegisterContent, FloatRegisters::TotalPhys> FPUArray;

  protected: 
    GPRArray regs_;
    FPUArray fpregs_;

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
    mozilla::Array<Registers::RegisterContent*, Registers::Total> regs_;
    mozilla::Array<FloatRegisters::RegisterContent*, FloatRegisters::Total> fpregs_;

  public:
    MachineState() {
        for (unsigned i = 0; i < Registers::Total; i++)
            regs_[i] = reinterpret_cast<Registers::RegisterContent*>(i + 0x100);
        for (unsigned i = 0; i < FloatRegisters::Total; i++)
            fpregs_[i] = reinterpret_cast<FloatRegisters::RegisterContent*>(i + 0x200);
    }

    static MachineState FromBailout(RegisterDump::GPRArray& regs, RegisterDump::FPUArray& fpregs);

    void setRegisterLocation(Register reg, uintptr_t* up) {
        regs_[reg.code()] = (Registers::RegisterContent*) up;
    }
    void setRegisterLocation(FloatRegister reg, float* fp) {
        MOZ_ASSERT(reg.isSingle());
        fpregs_[reg.code()] = (FloatRegisters::RegisterContent*) fp;
    }
    void setRegisterLocation(FloatRegister reg, double* dp) {
        fpregs_[reg.code()] = (FloatRegisters::RegisterContent*) dp;
    }
    void setRegisterLocation(FloatRegister reg, FloatRegisters::RegisterContent* rp) {
        fpregs_[reg.code()] = rp;
    }

    bool has(Register reg) const {
        return regs_[reg.code()] != nullptr;
    }
    bool has(FloatRegister reg) const {
        return fpregs_[reg.code()] != nullptr;
    }
    uintptr_t read(Register reg) const {
        return regs_[reg.code()]->r;
    }
    double read(FloatRegister reg) const {
        return fpregs_[reg.code()]->d;
    }
    void write(Register reg, uintptr_t value) const {
        regs_[reg.code()]->r = value;
    }
    const FloatRegisters::RegisterContent* address(FloatRegister reg) const {
        return fpregs_[reg.code()];
    }
};

} 
} 

#endif 
