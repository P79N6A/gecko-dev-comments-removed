





#include "jit/BaselineHelpers.h"
#include "jit/BaselineIC.h"

using namespace js;
using namespace js::jit;

bool
ICCompare_Double::Compiler::generateStubCode(MacroAssembler& masm)
{
    Label failure, notNaN;
    masm.ensureDouble(R0, FloatReg0, &failure);
    masm.ensureDouble(R1, FloatReg1, &failure);

    Register dest = R0.scratchReg();

    Assembler::DoubleCondition cond = JSOpToDoubleCondition(op);
    masm.mov(ImmWord(0), dest);
    masm.compareDouble(cond, FloatReg0, FloatReg1);
    masm.setCC(Assembler::ConditionFromDoubleCondition(cond), dest);

    
    Assembler::NaNCond nanCond = Assembler::NaNCondFromDoubleCondition(cond);
    if (nanCond != Assembler::NaN_HandledByCond) {
      masm.j(Assembler::NoParity, &notNaN);
      masm.mov(ImmWord(nanCond == Assembler::NaN_IsTrue), dest);
      masm.bind(&notNaN);
    }

    masm.tagValue(JSVAL_TYPE_BOOLEAN, dest, R0);
    EmitReturnFromIC(masm);

    
    masm.bind(&failure);
    EmitStubGuardFailure(masm);
    return true;
}
