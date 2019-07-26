






#include "ion/BaselineJIT.h"
#include "ion/BaselineIC.h"
#include "ion/BaselineCompiler.h"
#include "ion/BaselineHelpers.h"
#include "ion/IonLinker.h"

using namespace js;
using namespace js::ion;

namespace js {
namespace ion {



IonCode *
ICCompare_Int32::Compiler::generateStubCode()
{
    
    Assembler::Condition cond;
    switch(op) {
      case JSOP_LT: cond = Assembler::LessThan; break;
      case JSOP_GT: cond = Assembler::GreaterThan; break;
      default:
        JS_ASSERT(!"Unhandled op for ICCompare_Int32!");
        return NULL;
    }

    MacroAssembler masm;

    
    Label failure;
    masm.branchTestInt32(Assembler::NotEqual, R0, &failure);
    masm.branchTestInt32(Assembler::NotEqual, R1, &failure);

    
    masm.cmpl(R0.payloadReg(), R1.payloadReg());
    masm.setCC(cond, R0.payloadReg());
    masm.movzxbl(R0.payloadReg(), R0.payloadReg());

    
    masm.tagValue(JSVAL_TYPE_BOOLEAN, R0.payloadReg(), R0);
    masm.ret();

    
    masm.bind(&failure);
    EmitStubGuardFailure(masm);

    Linker linker(masm);
    return linker.newCode(cx);
}



IonCode *
ICBinaryArith_Int32::Compiler::generateStubCode()
{
    MacroAssembler masm;

    
    Label failure;
    masm.branchTestInt32(Assembler::NotEqual, R0, &failure);
    masm.branchTestInt32(Assembler::NotEqual, R1, &failure);

    
    
    Register scratchReg = BaselineTailCallReg;

    switch(op) {
      case JSOP_ADD:
        masm.movl(R1.payloadReg(), scratchReg);
        masm.addl(R0.payloadReg(), scratchReg);
        break;
      default:
        JS_ASSERT(!"Unhandled op for BinaryArith_Int32!");
        return NULL;
    }

    
    
    masm.j(Assembler::Overflow, &failure);

    
    masm.tagValue(JSVAL_TYPE_INT32, scratchReg, R0);
    masm.ret();

    
    masm.bind(&failure);
    EmitStubGuardFailure(masm);

    Linker linker(masm);
    return linker.newCode(cx);
}


} 
} 
