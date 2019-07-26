






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
        JS_ASSERT(!"Unhandled op for ICCompare_Int32!");
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

    switch(op) {
      case JSOP_ADD:
        masm.ma_add(R0.payloadReg(), R1.payloadReg(), scratchReg);
        break;
      default:
        JS_ASSERT(!"Unhandled op for BinaryArith_Int32!");
        return false;
    }

    
    
    masm.j(Assembler::Overflow, &failure);

    
    
    masm.movePtr(scratchReg, R0.payloadReg());
    EmitReturnFromIC(masm);

    
    masm.bind(&failure);
    EmitStubGuardFailure(masm);

    return true;
}


} 
} 
