






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

    
    masm.cmpl(R0.payloadReg(), R1.payloadReg());
    masm.setCC(cond, R0.payloadReg());
    masm.movzxbl(R0.payloadReg(), R0.payloadReg());

    
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

    
    
    Register scratchReg = BaselineTailCallReg;

    Label revertRegister, maybeNegZero;
    switch(op_) {
      case JSOP_ADD:
        
        masm.movl(R0.payloadReg(), scratchReg);
        masm.addl(R1.payloadReg(), scratchReg);

        
        
        masm.j(Assembler::Overflow, &failure);

        
        masm.movl(scratchReg, R0.payloadReg());
        break;
      case JSOP_SUB:
        masm.movl(R0.payloadReg(), scratchReg);
        masm.subl(R1.payloadReg(), scratchReg);
        masm.j(Assembler::Overflow, &failure);
        masm.movl(scratchReg, R0.payloadReg());
        break;
      case JSOP_MUL:
        masm.movl(R0.payloadReg(), scratchReg);
        masm.imull(R1.payloadReg(), scratchReg);
        masm.j(Assembler::Overflow, &failure);

        masm.testl(scratchReg, scratchReg);
        masm.j(Assembler::Zero, &maybeNegZero);

        masm.movl(scratchReg, R0.payloadReg());
        break;
      case JSOP_BITOR:
        
        
        masm.orl(R1.payloadReg(), R0.payloadReg());
        break;
      case JSOP_BITXOR:
        masm.xorl(R1.payloadReg(), R0.payloadReg());
        break;
      case JSOP_BITAND:
        masm.andl(R1.payloadReg(), R0.payloadReg());
        break;
      case JSOP_LSH:
        
        JS_ASSERT(R0.typeReg() == ecx);
        masm.movl(R1.payloadReg(), ecx);
        masm.shll_cl(R0.payloadReg());
        
        masm.tagValue(JSVAL_TYPE_INT32, R0.payloadReg(), R0);
        break;
      case JSOP_RSH:
        masm.movl(R1.payloadReg(), ecx);
        masm.sarl_cl(R0.payloadReg());
        masm.tagValue(JSVAL_TYPE_INT32, R0.payloadReg(), R0);
        break;
      case JSOP_URSH:
        if (!allowDouble_)
            masm.movl(R0.payloadReg(), scratchReg);

        masm.movl(R1.payloadReg(), ecx);
        masm.shrl_cl(R0.payloadReg());
        masm.testl(R0.payloadReg(), R0.payloadReg());
        if (allowDouble_) {
            Label toUint;
            masm.j(Assembler::Signed, &toUint);

            
            masm.tagValue(JSVAL_TYPE_INT32, R0.payloadReg(), R0);
            EmitReturnFromIC(masm);

            masm.bind(&toUint);
            masm.convertUInt32ToDouble(R0.payloadReg(), ScratchFloatReg);
            masm.boxDouble(ScratchFloatReg, R0);
        } else {
            masm.j(Assembler::Signed, &revertRegister);
            masm.tagValue(JSVAL_TYPE_INT32, R0.payloadReg(), R0);
        }
        break;
      default:
       JS_NOT_REACHED("Unhandled op for BinaryArith_Int32.  ");
       return false;
    }

    
    EmitReturnFromIC(masm);

    if (op_ == JSOP_MUL) {
        masm.bind(&maybeNegZero);

        
        masm.movl(R0.payloadReg(), scratchReg);
        masm.orl(R1.payloadReg(), scratchReg);
        masm.j(Assembler::Signed, &failure);

        
        masm.xorl(R0.payloadReg(), R0.payloadReg());
        EmitReturnFromIC(masm);
    }

    
    if (op_ == JSOP_URSH && !allowDouble_) {
        masm.bind(&revertRegister);
        masm.tagValue(JSVAL_TYPE_INT32, scratchReg, R0);
        
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
        masm.notl(R0.payloadReg());
        break;
      case JSOP_NEG:
        
        masm.branchTest32(Assembler::Zero, R0.payloadReg(), Imm32(0x7fffffff), &failure);
        masm.negl(R0.payloadReg());
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
