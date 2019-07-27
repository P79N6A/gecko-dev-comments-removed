





#include "jit/SharedIC.h"
#include "jit/SharedICHelpers.h"

#ifdef JS_ARM64_SIMULATOR

#include "jit/arm64/BaselineCompiler-arm64.h"
#include "jit/arm64/vixl/Debugger-vixl.h"
#endif


using namespace js;
using namespace js::jit;

namespace js {
namespace jit {



bool
ICCompare_Int32::Compiler::generateStubCode(MacroAssembler& masm)
{
    
    Label failure;
    masm.branchTestInt32(Assembler::NotEqual, R0, &failure);
    masm.branchTestInt32(Assembler::NotEqual, R1, &failure);

    
    Assembler::Condition cond = JSOpToCondition(op, true);
    masm.cmp32(R0.valueReg(), R1.valueReg());
    masm.Cset(ARMRegister(R0.valueReg(), 32), cond);

    
    masm.tagValue(JSVAL_TYPE_BOOLEAN, R0.valueReg(), R0);
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

    Register dest = R0.valueReg();

    Assembler::DoubleCondition doubleCond = JSOpToDoubleCondition(op);
    Assembler::Condition cond = Assembler::ConditionFromDoubleCondition(doubleCond);

    masm.compareDouble(doubleCond, FloatReg0, FloatReg1);
    masm.Cset(ARMRegister(dest, 32), cond);

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

    
    Register Rscratch = R2_;
    ARMRegister Wscratch = ARMRegister(Rscratch, 32);
#ifdef MERGE
    
    AllocatableGeneralRegisterSet savedRegs(availableGeneralRegs(2));
    savedRegs.set() = GeneralRegisterSet::Intersect(GeneralRegisterSet::NonVolatile(), savedRegs);
#endif
    
    ARMRegister W0(R0_, 32);
    ARMRegister X0(R0_, 64);
    ARMRegister W1(R1_, 32);
    ARMRegister X1(R1_, 64);
    ARMRegister WTemp(ExtractTemp0, 32);
    ARMRegister XTemp(ExtractTemp0, 64);
    Label maybeNegZero, revertRegister;
    switch(op_) {
      case JSOP_ADD:
        masm.Adds(WTemp, W0, Operand(W1));

        
        
        masm.j(Assembler::Overflow, &failure);

        
        
        masm.movePayload(ExtractTemp0, R0_);
        break;

      case JSOP_SUB:
        masm.Subs(WTemp, W0, Operand(W1));
        masm.j(Assembler::Overflow, &failure);
        masm.movePayload(ExtractTemp0, R0_);
        break;

      case JSOP_MUL:
        masm.mul32(R0.valueReg(), R1.valueReg(), Rscratch, &failure, &maybeNegZero);
        masm.movePayload(Rscratch, R0_);
        break;

      case JSOP_DIV:
      case JSOP_MOD: {

        
        Label check2;
        masm.Cmp(W0, Operand(INT_MIN));
        masm.B(&check2, Assembler::NotEqual);
        masm.Cmp(W1, Operand(-1));
        masm.j(Assembler::Equal, &failure);
        masm.bind(&check2);
        Label no_fail;
        
        masm.Cmp(W1, Operand(0));
        
        masm.B(&no_fail, Assembler::GreaterThan);
        
        
        
        masm.Ccmp(W0, Operand(0), vixl::ZFlag, Assembler::NotEqual);
        masm.B(&failure, Assembler::Equal);
        masm.bind(&no_fail);
        masm.Sdiv(Wscratch, W0, W1);
        
        masm.mul(WTemp, W1, Wscratch);
        if (op_ == JSOP_DIV) {
            
            
            masm.branch32(Assembler::NotEqual, R0.valueReg(), ExtractTemp0, &revertRegister);
            masm.movePayload(Rscratch, R0_);
        } else {
            
            masm.Subs(WTemp, W0, WTemp);

            
            masm.Ccmp(W0, Operand(0), vixl::NoFlag, Assembler::Equal);
            masm.branch(Assembler::LessThan, &revertRegister);
            masm.movePayload(ExtractTemp0, R0_);
        }
        break;
      }
        
        
      case JSOP_BITOR:
        masm.Orr(X0, X0, Operand(X1));
        break;
      case JSOP_BITXOR:
        masm.Eor(X0, X0, Operand(W1, vixl::UXTW));
        break;
      case JSOP_BITAND:
        masm.And(X0, X0, Operand(X1));
        break;
        
      case JSOP_LSH:
        
        masm.Lsl(Wscratch, W0, W1);
        masm.movePayload(Rscratch, R0.valueReg());
        break;
      case JSOP_RSH:
        masm.Asr(Wscratch, W0, W1);
        masm.movePayload(Rscratch, R0.valueReg());
        break;
      case JSOP_URSH:
        masm.Lsr(Wscratch, W0, W1);
        if (allowDouble_) {
            Label toUint;
            
            masm.Tbnz(Wscratch, 31, &toUint);
            
            masm.movePayload(Rscratch, R0_);
            EmitReturnFromIC(masm);

            masm.bind(&toUint);
            masm.convertUInt32ToDouble(Rscratch, ScratchDoubleReg);
            masm.boxDouble(ScratchDoubleReg, R0);
        } else {
            
            masm.Tbnz(Wscratch, 31, &failure);
            
            masm.movePayload(Rscratch, R0_);
        }
        break;
      default:
        MOZ_CRASH("Unhandled op for BinaryArith_Int32.");
    }

    EmitReturnFromIC(masm);

    switch (op_) {
      case JSOP_MUL:
        masm.bind(&maybeNegZero);

        
        masm.Cmn(W0, W1);
        masm.j(Assembler::Signed, &failure);

        
        masm.movePayload(rzr, R0_);
        EmitReturnFromIC(masm);
        break;
      case JSOP_DIV:
      case JSOP_MOD:
        masm.bind(&revertRegister);
        break;
      default:
        break;
    }

    
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
        masm.Mvn(ARMRegister(R1.valueReg(), 32), ARMRegister(R0.valueReg(), 32));
        masm.movePayload(R1.valueReg(), R0.valueReg());
        break;
      case JSOP_NEG:
        
        masm.branchTest32(Assembler::Zero, R0.valueReg(), Imm32(0x7fffffff), &failure);

        
        masm.Sub(ARMRegister(R1.valueReg(), 32), wzr, ARMRegister(R0.valueReg(), 32));
        masm.movePayload(R1.valueReg(), R0.valueReg());
        break;
      default:
        MOZ_CRASH("Unexpected op");
    }

    EmitReturnFromIC(masm);

    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;

}

} 
} 
