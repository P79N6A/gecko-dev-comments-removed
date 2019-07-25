








































#ifndef jsion_macro_assembler_x86_shared_h__
#define jsion_macro_assembler_x86_shared_h__

#ifdef JS_CPU_X86
# include "ion/x86/Assembler-x86.h"
#elif JS_CPU_X64
# include "ion/x64/Assembler-x64.h"
#endif
#include "ion/IonFrames.h"
#include "jsopcode.h"

#include "ion/IonCaches.h"

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

    void move32(const Address &address, const Register &dest) {
        movl(Operand(address), dest);
    }
    void move32(const Imm32 &imm, const Register &dest) {
        movl(imm, dest);
    }
    void and32(const Imm32 &imm, const Register &dest) {
        andl(imm, dest);
    }
    void neg32(const Register &reg) {
        negl(reg);
    }
    void cmp32(const Register &src, const Imm32 &imm) {
        cmpl(src, imm);
    }
    void test32(const Register &lhs, const Register &rhs) {
        testl(lhs, rhs);
    }
    void cmp32(Register a, Register b) {
        cmpl(a, b);
    }
    void add32(Imm32 imm, Register dest) {
        addl(imm, dest);
    }
    void sub32(Imm32 imm, Register dest) {
        subl(imm, dest);
    }

    void branch32(Condition cond, const Address &lhs, const Register &rhs, Label *label) {
        cmpl(Operand(lhs), rhs);
        j(cond, label);
    }
    void branch32(Condition cond, const Address &lhs, Imm32 imm, Label *label) {
        cmpl(Operand(lhs), imm);
        j(cond, label);
    }
    void branch32(Condition cond, const Register &lhs, Imm32 imm, Label *label) {
        cmpl(lhs, imm);
        j(cond, label);
    }
    void branch32(Condition cond, const Register &lhs, const Register &rhs, Label *label) {
        cmpl(lhs, rhs);
        j(cond, label);
    }
    void branchTest32(Condition cond, const Register &lhs, const Register &rhs, Label *label) {
        testl(lhs, rhs);
        j(cond, label);
    }
    void branchTest32(Condition cond, const Address &address, Imm32 imm, Label *label) {
        testl(Operand(address), imm);
        j(cond, label);
    }

    
    template <typename T>
    void Push(const T &t) {
        push(t);
        framePushed_ += STACK_SLOT_SIZE;
    }
    void Push(const FloatRegister &t) {
        push(t);
        framePushed_ += sizeof(double);
    }
    void Pop(const Register &reg) {
        pop(reg);
        framePushed_ -= STACK_SLOT_SIZE;
    }
    void implicitPop(uint32 args) {
        JS_ASSERT(args % STACK_SLOT_SIZE == 0);
        framePushed_ -= args;
    }
    uint32 framePushed() const {
        return framePushed_;
    }
    void setFramePushed(uint32 framePushed) {
        framePushed_ = framePushed;
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
    void store32(Imm32 src, const Address &dest) {
        movl(src, Operand(dest));
    }
    void store32(const Register &src, const Address &dest) {
        movl(src, Operand(dest));
    }

    template <typename T>
    void computeEffectiveAddress(const T &address, Register dest) {
        lea(Operand(address), dest);
    }

    
    
    uint32 buildFakeExitFrame(const Register &scratch) {
        DebugOnly<uint32> initialDepth = framePushed();
        Label pseudocall, endcall;

        call(&pseudocall);
        uint32 callOffset = currentOffset();
        jump(&endcall);

        align(0x10);
        {
            bind(&pseudocall);
#ifdef JS_CPU_X86
            movl(Operand(StackPointer, 0x0), scratch);
#else
            movq(Operand(StackPointer, 0x0), scratch);
#endif
            ret();
        }

        bind(&endcall);

        uint32 descriptor = MakeFrameDescriptor(framePushed(), IonFrame_JS);
        Push(Imm32(descriptor));
        Push(scratch);

        JS_ASSERT(framePushed() == initialDepth + IonExitFrameLayout::Size());
        return callOffset;
    }

    void callWithExitFrame(IonCode *target) {
        uint32 descriptor = MakeFrameDescriptor(framePushed(), IonFrame_JS);
        Push(Imm32(descriptor));
        call(target);
    }
    void callIon(const Register &callee) {
        call(callee);
    }

    void checkStackAlignment() {
        
    }

    CodeOffsetLabel labelForPatch() {
        return CodeOffsetLabel(size());
    }
};

} 
} 

#endif 

