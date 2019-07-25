







































#include "ion/arm/MacroAssembler-arm.h"
#include "ion/MoveEmitter.h"

using namespace js;
using namespace ion;

void
MacroAssemblerARM::convertInt32ToDouble(const Register &src, const FloatRegister &dest)
{
    
    as_vxfer(src, InvalidReg, VFPRegister(dest, VFPRegister::Single),
             CoreToFloat);
    as_vcvt(VFPRegister(dest, VFPRegister::Double),
            VFPRegister(dest, VFPRegister::Int));
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
MacroAssemblerARM::ma_alu(Register src1, Operand2 op2, Register dest, ALUOp op, SetCond_ sc, Condition c)
{
    as_alu(dest, src1, op2, op, sc, c);
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
    writeDataRelocation(nextOffset());
    ma_mov(Imm32(ptr.value), dest);
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
    ma_alu(src1, imm, InvalidReg, op_cmn);
}
void
MacroAssemblerARM::ma_cmn(Register src1, Register src2, Condition c)
{
    as_alu(InvalidReg, src2, O2Reg(src1), op_cmn);
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
MacroAssemblerARM::ma_cmp(Register src1, ImmGCPtr ptr, Condition c)
{
    writeDataRelocation(nextOffset());
    ma_alu(src1, Imm32(ptr.value), InvalidReg, op_cmp, SetCond, c);
}
void
MacroAssemblerARM::ma_cmp(Register src1, Operand op, Condition c)
{
    as_cmp(src1, op.toOp2(), c);
}
void
MacroAssemblerARM::ma_cmp(Register src1, Register src2, Condition c)
{
    as_cmp(src2, O2Reg(src1), c);
}

    
void
MacroAssemblerARM::ma_teq(Imm32 imm, Register src1, Condition c)
{
    ma_alu(src1, imm, InvalidReg, op_teq, SetCond, c);
}
void
MacroAssemblerARM::ma_teq(Register src2, Register src1, Condition c)
{
    as_tst(src2, O2Reg(src1), c);
}
void
MacroAssemblerARM::ma_teq(Register src1, Operand op, Condition c)
{
        as_teq(src1, op.toOp2(), c);
}



void
MacroAssemblerARM::ma_tst(Imm32 imm, Register src1, Condition c)
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
MacroAssemblerARM::ma_str(Register rt, DTRAddr addr, Index mode, Condition cc)
{
    as_dtr(IsStore, 32, mode, rt, addr, cc);
}

void
MacroAssemblerARM::ma_dtr(LoadStore ls, Register rt, const Operand &addr, Index mode, Condition cc)
{
    int off = addr.disp();
    Register base = Register::FromCode(addr.base());
    if (off > -4096 && off < 4096) {
        as_dtr(ls, 32, mode, rt, addr.toDTRAddr(), cc);
        return;
    }
    
    
    int bottom = off & 0xfff;
    int neg_bottom = 0x1000 - bottom;
    
    
    if (off < 0) {
        Operand2 sub_off = Imm8(-(off-bottom)); 
        if (!sub_off.invalid) {
            as_sub(ScratchRegister, base, sub_off, NoSetCond, cc); 
            as_dtr(ls, 32, Offset, rt, DTRAddr(ScratchRegister, DtrOffImm(bottom)), cc);
            return;
        }
        sub_off = Imm8(-(off+neg_bottom));
        if (!sub_off.invalid) {
            as_sub(ScratchRegister, base, sub_off, NoSetCond, cc); 
            as_dtr(ls, 32, Offset, rt, DTRAddr(ScratchRegister, DtrOffImm(-neg_bottom)), cc);
            return;
        }
    } else {
        Operand2 sub_off = Imm8(off-bottom); 
        if (!sub_off.invalid) {
            as_add(ScratchRegister, base, sub_off, NoSetCond, cc); 
            as_dtr(ls, 32, Offset, rt, DTRAddr(ScratchRegister, DtrOffImm(bottom)), cc);
            return;
        }
        sub_off = Imm8(off+neg_bottom);
        if (!sub_off.invalid) {
            as_add(ScratchRegister, base, sub_off, NoSetCond,  cc); 
            as_dtr(ls, 32, Offset, rt, DTRAddr(ScratchRegister, DtrOffImm(-neg_bottom)), cc);
            return;
        }
    }
    JS_NOT_REACHED("TODO: implement bigger offsets :(");
}
void
MacroAssemblerARM::ma_str(Register rt, const Operand &addr, Index mode, Condition cc)
{
    ma_dtr(IsStore, rt, addr, mode, cc);
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
    int x = offset.value;
    
    if (size == 32 || (size == 8 && !IsSigned) ) {
        if (x < 4096 && x > -4096) {
            as_dtr(ls, size, mode, rt, DTRAddr(rn, DtrOffImm(x)), cc);
        } else {
            JS_NOT_REACHED("Feature NYI");
        }
    } else {
        
        if (x < 256 && x > -256) {
            as_extdtr(ls, size, IsSigned, mode, rt, EDtrAddr(rn, EDtrOffImm(x)), cc);
        } else {
            JS_NOT_REACHED("Feature NYI");
        }
    }
}
void
MacroAssemblerARM::ma_pop(Register r)
{
    ma_dtr(IsLoad, sp, Imm32(4), r, PostIndex);
}
void
MacroAssemblerARM::ma_push(Register r)
{
    ma_dtr(IsStore, sp,Imm32(-4), r, PreIndex);
}


void
MacroAssemblerARM::ma_b(Label *dest, Assembler::Condition c)
{
    as_b(dest, c);
}

enum RelocBranchStyle {
    MOVWT,
    LDR_BX,
    LDR,
    MOVW_ADD
};
RelocBranchStyle
b_type()
{
    return MOVWT;
}
void
MacroAssemblerARM::ma_b(void *target, Relocation::Kind reloc, Assembler::Condition c)
{
    
    
    
    
    uint32 trg = (uint32)target;
    switch (b_type()) {
      case MOVWT:
        as_movw(ScratchRegister, Imm16(trg & 0xffff), c);
        as_movt(ScratchRegister, Imm16(trg >> 16), c);
        
        as_bx(ScratchRegister, c);
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
MacroAssemblerARM::ma_vimm(double value, FloatRegister dest)
{
    jsdpun dpun;
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
MacroAssemblerARM::ma_vcvt_F64_I32(FloatRegister src, FloatRegister dest)
{
    as_vcvt(VFPRegister(dest).sintOverlay(), VFPRegister(src));
}
void
MacroAssemblerARM::ma_vcvt_I32_F64(FloatRegister dest, FloatRegister src)
{
    as_vcvt(VFPRegister(dest), VFPRegister(src).sintOverlay());
}
void
MacroAssemblerARM::ma_vxfer(FloatRegister src, Register dest)
{
    as_vxfer(dest, InvalidReg, VFPRegister(src).singleOverlay(), FloatToCore);
}
void
MacroAssemblerARM::ma_vdtr(LoadStore ls, Operand &addr, FloatRegister rt, Condition cc)
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
MacroAssemblerARM::ma_vldr(VFPAddr addr, FloatRegister dest)
{
    as_vdtr(IsLoad, dest, addr);
}
void
MacroAssemblerARM::ma_vldr(const Operand &addr, FloatRegister dest)
{
    JS_ASSERT(addr.disp() < 1024 && addr.disp() > - 1024);
    as_vdtr(IsLoad, dest, VFPAddr(Register::FromCode(addr.base()), VFPOffImm(addr.disp())));
}

void
MacroAssemblerARM::ma_vstr(FloatRegister src, VFPAddr addr)
{
    as_vdtr(IsStore, src, addr);
}

void
MacroAssemblerARM::ma_vstr(FloatRegister src, const Operand &addr)
{
    if (addr.disp() < 1024 && addr.disp() > - 1024) {
        as_vdtr(IsStore, src, VFPAddr(Register::FromCode(addr.base()), VFPOffImm(addr.disp())));
        return;
    }
    
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
    writeDataRelocation(nextOffset());
    ma_mov(imm, dest);
}

void
MacroAssemblerARM::load32(const Address &address, Register dest)
{
    loadPtr(address, dest);
}
void
MacroAssemblerARM::loadPtr(const Address &address, Register dest)
{
    ma_ldr(DTRAddr(address.base, DtrOffImm(address.offset)), dest);
}
void
MacroAssemblerARM::loadPtr(const ImmWord &imm, Register dest)
{
    movePtr(imm, ScratchRegister);
    loadPtr(Address(ScratchRegister, 0x0), dest);
}
void
MacroAssemblerARM::setStackArg(const Register &reg, uint32 arg)
{
    ma_dataTransferN(IsStore, 32, true, sp, Imm32(arg * STACK_SLOT_SIZE), reg);

}

void
MacroAssemblerARM::subPtr(Imm32 imm, const Register dest)
{
    ma_sub(imm, dest);
}


Assembler::Condition
MacroAssemblerARM::compareDoubles(JSOp compare, FloatRegister lhs, FloatRegister rhs)
{
    ma_vcmp(lhs, rhs);
    as_vmrs(pc);
    switch (compare) {
      case JSOP_STRICTNE:
      case JSOP_NE:
        return Assembler::VFP_NotEqualOrUnordered;
      case JSOP_STRICTEQ:
      case JSOP_EQ:
        return Assembler::VFP_Equal;
      case JSOP_LT:
        return Assembler::VFP_LessThan;
      case JSOP_LE:
        return Assembler::VFP_LessThanOrEqual;
      case JSOP_GT:
        return Assembler::VFP_GreaterThan;
      case JSOP_GE:
        return Assembler::VFP_GreaterThanOrEqual;
      default:
        JS_NOT_REACHED("Unrecognized comparison operation");
        return Assembler::VFP_Unordered;
    }
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


void
MacroAssemblerARMCompat::unboxInt32(const ValueOperand &operand, const Register &dest)
{
    ma_mov(operand.payloadReg(), dest);
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
MacroAssemblerARMCompat::boolValueToDouble(const ValueOperand &operand, const FloatRegister &dest)
{
    VFPRegister d = VFPRegister(dest);
    ma_vimm(1.0, dest);
    ma_cmp(operand.payloadReg(), Imm32(0));
    as_vsub(d, d, d, NotEqual);
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
MacroAssemblerARMCompat::loadStaticDouble(const double *dp, const FloatRegister &dest)
{
    ma_mov(Imm32((uint32)dp), ScratchRegister);
    as_vdtr(IsLoad, dest, VFPAddr(ScratchRegister, VFPOffImm(0)));
#if 0
    _vldr()
        movsd(dp, dest);
#endif
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
Operand ToPayload(Operand base) {
    return base;
}
Operand ToType(Operand base) {
    return Operand(Register::FromCode(base.base()),
                   base.disp() + sizeof(void *));
}

Operand payloadOf(const Address &address) {
    return Operand(address.base, address.offset);
}
Operand tagOf(const Address &address) {
    return Operand(address.base, address.offset + 4);
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
MacroAssemblerARMCompat::loadValue(Operand src, ValueOperand val)
{
    Operand payload = ToPayload(src);
    Operand type = ToType(src);
    
    if (((val.payloadReg().code() & 1) == 0) &&
        val.typeReg().code() == val.payloadReg().code()+1)
    {
        
        
        int offset = src.disp();
        if (offset < 256 && offset > -256) {
            ma_ldrd(EDtrAddr(Register::FromCode(src.base()), EDtrOffImm(src.disp())), val.payloadReg(), val.typeReg());
            return;
        }
    }
    

    if (val.payloadReg().code() < val.typeReg().code()) {
        if (src.disp() <= 4 && src.disp() >= -8 && (src.disp() & 3) == 0) {
            
            
            DTMMode mode;
            switch(src.disp()) {
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
            startDataTransferM(IsLoad, Register::FromCode(src.base()), mode);
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
MacroAssemblerARMCompat::storePayload(const Value &val, Operand dest) {
    jsval_layout jv = JSVAL_TO_IMPL(val);
    if (val.isMarkable()) {
        ma_mov(ImmGCPtr((gc::Cell *)jv.s.payload.ptr), ScratchRegister);
    } else {
        ma_mov(Imm32(jv.s.payload.i32), ScratchRegister);
    }
    ma_str(ScratchRegister, ToPayload(dest));
}
void
MacroAssemblerARMCompat::storePayload(Register src, Operand dest) {
    if (dest.getTag() == Operand::MEM) {
        ma_str(src, ToPayload(dest));
        return;
    }
    JS_NOT_REACHED("why do we do all of these things?");

}
void
MacroAssemblerARMCompat::storeTypeTag(ImmTag tag, Operand dest) {
    if (dest.getTag() == Operand::MEM) {
        ma_mov(tag, ScratchRegister);
        ma_str(ScratchRegister, ToType(dest));
        return;
    }

    JS_NOT_REACHED("why do we do all of these things?");

}

void
MacroAssemblerARMCompat::linkExitFrame() {
    movePtr(ImmWord(JS_THREAD_DATA(GetIonContext()->cx)), ScratchRegister);
    ma_str(StackPointer, Operand(ScratchRegister, offsetof(ThreadData, ionTop)));
}










void
MacroAssemblerARM::ma_callIon(const Register r)
{
    
    

    as_dtr(IsStore, 32, PreIndex, pc, DTRAddr(sp, DtrOffImm(-8)));
    as_blx(r);
}
void
MacroAssemblerARM::ma_callIonNoPush(const Register r)
{
    as_dtr(IsStore, 32, Offset, pc, DTRAddr(sp, DtrOffImm(0)));
    as_blx(r);
}
void
MacroAssemblerARM::ma_callIonHalfPush(const Register r)
{
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
MacroAssemblerARM::breakpoint() {
    as_bkpt();
}

uint32
MacroAssemblerARM::setupABICall(uint32 args)
{
    JS_ASSERT(!inCall_);
    inCall_ = true;

    uint32 stackForArgs = args > NumArgRegs
                          ? (args - NumArgRegs) * sizeof(void *)
                          : 0;
    return stackForArgs;
}

void
MacroAssemblerARM::setupAlignedABICall(uint32 args)
{
    
    
    
    uint32 stackForCall = setupABICall(args);
    uint32 displacement = stackForCall + framePushed_;

    
    stackAdjust_ = stackForCall + ComputeByteAlignment(displacement, StackAlignment);
    dynamicAlignment_ = false;
    reserveStack(stackAdjust_);
}

void
MacroAssemblerARM::setupUnalignedABICall(uint32 args, const Register &scratch)
{
    uint32 stackForCall = setupABICall(args);
    uint32 displacement = stackForCall + STACK_SLOT_SIZE;

    
    
    
    
    ma_mov(sp, scratch);
    ma_and(Imm32(~(StackAlignment - 1)), sp, sp);
    ma_push(scratch);
    stackAdjust_ = stackForCall + ComputeByteAlignment(displacement, StackAlignment);
    dynamicAlignment_ = true;
    reserveStack(stackAdjust_);
}

void
MacroAssemblerARM::setABIArg(uint32 arg, const MoveOperand &from)
{
    MoveOperand to;
    Register dest;
    if (GetArgReg(arg, &dest)) {
        to = MoveOperand(dest);
    } else {
        
        
        uint32 disp = GetArgStackDisp(arg);
        to = MoveOperand(StackPointer, disp);
    }
    enoughMemory_ &= moveResolver_.addMove(from, to, Move::GENERAL);
}

void
MacroAssemblerARM::setABIArg(uint32 arg, const Register &reg)
{
    setABIArg(arg, MoveOperand(reg));
}
#ifdef DEBUG
void MacroAssemblerARM::checkStackAlignment()
{
        Label good;
        ma_tst(Imm32(StackAlignment - 1), sp);
        ma_b(&good, Equal);
        breakpoint();
        bind(&good);
}
#endif
void
MacroAssemblerARM::callWithABI(void *fun)
{
    JS_ASSERT(inCall_);

    
    {
        enoughMemory_ &= moveResolver_.resolve();
        if (!enoughMemory_)
            return;

        MoveEmitter emitter(*this);
        emitter.emit(moveResolver_);
        emitter.finish();
    }

#ifdef DEBUG
    checkStackAlignment();
#endif

    ma_call(fun);

    freeStack(stackAdjust_);
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

    
    setupAlignedABICall(1);
    setABIArg(0, r0);
    callWithABI(JS_FUNC_TO_DATA_PTR(void *, ion::HandleException));
    
    moveValue(MagicValue(JS_ION_ERROR), JSReturnOperand);
    ma_ldr(Operand(sp, offsetof(ResumeFromException, stackPointer)), sp);

    
    
    as_dtr(IsLoad, 32, PostIndex, pc, DTRAddr(sp, DtrOffImm(4)));
    
}

