







































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
