





#ifndef jit_arm64_SharedICRegisters_arm64_h
#define jit_arm64_SharedICRegisters_arm64_h

#include "jit/MacroAssembler.h"

namespace js {
namespace jit {


static constexpr Register BaselineFrameReg = r23;
static constexpr ARMRegister BaselineFrameReg64 = { BaselineFrameReg, 64 };



static constexpr Register BaselineStackReg = PseudoStackPointer;




static constexpr Register R0_ = r2;
static constexpr Register R1_ = r19;
static constexpr Register R2_ = r0;

static constexpr ValueOperand R0(R0_);
static constexpr ValueOperand R1(R1_);
static constexpr ValueOperand R2(R2_);


static constexpr Register ICTailCallReg = r30;
static constexpr Register ICStubReg = r9;







static constexpr Register ExtractTemp0 = r24;
static constexpr Register ExtractTemp1 = r25;








static constexpr FloatRegister FloatReg0 = { FloatRegisters::v0 };
static constexpr FloatRegister FloatReg1 = { FloatRegisters::v1 };

} 
} 

#endif 
