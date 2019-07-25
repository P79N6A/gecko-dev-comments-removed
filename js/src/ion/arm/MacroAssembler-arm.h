








































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


    void convertInt32ToDouble(const Register &src, const FloatRegister &dest) {
        
        as_vxfer(src, InvalidReg, VFPRegister(dest, VFPRegister::Single),
                 CoreToFloat);
        as_vcvt(VFPRegister(dest, VFPRegister::Double),
                VFPRegister(dest, VFPRegister::Single));
    }



    uint32 framePushed() const {
        return framePushed_;
    }

    
    static const uint32 StackAlignment = 8;
    
    
    
    

    bool alu_dbl(Register src1, Imm32 imm, Register dest, ALUOp op,
                 SetCond_ sc, Condition c)
    {
        if ((sc == SetCond && ! condsAreSafe(op)) || !can_dbl(op)) {
            return false;
        }
        ALUOp interop = getDestVariant(op);
        Imm8::TwoImm8mData both = Imm8::encodeTwoImms(imm.value);
        if (both.fst.invalid) {
            return false;
        }
        
        
        
        
        
        
        
        as_alu(ScratchRegister, src1, both.fst, interop, NoSetCond, c);
        as_alu(ScratchRegister, ScratchRegister, both.snd, op, sc, c);
        
        return true;
    }

    void ma_alu(Register src1, Imm32 imm, Register dest,
                ALUOp op,
                SetCond_ sc =  NoSetCond, Condition c = Always)
    {
        
        
        Imm8 imm8 = Imm8(imm.value);
        
        
        if (!imm8.invalid) {
            as_alu(dest, src1, imm8, op, sc, c);
            return;
        }
        
        Imm32 negImm = imm;
        ALUOp negOp = ALUNeg(op, &negImm);
        Imm8 negImm8 = Imm8(negImm.value);
        if (negOp != op_invalid && !negImm8.invalid) {
            as_alu(dest, src1, negImm8, negOp, sc, c);
            return;
        }
        if (hasMOVWT()) {
            
            
            
            
            if (sc == NoSetCond && (op == op_mov || op == op_mvn)) {
                
                
                
                
                
                if (op == op_mov && imm.value <= 0xffff) {
                    JS_ASSERT(src1 == InvalidReg);
                    as_movw(dest, (uint16)imm.value, c);
                    return;
                }
                
                
                if (op == op_mvn && ~imm.value <= 0xffff) {
                    JS_ASSERT(src1 == InvalidReg);
                    as_movw(dest, (uint16)-imm.value, c);
                    return;
                }
                
                
                
                
                
                
                
                
                
                if (op == op_mvn)
                    imm.value = ~imm.value;
                as_movw(dest, imm.value & 0xffff, c);
                as_movt(dest, (imm.value >> 16) & 0xffff, c);
                return;
            }
            
            
            
            
            
            
            
            
            
            
            
            
            
            
        }
        
        
        
        
        
        
        
        
        
        if (alu_dbl(src1, imm, dest, op, sc, c))
            return;
        
        if (negOp != op_invalid &&
            alu_dbl(src1, negImm, dest, negOp, sc, c))
            return;
        
        
        if (hasMOVWT()) {
            
            
            as_movw(ScratchRegister, imm.value & 0xffff, c);
            as_movt(ScratchRegister, (imm.value >> 16) & 0xffff, c);
        } else {
            JS_NOT_REACHED("non-ARMv7 loading of immediates NYI.");
        }
        as_alu(dest, src1, O2Reg(ScratchRegister), op, sc, c);
        
    }

    void ma_alu(Register src1, Operand op2, Register dest, ALUOp op,
                SetCond_ sc = NoSetCond, Condition c = Always)
    {
        JS_ASSERT(op2.getTag() == Operand::OP2);
        as_alu(dest, src1, op2.toOp2(), op, sc, c);
    }

    
    
    
    
    
    void ma_mov(Register src, Register dest,
                SetCond_ sc = NoSetCond, Condition c = Always)
    {
        as_mov(dest, O2Reg(src), sc, c);
    }
    void ma_mov(Imm32 imm, Register dest,
                SetCond_ sc = NoSetCond, Condition c = Always)
    {
        ma_alu(InvalidReg, imm, dest, op_mov, sc, c);
    }
    void ma_mov(const ImmGCPtr &ptr, Register dest) {
        ma_mov(Imm32(ptr.value), dest);
        JS_NOT_REACHED("todo:make gc more sane.");
    }

    
    void ma_lsl(Imm32 shift, Register src, Register dst) {
        as_mov(dst, lsl(src, shift.value));
    }
    void ma_lsr(Imm32 shift, Register src, Register dst) {
        as_mov(dst, lsr(src, shift.value));
    }
    void ma_asr(Imm32 shift, Register src, Register dst) {
        as_mov(dst, asr(src, shift.value));
    }
    void ma_ror(Imm32 shift, Register src, Register dst) {
        as_mov(dst, ror(src, shift.value));
    }
    void ma_rol(Imm32 shift, Register src, Register dst) {
        as_mov(dst, rol(src, shift.value));
    }

    
    void ma_mvn(Imm32 imm, Register dest,
                SetCond_ sc = NoSetCond, Condition c = Always)
    {
        ma_alu(InvalidReg, imm, dest, op_mvn, sc, c);
    }

    void ma_mvn(Register src1, Register dest,
                SetCond_ sc = NoSetCond, Condition c = Always)
    {
        as_alu(dest, InvalidReg, O2Reg(src1), op_mvn, sc, c);
    }

    
    void ma_and(Register src, Register dest,
                SetCond_ sc = NoSetCond, Condition c = Always)
    {
        ma_and(dest, src, dest);
    }
    void ma_and(Register src1, Register src2, Register dest,
                SetCond_ sc = NoSetCond, Condition c = Always)
    {
        as_and(dest, src1, O2Reg(src2), sc, c);
    }
    void ma_and(Imm32 imm, Register dest,
                SetCond_ sc = NoSetCond, Condition c = Always)
    {
        ma_alu(dest, imm, dest, op_and, sc, c);
    }
    void ma_and(Imm32 imm, Register src1, Register dest,
                SetCond_ sc = NoSetCond, Condition c = Always)
    {
        ma_alu(src1, imm, dest, op_and, sc, c);
    }


    
    void ma_bic(Imm32 imm, Register dest,
                SetCond_ sc = NoSetCond, Condition c = Always)
    {
        ma_alu(dest, imm, dest, op_bic, sc, c);
    }

    
    void ma_eor(Register src, Register dest,
                SetCond_ sc = NoSetCond, Condition c = Always)
    {
        ma_eor(dest, src, dest, sc, c);
    }
    void ma_eor(Register src1, Register src2, Register dest,
                SetCond_ sc = NoSetCond, Condition c = Always)
    {
        as_eor(dest, src1, O2Reg(src2), sc, c);
    }
    void ma_eor(Imm32 imm, Register dest,
                SetCond_ sc = NoSetCond, Condition c = Always)
    {
        ma_alu(dest, imm, dest, op_eor, sc, c);
    }
    void ma_eor(Imm32 imm, Register src1, Register dest,
                SetCond_ sc = NoSetCond, Condition c = Always)
    {
        ma_alu(src1, imm, dest, op_eor, sc, c);
    }

    
    void ma_orr(Register src, Register dest,
                SetCond_ sc = NoSetCond, Condition c = Always)
    {
        ma_orr(dest, src, dest, sc, c);
    }
    void ma_orr(Register src1, Register src2, Register dest,
                SetCond_ sc = NoSetCond, Condition c = Always)
    {
        as_orr(dest, src1, O2Reg(src2), sc, c);
    }
    void ma_orr(Imm32 imm, Register dest,
                SetCond_ sc = NoSetCond, Condition c = Always)
    {
        ma_alu(dest, imm, dest, op_orr, sc, c);
    }
    void ma_orr(Imm32 imm, Register src1, Register dest,
                SetCond_ sc = NoSetCond, Condition c = Always)
    {
        ma_alu(src1, imm, dest, op_orr, sc, c);
    }

    
    
    void ma_adc(Imm32 imm, Register dest) {
        ma_alu(dest, imm, dest, op_adc);
    }
    void ma_adc(Register src, Register dest) {
        as_alu(dest, dest, O2Reg(src), op_adc);
    }
    void ma_adc(Register src1, Register src2, Register dest) {
        as_alu(dest, src1, O2Reg(src2), op_adc);
    }

    
    void ma_add(Imm32 imm, Register dest) {
        ma_alu(dest, imm, dest, op_add);
    }
    void ma_add(Register src1, Register dest) {
        as_alu(dest, dest, O2Reg(src1), op_add);
    }
    void ma_add(Register src1, Register src2, Register dest) {
        as_alu(dest, src1, O2Reg(dest), op_add);
    }
    void ma_add(Register src1, Operand op, Register dest) {
        ma_alu(src1, op, dest, op_add);
    }
    void ma_add(Register src1, Imm32 op, Register dest) {
        ma_alu(src1, op, dest, op_add);
    }

    
    void ma_sbc(Imm32 imm, Register dest) {
        ma_alu(dest, imm, dest, op_sbc);
    }
    void ma_sbc(Register src1, Register dest) {
        as_alu(dest, dest, O2Reg(src1), op_sbc);
    }
    void ma_sbc(Register src1, Register src2, Register dest) {
        as_alu(dest, src1, O2Reg(dest), op_sbc);
    }

    
    void ma_sub(Imm32 imm, Register dest) {
        ma_alu(dest, imm, dest, op_sub);
    }
    void ma_sub(Register src1, Register dest) {
        ma_alu(dest, Operand(O2Reg(src1)), dest, op_sub);
    }
    void ma_sub(Register src1, Register src2, Register dest) {
        ma_alu(src1, Operand(O2Reg(src2)), dest, op_sub);
    }

    
    void ma_rsb(Imm32 imm, Register dest) {
        ma_alu(dest, imm, dest, op_rsb);
    }
    void ma_rsb(Register src1, Register dest) {
        as_alu(dest, dest, O2Reg(src1), op_add);
    }
    void ma_rsb(Register src1, Register src2, Register dest) {
        as_alu(dest, src1, O2Reg(dest), op_rsc);
    }
    void ma_rsb(Register src1, Imm32 src2, Register dest) {
        JS_NOT_REACHED("Feature NYI");
    }

    
    void ma_rsc(Imm32 imm, Register dest) {
        ma_alu(dest, imm, dest, op_rsc);
    }
    void ma_rsc(Register src1, Register dest) {
        as_alu(dest, dest, O2Reg(src1), op_rsc);
    }
    void ma_rsc(Register src1, Register src2, Register dest) {
        as_alu(dest, src1, O2Reg(dest), op_rsc);
    }

    
    
    void ma_cmn(Imm32 imm, Register src1) {
        ma_alu(src1, imm, InvalidReg, op_cmn);
    }
    void ma_cmn(Register src1, Register src2) {
        as_alu(InvalidReg, src2, O2Reg(src1), op_cmn);
    }
    void ma_cmn(Register src1, Operand op) {
        JS_NOT_REACHED("Feature NYI");
    }

    
    void ma_cmp(Imm32 imm, Register src1) {
        ma_alu(src1, imm, InvalidReg, op_cmp);
    }
    void ma_cmp(Register src1, Operand op) {
        JS_NOT_REACHED("Feature NYI");
    }
    void ma_cmp(Register src1, Register src2) {
        as_cmp(src2, O2Reg(src1));
        JS_NOT_REACHED("Feature NYI");
    }

    
    void ma_teq(Imm32 imm, Register src1) {
        ma_alu(src1, imm, InvalidReg, op_teq);
    }
    void ma_teq(Register src2, Register src1) {
        as_tst(src2, O2Reg(src1));
    }
    void ma_teq(Register src1, Operand op) {
        JS_NOT_REACHED("Feature NYI");
    }


    
    void ma_tst(Imm32 imm, Register src1) {
        ma_alu(src1, imm, InvalidReg, op_tst, SetCond);
    }
    void ma_tst(Register src1, Register src2) {
        as_tst(src1, O2Reg(src2));
    }
    void ma_tst(Register src1, Operand op) {
        as_tst(src1, op.toOp2());
    }


    
    
    void ma_dtr(LoadStore ls, Register rn, Imm32 offset, Register rt,
                Index mode = Offset, Condition cc = Always)
    {
        int off = offset.value;
        if (off < 4096 && off > -4096) {
            
            as_dtr(ls, 32, mode, rt, DTRAddr(rn, DtrOffImm(off)), cc);
            return;
        }
        
        datastore::Imm8mData imm = Imm8::encodeImm(off & (~0xfff));
        if (!imm.invalid) {
            as_add(ScratchRegister, rn, imm);
        }
        JS_NOT_REACHED("Feature NYI");
    }
    void ma_dtr(LoadStore ls, Register rn, Register rm, Register rt,
                Index mode = Offset, Condition cc = Always)
    {
        JS_NOT_REACHED("Feature NYI");
    }

    void ma_str(Register rt, DTRAddr addr, Index mode = Offset, Condition cc = Always)
    {
        as_dtr(IsStore, 32, mode, rt, addr, cc);
    }
    void ma_ldr(DTRAddr addr, Register rt, Index mode = Offset, Condition cc = Always)
    {
        as_dtr(IsLoad, 32, mode, rt, addr, cc);
    }
    
    void ma_dataTransferN(LoadStore ls, int size,
                          Register rn, Register rm, Register rt,
                          Index mode = Offset, Condition cc = Always) {
        JS_NOT_REACHED("Feature NYI");
    }

    void ma_dataTransferN(LoadStore ls, int size,
                          Register rn, Imm32 offset, Register rt,
                          Index mode = Offset, Condition cc = Always) {
        JS_NOT_REACHED("Feature NYI");
    }
    void ma_pop(Register r) {
        ma_dtr(IsLoad, sp, Imm32(4), r, PostIndex);
    }
    void ma_push(Register r) {
        ma_dtr(IsStore, sp ,Imm32(4), r, PreIndex);
    }

    
    void ma_b(Label *dest, Condition c = Always)
    {
        as_b(dest, c);
    }
    void ma_b(void *target, Relocation::Kind reloc)
    {
            JS_NOT_REACHED("Feature NYI");
    }
    void ma_b(void *target, Condition c, Relocation::Kind reloc)
    {
        JS_NOT_REACHED("Feature NYI");
    }
    
    
    void ma_bl(Label *dest, Condition c = Always)
    {
        as_bl(dest, c);
    }

    
    void ma_vadd(FloatRegister src1, FloatRegister src2, FloatRegister dst)
    {
        JS_NOT_REACHED("Feature NYI");
    }
    void ma_vmul(FloatRegister src1, FloatRegister src2, FloatRegister dst)
    {
        JS_NOT_REACHED("Feature NYI");
    }
    void ma_vcmp_F64(FloatRegister src1, FloatRegister src2)
    {
        JS_NOT_REACHED("Feature NYI");
    }
    void ma_vcvt_F64_I32(FloatRegister src, FloatRegister dest)
    {
        JS_NOT_REACHED("Feature NYI");
    }
    void ma_vcvt_I32_F64(FloatRegister src, FloatRegister dest)
    {
        JS_NOT_REACHED("Feature NYI");
    }
    void ma_vmov(FloatRegister src, Register dest)
    {
        JS_NOT_REACHED("Feature NYI");
    }
    void ma_vldr(VFPAddr addr, FloatRegister dest)
    {
        JS_NOT_REACHED("Feature NYI");
    }
    void ma_vstr(FloatRegister src, VFPAddr addr)
    {
        JS_NOT_REACHED("Feature NYI");
    }
  protected:
    uint32 alignStackForCall(uint32 stackForArgs) {
        
        uint32 displacement = stackForArgs + framePushed_;
        return stackForArgs + ComputeByteAlignment(displacement, StackAlignment);
    }

    uint32 dynamicallyAlignStackForCall(uint32 stackForArgs, const Register &scratch) {
        
        
        

        JS_NOT_REACHED("Codegen for dynamicallyAlignedStackForCall NYI");
#if 0
        ma_mov(sp, scratch);
        ma_bic(Imm32(StackAlignment - 1), sp);
        Push(scratch);
#endif
        uint32 displacement = stackForArgs + STACK_SLOT_SIZE;
        return stackForArgs + ComputeByteAlignment(displacement, StackAlignment);
    }

    void restoreStackFromDynamicAlignment() {
        
        
        as_dtr(IsLoad, 32, Offset, sp, DTRAddr(sp, DtrOffImm(0)));
    }

  public:
    void reserveStack(uint32 amount) {
        if (amount)
            ma_sub(Imm32(amount), sp);
        framePushed_ += amount;
    }
    void freeStack(uint32 amount) {
        JS_ASSERT(amount <= framePushed_);
        if (amount)
            ma_add(Imm32(amount), sp);
        framePushed_ -= amount;
    }
    void movePtr(ImmWord imm, const Register &dest) {
        ma_mov(Imm32(imm.value), dest);
    }
    void setStackArg(const Register &reg, uint32 arg) {
        ma_dataTransferN(IsStore, 32, sp, Imm32(arg * STACK_SLOT_SIZE), reg);

    }
#ifdef DEBUG
    void checkCallAlignment() {
        Label good;
        ma_tst(Imm32(StackAlignment - 1), sp);
        ma_b(&good, Equal);
        breakpoint();
        bind(&good);
    }
#endif

    
    Condition testInt32(Condition cond, const ValueOperand &value) {
        JS_ASSERT(cond == Assembler::Equal || cond == Assembler::NotEqual);
        ma_cmp(ImmType(JSVAL_TYPE_INT32), value.typeReg());
        return cond;
    }

    Condition testBoolean(Condition cond, const ValueOperand &value) {
        JS_ASSERT(cond == Assembler::Equal || cond == Assembler::NotEqual);
        ma_cmp(ImmType(JSVAL_TYPE_BOOLEAN), value.typeReg());
        return cond;
    }
    Condition testDouble(Condition cond, const ValueOperand &value) {
        JS_ASSERT(cond == Assembler::Equal || cond == Assembler::NotEqual);
        JS_NOT_REACHED("Codegen for testDouble NYI");
        Condition actual = (cond == Equal)
                           ? Below
                           : AboveOrEqual;
        ma_cmp(ImmTag(JSVAL_TAG_CLEAR), value.typeReg());
        return actual;
    }
    Condition testNull(Condition cond, const ValueOperand &value) {
        JS_ASSERT(cond == Assembler::Equal || cond == Assembler::NotEqual);
        ma_cmp(ImmType(JSVAL_TYPE_NULL), value.typeReg());
        return cond;
    }
    Condition testUndefined(Condition cond, const ValueOperand &value) {
        JS_ASSERT(cond == Assembler::Equal || cond == Assembler::NotEqual);
        ma_cmp(ImmType(JSVAL_TYPE_UNDEFINED), value.typeReg());
        return cond;
    }
    Condition testString(Condition cond, const ValueOperand &value) {
        return testString(cond, value.typeReg());
    }
    Condition testObject(Condition cond, const ValueOperand &value) {
        return testObject(cond, value.typeReg());
    }

    
    Condition testInt32(Condition cond, const Register &tag) {
        JS_ASSERT(cond == Equal || cond == NotEqual);
        ma_cmp(ImmTag(JSVAL_TAG_INT32), tag);
        return cond;
    }
    Condition testBoolean(Condition cond, const Register &tag) {
        JS_ASSERT(cond == Equal || cond == NotEqual);
        ma_cmp(ImmTag(JSVAL_TAG_BOOLEAN), tag);
        return cond;
    }
    Condition testNull(Condition cond, const Register &tag) {
        JS_ASSERT(cond == Equal || cond == NotEqual);
        ma_cmp(ImmTag(JSVAL_TAG_NULL), tag);
        return cond;
    }
    Condition testUndefined(Condition cond, const Register &tag) {
        JS_ASSERT(cond == Equal || cond == NotEqual);
        ma_cmp(ImmTag(JSVAL_TAG_UNDEFINED), tag);
        return cond;
    }
    Condition testString(Condition cond, const Register &tag) {
        JS_ASSERT(cond == Equal || cond == NotEqual);
        ma_cmp(ImmTag(JSVAL_TAG_STRING), tag);
        return cond;
    }
    Condition testObject(Condition cond, const Register &tag) {
        JS_ASSERT(cond == Equal || cond == NotEqual);
        ma_cmp(ImmTag(JSVAL_TAG_OBJECT), tag);
        return cond;
    }

    
    void unboxInt32(const ValueOperand &operand, const Register &dest) {
        ma_mov(operand.payloadReg(), dest);
    }
    void unboxBoolean(const ValueOperand &operand, const Register &dest) {
        ma_mov(operand.payloadReg(), dest);
    }
    void unboxDouble(const ValueOperand &operand, const FloatRegister &dest) {
        JS_ASSERT(dest != ScratchFloatReg);
        as_vxfer(operand.payloadReg(), operand.typeReg(),
                VFPRegister(dest), FloatToCore);
    }

    void boolValueToDouble(const ValueOperand &operand, const FloatRegister &dest) {
        JS_NOT_REACHED("Codegen for boolValueToDouble NYI");
#if 0
        cvtsi2sd(operand.payloadReg(), dest);
#endif
    }
    void int32ValueToDouble(const ValueOperand &operand, const FloatRegister &dest) {
        JS_NOT_REACHED("Codegen for int32ValueToDouble NYI");
        
        VFPRegister vfpdest = VFPRegister(dest);
        as_vxfer(operand.payloadReg(), InvalidReg,
                 vfpdest.intOverlay(), CoreToFloat);
        
        as_vcvt(dest, dest);
    }

    void loadStaticDouble(const double *dp, const FloatRegister &dest) {
        JS_NOT_REACHED("Codegen for loadStaticDouble NYI");
#if 0
        _vldr()
        movsd(dp, dest);
#endif
    }
    
    Condition testInt32Truthy(bool truthy, const ValueOperand &operand) {
        ma_tst(operand.payloadReg(), operand.payloadReg());
        return truthy ? NonZero : Zero;
    }
    Condition testBooleanTruthy(bool truthy, const ValueOperand &operand) {
        ma_tst(operand.payloadReg(), operand.payloadReg());
        return truthy ? NonZero : Zero;
    }
    Condition testDoubleTruthy(bool truthy, const FloatRegister &reg) {
        JS_NOT_REACHED("codegen for testDoubleTruthy NYI");
        
#if 0
        xorpd(ScratchFloatReg, ScratchFloatReg);
        ucomisd(ScratchFloatReg, reg);
#endif
        return truthy ? NonZero : Zero;
    }
#if 0
#endif
    void breakpoint() {
        as_bkpt();
    }
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
    void ret() {
        ma_pop(pc);
    }
    void Push(const Register &reg) {
        as_dtr(IsStore, STACK_SLOT_SIZE*8, PreIndex,
               reg,DTRAddr( sp, DtrOffImm(-STACK_SLOT_SIZE)));
        framePushed_ += STACK_SLOT_SIZE;
    }
    void jump(Label *label) {
        as_b(label);
    }
    template<typename T>
    void branchTestInt32(Condition cond, const T & t, Label *label) {
        JS_NOT_REACHED("feature NYI");
    }
    template<typename T>
    void branchTestBoolean(Condition cond, const T & t, Label *label) {
        JS_NOT_REACHED("feature NYI");
    }
    template<typename T>
    void branchTestDouble(Condition cond, const T & t, Label *label) {
        JS_NOT_REACHED("feature NYI");
    }
    template<typename T>
    void branchTestNull(Condition cond, const T & t, Label *label) {
        JS_NOT_REACHED("feature NYI");
    }
    template<typename T>
    void branchTestObject(Condition cond, const T & t, Label *label) {
        JS_NOT_REACHED("feature NYI");
    }
    template<typename T>
    void branchTestString(Condition cond, const T & t, Label *label) {
        JS_NOT_REACHED("feature NYI");
    }
    template<typename T>
    void branchTestUndefined(Condition cond, const T & t, Label *label) {
        JS_NOT_REACHED("feature NYI");
    }


    template<typename T>
    void branchTestBooleanTruthy(bool b, const T & t, Label *label) {
        JS_NOT_REACHED("feature NYI");
    }
};

typedef MacroAssemblerARMCompat MacroAssemblerSpecific;

} 
} 

#endif 
