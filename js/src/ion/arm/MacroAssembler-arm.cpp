







































#include "ion/arm/MacroAssembler-arm.h"
#include "ion/MoveEmitter.h"

using namespace js;
using namespace ion;
bool
isValueDTRDCandidate(ValueOperand &val)
{
    
    
    
    if ((val.typeReg().code() != (val.payloadReg().code() + 1)))
        return false;
    else if ((val.payloadReg().code() & 1) != 0)
        return false;
    return true;
}

void
MacroAssemblerARM::convertInt32ToDouble(const Register &src, const FloatRegister &dest_)
{
    
    VFPRegister dest = VFPRegister(dest_);
    as_vxfer(src, InvalidReg, dest.sintOverlay(),
             CoreToFloat);
    as_vcvt(dest, dest.sintOverlay());
}

void
MacroAssemblerARM::convertUInt32ToDouble(const Register &src, const FloatRegister &dest_)
{
    
    VFPRegister dest = VFPRegister(dest_);
    as_vxfer(src, InvalidReg, dest.uintOverlay(),
             CoreToFloat);
    as_vcvt(dest, dest.uintOverlay());
}

void MacroAssemblerARM::convertDoubleToFloat(const FloatRegister &src, const FloatRegister &dest)
{
    as_vcvt(VFPRegister(dest).singleOverlay(), VFPRegister(src));
}








void
MacroAssemblerARM::branchTruncateDouble(const FloatRegister &src, const Register &dest, Label *fail)
{
    ma_vcvt_F64_I32(src, ScratchFloatReg);
    ma_vxfer(ScratchFloatReg, dest);
    ma_cmp(dest, Imm32(0x7fffffff));
    ma_cmp(dest, Imm32(0x80000000), Assembler::NotEqual);
    ma_b(fail, Assembler::Equal);
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
    as_alu(dest, ScratchRegister, both.snd, op, sc, c);
    
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
    Register negDest;
    ALUOp negOp = ALUNeg(op, dest, &negImm, &negDest);
    Imm8 negImm8 = Imm8(negImm.value);
    
    
    
    
    
    
    if (negOp != op_invalid && !negImm8.invalid) {
        as_alu(negDest, src1, negImm8, negOp, sc, c);
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
        if ((imm.value >> 16) != 0)
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
MacroAssemblerARM::ma_alu(Register src1, Operand2 op2, Register dest, ALUOp op, SetCond_ sc, Condition c)
{
    as_alu(dest, src1, op2, op, sc, c);
}

void
MacroAssemblerARM::ma_nop()
{
    as_nop();
}

Instruction *
NextInst(Instruction *i)
{
    if (i == NULL)
        return NULL;
    return i->next();
}

void
MacroAssemblerARM::ma_movPatchable(Imm32 imm_, Register dest,
                                   Assembler::Condition c, RelocStyle rs, Instruction *i)
{
    int32 imm = imm_.value;
    switch(rs) {
      case L_MOVWT:
        as_movw(dest, Imm16(imm & 0xffff), c, i);
        i = NextInst(i);
        as_movt(dest, Imm16(imm >> 16 & 0xffff), c, i);
        break;
      case L_LDR:
        
        break;
    }
}

void
MacroAssemblerARM::ma_mov(Register src, Register dest,
            SetCond_ sc, Assembler::Condition c)
{
    if (sc == SetCond || dest != src)
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
    
    
    writeDataRelocation(ptr);
    ma_movPatchable(Imm32(ptr.value), dest, Always, L_MOVWT);
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
MacroAssemblerARM::ma_neg(Register src1, Register dest,
                          SetCond_ sc, Assembler::Condition c)
{
    as_rsb(dest, src1, Imm8(0), sc, c);
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
MacroAssemblerARM::ma_adc(Imm32 imm, Register dest, SetCond_ sc, Condition c)
{
    ma_alu(dest, imm, dest, op_adc, sc, c);
}
void
MacroAssemblerARM::ma_adc(Register src, Register dest, SetCond_ sc, Condition c)
{
    as_alu(dest, dest, O2Reg(src), op_adc, sc, c);
}
void
MacroAssemblerARM::ma_adc(Register src1, Register src2, Register dest, SetCond_ sc, Condition c)
{
    as_alu(dest, src1, O2Reg(src2), op_adc, sc, c);
}

    
void
MacroAssemblerARM::ma_add(Imm32 imm, Register dest, SetCond_ sc, Condition c)
{
    ma_alu(dest, imm, dest, op_add, sc, c);
}

void
MacroAssemblerARM::ma_add(Register src1, Register dest, SetCond_ sc, Condition c)
{
    ma_alu(dest, O2Reg(src1), dest, op_add, sc, c);
}
void
MacroAssemblerARM::ma_add(Register src1, Register src2, Register dest, SetCond_ sc, Condition c)
{
    as_alu(dest, src1, O2Reg(src2), op_add, sc, c);
}
void
MacroAssemblerARM::ma_add(Register src1, Operand op, Register dest, SetCond_ sc, Condition c)
{
    ma_alu(src1, op, dest, op_add, sc, c);
}
void
MacroAssemblerARM::ma_add(Register src1, Imm32 op, Register dest, SetCond_ sc, Condition c)
{
    ma_alu(src1, op, dest, op_add, sc, c);
}

    
void
MacroAssemblerARM::ma_sbc(Imm32 imm, Register dest, SetCond_ sc, Condition c)
{
    ma_alu(dest, imm, dest, op_sbc, sc, c);
}
void
MacroAssemblerARM::ma_sbc(Register src1, Register dest, SetCond_ sc, Condition c)
{
    as_alu(dest, dest, O2Reg(src1), op_sbc, sc, c);
}
void
MacroAssemblerARM::ma_sbc(Register src1, Register src2, Register dest, SetCond_ sc, Condition c)
{
    as_alu(dest, src1, O2Reg(src2), op_sbc, sc, c);
}

    
void
MacroAssemblerARM::ma_sub(Imm32 imm, Register dest, SetCond_ sc, Condition c)
{
    ma_alu(dest, imm, dest, op_sub, sc, c);
}
void
MacroAssemblerARM::ma_sub(Register src1, Register dest, SetCond_ sc, Condition c)
{
    ma_alu(dest, Operand(src1), dest, op_sub, sc, c);
}
void
MacroAssemblerARM::ma_sub(Register src1, Register src2, Register dest, SetCond_ sc, Condition c)
{
    ma_alu(src1, Operand(src2), dest, op_sub, sc, c);
}
void
MacroAssemblerARM::ma_sub(Register src1, Operand op, Register dest, SetCond_ sc, Condition c)
{
    ma_alu(src1, op, dest, op_sub, sc, c);
}
void
MacroAssemblerARM::ma_sub(Register src1, Imm32 op, Register dest, SetCond_ sc, Condition c)
{
    ma_alu(src1, op, dest, op_sub, sc, c);
}

    
void
MacroAssemblerARM::ma_rsb(Imm32 imm, Register dest, SetCond_ sc, Condition c)
{
    ma_alu(dest, imm, dest, op_rsb, sc, c);
}
void
MacroAssemblerARM::ma_rsb(Register src1, Register dest, SetCond_ sc, Condition c)
{
    as_alu(dest, dest, O2Reg(src1), op_add, sc, c);
}
void
MacroAssemblerARM::ma_rsb(Register src1, Register src2, Register dest, SetCond_ sc, Condition c)
{
    as_alu(dest, src1, O2Reg(src2), op_rsb, sc, c);
}
void
MacroAssemblerARM::ma_rsb(Register src1, Imm32 op2, Register dest, SetCond_ sc, Condition c)
{
    ma_alu(src1, op2, dest, op_rsb, sc, c);
}

    
void
MacroAssemblerARM::ma_rsc(Imm32 imm, Register dest, SetCond_ sc, Condition c)
{
    ma_alu(dest, imm, dest, op_rsc, sc, c);
}
void
MacroAssemblerARM::ma_rsc(Register src1, Register dest, SetCond_ sc, Condition c)
{
    as_alu(dest, dest, O2Reg(src1), op_rsc, sc, c);
}
void
MacroAssemblerARM::ma_rsc(Register src1, Register src2, Register dest, SetCond_ sc, Condition c)
{
    as_alu(dest, src1, O2Reg(src2), op_rsc, sc, c);
}

    
    
void
MacroAssemblerARM::ma_cmn(Register src1, Imm32 imm, Condition c)
{
    ma_alu(src1, imm, InvalidReg, op_cmn, SetCond, c);
}
void
MacroAssemblerARM::ma_cmn(Register src1, Register src2, Condition c)
{
    as_alu(InvalidReg, src2, O2Reg(src1), op_cmn, SetCond, c);
}
void
MacroAssemblerARM::ma_cmn(Register src1, Operand op, Condition c)
{
    JS_NOT_REACHED("Feature NYI");
}


void
MacroAssemblerARM::ma_cmp(Register src1, Imm32 imm, Condition c)
{
    ma_alu(src1, imm, InvalidReg, op_cmp, SetCond, c);
}

void
MacroAssemblerARM::ma_cmp(Register src1, ImmWord ptr, Condition c)
{
    ma_cmp(src1, Imm32(ptr.value), c);
}

void
MacroAssemblerARM::ma_cmp(Register src1, ImmGCPtr ptr, Condition c)
{
    ma_mov(ptr, ScratchRegister);
    ma_cmp(src1, ScratchRegister, c);
}
void
MacroAssemblerARM::ma_cmp(Register src1, Operand op, Condition c)
{
    switch (op.getTag()) {
      case Operand::OP2:
        as_cmp(src1, op.toOp2(), c);
        break;
      case Operand::MEM:
        ma_ldr(op, ScratchRegister);
        as_cmp(src1, O2Reg(ScratchRegister), c);
        break;
      default:
        JS_NOT_REACHED("trying to compare FP and integer registers");
        break;
    }
}
void
MacroAssemblerARM::ma_cmp(Register src1, Register src2, Condition c)
{
    as_cmp(src1, O2Reg(src2), c);
}

    
void
MacroAssemblerARM::ma_teq(Register src1, Imm32 imm, Condition c)
{
    ma_alu(src1, imm, InvalidReg, op_teq, SetCond, c);
}
void
MacroAssemblerARM::ma_teq(Register src1, Register src2, Condition c)
{
    as_tst(src1, O2Reg(src2), c);
}
void
MacroAssemblerARM::ma_teq(Register src1, Operand op, Condition c)
{
    as_teq(src1, op.toOp2(), c);
}



void
MacroAssemblerARM::ma_tst(Register src1, Imm32 imm, Condition c)
{
    ma_alu(src1, imm, InvalidReg, op_tst, SetCond, c);
}
void
MacroAssemblerARM::ma_tst(Register src1, Register src2, Condition c)
{
    as_tst(src1, O2Reg(src2), c);
}
void
MacroAssemblerARM::ma_tst(Register src1, Operand op, Condition c)
{
    as_tst(src1, op.toOp2(), c);
}

void
MacroAssemblerARM::ma_mul(Register src1, Register src2, Register dest)
{
    as_mul(dest, src1, src2);
}
void
MacroAssemblerARM::ma_mul(Register src1, Imm32 imm, Register dest)
{

    ma_mov(imm, ScratchRegister);
    as_mul( dest, src1, ScratchRegister);
}

Assembler::Condition
MacroAssemblerARM::ma_check_mul(Register src1, Register src2, Register dest, Condition cond)
{
    
    
    if (cond == Equal || cond == NotEqual) {
        as_smull(ScratchRegister, dest, src1, src2, SetCond);
        return cond;
    } else if (cond == Overflow) {
        as_smull(ScratchRegister, dest, src1, src2);
        as_cmp(ScratchRegister, asr(dest, 31));
        return NotEqual;
    }
    JS_NOT_REACHED("Condition NYI");
    return Always;

}

Assembler::Condition
MacroAssemblerARM::ma_check_mul(Register src1, Imm32 imm, Register dest, Condition cond)
{
    ma_mov(imm, ScratchRegister);
    if (cond == Equal || cond == NotEqual) {
        as_smull(ScratchRegister, dest, ScratchRegister, src1, SetCond);
        return cond;
    } else if (cond == Overflow) {
        as_smull(ScratchRegister, dest, ScratchRegister, src1);
        as_cmp(ScratchRegister, asr(dest, 31));
        return NotEqual;
    }
    JS_NOT_REACHED("Condition NYI");
    return Always;
}

void
MacroAssemblerARM::ma_mod_mask(Register src, Register dest, Register hold, int32 shift)
{
    
    
    
    
    
    
    
    
    
    
    
    
    int32 mask = (1 << shift) - 1;
    Label head;

    
    
    
    
    

    
    
    as_mov(ScratchRegister, O2Reg(src), SetCond);
    
    ma_mov(Imm32(0), dest);
    
    ma_mov(Imm32(1), hold);
    ma_mov(Imm32(-1), hold, NoSetCond, Signed);
    ma_rsb(Imm32(0), ScratchRegister, SetCond, Signed);
    
    bind(&head);

    
    ma_and(Imm32(mask), ScratchRegister, lr);
    
    ma_add(lr, dest, dest);
    
    ma_sub(dest, Imm32(mask), lr, SetCond);
    
    ma_mov(lr, dest, NoSetCond, Unsigned);
    
    as_mov(ScratchRegister, lsr(ScratchRegister, shift), SetCond);
    
    ma_b(&head, NonZero);
    
    
    ma_cmp(hold, Imm32(0));
    
    
    ma_rsb(Imm32(0), dest, SetCond, Signed);
    
    

}



void
MacroAssemblerARM::ma_dtr(LoadStore ls, Register rn, Imm32 offset, Register rt,
                          Index mode, Assembler::Condition cc)
{
    ma_dataTransferN(ls, 32, true, rn, offset, rt, mode, cc);
}

void
MacroAssemblerARM::ma_dtr(LoadStore ls, Register rn, Register rm, Register rt,
                          Index mode, Assembler::Condition cc)
{
    JS_NOT_REACHED("Feature NYI");
}

void
MacroAssemblerARM::ma_str(Register rt, DTRAddr addr, Index mode, Condition cc)
{
    as_dtr(IsStore, 32, mode, rt, addr, cc);
}

void
MacroAssemblerARM::ma_dtr(LoadStore ls, Register rt, const Operand &addr, Index mode, Condition cc)
{
    ma_dataTransferN(ls, 32, true,
                     Register::FromCode(addr.base()), Imm32(addr.disp()),
                     rt, mode, cc);
}

void
MacroAssemblerARM::ma_str(Register rt, const Operand &addr, Index mode, Condition cc)
{
    ma_dtr(IsStore, rt, addr, mode, cc);
}
void
MacroAssemblerARM::ma_strd(Register rt, DebugOnly<Register> rt2, EDtrAddr addr, Index mode, Condition cc)
{
    JS_ASSERT((rt.code() & 1) == 0);
    JS_ASSERT(rt2.value.code() == rt.code() + 1);
    as_extdtr(IsStore, 64, true, mode, rt, addr, cc);
}

void
MacroAssemblerARM::ma_ldr(DTRAddr addr, Register rt, Index mode, Condition cc)
{
    as_dtr(IsLoad, 32, mode, rt, addr, cc);
}
void
MacroAssemblerARM::ma_ldr(const Operand &addr, Register rt, Index mode, Condition cc)
{
    ma_dtr(IsLoad, rt, addr, mode, cc);
}

void
MacroAssemblerARM::ma_ldrb(DTRAddr addr, Register rt, Index mode, Condition cc)
{
    as_dtr(IsLoad, 8, mode, rt, addr, cc);
}

void
MacroAssemblerARM::ma_ldrsh(EDtrAddr addr, Register rt, Index mode, Condition cc)
{
    as_extdtr(IsLoad, 16, true, mode, rt, addr, cc);
}

void
MacroAssemblerARM::ma_ldrh(EDtrAddr addr, Register rt, Index mode, Condition cc)
{
    as_extdtr(IsLoad, 16, false, mode, rt, addr, cc);
}
void
MacroAssemblerARM::ma_ldrsb(EDtrAddr addr, Register rt, Index mode, Condition cc)
{
    as_extdtr(IsLoad, 8, true, mode, rt, addr, cc);
}
void
MacroAssemblerARM::ma_ldrd(EDtrAddr addr, Register rt, DebugOnly<Register> rt2, Index mode, Condition cc)
{
    JS_ASSERT((rt.code() & 1) == 0);
    JS_ASSERT(rt2.value.code() == rt.code() + 1);
    as_extdtr(IsLoad, 64, true, mode, rt, addr, cc);
}
void
MacroAssemblerARM::ma_strh(Register rt, EDtrAddr addr, Index mode, Condition cc)
{
    as_extdtr(IsStore, 16, false, mode, rt, addr, cc);
}

void
MacroAssemblerARM::ma_strb(Register rt, DTRAddr addr, Index mode, Condition cc)
{
    as_dtr(IsStore, 8, mode, rt, addr, cc);
}


void
MacroAssemblerARM::ma_dataTransferN(LoadStore ls, int size, bool IsSigned,
                          Register rn, Register rm, Register rt,
                          Index mode, Assembler::Condition cc)
{
    JS_NOT_REACHED("Feature NYI");
}

void
MacroAssemblerARM::ma_dataTransferN(LoadStore ls, int size, bool IsSigned,
                                    Register rn, Imm32 offset, Register rt,
                                    Index mode, Assembler::Condition cc)
{
    int off = offset.value;
    
    if (size == 32 || (size == 8 && !IsSigned) ) {
        if (off < 4096 && off > -4096) {
            
            
            as_dtr(ls, size, mode, rt, DTRAddr(rn, DtrOffImm(off)), cc);
            return;
        }
        
        
        
        
        
        
        
        
        

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        

        if (rt == pc && mode == PostIndex && ls == IsLoad) {
            ma_mov(rn, ScratchRegister);
            ma_alu(rn, offset, rn, op_add);
            as_dtr(IsLoad, size, Offset, pc, DTRAddr(ScratchRegister, DtrOffImm(0)), cc);
            return;
        }
        int bottom = off & 0xfff;
        int neg_bottom = 0x1000 - bottom;
        
        
        Register base = ScratchRegister;
        
        
        if (mode == PreIndex)
            base = rn;
        JS_ASSERT(mode != PostIndex);
        
        
        if (off < 0) {
            Operand2 sub_off = Imm8(-(off-bottom)); 
            if (!sub_off.invalid) {
                as_sub(ScratchRegister, rn, sub_off, NoSetCond, cc); 
                as_dtr(ls, size, Offset, rt, DTRAddr(ScratchRegister, DtrOffImm(bottom)), cc);
                return;
            }
            sub_off = Imm8(-(off+neg_bottom));
            if (!sub_off.invalid) {
                as_sub(ScratchRegister, rn, sub_off, NoSetCond, cc); 
                as_dtr(ls, size, Offset, rt, DTRAddr(ScratchRegister, DtrOffImm(-neg_bottom)), cc);
                return;
            }
        } else {
            Operand2 sub_off = Imm8(off-bottom); 
            if (!sub_off.invalid) {
                as_add(ScratchRegister, rn, sub_off, NoSetCond, cc); 
                as_dtr(ls, size, Offset, rt, DTRAddr(ScratchRegister, DtrOffImm(bottom)), cc);
                return;
            }
            sub_off = Imm8(off+neg_bottom);
            if (!sub_off.invalid) {
                as_add(ScratchRegister, rn, sub_off, NoSetCond,  cc); 
                as_dtr(ls, size, Offset, rt, DTRAddr(ScratchRegister, DtrOffImm(-neg_bottom)), cc);
                return;
            }
        }
        ma_mov(offset, ScratchRegister);
        as_dtr(ls, size, mode, rt, DTRAddr(rn, DtrRegImmShift(ScratchRegister, LSL, 0)));
    } else {
        
        if (off < 256 && off > -256) {
            as_extdtr(ls, size, IsSigned, mode, rt, EDtrAddr(rn, EDtrOffImm(off)), cc);
            return;
        }
        
        
        int bottom = off & 0xff;
        int neg_bottom = 0x100 - bottom;
        
        
        if (off < 0) {
            Operand2 sub_off = Imm8(-(off-bottom)); 
            if (!sub_off.invalid) {
                as_sub(ScratchRegister, rn, sub_off, NoSetCond, cc); 
                as_extdtr(ls, size, IsSigned, Offset, rt,
                          EDtrAddr(ScratchRegister, EDtrOffImm(bottom)),
                          cc);
                return;
            }
            sub_off = Imm8(-(off+neg_bottom));
            if (!sub_off.invalid) {
                as_sub(ScratchRegister, rn, sub_off, NoSetCond, cc); 
                as_extdtr(ls, size, IsSigned, Offset, rt,
                          EDtrAddr(ScratchRegister, EDtrOffImm(-neg_bottom)),
                          cc);
                return;
            }
        } else {
            Operand2 sub_off = Imm8(off-bottom); 
            if (!sub_off.invalid) {
                as_add(ScratchRegister, rn, sub_off, NoSetCond, cc); 
                as_extdtr(ls, size, IsSigned, Offset, rt,
                          EDtrAddr(ScratchRegister, EDtrOffImm(bottom)),
                          cc);
                return;
            }
            sub_off = Imm8(off+neg_bottom);
            if (!sub_off.invalid) {
                as_add(ScratchRegister, rn, sub_off, NoSetCond,  cc); 
                as_extdtr(ls, size, IsSigned, Offset, rt,
                          EDtrAddr(ScratchRegister, EDtrOffImm(-neg_bottom)),
                          cc);
                return;
            }
        }
        ma_mov(offset, ScratchRegister);
        as_extdtr(ls, size, IsSigned, mode, rt, EDtrAddr(rn, EDtrOffReg(ScratchRegister)), cc);
    }
}
void
MacroAssemblerARM::ma_pop(Register r)
{
    ma_dtr(IsLoad, sp, Imm32(4), r, PostIndex);
    if (r == pc)
        m_buffer.markGuard();
}
void
MacroAssemblerARM::ma_push(Register r)
{
    ma_dtr(IsStore, sp,Imm32(-4), r, PreIndex);
}

void
MacroAssemblerARM::ma_vpop(VFPRegister r)
{
    startFloatTransferM(IsLoad, sp, IA, WriteBack);
    transferFloatReg(r);
    finishFloatTransfer();
}
void
MacroAssemblerARM::ma_vpush(VFPRegister r)
{
    startFloatTransferM(IsStore, sp, DB, WriteBack);
    transferFloatReg(r);
    finishFloatTransfer();
}


void
MacroAssemblerARM::ma_b(Label *dest, Assembler::Condition c)
{
    as_b(dest, c);
}

void
MacroAssemblerARM::ma_bx(Register dest, Assembler::Condition c)
{
    as_bx(dest, c);
}

Assembler::RelocBranchStyle
b_type()
{
    return Assembler::B_LDR;
}
void
MacroAssemblerARM::ma_b(void *target, Relocation::Kind reloc, Assembler::Condition c)
{
    
    
    
    
    uint32 trg = (uint32)target;
    switch (b_type()) {
      case Assembler::B_MOVWT:
        as_movw(ScratchRegister, Imm16(trg & 0xffff), c);
        as_movt(ScratchRegister, Imm16(trg >> 16), c);
        
        as_bx(ScratchRegister, c);
        break;
      case Assembler::B_LDR_BX:
        as_Imm32Pool(ScratchRegister, trg, NULL, c);
        as_bx(ScratchRegister, c);
        break;
      case Assembler::B_LDR:
        as_Imm32Pool(pc, trg, NULL, c);
        if (c == Always)
            m_buffer.markGuard();
        break;
      default:
        JS_NOT_REACHED("Other methods of generating tracable jumps NYI");
    }
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
MacroAssemblerARM::ma_vsub(FloatRegister src1, FloatRegister src2, FloatRegister dst)
{
    as_vsub(VFPRegister(dst), VFPRegister(src1), VFPRegister(src2));
}

void
MacroAssemblerARM::ma_vmul(FloatRegister src1, FloatRegister src2, FloatRegister dst)
{
    as_vmul(VFPRegister(dst), VFPRegister(src1), VFPRegister(src2));
}

void
MacroAssemblerARM::ma_vdiv(FloatRegister src1, FloatRegister src2, FloatRegister dst)
{
    as_vdiv(VFPRegister(dst), VFPRegister(src1), VFPRegister(src2));
}

void
MacroAssemblerARM::ma_vmov(FloatRegister src, FloatRegister dest)
{
    as_vmov(dest, src);
}

void
MacroAssemblerARM::ma_vneg(FloatRegister src, FloatRegister dest)
{
    as_vneg(dest, src);
}

void
MacroAssemblerARM::ma_vabs(FloatRegister src, FloatRegister dest)
{
    as_vabs(dest, src);
}

void
MacroAssemblerARM::ma_vimm(double value, FloatRegister dest)
{
    union DoublePun {
        struct {
#if defined(IS_LITTLE_ENDIAN) && !defined(FPU_IS_ARM_FPA)
            uint32_t lo, hi;
#else
            uint32_t hi, lo;
#endif
        } s;
        double d;
    } dpun;
    dpun.d = value;
    if ((dpun.s.lo) == 0) {
        if (dpun.s.hi == 0) {
            
            VFPImm dblEnc(0x3FF00000);
            as_vimm(dest, dblEnc);
            as_vsub(dest, dest, dest);
            return;
        }
        VFPImm dblEnc(dpun.s.hi);
        if (dblEnc.isValid()) {
            as_vimm(dest, dblEnc);
            return;
        }

    }
    
    as_FImm64Pool(dest, value);
}

void
MacroAssemblerARM::ma_vcmp(FloatRegister src1, FloatRegister src2)
{
    as_vcmp(VFPRegister(src1), VFPRegister(src2));
}
void
MacroAssemblerARM::ma_vcmpz(FloatRegister src1)
{
    as_vcmpz(VFPRegister(src1));
}

void
MacroAssemblerARM::ma_vcvt_F64_I32(FloatRegister src, FloatRegister dest)
{
    as_vcvt(VFPRegister(dest).sintOverlay(), VFPRegister(src));
}
void
MacroAssemblerARM::ma_vcvt_F64_U32(FloatRegister src, FloatRegister dest)
{
    as_vcvt(VFPRegister(dest).uintOverlay(), VFPRegister(src));
}
void
MacroAssemblerARM::ma_vcvt_I32_F64(FloatRegister dest, FloatRegister src)
{
    as_vcvt(VFPRegister(dest), VFPRegister(src).sintOverlay());
}
void
MacroAssemblerARM::ma_vcvt_U32_F64(FloatRegister dest, FloatRegister src)
{
    as_vcvt(VFPRegister(dest), VFPRegister(src).uintOverlay());
}

void
MacroAssemblerARM::ma_vxfer(FloatRegister src, Register dest)
{
    as_vxfer(dest, InvalidReg, VFPRegister(src).singleOverlay(), FloatToCore);
}

void
MacroAssemblerARM::ma_vxfer(FloatRegister src, Register dest1, Register dest2)
{
    as_vxfer(dest1, dest2, VFPRegister(src), FloatToCore);
}

void
MacroAssemblerARM::ma_vxfer(VFPRegister src, Register dest)
{
    as_vxfer(dest, InvalidReg, src, FloatToCore);
}

void
MacroAssemblerARM::ma_vxfer(VFPRegister src, Register dest1, Register dest2)
{
    as_vxfer(dest1, dest2, src, FloatToCore);
}

void
MacroAssemblerARM::ma_vdtr(LoadStore ls, const Operand &addr, VFPRegister rt, Condition cc)
{
    int off = addr.disp();
    JS_ASSERT((off & 3) == 0);
    Register base = Register::FromCode(addr.base());
    if (off > -1024 && off < 1024) {
        as_vdtr(ls, rt, addr.toVFPAddr(), cc);
        return;
    }
    
    
    int bottom = off & (0xff << 2);
    int neg_bottom = (0x100 << 2) - bottom;
    
    
    if (off < 0) {
        Operand2 sub_off = Imm8(-(off-bottom)); 
        if (!sub_off.invalid) {
            as_sub(ScratchRegister, base, sub_off, NoSetCond, cc); 
            as_vdtr(ls, rt, VFPAddr(ScratchRegister, VFPOffImm(bottom)), cc);
            return;
        }
        sub_off = Imm8(-(off+neg_bottom));
        if (!sub_off.invalid) {
            as_sub(ScratchRegister, base, sub_off, NoSetCond, cc); 
            as_vdtr(ls, rt, VFPAddr(ScratchRegister, VFPOffImm(-neg_bottom)), cc);
            return;
        }
    } else {
        Operand2 sub_off = Imm8(off-bottom); 
        if (!sub_off.invalid) {
            as_add(ScratchRegister, base, sub_off, NoSetCond, cc); 
            as_vdtr(ls, rt, VFPAddr(ScratchRegister, VFPOffImm(bottom)), cc);
            return;
        }
        sub_off = Imm8(off+neg_bottom);
        if (!sub_off.invalid) {
            as_add(ScratchRegister, base, sub_off, NoSetCond,  cc); 
            as_vdtr(ls, rt, VFPAddr(ScratchRegister, VFPOffImm(-neg_bottom)), cc);
            return;
        }
    }
    JS_NOT_REACHED("TODO: implement bigger offsets :(");

}

void
MacroAssemblerARM::ma_vldr(VFPAddr addr, VFPRegister dest)
{
    as_vdtr(IsLoad, dest, addr);
}
void
MacroAssemblerARM::ma_vldr(const Operand &addr, VFPRegister dest)
{
    ma_vdtr(IsLoad, addr, dest);
}

void
MacroAssemblerARM::ma_vstr(VFPRegister src, VFPAddr addr)
{
    as_vdtr(IsStore, src, addr);
}

void
MacroAssemblerARM::ma_vstr(VFPRegister src, const Operand &addr)
{
    ma_vdtr(IsStore, addr, src);
}
void
MacroAssemblerARM::ma_vstr(VFPRegister src, Register base, Register index, int32 shift)
{
    as_add(ScratchRegister, base, lsl(index, shift));
    ma_vstr(src, Operand(ScratchRegister, 0));
}

bool
MacroAssemblerARMCompat::buildFakeExitFrame(const Register &scratch, uint32 *offset)
{
    DebugOnly<uint32> initialDepth = framePushed();
    uint32 descriptor = MakeFrameDescriptor(framePushed(), IonFrame_JS);

    Push(Imm32(descriptor)); 

    enterNoPool();
    DebugOnly<uint32> offsetBeforePush = currentOffset();
    Push(pc); 

    
    
    
    ma_nop();
    uint32 pseudoReturnOffset = currentOffset();
    leaveNoPool();

    JS_ASSERT(framePushed() == initialDepth + IonExitFrameLayout::Size());
    JS_ASSERT(pseudoReturnOffset - offsetBeforePush == 8);

    *offset = pseudoReturnOffset;
    return true;
}

void
MacroAssemblerARMCompat::callWithExitFrame(IonCode *target)
{
    uint32 descriptor = MakeFrameDescriptor(framePushed(), IonFrame_JS);
    Push(Imm32(descriptor)); 

    addPendingJump(m_buffer.nextOffset(), target->raw(), Relocation::IONCODE);
    ma_mov(Imm32((int) target->raw()), ScratchRegister);
    ma_callIonHalfPush(ScratchRegister);
}

void
MacroAssemblerARMCompat::callIon(const Register &callee)
{
    JS_ASSERT((framePushed() & 3) == 0);
    if ((framePushed() & 7) == 4) {
        ma_callIonHalfPush(callee);
    } else {
        adjustFrame(sizeof(void*));
        ma_callIon(callee);
    }
}

void
MacroAssemblerARMCompat::reserveStack(uint32 amount)
{
    if (amount)
        ma_sub(Imm32(amount), sp);
    adjustFrame(amount);
}
void
MacroAssemblerARMCompat::freeStack(uint32 amount)
{
    JS_ASSERT(amount <= framePushed_);
    if (amount)
        ma_add(Imm32(amount), sp);
    adjustFrame(-amount);
}

void
MacroAssemblerARMCompat::add32(Imm32 imm, Register dest)
{
    ma_add(imm, dest, SetCond);
}

void
MacroAssemblerARMCompat::sub32(Imm32 imm, Register dest)
{
    ma_sub(imm, dest, SetCond);
}

void
MacroAssemblerARMCompat::and32(Imm32 imm, Register dest)
{
    ma_and(imm, dest, SetCond);
}

void
MacroAssemblerARMCompat::addPtr(Register src, Register dest)
{
    ma_add(src, dest);
}

void
MacroAssemblerARMCompat::and32(Imm32 imm, const Address &dest)
{
    load32(dest, ScratchRegister);
    ma_and(imm, ScratchRegister);
    store32(ScratchRegister, dest);
}

void
MacroAssemblerARMCompat::or32(Imm32 imm, const Address &dest)
{
    load32(dest, ScratchRegister);
    ma_orr(imm, ScratchRegister);
    store32(ScratchRegister, dest);
}

void
MacroAssemblerARMCompat::orPtr(Imm32 imm, Register dest)
{
    ma_orr(imm, dest);
}

void
MacroAssemblerARMCompat::move32(const Imm32 &imm, const Register &dest)
{
    ma_mov(imm, dest);
}
void
MacroAssemblerARMCompat::move32(const Address &src, const Register &dest)
{
    movePtr(src, dest);
}
void
MacroAssemblerARMCompat::movePtr(const Register &src, const Register &dest)
{
    ma_mov(src, dest);
}
void
MacroAssemblerARMCompat::movePtr(const ImmWord &imm, const Register &dest)
{
    ma_mov(Imm32(imm.value), dest);
}
void
MacroAssemblerARMCompat::movePtr(const ImmGCPtr &imm, const Register &dest)
{
    ma_mov(imm, dest);
}
void
MacroAssemblerARMCompat::movePtr(const Address &src, const Register &dest)
{
    loadPtr(src, dest);
}
void
MacroAssemblerARMCompat::load8ZeroExtend(const Address &address, const Register &dest)
{
    ma_dataTransferN(IsLoad, 8, false, address.base, Imm32(address.offset), dest);
}

void
MacroAssemblerARMCompat::load8ZeroExtend(const BaseIndex &src, const Register &dest)
{
    Register base = src.base;
    uint32 scale = Imm32::ShiftOf(src.scale).value;

    if (src.offset != 0) {
        ma_mov(base, ScratchRegister);
        base = ScratchRegister;
        ma_add(base, Imm32(src.offset), base);
    }
    ma_ldrb(DTRAddr(base, DtrRegImmShift(src.index, LSL, scale)), dest);

}

void
MacroAssemblerARMCompat::load8SignExtend(const Address &address, const Register &dest)
{
    ma_dataTransferN(IsLoad, 8, true, address.base, Imm32(address.offset), dest);
}

void
MacroAssemblerARMCompat::load8SignExtend(const BaseIndex &src, const Register &dest)
{
    Register index = src.index;

    
    if (src.scale != TimesOne) {
        ma_lsl(Imm32::ShiftOf(src.scale), index, ScratchRegister);
        index = ScratchRegister;
    }
    if (src.offset != 0) {
        if (index != ScratchRegister) {
            ma_mov(index, ScratchRegister);
            index = ScratchRegister;
        }
        ma_add(Imm32(src.offset), index);
    }
    ma_ldrsb(EDtrAddr(src.base, EDtrOffReg(index)), dest);
}

void
MacroAssemblerARMCompat::load16ZeroExtend(const Address &address, const Register &dest)
{
    ma_dataTransferN(IsLoad, 16, false, address.base, Imm32(address.offset), dest);
}

void
MacroAssemblerARMCompat::load16ZeroExtend_mask(const Address &address, Imm32 mask, const Register &dest)
{
    load16ZeroExtend(address, dest);
    ma_and(mask, dest, dest);
}

void
MacroAssemblerARMCompat::load16ZeroExtend(const BaseIndex &src, const Register &dest)
{
    Register index = src.index;

    
    if (src.scale != TimesOne) {
        ma_lsl(Imm32::ShiftOf(src.scale), index, ScratchRegister);
        index = ScratchRegister;
    }
    if (src.offset != 0) {
        if (index != ScratchRegister) {
            ma_mov(index, ScratchRegister);
            index = ScratchRegister;
        }
        ma_add(Imm32(src.offset), index);
    }
    ma_ldrh(EDtrAddr(src.base, EDtrOffReg(index)), dest);
}

void
MacroAssemblerARMCompat::load16SignExtend(const Address &address, const Register &dest)
{
    ma_dataTransferN(IsLoad, 16, true, address.base, Imm32(address.offset), dest);
}

void
MacroAssemblerARMCompat::load16SignExtend(const BaseIndex &src, const Register &dest)
{
    Register index = src.index;

    
    if (src.scale != TimesOne) {
        ma_lsl(Imm32::ShiftOf(src.scale), index, ScratchRegister);
        index = ScratchRegister;
    }
    if (src.offset != 0) {
        if (index != ScratchRegister) {
            ma_mov(index, ScratchRegister);
            index = ScratchRegister;
        }
        ma_add(Imm32(src.offset), index);
    }
    ma_ldrsh(EDtrAddr(src.base, EDtrOffReg(index)), dest);
}

void
MacroAssemblerARMCompat::load32(const Address &address, const Register &dest)
{
    loadPtr(address, dest);
}

void
MacroAssemblerARMCompat::load32(const BaseIndex &address, const Register &dest)
{
    loadPtr(address, dest);
}

void
MacroAssemblerARMCompat::load32(const AbsoluteAddress &address, const Register &dest)
{
    loadPtr(address, dest);
}
void
MacroAssemblerARMCompat::loadPtr(const Address &address, const Register &dest)
{
    ma_ldr(Operand(address), dest);
}

void
MacroAssemblerARMCompat::loadPtr(const BaseIndex &src, const Register &dest)
{
    Register base = src.base;
    uint32 scale = Imm32::ShiftOf(src.scale).value;

    if (src.offset != 0) {
        ma_mov(base, ScratchRegister);
        base = ScratchRegister;
        ma_add(Imm32(src.offset), base);
    }
    ma_ldr(DTRAddr(base, DtrRegImmShift(src.index, LSL, scale)), dest);
}
void
MacroAssemblerARMCompat::loadPtr(const AbsoluteAddress &address, const Register &dest)
{
    movePtr(ImmWord(address.addr), ScratchRegister);
    loadPtr(Address(ScratchRegister, 0x0), dest);
}

Operand payloadOf(const Address &address) {
    return Operand(address.base, address.offset);
}
Operand tagOf(const Address &address) {
    return Operand(address.base, address.offset + 4);
}

void
MacroAssemblerARMCompat::loadPrivate(const Address &address, const Register &dest)
{
    ma_ldr(payloadOf(address), dest);
}

void
MacroAssemblerARMCompat::loadDouble(const Address &address, const FloatRegister &dest)
{
    ma_vldr(Operand(address), dest);
}

void
MacroAssemblerARMCompat::loadDouble(const BaseIndex &src, const FloatRegister &dest)
{
    
    
    Register base = src.base;
    Register index = src.index;
    uint32 scale = Imm32::ShiftOf(src.scale).value;
    int32 offset = src.offset;
    as_add(ScratchRegister, base, lsl(index, scale));

    ma_vldr(Operand(ScratchRegister, offset), dest);
}

void
MacroAssemblerARMCompat::loadFloatAsDouble(const Address &address, const FloatRegister &dest)
{
    VFPRegister rt = dest;
    ma_vdtr(IsLoad, address, rt.singleOverlay());
    as_vcvt(rt, rt.singleOverlay());
}

void
MacroAssemblerARMCompat::loadFloatAsDouble(const BaseIndex &src, const FloatRegister &dest)
{
    
    
    Register base = src.base;
    Register index = src.index;
    uint32 scale = Imm32::ShiftOf(src.scale).value;
    int32 offset = src.offset;
    VFPRegister rt = dest;
    as_add(ScratchRegister, base, lsl(index, scale));

    ma_vdtr(IsLoad, Operand(ScratchRegister, offset), rt.singleOverlay());
    as_vcvt(rt, rt.singleOverlay());
}

void
MacroAssemblerARMCompat::store8(const Imm32 &imm, const Address &address)
{
    ma_mov(imm, lr);
    store8(lr, address);
}

void
MacroAssemblerARMCompat::store8(const Register &src, const Address &address)
{
    ma_dataTransferN(IsStore, 8, false, address.base, Imm32(address.offset), src);
}

void
MacroAssemblerARMCompat::store8(const Imm32 &imm, const BaseIndex &dest)
{
    ma_mov(imm, lr);
    store8(lr, dest);
}

void
MacroAssemblerARMCompat::store8(const Register &src, const BaseIndex &dest)
{
    Register base = dest.base;
    uint32 scale = Imm32::ShiftOf(dest.scale).value;

    if (dest.offset != 0) {
        ma_add(base, Imm32(dest.offset), ScratchRegister);
        base = ScratchRegister;
    }
    ma_strb(src, DTRAddr(base, DtrRegImmShift(dest.index, LSL, scale)));
}

void
MacroAssemblerARMCompat::store16(const Imm32 &imm, const Address &address)
{
    ma_mov(imm, lr);
    store16(lr, address);
}

void
MacroAssemblerARMCompat::store16(const Register &src, const Address &address)
{
    ma_dataTransferN(IsStore, 16, false, address.base, Imm32(address.offset), src);
}

void
MacroAssemblerARMCompat::store16(const Imm32 &imm, const BaseIndex &dest)
{
    ma_mov(imm, lr);
    store16(lr, dest);
}
void
MacroAssemblerARMCompat::store16(const Register &src, const BaseIndex &address)
{
    Register index = address.index;

    
    if (address.scale != TimesOne) {
        ma_lsl(Imm32::ShiftOf(address.scale), index, ScratchRegister);
        index = ScratchRegister;
    }
    if (address.offset != 0) {
        ma_add(index, Imm32(address.offset), ScratchRegister);
        index = ScratchRegister;
    }
    ma_strh(src, EDtrAddr(address.base, EDtrOffReg(index)));
}
void
MacroAssemblerARMCompat::store32(const Register &src, const AbsoluteAddress &address)
{
    storePtr(src, address);
}

void
MacroAssemblerARMCompat::store32(const Register &src, const Address &address)
{
    storePtr(src, address);
}

void
MacroAssemblerARMCompat::store32(const Imm32 &src, const Address &address)
{
    move32(src, ScratchRegister);
    storePtr(ScratchRegister, address);
}

void
MacroAssemblerARMCompat::store32(const Imm32 &imm, const BaseIndex &dest)
{
    ma_mov(imm, lr);
    store32(lr, dest);
}

void
MacroAssemblerARMCompat::store32(const Register &src, const BaseIndex &dest)
{
    Register base = dest.base;
    uint32 scale = Imm32::ShiftOf(dest.scale).value;

    if (dest.offset != 0) {
        ma_add(base, Imm32(dest.offset), ScratchRegister);
        base = ScratchRegister;
    }
    ma_str(src, DTRAddr(base, DtrRegImmShift(dest.index, LSL, scale)));
}

void
MacroAssemblerARMCompat::storePtr(ImmWord imm, const Address &address)
{
    movePtr(imm, ScratchRegister);
    storePtr(ScratchRegister, address);
}
    
void
MacroAssemblerARMCompat::storePtr(ImmGCPtr imm, const Address &address)
{
    movePtr(imm, ScratchRegister);
    storePtr(ScratchRegister, address);
}

void
MacroAssemblerARMCompat::storePtr(Register src, const Address &address)
{
    ma_str(src, Operand(address));
}

void
MacroAssemblerARMCompat::storePtr(const Register &src, const AbsoluteAddress &dest)
{
    movePtr(ImmWord(dest.addr), ScratchRegister);
    storePtr(src, Address(ScratchRegister, 0x0));
}

void
MacroAssemblerARMCompat::cmp32(const Register &lhs, const Imm32 &rhs)
{
    JS_ASSERT(lhs != ScratchRegister);
    ma_cmp(lhs, rhs);
}

void
MacroAssemblerARMCompat::cmp32(const Register &lhs, const Register &rhs)
{
    ma_cmp(lhs, rhs);
}

void
MacroAssemblerARMCompat::cmpPtr(const Register &lhs, const ImmWord &rhs)
{
    JS_ASSERT(lhs != ScratchRegister);
    ma_cmp(lhs, Imm32(rhs.value));
}

void
MacroAssemblerARMCompat::cmpPtr(const Register &lhs, const Register &rhs)
{
    ma_cmp(lhs, rhs);
}

void
MacroAssemblerARMCompat::cmpPtr(const Register &lhs, const ImmGCPtr &rhs)
{
    ma_cmp(lhs, rhs);
}

void
MacroAssemblerARMCompat::cmpPtr(const Address &lhs, const Register &rhs)
{
    loadPtr(lhs, ScratchRegister);
    cmpPtr(ScratchRegister, rhs);
}

void
MacroAssemblerARMCompat::cmpPtr(const Address &lhs, const ImmWord &rhs)
{
    loadPtr(lhs, lr);
    ma_cmp(lr, Imm32(rhs.value));
}

void
MacroAssemblerARMCompat::setStackArg(const Register &reg, uint32 arg)
{
    ma_dataTransferN(IsStore, 32, true, sp, Imm32(arg * STACK_SLOT_SIZE), reg);

}

void
MacroAssemblerARMCompat::subPtr(Imm32 imm, const Register dest)
{
    ma_sub(imm, dest);
}

void
MacroAssemblerARMCompat::addPtr(Imm32 imm, const Register dest)
{
    ma_add(imm, dest);
}

void
MacroAssemblerARMCompat::addPtr(Imm32 imm, const Address &dest)
{
    loadPtr(dest, ScratchRegister);
    addPtr(imm, ScratchRegister);
    storePtr(ScratchRegister, dest);
}

void
MacroAssemblerARMCompat::compareDouble(FloatRegister lhs, FloatRegister rhs)
{
    if (rhs == InvalidFloatReg)
        ma_vcmpz(lhs);
    else
        ma_vcmp(lhs, rhs);
    as_vmrs(pc);
}

void
MacroAssemblerARMCompat::branchDouble(DoubleCondition cond, const FloatRegister &lhs,
                                      const FloatRegister &rhs, Label *label)
{
    compareDouble(lhs, rhs);

    if (cond == DoubleNotEqual) {
        
        Label unordered;
        ma_b(&unordered, VFP_Unordered);
        ma_b(label, VFP_NotEqualOrUnordered);
        bind(&unordered);
        return;
    }
    if (cond == DoubleEqualOrUnordered) {
        ma_b(label, VFP_Unordered);
        ma_b(label, VFP_Equal);
        return;
    }

    ma_b(label, ConditionFromDoubleCondition(cond));
}


Operand ToPayload(Operand base) {
    return Operand(Register::FromCode(base.base()),
                   base.disp());
}
Operand ToType(Operand base) {
    return Operand(Register::FromCode(base.base()),
                   base.disp() + sizeof(void *));
}

Assembler::Condition
MacroAssemblerARMCompat::testInt32(Assembler::Condition cond, const ValueOperand &value)
{
    JS_ASSERT(cond == Assembler::Equal || cond == Assembler::NotEqual);
    ma_cmp(value.typeReg(), ImmType(JSVAL_TYPE_INT32));
    return cond;
}

Assembler::Condition
MacroAssemblerARMCompat::testBoolean(Assembler::Condition cond, const ValueOperand &value)
{
    JS_ASSERT(cond == Assembler::Equal || cond == Assembler::NotEqual);
    ma_cmp(value.typeReg(), ImmType(JSVAL_TYPE_BOOLEAN));
    return cond;
}
Assembler::Condition
MacroAssemblerARMCompat::testDouble(Assembler::Condition cond, const ValueOperand &value)
{
    JS_ASSERT(cond == Assembler::Equal || cond == Assembler::NotEqual);
    Assembler::Condition actual = (cond == Equal)
        ? Below
        : AboveOrEqual;
    ma_cmp(value.typeReg(), ImmTag(JSVAL_TAG_CLEAR));
    return actual;
}

Assembler::Condition
MacroAssemblerARMCompat::testNull(Assembler::Condition cond, const ValueOperand &value)
{
    JS_ASSERT(cond == Assembler::Equal || cond == Assembler::NotEqual);
    ma_cmp(value.typeReg(), ImmType(JSVAL_TYPE_NULL));
    return cond;
}

Assembler::Condition
MacroAssemblerARMCompat::testUndefined(Assembler::Condition cond, const ValueOperand &value)
{
    JS_ASSERT(cond == Assembler::Equal || cond == Assembler::NotEqual);
    ma_cmp(value.typeReg(), ImmType(JSVAL_TYPE_UNDEFINED));
    return cond;
}

Assembler::Condition
MacroAssemblerARMCompat::testString(Assembler::Condition cond, const ValueOperand &value)
{
    return testString(cond, value.typeReg());
}

Assembler::Condition
MacroAssemblerARMCompat::testObject(Assembler::Condition cond, const ValueOperand &value)
{
    return testObject(cond, value.typeReg());
}

Assembler::Condition
MacroAssemblerARMCompat::testMagic(Assembler::Condition cond, const ValueOperand &value)
{
    return testMagic(cond, value.typeReg());
}

Assembler::Condition
MacroAssemblerARMCompat::testPrimitive(Assembler::Condition cond, const ValueOperand &value)
{
    return testPrimitive(cond, value.typeReg());
}


Assembler::Condition
MacroAssemblerARMCompat::testInt32(Assembler::Condition cond, const Register &tag)
{
    JS_ASSERT(cond == Equal || cond == NotEqual);
    ma_cmp(tag, ImmTag(JSVAL_TAG_INT32));
    return cond;
}

Assembler::Condition
MacroAssemblerARMCompat::testBoolean(Assembler::Condition cond, const Register &tag)
{
    JS_ASSERT(cond == Equal || cond == NotEqual);
    ma_cmp(tag, ImmTag(JSVAL_TAG_BOOLEAN));
    return cond;
}

Assembler::Condition
MacroAssemblerARMCompat::testNull(Assembler::Condition cond, const Register &tag) {
    JS_ASSERT(cond == Equal || cond == NotEqual);
    ma_cmp(tag, ImmTag(JSVAL_TAG_NULL));
    return cond;
}

Assembler::Condition
MacroAssemblerARMCompat::testUndefined(Assembler::Condition cond, const Register &tag) {
    JS_ASSERT(cond == Equal || cond == NotEqual);
    ma_cmp(tag, ImmTag(JSVAL_TAG_UNDEFINED));
    return cond;
}

Assembler::Condition
MacroAssemblerARMCompat::testString(Assembler::Condition cond, const Register &tag) {
    JS_ASSERT(cond == Equal || cond == NotEqual);
    ma_cmp(tag, ImmTag(JSVAL_TAG_STRING));
    return cond;
}

Assembler::Condition
MacroAssemblerARMCompat::testObject(Assembler::Condition cond, const Register &tag)
{
    JS_ASSERT(cond == Equal || cond == NotEqual);
    ma_cmp(tag, ImmTag(JSVAL_TAG_OBJECT));
    return cond;
}

Assembler::Condition
MacroAssemblerARMCompat::testMagic(Assembler::Condition cond, const Register &tag)
{
    JS_ASSERT(cond == Equal || cond == NotEqual);
    ma_cmp(tag, ImmTag(JSVAL_TAG_MAGIC));
    return cond;
}

Assembler::Condition
MacroAssemblerARMCompat::testPrimitive(Assembler::Condition cond, const Register &tag)
{
    JS_ASSERT(cond == Equal || cond == NotEqual);
    ma_cmp(tag, ImmTag(JSVAL_UPPER_EXCL_TAG_OF_PRIMITIVE_SET));
    return cond == Equal ? Below : AboveOrEqual;
}

Assembler::Condition
MacroAssemblerARMCompat::testGCThing(Assembler::Condition cond, const Address &address)
{
    JS_ASSERT(cond == Equal || cond == NotEqual);
    extractTag(address, ScratchRegister);
    ma_cmp(ScratchRegister, ImmTag(JSVAL_LOWER_INCL_TAG_OF_GCTHING_SET));
    return cond;
}

Assembler::Condition
MacroAssemblerARMCompat::testGCThing(Assembler::Condition cond, const BaseIndex &address)
{
    JS_ASSERT(cond == Equal || cond == NotEqual);
    extractTag(address, ScratchRegister);
    ma_cmp(ScratchRegister, ImmTag(JSVAL_LOWER_INCL_TAG_OF_GCTHING_SET));
    return cond;
}

Assembler::Condition
MacroAssemblerARMCompat::testMagic(Assembler::Condition cond, const Address &address)
{
    JS_ASSERT(cond == Equal || cond == NotEqual);
    extractTag(address, ScratchRegister);
    ma_cmp(ScratchRegister, ImmTag(JSVAL_TAG_MAGIC));
    return cond;
}

Assembler::Condition
MacroAssemblerARMCompat::testMagic(Assembler::Condition cond, const BaseIndex &address)
{
    JS_ASSERT(cond == Equal || cond == NotEqual);
    extractTag(address, ScratchRegister);
    ma_cmp(ScratchRegister, ImmTag(JSVAL_TAG_MAGIC));
    return cond;
}

Assembler::Condition
MacroAssemblerARMCompat::testDouble(Condition cond, const Register &tag)
{
    JS_ASSERT(cond == Assembler::Equal || cond == Assembler::NotEqual);
    Condition actual = (cond == Equal) ? Below : AboveOrEqual;
    ma_cmp(tag, ImmTag(JSVAL_TAG_CLEAR));
    return actual;
}

Assembler::Condition
MacroAssemblerARMCompat::testNumber(Condition cond, const Register &tag)
{
    JS_ASSERT(cond == Equal || cond == NotEqual);
    ma_cmp(tag, ImmTag(JSVAL_UPPER_INCL_TAG_OF_NUMBER_SET));
    return cond == Equal ? BelowOrEqual : Above;
}

void
MacroAssemblerARMCompat::branchTestValue(Condition cond, const ValueOperand &value, const Value &v,
                                         Label *label)
{
    
    
    
    
    
    
    
    jsval_layout jv = JSVAL_TO_IMPL(v);
    if (v.isMarkable())
        ma_cmp(value.payloadReg(), ImmGCPtr(reinterpret_cast<gc::Cell *>(v.toGCThing())));
    else
        ma_cmp(value.payloadReg(), Imm32(jv.s.payload.i32));
    ma_cmp(value.typeReg(), Imm32(jv.s.tag), Equal);
    ma_b(label, cond);
}


void
MacroAssemblerARMCompat::unboxInt32(const ValueOperand &operand, const Register &dest)
{
    ma_mov(operand.payloadReg(), dest);
}

void
MacroAssemblerARMCompat::unboxInt32(const Address &src, const Register &dest)
{
    ma_ldr(payloadOf(src), dest);
}

void
MacroAssemblerARMCompat::unboxBoolean(const ValueOperand &operand, const Register &dest)
{
    ma_mov(operand.payloadReg(), dest);
}

void
MacroAssemblerARMCompat::unboxDouble(const ValueOperand &operand, const FloatRegister &dest)
{
    JS_ASSERT(dest != ScratchFloatReg);
    as_vxfer(operand.payloadReg(), operand.typeReg(),
             VFPRegister(dest), CoreToFloat);
}

void
MacroAssemblerARMCompat::unboxValue(const ValueOperand &src, AnyRegister dest)
{
    if (dest.isFloat()) {
        Label notInt32, end;
        branchTestInt32(Assembler::NotEqual, src, &notInt32);
        convertInt32ToDouble(src.payloadReg(), dest.fpu());
        ma_b(&end);
        bind(&notInt32);
        unboxDouble(src, dest.fpu());
        bind(&end);
    } else {
        if (src.payloadReg() != dest.gpr())
            as_mov(dest.gpr(), O2Reg(src.payloadReg()));
    }
}

void
MacroAssemblerARMCompat::unboxPrivate(const ValueOperand &src, Register dest)
{
    ma_mov(src.payloadReg(), dest);
}

void
MacroAssemblerARMCompat::boxDouble(const FloatRegister &src, const ValueOperand &dest)
{
    as_vxfer(dest.payloadReg(), dest.typeReg(),
             VFPRegister(src), FloatToCore);
}


void
MacroAssemblerARMCompat::boolValueToDouble(const ValueOperand &operand, const FloatRegister &dest)
{
    VFPRegister d = VFPRegister(dest);
    ma_vimm(1.0, dest);
    ma_cmp(operand.payloadReg(), Imm32(0));
    
    as_vsub(d, d, d, Equal);
}

void
MacroAssemblerARMCompat::int32ValueToDouble(const ValueOperand &operand, const FloatRegister &dest)
{
    
    VFPRegister vfpdest = VFPRegister(dest);
    as_vxfer(operand.payloadReg(), InvalidReg,
             vfpdest.sintOverlay(), CoreToFloat);
    
    as_vcvt(vfpdest, vfpdest.sintOverlay());
}

void
MacroAssemblerARMCompat::loadInt32OrDouble(const Operand &src, const FloatRegister &dest)
{
    Label notInt32, end;
    
    ma_ldr(ToType(src), ScratchRegister);
    branchTestInt32(Assembler::NotEqual, ScratchRegister, &notInt32);
    ma_ldr(ToPayload(src), ScratchRegister);
    convertInt32ToDouble(ScratchRegister, dest);
    ma_b(&end);

    
    bind(&notInt32);
    ma_vldr(src, dest);
    bind(&end);
}

void
MacroAssemblerARMCompat::loadInt32OrDouble(Register base, Register index, const FloatRegister &dest, int32 shift)
{
    Label notInt32, end;

    JS_STATIC_ASSERT(NUNBOX32_PAYLOAD_OFFSET == 0);

    
    ma_alu(base, lsl(index, shift), ScratchRegister, op_add);

    
    ma_ldr(Address(ScratchRegister, NUNBOX32_TYPE_OFFSET), ScratchRegister);
    branchTestInt32(Assembler::NotEqual, ScratchRegister, &notInt32);

    
    ma_ldr(DTRAddr(base, DtrRegImmShift(index, LSL, shift)), ScratchRegister);
    convertInt32ToDouble(ScratchRegister, dest);
    ma_b(&end);

    
    bind(&notInt32);
    
    
    ma_alu(base, lsl(index, shift), ScratchRegister, op_add);
    ma_vldr(Address(ScratchRegister, 0), dest);
    bind(&end);
}

void
MacroAssemblerARMCompat::loadConstantDouble(double dp, const FloatRegister &dest)
{
    as_FImm64Pool(dest, dp);
}

void
MacroAssemblerARMCompat::loadStaticDouble(const double *dp, const FloatRegister &dest)
{
    loadConstantDouble(*dp, dest);
}
    

Assembler::Condition
MacroAssemblerARMCompat::testInt32Truthy(bool truthy, const ValueOperand &operand)
{
    ma_tst(operand.payloadReg(), operand.payloadReg());
    return truthy ? NonZero : Zero;
}

Assembler::Condition
MacroAssemblerARMCompat::testBooleanTruthy(bool truthy, const ValueOperand &operand)
{
    ma_tst(operand.payloadReg(), operand.payloadReg());
    return truthy ? NonZero : Zero;
}

Assembler::Condition
MacroAssemblerARMCompat::testDoubleTruthy(bool truthy, const FloatRegister &reg)
{
    as_vcmpz(VFPRegister(reg));
    as_vmrs(pc);
    as_cmp(r0, O2Reg(r0), Overflow);
    return truthy ? NonZero : Zero;
}

Register
MacroAssemblerARMCompat::extractObject(const Address &address, Register scratch)
{
    ma_ldr(payloadOf(address), scratch);
    return scratch;
}

Register
MacroAssemblerARMCompat::extractTag(const Address &address, Register scratch)
{
    ma_ldr(tagOf(address), scratch);
    return scratch;
}

Register
MacroAssemblerARMCompat::extractTag(const BaseIndex &address, Register scratch)
{
    ma_alu(address.base, lsl(address.index, address.scale), scratch, op_add, NoSetCond);
    return extractTag(Address(scratch, address.offset), scratch);
}

void
MacroAssemblerARMCompat::moveValue(const Value &val, Register type, Register data) {
    jsval_layout jv = JSVAL_TO_IMPL(val);
    ma_mov(Imm32(jv.s.tag), type);
    ma_mov(Imm32(jv.s.payload.i32), data);
}
void
MacroAssemblerARMCompat::moveValue(const Value &val, const ValueOperand &dest) {
    moveValue(val, dest.typeReg(), dest.payloadReg());
}




void
MacroAssemblerARMCompat::storeValue(ValueOperand val, Operand dst) {
    ma_str(val.payloadReg(), ToPayload(dst));
    ma_str(val.typeReg(), ToType(dst));
}

void
MacroAssemblerARMCompat::storeValue(ValueOperand val, Register base, Register index, int32 shift)
{
    if (isValueDTRDCandidate(val)) {
        Register tmpIdx;
        if (shift == 0) {
            tmpIdx = index;
        } else if (shift < 0) {
            ma_asr(Imm32(-shift), index, ScratchRegister);
            tmpIdx = ScratchRegister;
        } else {
            ma_lsl(Imm32(shift), index, ScratchRegister);
            tmpIdx = ScratchRegister;
        }
        ma_strd(val.payloadReg(), val.typeReg(), EDtrAddr(base, EDtrOffReg(tmpIdx)));
    } else {
        
        
        
        
        
        ma_alu(base, lsl(index, shift), ScratchRegister, op_add);

        
        storeValue(val, Address(ScratchRegister, 0));
    }
}

void
MacroAssemblerARMCompat::loadValue(Register base, Register index, ValueOperand val, Imm32 off)
{

    ma_alu(base, lsl(index, TimesEight), ScratchRegister, op_add);

    
    loadValue(Address(ScratchRegister, off.value), val);
}

void
MacroAssemblerARMCompat::loadValue(Address src, ValueOperand val)
{
    Operand srcOp = Operand(src);
    Operand payload = ToPayload(srcOp);
    Operand type = ToType(srcOp);
    
    if (isValueDTRDCandidate(val)) {
        
        
        int offset = srcOp.disp();
        if (offset < 256 && offset > -256) {
            ma_ldrd(EDtrAddr(Register::FromCode(srcOp.base()), EDtrOffImm(srcOp.disp())), val.payloadReg(), val.typeReg());
            return;
        }
    }
    

    if (val.payloadReg().code() < val.typeReg().code()) {
        if (srcOp.disp() <= 4 && srcOp.disp() >= -8 && (srcOp.disp() & 3) == 0) {
            
            
            DTMMode mode;
            switch(srcOp.disp()) {
              case -8:
                mode = DB;
                break;
              case -4:
                mode = DA;
                break;
              case 0:
                mode = IA;
                break;
              case 4:
                mode = IB;
                break;
              default:
                JS_NOT_REACHED("Bogus Offset for LoadValue as DTM");
            }
            startDataTransferM(IsLoad, Register::FromCode(srcOp.base()), mode);
            transferReg(val.payloadReg());
            transferReg(val.typeReg());
            finishDataTransfer();
            return;
        }
    }
    
    
    if (Register::FromCode(type.base()) != val.payloadReg()) {
        ma_ldr(payload, val.payloadReg());
        ma_ldr(type, val.typeReg());
    } else {
        ma_ldr(type, val.typeReg());
        ma_ldr(payload, val.payloadReg());
    }
}

void
MacroAssemblerARMCompat::tagValue(JSValueType type, Register payload, ValueOperand dest)
{
    JS_ASSERT(payload != dest.typeReg());
    ma_mov(ImmType(type), dest.typeReg());
    if (payload != dest.payloadReg())
        ma_mov(payload, dest.payloadReg());
}

void
MacroAssemblerARMCompat::pushValue(ValueOperand val) {
    ma_push(val.typeReg());
    ma_push(val.payloadReg());
}
void
MacroAssemblerARMCompat::popValue(ValueOperand val) {
    ma_pop(val.payloadReg());
    ma_pop(val.typeReg());
}
void
MacroAssemblerARMCompat::storePayload(const Value &val, Operand dest)
{
    jsval_layout jv = JSVAL_TO_IMPL(val);
    if (val.isMarkable()) {
        ma_mov(ImmGCPtr((gc::Cell *)jv.s.payload.ptr), lr);
    } else {
        ma_mov(Imm32(jv.s.payload.i32), lr);
    }
    ma_str(lr, ToPayload(dest));
}
void
MacroAssemblerARMCompat::storePayload(Register src, Operand dest)
{
    if (dest.getTag() == Operand::MEM) {
        ma_str(src, ToPayload(dest));
        return;
    }
    JS_NOT_REACHED("why do we do all of these things?");

}

void
MacroAssemblerARMCompat::storePayload(const Value &val, Register base, Register index, int32 shift)
{
    jsval_layout jv = JSVAL_TO_IMPL(val);
    if (val.isMarkable()) {
        ma_mov(ImmGCPtr((gc::Cell *)jv.s.payload.ptr), ScratchRegister);
    } else {
        ma_mov(Imm32(jv.s.payload.i32), ScratchRegister);
    }
    JS_STATIC_ASSERT(NUNBOX32_PAYLOAD_OFFSET == 0);
    
    
    as_dtr(IsStore, 32, Offset, ScratchRegister, DTRAddr(base, DtrRegImmShift(index, LSL, shift)));
}
void
MacroAssemblerARMCompat::storePayload(Register src, Register base, Register index, int32 shift)
{
    JS_ASSERT((shift < 32) && (shift >= 0));
    
    
    JS_STATIC_ASSERT(NUNBOX32_PAYLOAD_OFFSET == 0);
    
    
    as_dtr(IsStore, 32, Offset, src, DTRAddr(base, DtrRegImmShift(index, LSL, shift)));
}

void
MacroAssemblerARMCompat::storeTypeTag(ImmTag tag, Operand dest) {
    if (dest.getTag() == Operand::MEM) {
        ma_mov(tag, lr);
        ma_str(lr, ToType(dest));
        return;
    }

    JS_NOT_REACHED("why do we do all of these things?");

}

void
MacroAssemblerARMCompat::storeTypeTag(ImmTag tag, Register base, Register index, int32 shift) {
    JS_ASSERT(base != ScratchRegister);
    JS_ASSERT(index != ScratchRegister);
    
    
    
    
    
    
    ma_add(base, Imm32(NUNBOX32_TYPE_OFFSET), base);
    ma_mov(tag, ScratchRegister);
    ma_str(ScratchRegister, DTRAddr(base, DtrRegImmShift(index, LSL, shift)));
    ma_sub(base, Imm32(NUNBOX32_TYPE_OFFSET), base);
}

void
MacroAssemblerARMCompat::linkExitFrame() {
    uint8 *dest = ((uint8*)GetIonContext()->cx->runtime) + offsetof(JSRuntime, ionTop);
    movePtr(ImmWord(dest), ScratchRegister);
    ma_str(StackPointer, Operand(ScratchRegister, 0));
}










void
MacroAssemblerARM::ma_callIon(const Register r)
{
    
    
    
    AutoForbidPools afp(this);
    as_dtr(IsStore, 32, PreIndex, pc, DTRAddr(sp, DtrOffImm(-8)));
    as_blx(r);
}
void
MacroAssemblerARM::ma_callIonNoPush(const Register r)
{
    
    
    AutoForbidPools afp(this);
    as_dtr(IsStore, 32, Offset, pc, DTRAddr(sp, DtrOffImm(0)));
    as_blx(r);
}

void
MacroAssemblerARM::ma_callIonHalfPush(const Register r)
{
    
    
    
    AutoForbidPools afp(this);
    ma_push(pc);
    as_blx(r);
}

void
MacroAssemblerARM::ma_call(void *dest)
{
    ma_mov(Imm32((uint32)dest), CallReg);
    as_blx(CallReg);
}

void
MacroAssemblerARMCompat::breakpoint()
{
    as_bkpt();
}

void
MacroAssemblerARMCompat::setupABICall(uint32 args)
{
    JS_ASSERT(!inCall_);
    inCall_ = true;
    args_ = args;
    passedArgs_ = 0;
    usedSlots_ = 0;
    floatArgsInGPR[0] = VFPRegister();
    floatArgsInGPR[1] = VFPRegister();
}

void
MacroAssemblerARMCompat::setupAlignedABICall(uint32 args)
{
    setupABICall(args);

    dynamicAlignment_ = false;
}

void
MacroAssemblerARMCompat::setupUnalignedABICall(uint32 args, const Register &scratch)
{
    setupABICall(args);
    dynamicAlignment_ = true;

    ma_mov(sp, scratch);

    
    ma_and(Imm32(~(StackAlignment - 1)), sp, sp);
    ma_push(scratch);
}

void
MacroAssemblerARMCompat::passABIArg(const MoveOperand &from)
{
    MoveOperand to;
    uint32 increment = 1;
    bool useResolver = true;
    ++passedArgs_;
    Move::Kind kind = Move::GENERAL;
    if (from.isDouble()) {
        
        
        usedSlots_ = (usedSlots_ + 1) & ~1;
        increment = 2;
        kind = Move::DOUBLE;
    }

    Register destReg;
    MoveOperand dest;
    if (GetArgReg(usedSlots_, &destReg)) {
        if (from.isDouble()) {
            floatArgsInGPR[destReg.code() >> 1] = VFPRegister(from.floatReg());
            useResolver = false;
        } else {
            dest = MoveOperand(destReg);
        }
    } else {
        uint32 disp = GetArgStackDisp(usedSlots_);
        dest = MoveOperand(sp, disp);
    }

    if (useResolver)
        enoughMemory_ = enoughMemory_ && moveResolver_.addMove(from, dest, kind);
    usedSlots_ += increment;
}

void
MacroAssemblerARMCompat::passABIArg(const Register &reg)
{
    passABIArg(MoveOperand(reg));
}

void
MacroAssemblerARMCompat::passABIArg(const FloatRegister &freg)
{
    passABIArg(MoveOperand(freg));
}

void MacroAssemblerARMCompat::checkStackAlignment()
{
#ifdef DEBUG
        Label good;
        ma_tst(sp, Imm32(StackAlignment - 1));
        ma_b(&good, Equal);
        breakpoint();
        bind(&good);
#endif
}

void
MacroAssemblerARMCompat::callWithABI(void *fun, Result result)
{
    JS_ASSERT(inCall_);
    uint32 stackAdjust = ((usedSlots_ > NumArgRegs) ? usedSlots_ - NumArgRegs : 0) * STACK_SLOT_SIZE;
    if (!dynamicAlignment_)
        stackAdjust +=
            ComputeByteAlignment(framePushed_ + stackAdjust, StackAlignment);
    else
        
        stackAdjust += ComputeByteAlignment(stackAdjust + STACK_SLOT_SIZE, StackAlignment);

    reserveStack(stackAdjust);
    
    {
        enoughMemory_ = enoughMemory_ && moveResolver_.resolve();
        if (!enoughMemory_)
            return;

        MoveEmitter emitter(*this);
        emitter.emit(moveResolver_);
        emitter.finish();
    }
    for (int i = 0; i < 2; i++) {
        if (!floatArgsInGPR[i].isInvalid()) {
            ma_vxfer(floatArgsInGPR[i], Register::FromCode(i*2), Register::FromCode(i*2+1));
        }
    }
    checkStackAlignment();
    ma_call(fun);

    if (result == DOUBLE) {
        
        as_vxfer(r0, r1, ReturnFloatReg, CoreToFloat);
    }

    freeStack(stackAdjust);
    if (dynamicAlignment_) {
        
        
        as_dtr(IsLoad, 32, Offset, sp, DTRAddr(sp, DtrOffImm(0)));
    }

    JS_ASSERT(inCall_);
    inCall_ = false;
}

void
MacroAssemblerARMCompat::handleException()
{
    
    int size = (sizeof(ResumeFromException) + 7) & ~7;
    ma_sub(Imm32(size), sp);
    ma_mov(sp, r0);

    
    setupUnalignedABICall(1, r1);
    passABIArg(r0);
    callWithABI(JS_FUNC_TO_DATA_PTR(void *, ion::HandleException));
    
    moveValue(MagicValue(JS_ION_ERROR), JSReturnOperand);
    ma_ldr(Operand(sp, offsetof(ResumeFromException, stackPointer)), sp);

    
    
    as_dtr(IsLoad, 32, PostIndex, pc, DTRAddr(sp, DtrOffImm(4)));

}

Assembler::Condition
MacroAssemblerARMCompat::testStringTruthy(bool truthy, const ValueOperand &value)
{
    Register string = value.payloadReg();

    size_t mask = (0xFFFFFFFF << JSString::LENGTH_SHIFT);
    ma_dtr(IsLoad, string, Imm32(JSString::offsetOfLengthAndFlags()), ScratchRegister);
    
    
    
    ma_bic(Imm32(~mask), ScratchRegister, SetCond);
    return truthy ? Assembler::NonZero : Assembler::Zero;
}

void
MacroAssemblerARMCompat::enterOsr(Register calleeToken, Register code)
{
    push(Imm32(0)); 
    push(calleeToken);
    push(Imm32(MakeFrameDescriptor(0, IonFrame_Osr)));
    ma_add(sp, Imm32(sizeof(uintptr_t)), sp);   
    ma_callIonHalfPush(code);
    ma_sub(sp, Imm32(sizeof(uintptr_t) * 3), sp);
}


void
MacroAssemblerARMCompat::floor(FloatRegister input, Register output, Label *bail)
{
    Label handleZero;
    Label handleNeg;
    Label fin;
    compareDouble(input, InvalidFloatReg);
    ma_b(&handleZero, Assembler::Equal);
    ma_b(&handleNeg, Assembler::Signed);
    
    ma_b(bail, Assembler::Overflow);

    
    
    
    
    ma_vcvt_F64_U32(input, ScratchFloatReg);
    ma_vxfer(VFPRegister(ScratchFloatReg).uintOverlay(), output);
    ma_mov(output, output, SetCond);
    ma_b(bail, Signed);
    ma_b(&fin);

    bind(&handleZero);
    
    
    as_vxfer(output, InvalidReg, input, FloatToCore, Always, 1);
    ma_cmp(output, Imm32(0));
    ma_b(bail, NonZero);
    ma_b(&fin);

    bind(&handleNeg);
    
    ma_vneg(input, input);
    ma_vcvt_F64_U32(input, ScratchFloatReg);
    ma_vxfer(VFPRegister(ScratchFloatReg).uintOverlay(), output);
    ma_vcvt_U32_F64(ScratchFloatReg, ScratchFloatReg);
    compareDouble(ScratchFloatReg, input);
    ma_add(output, Imm32(1), output, NoSetCond, NotEqual);
    
    
    ma_rsb(output, Imm32(0), output, SetCond);
    
    ma_vneg(input, input);
    
    
    
    
    ma_b(bail, Unsigned);

    bind(&fin);
}

CodeOffsetLabel
MacroAssemblerARMCompat::toggledJump(Label *label)
{
    
    CodeOffsetLabel ret(nextOffset().getOffset());
    ma_b(label);
    return ret;
}

void
MacroAssemblerARMCompat::round(FloatRegister input, Register output, Label *bail, FloatRegister tmp)
{
    Label handleZero;
    Label handleNeg;
    Label fin;
    
    
    ma_vcmpz(input);
    
    
    ma_vimm(0.5, ScratchFloatReg);
    
    ma_vabs(input, tmp);
    
    ma_vadd(ScratchFloatReg, tmp, tmp);
    as_vmrs(pc);
    ma_b(&handleZero, Assembler::Equal);
    ma_b(&handleNeg, Assembler::Signed);
    
    ma_b(bail, Assembler::Overflow);

    
    
    
    
    ma_vcvt_F64_U32(tmp, ScratchFloatReg);
    ma_vxfer(VFPRegister(ScratchFloatReg).uintOverlay(), output);
    ma_mov(output, output, SetCond);
    ma_b(bail, Signed);
    ma_b(&fin);

    bind(&handleZero);
    
    
    as_vxfer(output, InvalidReg, input, FloatToCore, Always, 1);
    ma_cmp(output, Imm32(0));
    ma_b(bail, NonZero);
    ma_b(&fin);

    bind(&handleNeg);
    
    ma_vcvt_F64_U32(tmp, ScratchFloatReg);
    ma_vxfer(VFPRegister(ScratchFloatReg).uintOverlay(), output);

    
    
    
    ma_vcvt_U32_F64(ScratchFloatReg, ScratchFloatReg);
    compareDouble(ScratchFloatReg, tmp);
    ma_sub(output, Imm32(1), output, NoSetCond, Equal);
    
    
    ma_rsb(output, Imm32(0), output, SetCond);

    
    
    
    ma_b(bail, Unsigned);

    bind(&fin);
}

CodeOffsetJump
MacroAssemblerARMCompat::jumpWithPatch(RepatchLabel *label, Condition cond)
{
    ARMBuffer::PoolEntry pe;
    BufferOffset bo = as_BranchPool(0xdeadbeef, label, &pe, cond);

    
    
    CodeOffsetJump ret(bo.getOffset(), pe.encode());
    return ret;
}
