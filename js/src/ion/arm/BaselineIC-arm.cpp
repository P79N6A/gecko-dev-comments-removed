






#include "ion/BaselineJIT.h"
#include "ion/BaselineIC.h"
#include "ion/BaselineCompiler.h"
#include "ion/BaselineHelpers.h"
#include "ion/IonLinker.h"

using namespace js;
using namespace js::ion;

namespace js {
namespace ion {



bool
ICCompare_Int32::Compiler::generateStubCode(MacroAssembler &masm)
{
    
    Assembler::Condition cond;
    Assembler::Condition notcond;
    switch(op) {
      case JSOP_LT:
        cond = Assembler::LessThan;
        break;
      case JSOP_GT:
        cond = Assembler::GreaterThan;
        break;
      default:
        JS_NOT_REACHED("Unhandled op for ICCompare_Int32.");
        return false;
    }

    
    Label failure;
    masm.branchTestInt32(Assembler::NotEqual, R0, &failure);
    masm.branchTestInt32(Assembler::NotEqual, R1, &failure);

    
    masm.cmp32(R0.payloadReg(), R1.payloadReg());
    masm.ma_mov(Imm32(1), R0.payloadReg(), NoSetCond, cond);
    masm.ma_mov(Imm32(0), R0.payloadReg(), NoSetCond, Assembler::InvertCondition(cond));

    
    masm.tagValue(JSVAL_TYPE_BOOLEAN, R0.payloadReg(), R0);
    EmitReturnFromIC(masm);

    
    masm.bind(&failure);
    EmitStubGuardFailure(masm);

    return true;
}



bool
ICBinaryArith_Int32::Compiler::generateStubCode(MacroAssembler &masm)
{
    
    Label failure;
    masm.branchTestInt32(Assembler::NotEqual, R0, &failure);
    masm.branchTestInt32(Assembler::NotEqual, R1, &failure);

    
    Register scratchReg = R2.payloadReg();

    Label maybeNegZero;
    switch(op_) {
      case JSOP_ADD:
        masm.ma_add(R0.payloadReg(), R1.payloadReg(), scratchReg);

        
        
        masm.j(Assembler::Overflow, &failure);

        
        
        masm.mov(scratchReg, R0.payloadReg());
        break;
      case JSOP_SUB:
        masm.ma_sub(R0.payloadReg(), R1.payloadReg(), scratchReg);
        masm.j(Assembler::Overflow, &failure);
        masm.mov(scratchReg, R0.payloadReg());
        break;
      case JSOP_MUL: {
        Assembler::Condition cond = masm.ma_check_mul(R0.payloadReg(), R1.payloadReg(), scratchReg,
                                                      Assembler::Overflow);
        masm.j(cond, &failure);

        masm.ma_cmp(scratchReg, Imm32(0));
        masm.j(Assembler::Equal, &maybeNegZero);

        masm.mov(scratchReg, R0.payloadReg());
        break;
      }
      case JSOP_BITOR:
        masm.ma_orr(R1.payloadReg(), R0.payloadReg(), R0.payloadReg());
        break;
      case JSOP_BITXOR:
        masm.ma_eor(R1.payloadReg(), R0.payloadReg(), R0.payloadReg());
        break;
      case JSOP_BITAND:
        masm.ma_and(R1.payloadReg(), R0.payloadReg(), R0.payloadReg());
        break;
      case JSOP_LSH:
        
        masm.ma_and(Imm32(0x1F), R1.payloadReg(), R1.payloadReg());
        masm.ma_lsl(R1.payloadReg(), R0.payloadReg(), R0.payloadReg());
        break;
      case JSOP_RSH:
        masm.ma_and(Imm32(0x1F), R1.payloadReg(), R1.payloadReg());
        masm.ma_asr(R1.payloadReg(), R0.payloadReg(), R0.payloadReg());
        break;
      case JSOP_URSH:
        masm.ma_and(Imm32(0x1F), R1.payloadReg(), scratchReg);
        masm.ma_lsr(scratchReg, R0.payloadReg(), scratchReg);
        masm.ma_cmp(scratchReg, Imm32(0));
        masm.j(Assembler::LessThan, &failure);
        
        masm.mov(scratchReg, R0.payloadReg());
        break;
      default:
        JS_NOT_REACHED("Unhandled op for BinaryArith_Int32.");
        return false;
    }

    EmitReturnFromIC(masm);

    if (op_ == JSOP_MUL) {
        masm.bind(&maybeNegZero);

        
        masm.ma_cmn(R0.payloadReg(), R1.payloadReg());
        masm.j(Assembler::Signed, &failure);

        
        masm.ma_mov(Imm32(0), R0.payloadReg());
        EmitReturnFromIC(masm);
    }

    
    masm.bind(&failure);
    EmitStubGuardFailure(masm);

    return true;
}

bool
ICUnaryArith_Int32::Compiler::generateStubCode(MacroAssembler &masm)
{
    Label failure;
    masm.branchTestInt32(Assembler::NotEqual, R0, &failure);

    switch (op) {
      case JSOP_BITNOT:
        masm.ma_mvn(R0.payloadReg(), R0.payloadReg());
        break;
      case JSOP_NEG:
        
        masm.branchTest32(Assembler::Zero, R0.payloadReg(), Imm32(0x7fffffff), &failure);

        
        masm.ma_rsb(R0.payloadReg(), Imm32(0), R0.payloadReg());
        break;
      default:
        JS_NOT_REACHED("Unexpected op");
        return false;
    }

    EmitReturnFromIC(masm);

    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}

} 
} 
