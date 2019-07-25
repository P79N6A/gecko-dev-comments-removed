







































#include "ion/arm/MacroAssembler-arm.h"
using namespace js;
using namespace ion;
void
MacroAssemblerARM::convertInt32ToDouble(const Register &src, const FloatRegister &dest)
{
    
    as_vxfer(src, InvalidReg, VFPRegister(dest, VFPRegister::Single),
             CoreToFloat);
    as_vcvt(VFPRegister(dest, VFPRegister::Double),
            VFPRegister(dest, VFPRegister::Single));
}

bool
MacroAssemblerARM::alu_dbl(Register src1, Imm32 imm, Register dest, ALUOp op,
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


void
MacroAssemblerARM::ma_alu(Register src1, Imm32 imm, Register dest,
                          ALUOp op,
                          SetCond_ sc, Condition c)
{
    
    
    if (dest == InvalidReg) {
        JS_ASSERT(sc == SetCond);
    }

    
    
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
            
            
            
            
            
            if (op == op_mov && ((imm.value & ~ 0xffff) == 0)) {
                JS_ASSERT(src1 == InvalidReg);
                as_movw(dest, (uint16)imm.value, c);
                return;
            }
            
            
            if (op == op_mvn && (((~imm.value) & ~ 0xffff) == 0)) {
                JS_ASSERT(src1 == InvalidReg);
                as_movw(dest, (uint16)~imm.value, c);
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

void
MacroAssemblerARM::ma_alu(Register src1, Operand op2, Register dest, ALUOp op,
            SetCond_ sc, Assembler::Condition c)
{
    JS_ASSERT(op2.getTag() == Operand::OP2);
    as_alu(dest, src1, op2.toOp2(), op, sc, c);
}


void
MacroAssemblerARM::ma_mov(Register src, Register dest,
            SetCond_ sc, Assembler::Condition c)
{
    as_mov(dest, O2Reg(src), sc, c);
}

void
MacroAssemblerARM::ma_mov(Imm32 imm, Register dest,
                          SetCond_ sc, Assembler::Condition c)
{
    ma_alu(InvalidReg, imm, dest, op_mov, sc, c);
}
void
MacroAssemblerARM::ma_mov(const ImmGCPtr &ptr, Register dest)
{
    ma_mov(Imm32(ptr.value), dest);
    JS_NOT_REACHED("todo:make gc more sane.");
}

    
void
MacroAssemblerARM::ma_lsl(Imm32 shift, Register src, Register dst)
{
    as_mov(dst, lsl(src, shift.value));
}
void
MacroAssemblerARM::ma_lsr(Imm32 shift, Register src, Register dst)
{
    as_mov(dst, lsr(src, shift.value));
}
void
MacroAssemblerARM::ma_asr(Imm32 shift, Register src, Register dst)
{
    as_mov(dst, asr(src, shift.value));
}
void
MacroAssemblerARM::ma_ror(Imm32 shift, Register src, Register dst)
{
    as_mov(dst, ror(src, shift.value));
}
void
MacroAssemblerARM::ma_rol(Imm32 shift, Register src, Register dst)
{
    as_mov(dst, rol(src, shift.value));
}
    
void
MacroAssemblerARM::ma_lsl(Register shift, Register src, Register dst)
{
    as_mov(dst, lsl(src, shift));
}
void
MacroAssemblerARM::ma_lsr(Register shift, Register src, Register dst)
{
    as_mov(dst, lsr(src, shift));
}
void
MacroAssemblerARM::ma_asr(Register shift, Register src, Register dst)
{
    as_mov(dst, asr(src, shift));
}
void
MacroAssemblerARM::ma_ror(Register shift, Register src, Register dst)
{
    as_mov(dst, ror(src, shift));
}
void
MacroAssemblerARM::ma_rol(Register shift, Register src, Register dst)
{
    ma_rsb(shift, Imm32(32), ScratchRegister);
    as_mov(dst, ror(src, ScratchRegister));
}

    

void
MacroAssemblerARM::ma_mvn(Imm32 imm, Register dest,
                          SetCond_ sc, Assembler::Condition c)
{
    ma_alu(InvalidReg, imm, dest, op_mvn, sc, c);
}

void
MacroAssemblerARM::ma_mvn(Register src1, Register dest,
                          SetCond_ sc, Assembler::Condition c)
{
    as_alu(dest, InvalidReg, O2Reg(src1), op_mvn, sc, c);
}

    
void
MacroAssemblerARM::ma_and(Register src, Register dest,
                          SetCond_ sc, Assembler::Condition c)
{
    ma_and(dest, src, dest);
}
void
MacroAssemblerARM::ma_and(Register src1, Register src2, Register dest,
            SetCond_ sc, Assembler::Condition c)
{
    as_and(dest, src1, O2Reg(src2), sc, c);
}
void
MacroAssemblerARM::ma_and(Imm32 imm, Register dest,
                          SetCond_ sc, Assembler::Condition c)
{
    ma_alu(dest, imm, dest, op_and, sc, c);
}
void
MacroAssemblerARM::ma_and(Imm32 imm, Register src1, Register dest,
                          SetCond_ sc, Assembler::Condition c)
{
    ma_alu(src1, imm, dest, op_and, sc, c);
}


    
void
MacroAssemblerARM::ma_bic(Imm32 imm, Register dest,
                          SetCond_ sc, Assembler::Condition c)
{
    ma_alu(dest, imm, dest, op_bic, sc, c);
}

    
void
MacroAssemblerARM::ma_eor(Register src, Register dest,
            SetCond_ sc, Assembler::Condition c)
{
    ma_eor(dest, src, dest, sc, c);
}
void
MacroAssemblerARM::ma_eor(Register src1, Register src2, Register dest,
                          SetCond_ sc, Assembler::Condition c)
{
    as_eor(dest, src1, O2Reg(src2), sc, c);
}
void
MacroAssemblerARM::ma_eor(Imm32 imm, Register dest,
                          SetCond_ sc, Assembler::Condition c)
{
    ma_alu(dest, imm, dest, op_eor, sc, c);
}
void
MacroAssemblerARM::ma_eor(Imm32 imm, Register src1, Register dest,
       SetCond_ sc, Assembler::Condition c)
{
    ma_alu(src1, imm, dest, op_eor, sc, c);
}

    
void
MacroAssemblerARM::ma_orr(Register src, Register dest,
                          SetCond_ sc, Assembler::Condition c)
{
    ma_orr(dest, src, dest, sc, c);
}
void
MacroAssemblerARM::ma_orr(Register src1, Register src2, Register dest,
                          SetCond_ sc, Assembler::Condition c)
{
    as_orr(dest, src1, O2Reg(src2), sc, c);
}
void
MacroAssemblerARM::ma_orr(Imm32 imm, Register dest,
                          SetCond_ sc, Assembler::Condition c)
{
    ma_alu(dest, imm, dest, op_orr, sc, c);
}
void
MacroAssemblerARM::ma_orr(Imm32 imm, Register src1, Register dest,
                SetCond_ sc, Assembler::Condition c)
{
    ma_alu(src1, imm, dest, op_orr, sc, c);
}

    
    
void
MacroAssemblerARM::ma_adc(Imm32 imm, Register dest)
{
    ma_alu(dest, imm, dest, op_adc);
}
void
MacroAssemblerARM::ma_adc(Register src, Register dest)
{
    as_alu(dest, dest, O2Reg(src), op_adc);
}
void
MacroAssemblerARM::ma_adc(Register src1, Register src2, Register dest)
{
    as_alu(dest, src1, O2Reg(src2), op_adc);
}

    
void
MacroAssemblerARM::ma_add(Imm32 imm, Register dest)
{
    ma_alu(dest, imm, dest, op_add);
}
void
MacroAssemblerARM::ma_add(Register src1, Register dest)
{
    as_alu(dest, dest, O2Reg(src1), op_add);
}
void
MacroAssemblerARM::ma_add(Register src1, Register src2, Register dest)
{
    as_alu(dest, src1, O2Reg(dest), op_add);
}
void
MacroAssemblerARM::ma_add(Register src1, Operand op, Register dest)
{
    ma_alu(src1, op, dest, op_add);
}
void
MacroAssemblerARM::ma_add(Register src1, Imm32 op, Register dest)
{
    ma_alu(src1, op, dest, op_add);
}

    
void
MacroAssemblerARM::ma_sbc(Imm32 imm, Register dest)
{
    ma_alu(dest, imm, dest, op_sbc);
}
void
MacroAssemblerARM::ma_sbc(Register src1, Register dest)
{
    as_alu(dest, dest, O2Reg(src1), op_sbc);
}
void
MacroAssemblerARM::ma_sbc(Register src1, Register src2, Register dest)
{
    as_alu(dest, src1, O2Reg(dest), op_sbc);
}

    
void
MacroAssemblerARM::ma_sub(Imm32 imm, Register dest)
{
    ma_alu(dest, imm, dest, op_sub);
}
void
MacroAssemblerARM::ma_sub(Register src1, Register dest)
{
    ma_alu(dest, Operand(O2Reg(src1)), dest, op_sub);
}
void
MacroAssemblerARM::ma_sub(Register src1, Register src2, Register dest)
{
    ma_alu(src1, Operand(O2Reg(src2)), dest, op_sub);
}
void
MacroAssemblerARM::ma_sub(Register src1, Operand op, Register dest)
{
    ma_alu(src1, op, dest, op_sub);
}
void
MacroAssemblerARM::ma_sub(Register src1, Imm32 op, Register dest)
{
    ma_alu(src1, op, dest, op_sub);
}

    
void
MacroAssemblerARM::ma_rsb(Imm32 imm, Register dest)
{
    ma_alu(dest, imm, dest, op_rsb);
}
void
MacroAssemblerARM::ma_rsb(Register src1, Register dest)
{
    as_alu(dest, dest, O2Reg(src1), op_add);
}
void
MacroAssemblerARM::ma_rsb(Register src1, Register src2, Register dest)
{
    as_alu(dest, src1, O2Reg(dest), op_rsc);
}
void
MacroAssemblerARM::ma_rsb(Register src1, Imm32 op2, Register dest)
{
    ma_alu(src1, op2, dest, op_rsb);
}

    
void
MacroAssemblerARM::ma_rsc(Imm32 imm, Register dest)
{
    ma_alu(dest, imm, dest, op_rsc);
}
void
MacroAssemblerARM::ma_rsc(Register src1, Register dest)
{
    as_alu(dest, dest, O2Reg(src1), op_rsc);
}
void
MacroAssemblerARM::ma_rsc(Register src1, Register src2, Register dest)
{
    as_alu(dest, src1, O2Reg(dest), op_rsc);
}

    
    
void
MacroAssemblerARM::ma_cmn(Imm32 imm, Register src1)
{
    ma_alu(src1, imm, InvalidReg, op_cmn);
}
void
MacroAssemblerARM::ma_cmn(Register src1, Register src2)
{
    as_alu(InvalidReg, src2, O2Reg(src1), op_cmn);
}
void
MacroAssemblerARM::ma_cmn(Register src1, Operand op)
{
    JS_NOT_REACHED("Feature NYI");
}

    
void
MacroAssemblerARM::ma_cmp(Imm32 imm, Register src1)
{
    ma_alu(src1, imm, InvalidReg, op_cmp, SetCond);
}
void
MacroAssemblerARM::ma_cmp(Register src1, Operand op)
{
    as_cmp(src1, op.toOp2());
}
void
MacroAssemblerARM::ma_cmp(Register src1, Register src2)
{
    as_cmp(src2, O2Reg(src1));
}

    
void
MacroAssemblerARM::ma_teq(Imm32 imm, Register src1)
{
    ma_alu(src1, imm, InvalidReg, op_teq, SetCond);
}
void
MacroAssemblerARM::ma_teq(Register src2, Register src1)
{
    as_tst(src2, O2Reg(src1));
}
void
MacroAssemblerARM::ma_teq(Register src1, Operand op)
{
    JS_NOT_REACHED("Feature NYI");
}



void
MacroAssemblerARM::ma_tst(Imm32 imm, Register src1)
{
    ma_alu(src1, imm, InvalidReg, op_tst, SetCond);
}
void
MacroAssemblerARM::ma_tst(Register src1, Register src2)
{
    as_tst(src1, O2Reg(src2));
}
void
MacroAssemblerARM::ma_tst(Register src1, Operand op)
{
    as_tst(src1, op.toOp2());
}


    
    
void
MacroAssemblerARM::ma_dtr(LoadStore ls, Register rn, Imm32 offset, Register rt,
                          Index mode, Assembler::Condition cc)
{
    int off = offset.value;
    if (off < 4096 && off > -4096) {
        
        as_dtr(ls, 32, mode, rt, DTRAddr(rn, DtrOffImm(off)), cc);
        return;
    }
    
    datastore::Imm8mData imm = Imm8::encodeImm(off & (~0xfff));
    if (!imm.invalid) {
        as_add(ScratchRegister, rn, imm);
        as_dtr(ls, 32, mode, rt, DTRAddr(ScratchRegister, DtrOffImm(off & 0xfff)), cc);
    } else {
        ma_mov(offset, ScratchRegister);
        as_dtr(ls, 32, mode, rt, DTRAddr(rn, DtrRegImmShift(ScratchRegister, LSL, 0)));
    }
}

void
MacroAssemblerARM::ma_dtr(LoadStore ls, Register rn, Register rm, Register rt,
                          Index mode, Assembler::Condition cc)
{
    JS_NOT_REACHED("Feature NYI");
}

void
MacroAssemblerARM::ma_str(Register rt, DTRAddr addr, Index mode, Assembler::Condition cc)
{
    as_dtr(IsStore, 32, mode, rt, addr, cc);
}
void
MacroAssemblerARM::ma_ldr(DTRAddr addr, Register rt, Index mode, Assembler::Condition cc)
{
    as_dtr(IsLoad, 32, mode, rt, addr, cc);
}
    
void
MacroAssemblerARM::ma_dataTransferN(LoadStore ls, int size,
                          Register rn, Register rm, Register rt,
                          Index mode, Assembler::Condition cc)
{
    JS_NOT_REACHED("Feature NYI");
}

void
MacroAssemblerARM::ma_dataTransferN(LoadStore ls, int size,
                          Register rn, Imm32 offset, Register rt,
                          Index mode, Assembler::Condition cc)
{
    JS_NOT_REACHED("Feature NYI");
}
void
MacroAssemblerARM::ma_pop(Register r)
{
    ma_dtr(IsLoad, sp, Imm32(4), r, PostIndex);
}
void
MacroAssemblerARM::ma_push(Register r)
{
    ma_dtr(IsStore, sp,Imm32(4), r, PreIndex);
}


void
MacroAssemblerARM::ma_b(Label *dest, Assembler::Condition c)
{
    as_b(dest, c);
}
void
MacroAssemblerARM::ma_b(void *target, Relocation::Kind reloc)
{
    JS_NOT_REACHED("Feature NYI");
}
void
MacroAssemblerARM::ma_b(void *target, Assembler::Condition c, Relocation::Kind reloc)
{
    
    
    
    
    uint32 trg = (uint32)target;
    as_movw(ScratchRegister, Imm16(trg & 0xffff), c);
    as_movt(ScratchRegister, Imm16(trg >> 16), c);
    
    as_bx(ScratchRegister, c);
}



void
MacroAssemblerARM::ma_bl(Label *dest, Assembler::Condition c)
{
    as_bl(dest, c);
}


void
MacroAssemblerARM::ma_vadd(FloatRegister src1, FloatRegister src2, FloatRegister dst)
{
    as_vadd(VFPRegister(dst), VFPRegister(src1), VFPRegister(src2));
}
void
MacroAssemblerARM::ma_vmul(FloatRegister src1, FloatRegister src2, FloatRegister dst)
{
    as_vmul(VFPRegister(dst), VFPRegister(src1), VFPRegister(src2));
}
void
MacroAssemblerARM::ma_vcmp_F64(FloatRegister src1, FloatRegister src2)
{
    as_vcmp(VFPRegister(src1), VFPRegister(src2));
}
void
MacroAssemblerARM::ma_vcvt_F64_I32(FloatRegister src, FloatRegister dest)
{
    JS_NOT_REACHED("Feature NYI");
}
void
MacroAssemblerARM::ma_vcvt_I32_F64(FloatRegister src, FloatRegister dest)
{
    JS_NOT_REACHED("Feature NYI");
}
void
MacroAssemblerARM::ma_vmov(FloatRegister src, Register dest)
{
    JS_NOT_REACHED("Feature NYI");
    
}
void
MacroAssemblerARM::ma_vldr(VFPAddr addr, FloatRegister dest)
{
    as_vdtr(IsLoad, dest, addr);
}
void
MacroAssemblerARM::ma_vstr(FloatRegister src, VFPAddr addr)
{
    as_vdtr(IsStore, src, addr);
}

uint32
MacroAssemblerARM::alignStackForCall(uint32 stackForArgs)
{
    
    uint32 displacement = stackForArgs + framePushed_;
    return stackForArgs + ComputeByteAlignment(displacement, StackAlignment);
}

uint32
MacroAssemblerARM::dynamicallyAlignStackForCall(uint32 stackForArgs, const Register &scratch)
{
    
    
    

    JS_NOT_REACHED("Codegen for dynamicallyAlignedStackForCall NYI");
#if 0
    ma_mov(sp, scratch);
    ma_bic(Imm32(StackAlignment - 1), sp);
    Push(scratch);
#endif
    uint32 displacement = stackForArgs + STACK_SLOT_SIZE;
    return stackForArgs + ComputeByteAlignment(displacement, StackAlignment);
}

void
MacroAssemblerARM::restoreStackFromDynamicAlignment()
{
    
    
    as_dtr(IsLoad, 32, Offset, sp, DTRAddr(sp, DtrOffImm(0)));
}

void
MacroAssemblerARM::reserveStack(uint32 amount)
{
    if (amount)
        ma_sub(Imm32(amount), sp);
    framePushed_ += amount;
}
void
MacroAssemblerARM::freeStack(uint32 amount)
{
    JS_ASSERT(amount <= framePushed_);
    if (amount)
        ma_add(Imm32(amount), sp);
    framePushed_ -= amount;
}
void
MacroAssemblerARM::movePtr(ImmWord imm, const Register dest)
{
    ma_mov(Imm32(imm.value), dest);
}
void
MacroAssemblerARM::movePtr(ImmGCPtr imm, const Register dest)
{
    ma_mov(imm, dest);
}

void
MacroAssemblerARM::loadPtr(const Address &address, Register dest)
{
    JS_NOT_REACHED("NYI");
}
void
MacroAssemblerARM::setStackArg(const Register &reg, uint32 arg)
{
    ma_dataTransferN(IsStore, 32, sp, Imm32(arg * STACK_SLOT_SIZE), reg);

}
#ifdef DEBUG
void
MacroAssemblerARM::checkCallAlignment()
{
    Label good;
    ma_tst(Imm32(StackAlignment - 1), sp);
    ma_b(&good, Equal);
    breakpoint();
    bind(&good);
}
#endif

    
Assembler::Condition
MacroAssemblerARM::testInt32(Assembler::Condition cond, const ValueOperand &value)
{
    JS_ASSERT(cond == Assembler::Equal || cond == Assembler::NotEqual);
    ma_cmp(ImmType(JSVAL_TYPE_INT32), value.typeReg());
    return cond;
}

Assembler::Condition
MacroAssemblerARM::testBoolean(Assembler::Condition cond, const ValueOperand &value)
{
    JS_ASSERT(cond == Assembler::Equal || cond == Assembler::NotEqual);
    ma_cmp(ImmType(JSVAL_TYPE_BOOLEAN), value.typeReg());
    return cond;
}
Assembler::Condition
MacroAssemblerARM::testDouble(Assembler::Condition cond, const ValueOperand &value)
{
    JS_ASSERT(cond == Assembler::Equal || cond == Assembler::NotEqual);
    Assembler::Condition actual = (cond == Equal)
        ? Below
        : AboveOrEqual;
    ma_cmp(ImmTag(JSVAL_TAG_CLEAR), value.typeReg());
    return actual;
}
Assembler::Condition
MacroAssemblerARM::testNull(Assembler::Condition cond, const ValueOperand &value)
{
    JS_ASSERT(cond == Assembler::Equal || cond == Assembler::NotEqual);
    ma_cmp(ImmType(JSVAL_TYPE_NULL), value.typeReg());
    return cond;
}
Assembler::Condition
MacroAssemblerARM::testUndefined(Assembler::Condition cond, const ValueOperand &value)
{
    JS_ASSERT(cond == Assembler::Equal || cond == Assembler::NotEqual);
    ma_cmp(ImmType(JSVAL_TYPE_UNDEFINED), value.typeReg());
    return cond;
}
Assembler::Condition
MacroAssemblerARM::testString(Assembler::Condition cond, const ValueOperand &value)
{
    return testString(cond, value.typeReg());
}
Assembler::Condition
MacroAssemblerARM::testObject(Assembler::Condition cond, const ValueOperand &value)
{
    return testObject(cond, value.typeReg());
}

    
Assembler::Condition
MacroAssemblerARM::testInt32(Assembler::Condition cond, const Register &tag)
{
    JS_ASSERT(cond == Equal || cond == NotEqual);
    ma_cmp(ImmTag(JSVAL_TAG_INT32), tag);
    return cond;
}
Assembler::Condition
MacroAssemblerARM::testBoolean(Assembler::Condition cond, const Register &tag)
{
    JS_ASSERT(cond == Equal || cond == NotEqual);
    ma_cmp(ImmTag(JSVAL_TAG_BOOLEAN), tag);
    return cond;
}
Assembler::Condition
MacroAssemblerARM::testNull(Assembler::Condition cond, const Register &tag) {
        JS_ASSERT(cond == Equal || cond == NotEqual);
        ma_cmp(ImmTag(JSVAL_TAG_NULL), tag);
        return cond;
    }

Assembler::Condition
MacroAssemblerARM::testUndefined(Assembler::Condition cond, const Register &tag) {
        JS_ASSERT(cond == Equal || cond == NotEqual);
        ma_cmp(ImmTag(JSVAL_TAG_UNDEFINED), tag);
        return cond;
    }
Assembler::Condition
MacroAssemblerARM::testString(Assembler::Condition cond, const Register &tag) {
        JS_ASSERT(cond == Equal || cond == NotEqual);
        ma_cmp(ImmTag(JSVAL_TAG_STRING), tag);
        return cond;
    }

Assembler::Condition
MacroAssemblerARM::testObject(Assembler::Condition cond, const Register &tag)
{
    JS_ASSERT(cond == Equal || cond == NotEqual);
    ma_cmp(ImmTag(JSVAL_TAG_OBJECT), tag);
    return cond;
}

    
void
MacroAssemblerARM::unboxInt32(const ValueOperand &operand, const Register &dest)
{
    ma_mov(operand.payloadReg(), dest);
}

void
MacroAssemblerARM::unboxBoolean(const ValueOperand &operand, const Register &dest)
{
    ma_mov(operand.payloadReg(), dest);
}

void
MacroAssemblerARM::unboxDouble(const ValueOperand &operand, const FloatRegister &dest)
{
    JS_ASSERT(dest != ScratchFloatReg);
    as_vxfer(operand.payloadReg(), operand.typeReg(),
             VFPRegister(dest), CoreToFloat);
}

void
MacroAssemblerARM::boolValueToDouble(const ValueOperand &operand, const FloatRegister &dest)
{
    JS_NOT_REACHED("Codegen for boolValueToDouble NYI");
#if 0
    cvtsi2sd(operand.payloadReg(), dest);
#endif
}

void
MacroAssemblerARM::int32ValueToDouble(const ValueOperand &operand, const FloatRegister &dest)
{
    JS_NOT_REACHED("Codegen for int32ValueToDouble NYI");
    
    VFPRegister vfpdest = VFPRegister(dest);
    as_vxfer(operand.payloadReg(), InvalidReg,
             vfpdest.intOverlay(), CoreToFloat);
    
    as_vcvt(dest, dest);
}

void
MacroAssemblerARM::loadStaticDouble(const double *dp, const FloatRegister &dest)
{
    JS_NOT_REACHED("Codegen for loadStaticDouble NYI");
#if 0
    _vldr()
        movsd(dp, dest);
#endif
}
    

Assembler::Condition
MacroAssemblerARM::testInt32Truthy(bool truthy, const ValueOperand &operand)
{
    ma_tst(operand.payloadReg(), operand.payloadReg());
    return truthy ? NonZero : Zero;
}

Assembler::Condition
MacroAssemblerARM::testBooleanTruthy(bool truthy, const ValueOperand &operand)
{
    ma_tst(operand.payloadReg(), operand.payloadReg());
    return truthy ? NonZero : Zero;
}

Assembler::Condition
MacroAssemblerARM::testDoubleTruthy(bool truthy, const FloatRegister &reg)
{
    JS_NOT_REACHED("codegen for testDoubleTruthy NYI");
    
#if 0
    xorpd(ScratchFloatReg, ScratchFloatReg);
    ucomisd(ScratchFloatReg, reg);
#endif
    return truthy ? NonZero : Zero;
}

void
MacroAssemblerARM::breakpoint() {
    as_bkpt();
}
