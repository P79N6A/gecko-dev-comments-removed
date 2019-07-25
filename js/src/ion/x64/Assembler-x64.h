








































#ifndef jsion_cpu_x64_assembler_h__
#define jsion_cpu_x64_assembler_h__

#include "ion/shared/Assembler-shared.h"
#include "ion/CompactBuffer.h"
#include "ion/IonCode.h"

namespace js {
namespace ion {

static const Register rax = { JSC::X86Registers::eax };
static const Register rbx = { JSC::X86Registers::ebx };
static const Register rcx = { JSC::X86Registers::ecx };
static const Register rdx = { JSC::X86Registers::edx };
static const Register rsi = { JSC::X86Registers::esi };
static const Register rdi = { JSC::X86Registers::edi };
static const Register rbp = { JSC::X86Registers::ebp };
static const Register r8  = { JSC::X86Registers::r8  };
static const Register r9  = { JSC::X86Registers::r9  };
static const Register r10 = { JSC::X86Registers::r10 };
static const Register r11 = { JSC::X86Registers::r11 };
static const Register r12 = { JSC::X86Registers::r12 };
static const Register r13 = { JSC::X86Registers::r13 };
static const Register r14 = { JSC::X86Registers::r14 };
static const Register r15 = { JSC::X86Registers::r15 };
static const Register rsp = { JSC::X86Registers::esp };


static const Register eax = rax;
static const Register ebx = rbx;
static const Register ecx = rcx;
static const Register edx = rdx;
static const Register esi = rsi;
static const Register edi = rdi;
static const Register ebp = rbp;
static const Register esp = rsp;

static const Register InvalidReg = { JSC::X86Registers::invalid_reg };
static const FloatRegister InvalidFloatReg = { JSC::X86Registers::invalid_xmm };

static const Register StackPointer = rsp;
static const Register JSReturnReg = rcx;
static const Register ReturnReg = rax;
static const Register ScratchReg = r11;
static const FloatRegister ScratchFloatReg = { JSC::X86Registers::xmm15 };

static const Register ArgumentsRectifierReg = { JSC::X86Registers::r8 };


#if defined(_WIN64)
static const Register ArgReg0 = rcx;
static const Register ArgReg1 = rdx;
static const Register ArgReg2 = r8;
static const Register ArgReg3 = r9;
#else
static const Register ArgReg0 = rdi;
static const Register ArgReg1 = rsi;
static const Register ArgReg2 = rdx;
static const Register ArgReg3 = rcx;
static const Register ArgReg4 = r8;
static const Register ArgReg5 = r9;
#endif

enum Scale {
    TimesOne,
    TimesTwo,
    TimesFour,
    TimesEight
};

static const Scale ScalePointer = TimesEight;

class Operand
{
  public:
    enum Kind {
        REG,
        REG_DISP,
        FPREG,
        SCALE
    };

    Kind kind_ : 2;
    int32 base_ : 5;
    Scale scale_ : 2;
    int32 disp_;
    int32 index_ : 5;

  public:
    explicit Operand(const Register &reg)
      : kind_(REG),
        base_(reg.code())
    { }
    explicit Operand(const FloatRegister &reg)
      : kind_(FPREG),
        base_(reg.code())
    { }
    explicit Operand(const Register &base, const Register &index, Scale scale, int32 disp = 0)
      : kind_(SCALE),
        base_(base.code()),
        scale_(scale),
        disp_(disp),
        index_(index.code())
    { }
    Operand(const Register &reg, int32 disp)
      : kind_(REG_DISP),
        base_(reg.code()),
        disp_(disp)
    { }

    Kind kind() const {
        return kind_;
    }
    Register::Code reg() const {
        JS_ASSERT(kind() == REG);
        return (Registers::Code)base_;
    }
    Registers::Code base() const {
        JS_ASSERT(kind() == REG_DISP || kind() == SCALE);
        return (Registers::Code)base_;
    }
    Registers::Code index() const {
        JS_ASSERT(kind() == SCALE);
        return (Registers::Code)index_;
    }
    Scale scale() const {
        JS_ASSERT(kind() == SCALE);
        return scale_;
    }
    FloatRegisters::Code fpu() const {
        JS_ASSERT(kind() == FPREG);
        return (FloatRegisters::Code)base_;
    }
    int32 disp() const {
        JS_ASSERT(kind() == REG_DISP || kind() == SCALE);
        return disp_;
    }
};

} 
} 

#include "ion/shared/Assembler-x86-shared.h"

namespace js {
namespace ion {

class ValueOperand
{
    Register value_;

  public:
    explicit ValueOperand(Register value)
      : value_(value)
    { }

    Operand value() const {
        return Operand(value_);
    }
    Register valueReg() const {
        return value_;
    }
};

class Assembler : public AssemblerX86Shared
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    static const uint32 SizeOfExtendedJump = 1 + 1 + 4 + 8;
    static const uint32 SizeOfJumpTableEntry = 16;

    uint32 extendedJumpTable_;

    static IonCode *CodeFromJump(IonCode *code, uint8 *jump);

  private:
    void writeRelocation(JmpSrc src);
    void addPendingJump(JmpSrc src, void *target, Relocation::Kind reloc);

  public:
    using AssemblerX86Shared::j;
    using AssemblerX86Shared::jmp;

    Assembler()
      : extendedJumpTable_(0)
    {
    }

    static void TraceRelocations(JSTracer *trc, IonCode *code, CompactBufferReader &reader);

    
    
    void flush();

    
    
    void executableCopy(uint8 *buffer);

    

    void movq(ImmWord word, const Register &dest) {
        masm.movq_i64r(word.value, dest.code());
    }
    void movq(ImmGCPtr ptr, const Register &dest) {
        masm.movq_i64r(ptr.value, dest.code());
    }
    void movq(const Operand &src, const Register &dest) {
        switch (src.kind()) {
          case Operand::REG:
            masm.movq_rr(src.reg(), dest.code());
            break;
          case Operand::REG_DISP:
            masm.movq_mr(src.disp(), src.base(), dest.code());
            break;
          default:
            JS_NOT_REACHED("unexpected operand kind");
        }
    }
    void movq(const Register &src, const Operand &dest) {
        switch (dest.kind()) {
          case Operand::REG:
            masm.movq_rr(src.code(), dest.reg());
            break;
          case Operand::REG_DISP:
            masm.movq_rm(src.code(), dest.disp(), dest.base());
            break;
          default:
            JS_NOT_REACHED("unexpected operand kind");
        }
    }
    void movqsd(const Register &src, const FloatRegister &dest) {
        masm.movq_rr(src.code(), dest.code());
    }
    void movqsd(const FloatRegister &src, const Register &dest) {
        masm.movq_rr(src.code(), dest.code());
    }
    void movq(const Register &src, const Register &dest) {
        masm.movq_rr(src.code(), dest.code());
    }

    void andq(const Register &src, const Register &dest) {
        masm.andq_rr(src.code(), dest.code());
    }
    void andq(Imm32 imm, const Register &dest) {
        masm.andq_ir(imm.value, dest.code());
    }

    void addq(Imm32 imm, const Register &dest) {
        masm.addq_ir(imm.value, dest.code());
    }
    void addq(Imm32 imm, const Operand &dest) {
        switch (dest.kind()) {
          case Operand::REG:
            masm.addq_ir(imm.value, dest.reg());
            break;
          case Operand::REG_DISP:
            masm.addq_im(imm.value, dest.disp(), dest.base());
            break;
          default:
            JS_NOT_REACHED("unexpected operand kind");
        }
    }
    void addq(const Register &src, const Register &dest) {
        masm.addq_rr(src.code(), dest.code());
    }

    void subq(Imm32 imm, const Register &dest) {
        masm.subq_ir(imm.value, dest.code());
    }
    void subq(const Register &src, const Register &dest) {
        masm.subq_rr(src.code(), dest.code());
    }
    void shlq(Imm32 imm, const Register &dest) {
        masm.shlq_i8r(imm.value, dest.code());
    }
    void shrq(Imm32 imm, const Register &dest) {
        masm.shrq_i8r(imm.value, dest.code());
    }
    void orq(const Register &src, const Register &dest) {
        masm.orq_rr(src.code(), dest.code());
    }
    void orq(const Operand &src, const Register &dest) {
        switch (src.kind()) {
          case Operand::REG:
            masm.orq_rr(src.reg(), dest.code());
            break;
          case Operand::REG_DISP:
            masm.orq_mr(src.disp(), src.base(), dest.code());
            break;
          default:
            JS_NOT_REACHED("unexpected operand kind");
        }
    }

    void mov(const Imm32 &imm32, const Register &dest) {
        movl(imm32, dest);
    }
    void mov(const Operand &src, const Register &dest) {
        movq(src, dest);
    }
    void mov(const Register &src, const Operand &dest) {
        movq(src, dest);
    }
    void mov(const Register &src, const Register &dest) {
        movq(src, dest);
    }
    void mov(AbsoluteLabel *label, const Register &dest) {
        JS_ASSERT(!label->bound());
        
        
        masm.movq_i64r(label->prev(), dest.code());
        label->setPrev(masm.size());
    }
    void lea(const Operand &src, const Register &dest) {
        switch (src.kind()) {
          case Operand::REG_DISP:
            masm.leaq_mr(src.disp(), src.base(), dest.code());
            break;
          case Operand::SCALE:
            masm.leaq_mr(src.disp(), src.base(), src.index(), src.scale(), dest.code());
            break;
          default:
            JS_NOT_REACHED("unexepcted operand kind");
        }
    }

    
    
    
    void cmpq(const Operand &lhs, const Register &rhs) {
        switch (lhs.kind()) {
          case Operand::REG:
            masm.cmpq_rr(rhs.code(), lhs.reg());
            break;
          case Operand::REG_DISP:
            masm.cmpq_rm(rhs.code(), lhs.disp(), lhs.base());
            break;
          default:
            JS_NOT_REACHED("unexpected operand kind");
        }
    }
    void cmpq(const Register &lhs, const Operand &rhs) {
        switch (rhs.kind()) {
          case Operand::REG:
            masm.cmpq_rr(rhs.reg(), lhs.code());
            break;
          case Operand::REG_DISP:
            masm.cmpq_mr(rhs.disp(), rhs.base(), lhs.code());
            break;
          default:
            JS_NOT_REACHED("unexpected operand kind");
        }
    }
    void cmpq(const Register &lhs, const Register &rhs) {
        masm.cmpq_rr(rhs.code(), lhs.code());
    }
    void cmpq(Imm32 lhs, const Register &rhs) {
        masm.cmpq_ir(lhs.value, rhs.code());
    }
    
    void testq(Imm32 lhs, const Register &rhs) {
        masm.testq_i32r(lhs.value, rhs.code());
    }
    void testq(const Register &lhs, const Register &rhs) {
        masm.testq_rr(lhs.code(), rhs.code());
    }

    void jmp(void *target, Relocation::Kind reloc) {
        JmpSrc src = masm.jmp();
        addPendingJump(src, target, reloc);
    }
    void j(Condition cond, void *target, Relocation::Kind reloc) {
        JmpSrc src = masm.jCC(static_cast<JSC::X86Assembler::Condition>(cond));
        addPendingJump(src, target, reloc);
    }

    void cvttsd2sq(const FloatRegister &src, const Register &dest) {
        masm.cvttsd2sq_rr(src.code(), dest.code());
    }
    void cvttsd2s(const FloatRegister &src, const Register &dest) {
        cvttsd2sq(src, dest);
    }

};

#ifdef _WIN64
static const uint32 NumArgRegs = 4;
#else
static const uint32 NumArgRegs = 6;
#endif

static inline bool
GetArgReg(uint32 arg, Register *out)
{
#ifdef _WIN64
    static const Register regs[] = { rcx, rdx, r8, r9 };
#else
    static const Register regs[] = { rdi, rsi, rdx, rcx, r8, r9 };
#endif
    JS_STATIC_ASSERT(NumArgRegs == JS_ARRAY_LENGTH(regs));
    if (arg >= NumArgRegs)
        return false;
    *out = regs[arg];
    return true;
}

} 
} 

#endif

