








































#ifndef jsion_macro_assembler_arm_h__
#define jsion_macro_assembler_arm_h__

#include "ion/arm/Assembler-arm.h"

namespace js {
namespace ion {

static Register CallReg = ip;


class MacroAssemblerARM : public Assembler
{
protected:
    
    
    
    
    
    uint32 framePushed_;

public:
    MacroAssemblerARM()
      : framePushed_(0)
    { }


    void convertInt32ToDouble(const Register &src, const FloatRegister &dest);


    uint32 framePushed() const {
        return framePushed_;
    }

    
    static const uint32 StackAlignment = 8;
    
    
    
    
  private:
    bool alu_dbl(Register src1, Imm32 imm, Register dest, ALUOp op,
                 SetCond_ sc, Condition c);
  public:
    void ma_alu(Register src1, Imm32 imm, Register dest,
                ALUOp op,
                SetCond_ sc =  NoSetCond, Condition c = Always);

    void ma_alu(Register src1, Operand op2, Register dest, ALUOp op,
                SetCond_ sc = NoSetCond, Condition c = Always);

    
    
    
    
    
    void ma_mov(Register src, Register dest,
                SetCond_ sc = NoSetCond, Condition c = Always);

    void ma_mov(Imm32 imm, Register dest,
                SetCond_ sc = NoSetCond, Condition c = Always);

    void ma_mov(const ImmGCPtr &ptr, Register dest);

    
    void ma_lsl(Imm32 shift, Register src, Register dst);
    void ma_lsr(Imm32 shift, Register src, Register dst);
    void ma_asr(Imm32 shift, Register src, Register dst);
    void ma_ror(Imm32 shift, Register src, Register dst);
    void ma_rol(Imm32 shift, Register src, Register dst);
    
    void ma_lsl(Register shift, Register src, Register dst);
    void ma_lsr(Register shift, Register src, Register dst);
    void ma_asr(Register shift, Register src, Register dst);
    void ma_ror(Register shift, Register src, Register dst);
    void ma_rol(Register shift, Register src, Register dst);

    
    void ma_mvn(Imm32 imm, Register dest,
                SetCond_ sc = NoSetCond, Condition c = Always);


    void ma_mvn(Register src1, Register dest,
                SetCond_ sc = NoSetCond, Condition c = Always);


    
    void ma_and(Register src, Register dest,
                SetCond_ sc = NoSetCond, Condition c = Always);

    void ma_and(Register src1, Register src2, Register dest,
                SetCond_ sc = NoSetCond, Condition c = Always);

    void ma_and(Imm32 imm, Register dest,
                SetCond_ sc = NoSetCond, Condition c = Always);

    void ma_and(Imm32 imm, Register src1, Register dest,
                SetCond_ sc = NoSetCond, Condition c = Always);



    
    void ma_bic(Imm32 imm, Register dest,
                SetCond_ sc = NoSetCond, Condition c = Always);


    
    void ma_eor(Register src, Register dest,
                SetCond_ sc = NoSetCond, Condition c = Always);

    void ma_eor(Register src1, Register src2, Register dest,
                SetCond_ sc = NoSetCond, Condition c = Always);

    void ma_eor(Imm32 imm, Register dest,
                SetCond_ sc = NoSetCond, Condition c = Always);

    void ma_eor(Imm32 imm, Register src1, Register dest,
                SetCond_ sc = NoSetCond, Condition c = Always);


    
    void ma_orr(Register src, Register dest,
                SetCond_ sc = NoSetCond, Condition c = Always);

    void ma_orr(Register src1, Register src2, Register dest,
                SetCond_ sc = NoSetCond, Condition c = Always);

    void ma_orr(Imm32 imm, Register dest,
                SetCond_ sc = NoSetCond, Condition c = Always);

    void ma_orr(Imm32 imm, Register src1, Register dest,
                SetCond_ sc = NoSetCond, Condition c = Always);


    
    
    void ma_adc(Imm32 imm, Register dest, SetCond_ sc = NoSetCond, Condition c = Always);
    void ma_adc(Register src, Register dest, SetCond_ sc = NoSetCond, Condition c = Always);
    void ma_adc(Register src1, Register src2, Register dest, SetCond_ sc = NoSetCond, Condition c = Always);

    
    void ma_add(Imm32 imm, Register dest, SetCond_ sc = NoSetCond, Condition c = Always);
    void ma_add(Register src1, Register dest, SetCond_ sc = NoSetCond, Condition c = Always);
    void ma_add(Register src1, Register src2, Register dest, SetCond_ sc = NoSetCond, Condition c = Always);
    void ma_add(Register src1, Operand op, Register dest, SetCond_ sc = NoSetCond, Condition c = Always);
    void ma_add(Register src1, Imm32 op, Register dest, SetCond_ sc = NoSetCond, Condition c = Always);

    
    void ma_sbc(Imm32 imm, Register dest, SetCond_ sc = NoSetCond, Condition c = Always);
    void ma_sbc(Register src1, Register dest, SetCond_ sc = NoSetCond, Condition c = Always);
    void ma_sbc(Register src1, Register src2, Register dest, SetCond_ sc = NoSetCond, Condition c = Always);

    
    void ma_sub(Imm32 imm, Register dest, SetCond_ sc = NoSetCond, Condition c = Always);
    void ma_sub(Register src1, Register dest, SetCond_ sc = NoSetCond, Condition c = Always);
    void ma_sub(Register src1, Register src2, Register dest, SetCond_ sc = NoSetCond, Condition c = Always);
    void ma_sub(Register src1, Operand op, Register dest, SetCond_ sc = NoSetCond, Condition c = Always);
    void ma_sub(Register src1, Imm32 op, Register dest, SetCond_ sc = NoSetCond, Condition c = Always);

    
    void ma_rsb(Imm32 imm, Register dest, SetCond_ sc = NoSetCond, Condition c = Always);
    void ma_rsb(Register src1, Register dest, SetCond_ sc = NoSetCond, Condition c = Always);
    void ma_rsb(Register src1, Register src2, Register dest, SetCond_ sc = NoSetCond, Condition c = Always);
    void ma_rsb(Register src1, Imm32 op2, Register dest, SetCond_ sc = NoSetCond, Condition c = Always);

    
    void ma_rsc(Imm32 imm, Register dest, SetCond_ sc = NoSetCond, Condition c = Always);
    void ma_rsc(Register src1, Register dest, SetCond_ sc = NoSetCond, Condition c = Always);
    void ma_rsc(Register src1, Register src2, Register dest, SetCond_ sc = NoSetCond, Condition c = Always);

    
    
    void ma_cmn(Imm32 imm, Register src1);
    void ma_cmn(Register src1, Register src2);
    void ma_cmn(Register src1, Operand op);

    
    void ma_cmp(Imm32 imm, Register src1);
    void ma_cmp(Register src1, Operand op);
    void ma_cmp(Register src1, Register src2);

    
    void ma_teq(Imm32 imm, Register src1);
    void ma_teq(Register src2, Register src1);
    void ma_teq(Register src1, Operand op);


    
    void ma_tst(Imm32 imm, Register src1);
    void ma_tst(Register src1, Register src2);
    void ma_tst(Register src1, Operand op);


    
    
    void ma_dtr(LoadStore ls, Register rn, Imm32 offset, Register rt,
                Index mode = Offset, Condition cc = Always);

    void ma_dtr(LoadStore ls, Register rn, Register rm, Register rt,
                Index mode = Offset, Condition cc = Always);


    void ma_str(Register rt, DTRAddr addr, Index mode = Offset, Condition cc = Always);

    void ma_ldr(DTRAddr addr, Register rt, Index mode = Offset, Condition cc = Always);
    void ma_ldrb(DTRAddr addr, Register rt, Index mode = Offset, Condition cc = Always);
    void ma_ldrh(EDtrAddr addr, Register rt, Index mode = Offset, Condition cc = Always);
    void ma_ldrsh(EDtrAddr addr, Register rt, Index mode = Offset, Condition cc = Always);
    void ma_ldrsb(EDtrAddr addr, Register rt, Index mode = Offset, Condition cc = Always);

    
    void ma_dataTransferN(LoadStore ls, int size,
                          Register rn, Register rm, Register rt,
                          Index mode = Offset, Condition cc = Always);

    void ma_dataTransferN(LoadStore ls, int size,
                          Register rn, Imm32 offset, Register rt,
                          Index mode = Offset, Condition cc = Always);
    void ma_pop(Register r);
    void ma_push(Register r);

    
    void ma_b(Label *dest, Condition c = Always);

    void ma_b(void *target, Relocation::Kind reloc);

    void ma_b(void *target, Condition c, Relocation::Kind reloc);

    
    
    void ma_bl(Label *dest, Condition c = Always);


    
    void ma_vadd(FloatRegister src1, FloatRegister src2, FloatRegister dst);

    void ma_vmul(FloatRegister src1, FloatRegister src2, FloatRegister dst);

    void ma_vmov(FloatRegister src, FloatRegister dest);

    void ma_vimm(double value, FloatRegister dest);

    void ma_vcmp(FloatRegister src1, FloatRegister src2);

    
    void ma_vcvt_F64_I32(FloatRegister src, FloatRegister dest);

    
    void ma_vcvt_I32_F64(FloatRegister src, FloatRegister dest);

    void ma_vxfer(FloatRegister src, Register dest);

    void ma_vldr(VFPAddr addr, FloatRegister dest);

    void ma_vstr(FloatRegister src, VFPAddr addr);

  protected:
    uint32 alignStackForCall(uint32 stackForArgs);

    uint32 dynamicallyAlignStackForCall(uint32 stackForArgs, const Register &scratch);

    void restoreStackFromDynamicAlignment();

  public:
    void reserveStack(uint32 amount);
    void freeStack(uint32 amount);

    void movePtr(ImmWord imm, Register dest);
    void movePtr(ImmGCPtr imm, Register dest);
    void loadPtr(const Address &address, Register dest);
    void setStackArg(const Register &reg, uint32 arg);
#ifdef DEBUG
    void checkCallAlignment();
#endif

    
    void ma_callIon(const Register reg);
    
    void ma_callIonNoPush(const Register reg);
    
    void ma_callIonHalfPush(const Register reg);
    void breakpoint();

};

class MacroAssemblerARMCompat : public MacroAssemblerARM
{
public:
    
    
    
    void j(Condition code , Label *dest)
    {
        as_b(dest, code);
    }
    void j(Label *dest)
    {
        as_b(dest, Always);
    }

    void mov(Imm32 imm, Register dest) {
        ma_mov(imm, dest);
    }
    void call(const Register reg) {
        as_blx(reg);
    }

    void call(Label *label) {
        JS_NOT_REACHED("Feature NYI");
        


    }
    void call(void *dest) {
        mov(Imm32((uint32)dest), r12);
        call(r12);
        


    }

    void ret() {
        ma_pop(pc);
        dumpPool();
    }
#if 0
    void Push(const Register &reg) {
        as_dtr(IsStore, STACK_SLOT_SIZE*8, PreIndex,
               reg,DTRAddr( sp, DtrOffImm(-STACK_SLOT_SIZE)));
        framePushed_ += STACK_SLOT_SIZE;
    }
#endif
    void push(Imm32 imm) {
        ma_mov(imm, ScratchRegister);
        ma_push(ScratchRegister);
    }

    void jump(Label *label) {
        as_b(label);
    }


    
    Register splitTagForTest(const ValueOperand &value) {
        return value.typeReg();
    }

    
    Condition testInt32(Condition cond, const ValueOperand &value);

    Condition testBoolean(Condition cond, const ValueOperand &value);
    Condition testDouble(Condition cond, const ValueOperand &value);
    Condition testNull(Condition cond, const ValueOperand &value);
    Condition testUndefined(Condition cond, const ValueOperand &value);
    Condition testString(Condition cond, const ValueOperand &value);
    Condition testObject(Condition cond, const ValueOperand &value);

    
    Condition testInt32(Condition cond, const Register &tag);
    Condition testBoolean(Condition cond, const Register &tag);
    Condition testNull(Condition cond, const Register &tag);
    Condition testUndefined(Condition cond, const Register &tag);
    Condition testString(Condition cond, const Register &tag);
    Condition testObject(Condition cond, const Register &tag);

    
    void unboxInt32(const ValueOperand &operand, const Register &dest);
    void unboxBoolean(const ValueOperand &operand, const Register &dest);
    void unboxDouble(const ValueOperand &operand, const FloatRegister &dest);

    
    
    
    Register extractObject(const Address &address, Register scratch) {
        JS_NOT_REACHED("NYI");
        return scratch;
    }
    Register extractObject(const ValueOperand &value, Register scratch) {
        return value.payloadReg();
    }
    Register extractTag(const Address &address, Register scratch) {
        JS_NOT_REACHED("NYI");
        return scratch;
    }
    Register extractTag(const ValueOperand &value, Register scratch) {
        return value.typeReg();
    }

    void boolValueToDouble(const ValueOperand &operand, const FloatRegister &dest);
    void int32ValueToDouble(const ValueOperand &operand, const FloatRegister &dest);

    void loadStaticDouble(const double *dp, const FloatRegister &dest);
    
    Condition testInt32Truthy(bool truthy, const ValueOperand &operand);
    Condition testBooleanTruthy(bool truthy, const ValueOperand &operand);
    Condition testDoubleTruthy(bool truthy, const FloatRegister &reg);

    template<typename T>
    void branchTestInt32(Condition cond, const T & t, Label *label) {
        Condition c = testInt32(cond, t);
        ma_b(label, c);
    }
    template<typename T>
    void branchTestBoolean(Condition cond, const T & t, Label *label) {
        Condition c = testBoolean(cond, t);
        ma_b(label, c);
    }
    template<typename T>
    void branchTestDouble(Condition cond, const T & t, Label *label) {
        Condition c = testDouble(cond, t);
        ma_b(label, c);
    }
    template<typename T>
    void branchTestNull(Condition cond, const T & t, Label *label) {
        Condition c = testNull(cond, t);
        ma_b(label, c);
    }
    template<typename T>
    void branchTestObject(Condition cond, const T & t, Label *label) {
        Condition c = testObject(cond, t);
        ma_b(label, c);
    }
    template<typename T>
    void branchTestString(Condition cond, const T & t, Label *label) {
        Condition c = testString(cond, t);
        ma_b(label, c);
    }
    template<typename T>
    void branchTestUndefined(Condition cond, const T & t, Label *label) {
        Condition c = testUndefined(cond, t);
        ma_b(label, c);
    }

    template <typename T>
    void branchTestNumber(Condition cond, const T &t, Label *label) {
        JS_NOT_REACHED("feature NYI");
    }

    template<typename T>
    void branchTestBooleanTruthy(bool b, const T & t, Label *label) {
        Condition c = testBooleanTruthy(b, t);
        ma_b(label, c);
    }
    void branchTest32(Condition cond, const Address &address, Imm32 imm, Label *label) {
        JS_NOT_REACHED("NYI");
    }
    void branchPtr(Condition cond, Register lhs, ImmGCPtr ptr, Label *label) {
        JS_NOT_REACHED("NYI");
    }

};

typedef MacroAssemblerARMCompat MacroAssemblerSpecific;

} 
} 

#endif 
