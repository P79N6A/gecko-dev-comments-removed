





#include "jsiter.h"

#include "jit/BaselineCompiler.h"
#include "jit/BaselineHelpers.h"
#include "jit/BaselineIC.h"
#include "jit/BaselineJIT.h"
#include "jit/Linker.h"

#include "jsboolinlines.h"

using namespace js;
using namespace js::jit;

namespace js {
namespace jit {



bool
ICCompare_Int32::Compiler::generateStubCode(MacroAssembler& masm)
{
    
    Label failure;
    Label conditionTrue;
    masm.branchTestInt32(Assembler::NotEqual, R0, &failure);
    masm.branchTestInt32(Assembler::NotEqual, R1, &failure);

    
    Assembler::Condition cond = JSOpToCondition(op, true);
    masm.ma_cmp_set(R0.payloadReg(), R0.payloadReg(), R1.payloadReg(), cond);

    masm.tagValue(JSVAL_TYPE_BOOLEAN, R0.payloadReg(), R0);
    EmitReturnFromIC(masm);

    
    masm.bind(&failure);
    EmitStubGuardFailure(masm);

    return true;
}

bool
ICCompare_Double::Compiler::generateStubCode(MacroAssembler& masm)
{
    Label failure, isNaN;
    masm.ensureDouble(R0, FloatReg0, &failure);
    masm.ensureDouble(R1, FloatReg1, &failure);

    Register dest = R0.scratchReg();

    Assembler::DoubleCondition doubleCond = JSOpToDoubleCondition(op);

    masm.ma_cmp_set_double(dest, FloatReg0, FloatReg1, doubleCond);

    masm.tagValue(JSVAL_TYPE_BOOLEAN, dest, R0);
    EmitReturnFromIC(masm);

    
    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}



bool
ICBinaryArith_Int32::Compiler::generateStubCode(MacroAssembler& masm)
{
    
    Label failure;
    masm.branchTestInt32(Assembler::NotEqual, R0, &failure);
    masm.branchTestInt32(Assembler::NotEqual, R1, &failure);

    
    Register scratchReg = R2.payloadReg();

    
    AllocatableGeneralRegisterSet savedRegs(availableGeneralRegs(2));
    savedRegs.set() = GeneralRegisterSet::Intersect(GeneralRegisterSet::NonVolatile(), savedRegs.set());

    Label goodMul, divTest1, divTest2;
    switch(op_) {
      case JSOP_ADD:
        
        
        masm.ma_addTestOverflow(scratchReg, R0.payloadReg(), R1.payloadReg(), &failure);
        masm.move32(scratchReg, R0.payloadReg());
        break;
      case JSOP_SUB:
        masm.ma_subTestOverflow(scratchReg, R0.payloadReg(), R1.payloadReg(), &failure);
        masm.move32(scratchReg, R0.payloadReg());
        break;
      case JSOP_MUL: {
        masm.ma_mul_branch_overflow(scratchReg, R0.payloadReg(), R1.payloadReg(), &failure);

        masm.ma_b(scratchReg, Imm32(0), &goodMul, Assembler::NotEqual, ShortJump);

        
        masm.as_xor(t8, R0.payloadReg(), R1.payloadReg());
        masm.ma_b(t8, Imm32(0), &failure, Assembler::LessThan, ShortJump);

        masm.bind(&goodMul);
        masm.move32(scratchReg, R0.payloadReg());
        break;
      }
      case JSOP_DIV:
      case JSOP_MOD: {
        
        masm.ma_b(R0.payloadReg(), Imm32(INT_MIN), &divTest1, Assembler::NotEqual, ShortJump);
        masm.ma_b(R1.payloadReg(), Imm32(-1), &failure, Assembler::Equal, ShortJump);
        masm.bind(&divTest1);

        
        masm.ma_b(R1.payloadReg(), Imm32(0), &failure, Assembler::Equal, ShortJump);

        
        masm.ma_b(R0.payloadReg(), Imm32(0), &divTest2, Assembler::NotEqual, ShortJump);
        masm.ma_b(R1.payloadReg(), Imm32(0), &failure, Assembler::LessThan, ShortJump);
        masm.bind(&divTest2);

        masm.as_div(R0.payloadReg(), R1.payloadReg());

        if (op_ == JSOP_DIV) {
            
            masm.as_mfhi(scratchReg);
            masm.ma_b(scratchReg, Imm32(0), &failure, Assembler::NotEqual, ShortJump);
            masm.as_mflo(scratchReg);
            masm.tagValue(JSVAL_TYPE_INT32, scratchReg, R0);
        } else {
            Label done;
            
            masm.as_mfhi(scratchReg);
            masm.ma_b(scratchReg, Imm32(0), &done, Assembler::NotEqual, ShortJump);
            masm.ma_b(R0.payloadReg(), Imm32(0), &failure, Assembler::LessThan, ShortJump);
            masm.bind(&done);
            masm.tagValue(JSVAL_TYPE_INT32, scratchReg, R0);
        }
        break;
      }
      case JSOP_BITOR:
        masm.ma_or(R0.payloadReg() , R0.payloadReg(), R1.payloadReg());
        break;
      case JSOP_BITXOR:
        masm.ma_xor(R0.payloadReg() , R0.payloadReg(), R1.payloadReg());
        break;
      case JSOP_BITAND:
        masm.ma_and(R0.payloadReg() , R0.payloadReg(), R1.payloadReg());
        break;
      case JSOP_LSH:
        
        masm.ma_sll(R0.payloadReg(), R0.payloadReg(), R1.payloadReg());
        break;
      case JSOP_RSH:
        masm.ma_sra(R0.payloadReg(), R0.payloadReg(), R1.payloadReg());
        break;
      case JSOP_URSH:
        masm.ma_srl(scratchReg, R0.payloadReg(), R1.payloadReg());
        if (allowDouble_) {
            Label toUint;
            masm.ma_b(scratchReg, Imm32(0), &toUint, Assembler::LessThan, ShortJump);

            
            masm.move32(scratchReg, R0.payloadReg());
            EmitReturnFromIC(masm);

            masm.bind(&toUint);
            masm.convertUInt32ToDouble(scratchReg, FloatReg1);
            masm.boxDouble(FloatReg1, R0);
        } else {
            masm.ma_b(scratchReg, Imm32(0), &failure, Assembler::LessThan, ShortJump);
            
            masm.move32(scratchReg, R0.payloadReg());
        }
        break;
      default:
        MOZ_CRASH("Unhandled op for BinaryArith_Int32.");
    }

    EmitReturnFromIC(masm);

    
    masm.bind(&failure);
    EmitStubGuardFailure(masm);

    return true;
}

bool
ICUnaryArith_Int32::Compiler::generateStubCode(MacroAssembler& masm)
{
    Label failure;
    masm.branchTestInt32(Assembler::NotEqual, R0, &failure);

    switch (op) {
      case JSOP_BITNOT:
        masm.not32(R0.payloadReg());
        break;
      case JSOP_NEG:
        
        masm.branchTest32(Assembler::Zero, R0.payloadReg(), Imm32(INT32_MAX), &failure);

        masm.neg32(R0.payloadReg());
        break;
      default:
        MOZ_CRASH("Unexpected op");
        return false;
    }

    EmitReturnFromIC(masm);

    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}


} 
} 
