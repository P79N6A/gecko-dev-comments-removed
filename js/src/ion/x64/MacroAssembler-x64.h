








































#ifndef jsion_macro_assembler_x64_h__
#define jsion_macro_assembler_x64_h__

#include "ion/shared/MacroAssembler-x86-shared.h"
#include "ion/MoveResolver.h"
#include "ion/IonFrames.h"
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
    
    
    uint32 stackAdjust_;
    bool dynamicAlignment_;
    bool inCall_;
    bool enoughMemory_;

    
    
    
    
    
    uint32 setupABICall(uint32 arg);

  protected:
    MoveResolver moveResolver_;

  public:
    using MacroAssemblerX86Shared::call;

    typedef MoveResolver::MoveOperand MoveOperand;
    typedef MoveResolver::Move Move;

    MacroAssemblerX64()
      : stackAdjust_(0),
        inCall_(false),
        enoughMemory_(true)
    {
    }

    bool oom() const {
        return MacroAssemblerX86Shared::oom() || !enoughMemory_;
    }

    
    
    
    void call(ImmWord target) {
        movq(target, rax);
        call(rax);
    }

    
    
    
    void storeValue(ValueOperand val, Operand dest) {
        movq(val.valueReg(), dest);
    }
    void loadValue(Operand src, ValueOperand val) {
        movq(src, val.valueReg());
    }
    void pushValue(ValueOperand val) {
        push(val.valueReg());
    }
    void popValue(ValueOperand val) {
        pop(val.valueReg());
    }

    void movePtr(Operand op, const Register &dest) {
        movq(op, dest);
    }
    void moveValue(const Value &val, const Register &dest) {
        movq(ImmWord((void *)val.asRawBits()), dest);
        if (val.isMarkable())
            writeDataRelocation(masm.currentOffset());
    }
    void moveValue(const Value &src, const ValueOperand &dest) {
        moveValue(src, dest.valueReg());
    }
    void boxValue(JSValueShiftedTag tag, const Operand &src, const Register &dest) {
        movq(ImmShiftedTag(tag), dest);
        orq(src, dest);
    }

    Condition testUndefined(Condition cond, Register tag) {
        JS_ASSERT(cond == Equal || cond == NotEqual);
        cmpl(tag, ImmTag(JSVAL_TAG_UNDEFINED));
        return cond;
    }
    Condition testInt32(Condition cond, Register tag) {
        JS_ASSERT(cond == Equal || cond == NotEqual);
        cmpl(tag, ImmTag(JSVAL_TAG_INT32));
        return cond;
    }
    Condition testBoolean(Condition cond, Register tag) {
        JS_ASSERT(cond == Equal || cond == NotEqual);
        cmpl(tag, ImmTag(JSVAL_TAG_BOOLEAN));
        return cond;
    }
    Condition testNull(Condition cond, Register tag) {
        JS_ASSERT(cond == Equal || cond == NotEqual);
        cmpl(tag, ImmTag(JSVAL_TAG_NULL));
        return cond;
    }
    Condition testString(Condition cond, Register tag) {
        JS_ASSERT(cond == Equal || cond == NotEqual);
        cmpl(tag, ImmTag(JSVAL_TAG_STRING));
        return cond;
    }
    Condition testObject(Condition cond, Register tag) {
        JS_ASSERT(cond == Equal || cond == NotEqual);
        cmpl(tag, ImmTag(JSVAL_TAG_OBJECT));
        return cond;
    }
    Condition testNumber(Condition cond, Register tag) {
        JS_ASSERT(cond == Equal || cond == NotEqual);
        cmpl(tag, Imm32(JSVAL_UPPER_INCL_TAG_OF_NUMBER_SET));
        return cond == Equal ? BelowOrEqual : Above;
    }
    Condition testGCThing(Condition cond, Register tag) {
        JS_ASSERT(cond == Equal || cond == NotEqual);
        cmpl(tag, Imm32(JSVAL_LOWER_INCL_TAG_OF_GCTHING_SET));
        return cond == Equal ? AboveOrEqual : Below;
    }

    Condition testUndefined(Condition cond, const ValueOperand &src) {
        splitTag(src, ScratchReg);
        return testUndefined(cond, ScratchReg);
    }
    Condition testInt32(Condition cond, const ValueOperand &src) {
        splitTag(src, ScratchReg);
        return testInt32(cond, ScratchReg);
    }
    Condition testBoolean(Condition cond, const ValueOperand &src) {
        splitTag(src, ScratchReg);
        return testBoolean(cond, ScratchReg);
    }
    Condition testDouble(Condition cond, const ValueOperand &src) {
        JS_ASSERT(cond == Equal || cond == NotEqual);
        movq(ImmShiftedTag(JSVAL_SHIFTED_TAG_MAX_DOUBLE), ScratchReg);
        cmpq(src.value(), ScratchReg);
        return (cond == NotEqual) ? Above : BelowOrEqual;
    }
    Condition testNull(Condition cond, const ValueOperand &src) {
        splitTag(src, ScratchReg);
        return testNull(cond, ScratchReg);
    }
    Condition testString(Condition cond, const ValueOperand &src) {
        splitTag(src, ScratchReg);
        return testString(cond, ScratchReg);
    }
    Condition testObject(Condition cond, const ValueOperand &src) {
        splitTag(src, ScratchReg);
        return testObject(cond, ScratchReg);
    }
    Condition testGCThing(Condition cond, const ValueOperand &src) {
        splitTag(src, ScratchReg);
        return testGCThing(cond, ScratchReg);
    }

    void cmpPtr(const Register &lhs, const ImmWord rhs) {
        JS_ASSERT(lhs != ScratchReg);
        movq(rhs, ScratchReg);
        return cmpq(lhs, ScratchReg);
    }
    void cmpPtr(const Register &lhs, const ImmGCPtr rhs) {
        JS_ASSERT(lhs != ScratchReg);
        movq(rhs, ScratchReg);
        return cmpq(lhs, ScratchReg);
    }
    void testPtr(const Register &lhs, const Register &rhs) {
        testq(lhs, rhs);
    }

    
    
    
    void reserveStack(uint32 amount) {
        if (amount)
            subq(Imm32(amount), StackPointer);
        framePushed_ += amount;
    }
    void freeStack(uint32 amount) {
        JS_ASSERT(amount <= framePushed_);
        if (amount)
            addq(Imm32(amount), StackPointer);
        framePushed_ -= amount;
    }

    void addPtr(Imm32 imm, const Register &dest) {
        addq(imm, dest);
    }
    void subPtr(Imm32 imm, const Register &dest) {
        subq(imm, dest);
    }

    void branchPtr(Condition cond, Register lhs, ImmGCPtr ptr, Label *label) {
        cmpPtr(lhs, ptr);
        j(cond, label);
    }

    void movePtr(ImmWord imm, Register dest) {
        movq(imm, dest);
    }
    void movePtr(ImmGCPtr imm, Register dest) {
        movq(imm, dest);
    }
    void loadPtr(const Address &address, Register dest) {
        movq(Operand(address), dest);
    }
    void setStackArg(const Register &reg, uint32 arg) {
        uint32 disp = GetArgStackDisp(arg);
        movq(reg, Operand(rsp, disp));
    }

    void splitTag(Register src, Register dest) {
        if (src != dest)
            movq(src, dest);
        shrq(Imm32(JSVAL_TAG_SHIFT), dest);
    }

    void splitTag(const ValueOperand &operand, const Register &dest) {
        JS_ASSERT(operand.valueReg() != dest);
        splitTag(operand.valueReg(), dest);
    }

    
    Register splitTagForTest(const ValueOperand &value) {
        splitTag(value, ScratchReg);
        return ScratchReg;
    }
    void cmpTag(const ValueOperand &operand, ImmTag tag) {
        Register reg = splitTagForTest(operand);
        cmpl(Operand(reg), tag);
    }

    void branchTestUndefined(Condition cond, Register tag, Label *label) {
        cond = testUndefined(cond, tag);
        j(cond, label);
    }
    void branchTestInt32(Condition cond, Register tag, Label *label) {
        cond = testInt32(cond, tag);
        j(cond, label);
    }
    void branchTestBoolean(Condition cond, Register tag, Label *label) {
        cond = testBoolean(cond, tag);
        j(cond, label);
    }
    void branchTestNull(Condition cond, Register tag, Label *label) {
        cond = testNull(cond, tag);
        j(cond, label);
    }
    void branchTestString(Condition cond, Register tag, Label *label) {
        cond = testString(cond, tag);
        j(cond, label);
    }
    void branchTestObject(Condition cond, Register tag, Label *label) {
        cond = testObject(cond, tag);
        j(cond, label);
    }
    void branchTestNumber(Condition cond, Register tag, Label *label) {
        cond = testNumber(cond, tag);
        j(cond, label);
    }
    Condition testError(Condition cond, const Register &tag) {
        JS_ASSERT(cond == Equal || cond == NotEqual);
        cmpl(tag, ImmTag(JSVAL_TAG_MAGIC));
        return cond;
    }

    
    
    void branchTestUndefined(Condition cond, const ValueOperand &src, Label *label) {
        cond = testUndefined(cond, src);
        j(cond, label);
    }
    void branchTestInt32(Condition cond, const ValueOperand &src, Label *label) {
        splitTag(src, ScratchReg);
        branchTestInt32(cond, ScratchReg, label);
    }
    void branchTestBoolean(Condition cond, const ValueOperand &src, Label *label) {
        splitTag(src, ScratchReg);
        branchTestBoolean(cond, ScratchReg, label);
    }
    void branchTestDouble(Condition cond, const ValueOperand &src, Label *label) {
        cond = testDouble(cond, src);
        j(cond, label);
    }
    void branchTestNull(Condition cond, const ValueOperand &src, Label *label) {
        cond = testNull(cond, src);
        j(cond, label);
    }
    void branchTestString(Condition cond, const ValueOperand &src, Label *label) {
        cond = testString(cond, src);
        j(cond, label);
    }
    void branchTestObject(Condition cond, const ValueOperand &src, Label *label) {
        cond = testObject(cond, src);
        j(cond, label);
    }
    void branchTestGCThing(Condition cond, const ValueOperand &src, Label *label) {
        cond = testGCThing(cond, src);
        j(cond, label);
    }
    Condition testError(Condition cond, const ValueOperand &src) {
        splitTag(src, ScratchReg);
        return testError(cond, ScratchReg);
    }

    
    
    void unboxInt32(const ValueOperand &src, const Register &dest) {
        movl(src.value(), dest);
    }
    void unboxInt32(const Operand &src, const Register &dest) {
        movl(src, dest);
    }
    void unboxBoolean(const ValueOperand &src, const Register &dest) {
        movl(src.value(), dest);
    }
    void unboxBoolean(const Operand &src, const Register &dest) {
        movl(src, dest);
    }
    void unboxDouble(const ValueOperand &src, const FloatRegister &dest) {
        movqsd(src.valueReg(), dest);
    }
    void unboxDouble(const Operand &src, const FloatRegister &dest) {
        lea(src, ScratchReg);
        movqsd(ScratchReg, dest);
    }
    void unboxString(const ValueOperand &src, const Register &dest) {
        JS_ASSERT(src.valueReg() != dest);
        movq(ImmWord(JSVAL_PAYLOAD_MASK), dest);
        andq(src.valueReg(), dest);
    }
    void unboxString(const Operand &src, const Register &dest) {
        
        JS_ASSERT(dest != ScratchReg);
        movq(ImmWord(JSVAL_PAYLOAD_MASK), ScratchReg);
        movq(src, dest);
        andq(ScratchReg, dest);
    }
    void unboxObject(const ValueOperand &src, const Register &dest) {
        JS_ASSERT(src.valueReg() != dest);
        movq(ImmWord(JSVAL_PAYLOAD_MASK), dest);
        andq(src.valueReg(), dest);
    }
    void unboxObject(const Operand &src, const Register &dest) {
        
        JS_ASSERT(dest != ScratchReg);
        movq(ImmWord(JSVAL_PAYLOAD_MASK), ScratchReg);
        movq(src, dest);
        andq(ScratchReg, dest);
    }

    
    
    
    Register extractObject(const Address &address, Register scratch) {
        JS_ASSERT(scratch != ScratchReg);
        loadPtr(address, ScratchReg);
        unboxObject(ValueOperand(ScratchReg), scratch);
        return scratch;
    }
    Register extractObject(const ValueOperand &value, Register scratch) {
        JS_ASSERT(scratch != ScratchReg);
        unboxObject(value, scratch);
        return scratch;
    }
    Register extractTag(const Address &address, Register scratch) {
        JS_ASSERT(scratch != ScratchReg);
        loadPtr(address, scratch);
        splitTag(scratch, scratch);
        return scratch;
    }
    Register extractTag(const ValueOperand &value, Register scratch) {
        JS_ASSERT(scratch != ScratchReg);
        splitTag(value, scratch);
        return scratch;
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

    Condition testInt32Truthy(bool truthy, const ValueOperand &operand) {
        testl(operand.valueReg(), operand.valueReg());
        return truthy ? NonZero : Zero;
    }
    void branchTestBooleanTruthy(bool truthy, const ValueOperand &operand, Label *label) {
        testl(operand.valueReg(), operand.valueReg());
        j(truthy ? NonZero : Zero, label);
    }

    
    
    
    
    
    
    
    void setupAlignedABICall(uint32 args);

    
    
    void setupUnalignedABICall(uint32 args, const Register &scratch);

    
    
    
    
    
    
    void setABIArg(uint32 arg, const MoveOperand &from);
    void setABIArg(uint32 arg, const Register &reg);

    
    void callWithABI(void *fun);

    void handleException();

    void makeFrameDescriptor(Register frameSizeReg, FrameType type) {
        shlq(Imm32(FRAMETYPE_BITS), frameSizeReg);
        orq(Imm32(type), frameSizeReg);
    }

    
    
    void linkExitFrame() {
        mov(ImmWord(JS_THREAD_DATA(GetIonContext()->cx)), ScratchReg);
        mov(StackPointer, Operand(ScratchReg, offsetof(ThreadData, ionTop)));
    }
};

typedef MacroAssemblerX64 MacroAssemblerSpecific;

} 
} 

#endif 

