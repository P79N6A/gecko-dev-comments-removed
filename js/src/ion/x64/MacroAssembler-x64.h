








































#ifndef jsion_macro_assembler_x64_h__
#define jsion_macro_assembler_x64_h__

#include "ion/shared/MacroAssembler-x86-shared.h"
#include "jsnum.h"

namespace js {
namespace ion {

struct ImmShiftedTag : public ImmWord
{
    ImmShiftedTag(JSValueShiftedTag shtag)
      : ImmWord((uintptr_t)shtag)
    { }

    ImmShiftedTag(JSValueType type)
      : ImmWord(uintptr_t(JSValueShiftedTag(JSVAL_TYPE_TO_SHIFTED_TAG(type))))
    { }
};

struct ImmTag : public Imm32
{
    ImmTag(JSValueTag tag)
      : Imm32(tag)
    { }
};

class MacroAssemblerX64 : public MacroAssemblerX86Shared
{
    static const uint32 StackAlignment = 16;

  protected:
    uint32 alignStackForCall(uint32 stackForArgs) {
        uint32 total = stackForArgs + ShadowStackSpace;
        uint32 displacement = total + framePushed_;
        return total + ComputeByteAlignment(displacement, StackAlignment);
    }

    uint32 dynamicallyAlignStackForCall(uint32 stackForArgs, const Register &scratch) {
        
        
        
        movq(rsp, scratch);
        andq(Imm32(~(StackAlignment - 1)), rsp);
        push(scratch);
        uint32 total = stackForArgs + ShadowStackSpace;
        uint32 displacement = total + STACK_SLOT_SIZE;
        return total + ComputeByteAlignment(displacement, StackAlignment);
    }

    void restoreStackFromDynamicAlignment() {
        pop(rsp);
    }

  public:
    void reserveStack(uint32 amount) {
        if (amount)
            subq(Imm32(amount), rsp);
        framePushed_ += amount;
    }
    void freeStack(uint32 amount) {
        JS_ASSERT(amount <= framePushed_);
        if (amount)
            addq(Imm32(amount), rsp);
        framePushed_ -= amount;
    }
    void movePtr(ImmWord imm, const Register &dest) {
        movq(imm, dest);
    }
    void setStackArg(const Register &reg, uint32 arg) {
        movq(reg, Operand(rsp, (arg - NumArgRegs) * STACK_SLOT_SIZE + ShadowStackSpace));
    }
    void checkCallAlignment() {
#ifdef DEBUG
        Label good;
        movl(rsp, rax);
        testq(Imm32(StackAlignment - 1), rax);
        j(Equal, &good);
        breakpoint();
        bind(&good);
#endif
    }

    void cmpTag(const ValueOperand &operand, ImmTag tag) {
        movq(operand.value(), ScratchReg);
        shrq(Imm32(JSVAL_TAG_SHIFT), ScratchReg);
        cmpl(tag, ScratchReg);
    }

    
    Condition testInt32(Condition cond, const ValueOperand &src) {
        JS_ASSERT(cond == Equal || cond == NotEqual);
        cmpTag(src, ImmTag(JSVAL_TAG_INT32));
        return cond;
    }
    Condition testBoolean(Condition cond, const ValueOperand &src) {
        JS_ASSERT(cond == Equal || cond == NotEqual);
        cmpTag(src, ImmTag(JSVAL_TAG_BOOLEAN));
        return cond;
    }
    Condition testDouble(Condition cond, const ValueOperand &src) {
        JS_ASSERT(cond == Equal || cond == NotEqual);
        movq(ImmShiftedTag(JSVAL_SHIFTED_TAG_MAX_DOUBLE), ScratchReg);
        cmpq(src.value(), ScratchReg);
        return (cond == NotEqual) ? Above : BelowOrEqual;
    }
    Condition testNull(Condition cond, const ValueOperand &src) {
        JS_ASSERT(cond == Equal || cond == NotEqual);
        cmpTag(src, ImmTag(JSVAL_TAG_NULL));
        return cond;
    }
    Condition testUndefined(Condition cond, const ValueOperand &src) {
        JS_ASSERT(cond == Equal || cond == NotEqual);
        cmpTag(src, ImmTag(JSVAL_TAG_UNDEFINED));
        return cond;
    }

    void unboxInt32(const ValueOperand &src, const Register &dest) {
        movl(src.value(), dest);
    }
    void unboxBoolean(const ValueOperand &src, const Register &dest) {
        movl(src.value(), dest);
    }
    void unboxDouble(const ValueOperand &src, const FloatRegister &dest) {
        movqsd(src.valueReg(), dest);
    }

    
    void boolValueToDouble(const ValueOperand &operand, const FloatRegister &dest) {
        cvtsi2sd(operand.value(), dest);
    }
    void int32ValueToDouble(const ValueOperand &operand, const FloatRegister &dest) {
        cvtsi2sd(operand.value(), dest);
    }

    void loadDouble(double d, const FloatRegister &dest) {
        jsdpun dpun;
        dpun.d = d;
        if (dpun.u64 == 0) {
            xorpd(dest, dest);
        } else {
            movq(ImmWord(dpun.u64), ScratchReg);
            movqsd(ScratchReg, dest);
        }
    }
    void loadStaticDouble(const double *dp, const FloatRegister &dest) {
        loadDouble(*dp, dest);
    }
};

typedef MacroAssemblerX64 MacroAssemblerSpecific;

} 
} 

#endif 

