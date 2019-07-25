








































#ifndef jsion_cpu_x64_assembler_h__
#define jsion_cpu_x64_assembler_h__

#include "ion/shared/Assembler-shared.h"

namespace js {
namespace ion {

static const Register rcx = { JSC::X86Registers::ecx };
static const Register rbp = { JSC::X86Registers::ebp };
static const Register rsp = { JSC::X86Registers::esp };

static const Register StackPointer = rsp;
static const Register JSReturnReg = rcx;

class Operand
{
  public:
    enum Kind {
        REG,
        REG_DISP,
        FPREG
    };

    Kind kind_ : 2;
    int32 base_ : 5;
    int32 disp_;

  public:
    explicit Operand(const Register &reg)
      : kind_(REG),
        base_(reg.code())
    { }
    explicit Operand(const FloatRegister &reg)
      : kind_(FPREG),
        base_(reg.code())
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
    Register::Code base() const {
        JS_ASSERT(kind() == REG_DISP);
        return (Registers::Code)base_;
    }
    FloatRegisters::Code fpu() const {
        JS_ASSERT(kind() == FPREG);
        return (FloatRegisters::Code)base_;
    }
    int32 disp() const {
        JS_ASSERT(kind() == REG_DISP);
        return disp_;
    }
};

} 
} 

#include "ion/shared/Assembler-x86-shared.h"

namespace js {
namespace ion {

class Assembler : public AssemblerX86Shared
{
  public:
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
      void subq(Imm32 imm, const Register &dest) {
          masm.subq_ir(imm.value, dest.code());
      }
      void shlq(Imm32 imm, const Register &dest) {
          masm.shlq_i8r(imm.value, dest.code());
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
};

} 
} 

#endif

