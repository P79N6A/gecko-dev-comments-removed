








































#ifndef jsion_macro_assembler_x86_shared_h__
#define jsion_macro_assembler_x86_shared_h__

#ifdef JS_CPU_X86
# include "ion/x86/Assembler-x86.h"
#elif JS_CPU_X64
# include "ion/x64/Assembler-x64.h"
#endif
#include "jsopcode.h"

namespace js {
namespace ion {

static Register CallReg = ReturnReg;

class MacroAssemblerX86Shared : public Assembler
{
  protected:
    
    
    
    
    
    uint32 framePushed_;

  public:
    MacroAssemblerX86Shared()
      : framePushed_(0)
    { }

    
    Condition compareDoubles(JSOp compare, const FloatRegister &lhs, const FloatRegister &rhs) {
        
        
        
        
        switch (compare) {
          case JSOP_EQ:
          case JSOP_STRICTEQ:
            ucomisd(lhs, rhs);
            return Equal;

          case JSOP_NE:
          case JSOP_STRICTNE:
            ucomisd(lhs, rhs);
            return NotEqual;

          case JSOP_LT:
          case JSOP_LE:
            ucomisd(rhs, lhs);
            return (compare == JSOP_LT) ? Above : AboveOrEqual;

          case JSOP_GT:
          case JSOP_GE:
            ucomisd(lhs, rhs);
            return (compare == JSOP_GT) ? Above : AboveOrEqual;

          default:
            JS_NOT_REACHED("unexpected opcode kind");
            return Parity;
        }
    }

    void branchTest32(Condition cond, const Address &address, Imm32 imm, Label *label) {
        testl(Operand(address.base, address.offset), imm);
        j(cond, label);
    }

    
    void Push(const Register &reg) {
        push(reg);
        framePushed_ += STACK_SLOT_SIZE;
    }
    uint32 framePushed() const {
        return framePushed_;
    }

    void jump(Label *label) {
        jmp(label);
    }
    void convertInt32ToDouble(const Register &src, const FloatRegister &dest) {
        cvtsi2sd(Operand(src), dest);
    }
    Condition testDoubleTruthy(bool truthy, const FloatRegister &reg) {
        xorpd(ScratchFloatReg, ScratchFloatReg);
        ucomisd(ScratchFloatReg, reg);
        return truthy ? NonZero : Zero;
    }
    void load32(const Address &address, Register dest) {
        movl(Operand(address), dest);
    }
};

} 
} 

#endif 

