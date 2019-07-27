





#include "jit/BaselineCompiler.h"
#include "jit/BaselineHelpers.h"
#include "jit/BaselineIC.h"
#include "jit/BaselineJIT.h"
#include "jit/IonLinker.h"

using namespace js;
using namespace js::jit;

namespace js {
namespace jit {



bool
ICCompare_Int32::Compiler::generateStubCode(MacroAssembler &masm)
{
    
    Label failure;
    masm.branchTestInt32(Assembler::NotEqual, R0, &failure);
    masm.branchTestInt32(Assembler::NotEqual, R1, &failure);

    
    Assembler::Condition cond = JSOpToCondition(op, true);
    masm.cmpl(R0.payloadReg(), R1.payloadReg());
    masm.setCC(cond, R0.payloadReg());
    masm.movzbl(R0.payloadReg(), R0.payloadReg());

    
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
      case JSOP_DIV:
      {
        
        masm.branchTest32(Assembler::Zero, R1.payloadReg(), R1.payloadReg(), &failure);

        
        masm.branch32(Assembler::Equal, R0.payloadReg(), Imm32(INT32_MIN), &failure);

        Label notZero;
        masm.branch32(Assembler::NotEqual, R0.payloadReg(), Imm32(0), &notZero);
        masm.branchTest32(Assembler::Signed, R1.payloadReg(), R1.payloadReg(), &failure);
        masm.bind(&notZero);

        
        JS_ASSERT(R1.typeReg() == eax);
        masm.movl(R0.payloadReg(), eax);
        
        masm.movl(R0.payloadReg(), scratchReg);
        
        masm.cdq();
        masm.idiv(R1.payloadReg());

        
        masm.branchTest32(Assembler::NonZero, edx, edx, &revertRegister);

        masm.movl(eax, R0.payloadReg());
        break;
      }
      case JSOP_MOD:
      {
        
        masm.branchTest32(Assembler::Zero, R1.payloadReg(), R1.payloadReg(), &failure);

        
        masm.branchTest32(Assembler::Zero, R0.payloadReg(), Imm32(0x7fffffff), &failure);

        
        JS_ASSERT(R1.typeReg() == eax);
        masm.movl(R0.payloadReg(), eax);
        
        masm.movl(R0.payloadReg(), scratchReg);
        
        masm.cdq();
        masm.idiv(R1.payloadReg());

        
        Label done;
        masm.branchTest32(Assembler::NonZero, edx, edx, &done);
        masm.branchTest32(Assembler::Signed, scratchReg, scratchReg, &revertRegister);
        masm.branchTest32(Assembler::Signed, R1.payloadReg(), R1.payloadReg(), &revertRegister);

        masm.bind(&done);
        
        JS_ASSERT(R0.payloadReg() == edx);
        JS_ASSERT(R0.typeReg() == ecx);
        break;
      }
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
            masm.convertUInt32ToDouble(R0.payloadReg(), ScratchDoubleReg);
            masm.boxDouble(ScratchDoubleReg, R0);
        } else {
            masm.j(Assembler::Signed, &revertRegister);
            masm.tagValue(JSVAL_TYPE_INT32, R0.payloadReg(), R0);
        }
        break;
      default:
       MOZ_ASSUME_UNREACHABLE("Unhandled op for BinaryArith_Int32.  ");
    }

    
    EmitReturnFromIC(masm);

    switch(op_) {
      case JSOP_MUL:
        masm.bind(&maybeNegZero);

        
        masm.movl(R0.payloadReg(), scratchReg);
        masm.orl(R1.payloadReg(), scratchReg);
        masm.j(Assembler::Signed, &failure);

        
        masm.mov(ImmWord(0), R0.payloadReg());
        EmitReturnFromIC(masm);
        break;
      case JSOP_DIV:
      case JSOP_MOD:
        masm.bind(&revertRegister);
        masm.movl(scratchReg, R0.payloadReg());
        masm.movl(ImmType(JSVAL_TYPE_INT32), R1.typeReg());
        break;
      case JSOP_URSH:
        
        if (!allowDouble_) {
            masm.bind(&revertRegister);
            masm.tagValue(JSVAL_TYPE_INT32, scratchReg, R0);
        }
        break;
      default:
        
        
        break;
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
        MOZ_ASSUME_UNREACHABLE("Unexpected op");
    }

    EmitReturnFromIC(masm);

    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}

} 
} 
