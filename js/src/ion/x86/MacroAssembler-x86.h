








































#ifndef jsion_macro_assembler_x86_h__
#define jsion_macro_assembler_x86_h__

#include "ion/shared/MacroAssembler-x86-shared.h"

namespace js {
namespace ion {

class MacroAssemblerX86 : public MacroAssemblerX86Shared
{
    static const uint32 StackAlignment = 16;

  private:
    Operand payloadOf(const Address &address) {
        return Operand(address.base, address.offset);
    }
    Operand tagOf(const Address &address) {
        return Operand(address.base, address.offset + 4);
    }

  protected:
    uint32 alignStackForCall(uint32 stackForArgs) {
        
        uint32 displacement = stackForArgs + framePushed_;
        return stackForArgs + ComputeByteAlignment(displacement, StackAlignment);
    }

    uint32 dynamicallyAlignStackForCall(uint32 stackForArgs, const Register &scratch) {
        
        
        
        movl(esp, scratch);
        andl(Imm32(~(StackAlignment - 1)), esp);
        push(scratch);
        uint32 displacement = stackForArgs + STACK_SLOT_SIZE;
        return stackForArgs + ComputeByteAlignment(displacement, StackAlignment);
    }

    void restoreStackFromDynamicAlignment() {
        pop(esp);
    }

  public:
    
    
    
    Operand ToPayload(Operand base) {
        return base;
    }
    Operand ToType(Operand base) {
        return Operand(Register::FromCode(base.base()),
                       base.disp() + sizeof(void *));
    }
    void moveValue(const Value &val, Register type, Register data) {
        jsval_layout jv = JSVAL_TO_IMPL(val);
        movl(Imm32(jv.s.tag), type);
        movl(Imm32(jv.s.payload.i32), data);
    }

    
    
    
    void storeValue(ValueOperand val, Operand dest) {
        movl(val.payloadReg(), ToPayload(dest));
        movl(val.typeReg(), ToType(dest));
    }
    void loadValue(Operand src, ValueOperand val) {
        Operand payload = ToPayload(src);
        Operand type = ToType(src);

        
        
        if (Register::FromCode(type.base()) != val.payloadReg()) {
            movl(payload, val.payloadReg());
            movl(type, val.typeReg());
        } else {
            movl(type, val.typeReg());
            movl(payload, val.payloadReg());
        }
    }
    void pushValue(ValueOperand val) {
        push(val.typeReg());
        push(val.payloadReg());
    }
    void popValue(ValueOperand val) {
        pop(val.payloadReg());
        pop(val.typeReg());
    }

    void movePtr(Operand op, const Register &dest) {
        movl(op, dest);
    }

    
    Register splitTagForTest(const ValueOperand &value) {
        return value.typeReg();
    }

    Condition testUndefined(Condition cond, const Register &tag) {
        JS_ASSERT(cond == Equal || cond == NotEqual);
        cmpl(tag, ImmTag(JSVAL_TAG_UNDEFINED));
        return cond;
    }
    Condition testBoolean(Condition cond, const Register &tag) {
        JS_ASSERT(cond == Equal || cond == NotEqual);
        cmpl(tag, ImmTag(JSVAL_TAG_BOOLEAN));
        return cond;
    }
    Condition testInt32(Condition cond, const Register &tag) {
        JS_ASSERT(cond == Equal || cond == NotEqual);
        cmpl(tag, ImmTag(JSVAL_TAG_INT32));
        return cond;
    }
    Condition testDouble(Condition cond, const Register &tag) {
        JS_ASSERT(cond == Assembler::Equal || cond == Assembler::NotEqual);
        Condition actual = (cond == Equal) ? Below : AboveOrEqual;
        cmpl(tag, ImmTag(JSVAL_TAG_CLEAR));
        return actual;
    }
    Condition testNull(Condition cond, const Register &tag) {
        JS_ASSERT(cond == Equal || cond == NotEqual);
        cmpl(tag, ImmTag(JSVAL_TAG_NULL));
        return cond;
    }
    Condition testString(Condition cond, const Register &tag) {
        JS_ASSERT(cond == Equal || cond == NotEqual);
        cmpl(tag, ImmTag(JSVAL_TAG_STRING));
        return cond;
    }
    Condition testObject(Condition cond, const Register &tag) {
        JS_ASSERT(cond == Equal || cond == NotEqual);
        cmpl(tag, ImmTag(JSVAL_TAG_OBJECT));
        return cond;
    }
    Condition testNumber(Condition cond, const Register &tag) {
        JS_ASSERT(cond == Equal || cond == NotEqual);
        cmpl(tag, ImmTag(JSVAL_UPPER_INCL_TAG_OF_NUMBER_SET));
        return cond == Equal ? BelowOrEqual : Above;
    }
    Condition testUndefined(Condition cond, const ValueOperand &value) {
        return testUndefined(cond, value.typeReg());
    }
    Condition testBoolean(Condition cond, const ValueOperand &value) {
        return testBoolean(cond, value.typeReg());
    }
    Condition testInt32(Condition cond, const ValueOperand &value) {
        return testInt32(cond, value.typeReg());
    }
    Condition testDouble(Condition cond, const ValueOperand &value) {
        return testDouble(cond, value.typeReg());
    }
    Condition testNull(Condition cond, const ValueOperand &value) {
        return testNull(cond, value.typeReg());
    }
    Condition testString(Condition cond, const ValueOperand &value) {
        return testString(cond, value.typeReg());
    }
    Condition testObject(Condition cond, const ValueOperand &value) {
        return testObject(cond, value.typeReg());
    }
    Condition testNumber(Condition cond, const ValueOperand &value) {
        return testNumber(cond, value.typeReg());
    }

    void cmpPtr(const Register &lhs, const ImmWord rhs) {
        cmpl(lhs, Imm32(rhs.value));
    }
    void testPtr(const Register &lhs, const Register &rhs) {
        return testl(lhs, rhs);
    }

    
    
    
    void reserveStack(uint32 amount) {
        if (amount)
            subl(Imm32(amount), StackPointer);
        framePushed_ += amount;
    }
    void freeStack(uint32 amount) {
        JS_ASSERT(amount <= framePushed_);
        if (amount)
            addl(Imm32(amount), StackPointer);
        framePushed_ -= amount;
    }

    void addPtr(Imm32 imm, const Register &dest) {
        addl(imm, dest);
    }
    void subPtr(Imm32 imm, const Register &dest) {
        subl(imm, dest);
    }

    void branchPtr(Condition cond, Register lhs, ImmGCPtr ptr, Label *label) {
        cmpl(lhs, ptr);
        j(cond, label);
    }

    void movePtr(ImmWord imm, Register dest) {
        movl(Imm32(imm.value), dest);
    }
    void movePtr(ImmGCPtr imm, Register dest) {
        movl(imm, dest);
    }
    void loadPtr(const Address &address, Register dest) {
        movl(Operand(address), dest);
    }
    void setStackArg(const Register &reg, uint32 arg) {
        movl(reg, Operand(esp, arg * STACK_SLOT_SIZE));
    }
    void checkCallAlignment() {
#ifdef DEBUG
        Label good;
        movl(esp, eax);
        testl(eax, Imm32(StackAlignment - 1));
        j(Equal, &good);
        breakpoint();
        bind(&good);
#endif
    }

    
    
    template <typename T>
    void branchTestUndefined(Condition cond, const T &t, Label *label) {
        cond = testUndefined(cond, t);
        j(cond, label);
    }
    template <typename T>
    void branchTestInt32(Condition cond, const T &t, Label *label) {
        cond = testInt32(cond, t);
        j(cond, label);
    }
    template <typename T>
    void branchTestBoolean(Condition cond, const T &t, Label *label) {
        cond = testBoolean(cond, t);
        j(cond, label);
    }
    template <typename T>
    void branchTestDouble(Condition cond, const T &t, Label *label) {
        cond = testDouble(cond, t);
        j(cond, label);
    }
    template <typename T>
    void branchTestNull(Condition cond, const T &t, Label *label) {
        cond = testNull(cond, t);
        j(cond, label);
    }
    template <typename T>
    void branchTestString(Condition cond, const T &t, Label *label) {
        cond = testString(cond, t);
        j(cond, label);
    }
    template <typename T>
    void branchTestObject(Condition cond, const T &t, Label *label) {
        cond = testObject(cond, t);
        j(cond, label);
    }
    template <typename T>
    void branchTestNumber(Condition cond, const T &t, Label *label) {
        cond = testNumber(cond, t);
        j(cond, label);
    }

    void unboxInt32(const ValueOperand &operand, const Register &dest) {
        movl(operand.payloadReg(), dest);
    }
    void unboxBoolean(const ValueOperand &operand, const Register &dest) {
        movl(operand.payloadReg(), dest);
    }
    void unboxDouble(const ValueOperand &operand, const FloatRegister &dest) {
        JS_ASSERT(dest != ScratchFloatReg);
        if (Assembler::HasSSE41()) {
            movd(operand.payloadReg(), dest);
            pinsrd(operand.typeReg(), dest);
        } else {
            movd(operand.payloadReg(), dest);
            movd(operand.typeReg(), ScratchFloatReg);
            unpcklps(ScratchFloatReg, dest);
        }
    }

    
    
    
    Register extractObject(const Address &address, Register scratch) {
        movl(payloadOf(address), scratch);
        return scratch;
    }
    Register extractObject(const ValueOperand &value, Register scratch) {
        return value.payloadReg();
    }
    Register extractTag(const Address &address, Register scratch) {
        movl(tagOf(address), scratch);
        return scratch;
    }
    Register extractTag(const ValueOperand &value, Register scratch) {
        return value.typeReg();
    }

    void boolValueToDouble(const ValueOperand &operand, const FloatRegister &dest) {
        cvtsi2sd(operand.payloadReg(), dest);
    }
    void int32ValueToDouble(const ValueOperand &operand, const FloatRegister &dest) {
        cvtsi2sd(operand.payloadReg(), dest);
    }

    void loadStaticDouble(const double *dp, const FloatRegister &dest) {
        movsd(dp, dest);
    }

    Condition testInt32Truthy(bool truthy, const ValueOperand &operand) {
        testl(operand.payloadReg(), operand.payloadReg());
        return truthy ? NonZero : Zero;
    }
    void branchTestBooleanTruthy(bool truthy, const ValueOperand &operand, Label *label) {
        testl(operand.payloadReg(), operand.payloadReg());
        j(truthy ? NonZero : Zero, label);
    }
};

typedef MacroAssemblerX86 MacroAssemblerSpecific;

} 
} 

#endif 

