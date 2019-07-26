






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

    
    
    masm.unboxNonDouble(R0, rdx);
    masm.unboxNonDouble(R1, ScratchReg);

    switch(op) {
      case JSOP_ADD:
        masm.addl(rdx, ScratchReg);
        break;
      default:
        JS_ASSERT(!"Unhandled op for BinaryArith_Int32!");
        return false;
    }

    
    
    masm.j(Assembler::Overflow, &failure);

    
    masm.boxValue(JSVAL_TYPE_INT32, ScratchReg, R0.valueReg());
    EmitReturnFromIC(masm);

    
    masm.bind(&failure);
    EmitStubGuardFailure(masm);

    return true;
}


} 
} 
