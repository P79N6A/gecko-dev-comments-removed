





#ifndef jit_mips_SharedICRegisters_mips_h
#define jit_mips_SharedICRegisters_mips_h

#include "jit/MacroAssembler.h"

namespace js {
namespace jit {

static MOZ_CONSTEXPR_VAR Register BaselineFrameReg = s5;
static MOZ_CONSTEXPR_VAR Register BaselineStackReg = sp;

static MOZ_CONSTEXPR_VAR ValueOperand R0(a3, a2);
static MOZ_CONSTEXPR_VAR ValueOperand R1(s7, s6);
static MOZ_CONSTEXPR_VAR ValueOperand R2(t7, t6);



static MOZ_CONSTEXPR_VAR Register ICTailCallReg = ra;
static MOZ_CONSTEXPR_VAR Register ICStubReg = t5;

static MOZ_CONSTEXPR_VAR Register ExtractTemp0 = InvalidReg;
static MOZ_CONSTEXPR_VAR Register ExtractTemp1 = InvalidReg;


static MOZ_CONSTEXPR_VAR Register BaselineSecondScratchReg = SecondScratchReg;






static MOZ_CONSTEXPR_VAR FloatRegister FloatReg0 = f0;
static MOZ_CONSTEXPR_VAR FloatRegister FloatReg1 = f2;

} 
} 

#endif 
