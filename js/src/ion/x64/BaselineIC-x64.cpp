






#include "ion/BaselineJIT.h"
#include "ion/BaselineIC.h"
#include "ion/BaselineHelpers.h"
#include "ion/BaselineCompiler.h"
#include "ion/IonLinker.h"

using namespace js;
using namespace js::ion;

namespace js {
namespace ion {



bool
ICCompare_Int32::Compiler::generateStubCode(MacroAssembler &masm)
{
    
    Assembler::Condition cond;
    switch(op) {
      case JSOP_LT: cond = Assembler::LessThan; break;
      case JSOP_GT: cond = Assembler::GreaterThan; break;
      default:
        JS_ASSERT(!"Unhandled op for ICCompare_Int32!");
        return false;
    }

    
    Label failure;
    masm.branchTestInt32(Assembler::NotEqual, R0, &failure);
    masm.branchTestInt32(Assembler::NotEqual, R1, &failure);

    
    masm.cmpl(R0.valueReg(), R1.valueReg());
    masm.setCC(cond, ScratchReg);
    masm.movzxbl(ScratchReg, ScratchReg);

    
    masm.boxValue(JSVAL_TYPE_BOOLEAN, ScratchReg, R0.valueReg());
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

    Label revertRegister, maybeNegZero;
    switch(op_) {
      case JSOP_ADD:
        masm.unboxInt32(R0, ExtractTemp0);
        
        
        masm.addl(R1.valueReg(), ExtractTemp0);
        masm.j(Assembler::Overflow, &failure);

        
        masm.boxValue(JSVAL_TYPE_INT32, ExtractTemp0, R0.valueReg());
        break;
      case JSOP_SUB:
        masm.unboxInt32(R0, ExtractTemp0);
        masm.subl(R1.valueReg(), ExtractTemp0);
        masm.j(Assembler::Overflow, &failure);
        masm.boxValue(JSVAL_TYPE_INT32, ExtractTemp0, R0.valueReg());
        break;
      case JSOP_MUL:
        masm.unboxInt32(R0, ExtractTemp0);
        masm.imull(R1.valueReg(), ExtractTemp0);
        masm.j(Assembler::Overflow, &failure);

        masm.testl(ExtractTemp0, ExtractTemp0);
        masm.j(Assembler::Zero, &maybeNegZero);

        masm.boxValue(JSVAL_TYPE_INT32, ExtractTemp0, R0.valueReg());
        break;
      case JSOP_BITOR:
        
        
        masm.orq(R1.valueReg(), R0.valueReg());
        break;
      case JSOP_BITXOR:
        masm.xorl(R1.valueReg(), R0.valueReg());
        masm.tagValue(JSVAL_TYPE_INT32, R0.valueReg(), R0);
        break;
      case JSOP_BITAND:
        masm.andq(R1.valueReg(), R0.valueReg());
        break;
      case JSOP_LSH:
        masm.unboxInt32(R0, ExtractTemp0);
        masm.unboxInt32(R1, ecx); 
        masm.shll_cl(ExtractTemp0);
        masm.boxValue(JSVAL_TYPE_INT32, ExtractTemp0, R0.valueReg());
        break;
      case JSOP_RSH:
        masm.unboxInt32(R0, ExtractTemp0);
        masm.unboxInt32(R1, ecx);
        masm.sarl_cl(ExtractTemp0);
        masm.boxValue(JSVAL_TYPE_INT32, ExtractTemp0, R0.valueReg());
        break;
      case JSOP_URSH:
        if (!allowDouble_)
            masm.movq(R0.valueReg(), ScratchReg);

        masm.unboxInt32(R0, ExtractTemp0);
        masm.unboxInt32(R1, ecx); 

        masm.shrl_cl(ExtractTemp0);
        masm.testl(ExtractTemp0, ExtractTemp0);
        if (allowDouble_) {
            Label toUint;
            masm.j(Assembler::Signed, &toUint);

            
            masm.boxValue(JSVAL_TYPE_INT32, ExtractTemp0, R0.valueReg());
            EmitReturnFromIC(masm);

            masm.bind(&toUint);
            masm.convertUInt32ToDouble(ExtractTemp0, ScratchFloatReg);
            masm.boxDouble(ScratchFloatReg, R0);
        } else {
            masm.j(Assembler::Signed, &revertRegister);
            masm.boxValue(JSVAL_TYPE_INT32, ExtractTemp0, R0.valueReg());
        }
        break;
      default:
        JS_NOT_REACHED("Unhandled op in BinaryArith_Int32");
        return false;
    }

    
    EmitReturnFromIC(masm);

    if (op_ == JSOP_MUL) {
        masm.bind(&maybeNegZero);

        
        masm.movl(R0.valueReg(), ScratchReg);
        masm.orl(R1.valueReg(), ScratchReg);
        masm.j(Assembler::Signed, &failure);

        
        masm.moveValue(Int32Value(0), R0);
        EmitReturnFromIC(masm);
    }

    
    if (op_ == JSOP_URSH && !allowDouble_) {
        masm.bind(&revertRegister);
        
        masm.movq(ScratchReg, R0.valueReg());
        
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
        masm.notl(R0.valueReg());
        break;
      case JSOP_NEG:
        
        masm.branchTest32(Assembler::Zero, R0.valueReg(), Imm32(0x7fffffff), &failure);
        masm.negl(R0.valueReg());
        break;
      default:
        JS_NOT_REACHED("Unexpected op");
        return false;
    }

    masm.tagValue(JSVAL_TYPE_INT32, R0.valueReg(), R0);

    EmitReturnFromIC(masm);

    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}

} 
} 
