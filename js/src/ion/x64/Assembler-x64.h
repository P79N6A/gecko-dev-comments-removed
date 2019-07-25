








































#ifndef jsion_cpu_x64_assembler_h__
#define jsion_cpu_x64_assembler_h__

#include "ion/shared/Assembler-shared.h"

namespace js {
namespace ion {

static const Register rcx = { JSC::X86Registers::ecx };
static const Register rbp = { JSC::X86Registers::ebp };

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
    Register reg() const {
        JS_ASSERT(kind() == REG);
        return Register::FromCode(base_);
    }
    FloatRegisters::Code fpu() const {
        JS_ASSERT(kind() == FPREG);
        return (FloatRegisters::Code)base_;
    }
    int32 displacement() const {
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
};

} 
} 

#endif

