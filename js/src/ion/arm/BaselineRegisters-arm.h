





#ifndef ion_arm_BaselineRegisters_arm_h
#define ion_arm_BaselineRegisters_arm_h

#ifdef JS_ION

#include "ion/IonMacroAssembler.h"

namespace js {
namespace ion {






static MOZ_CONSTEXPR_VAR Register BaselineFrameReg = r11;
static MOZ_CONSTEXPR_VAR Register BaselineStackReg = sp;





static MOZ_CONSTEXPR_VAR ValueOperand R0(r3, r2);
static MOZ_CONSTEXPR_VAR ValueOperand R1(r5, r4);
static MOZ_CONSTEXPR_VAR ValueOperand R2(r1, r0);




static MOZ_CONSTEXPR_VAR Register BaselineTailCallReg = r14;
static MOZ_CONSTEXPR_VAR Register BaselineStubReg     = r9;

static MOZ_CONSTEXPR_VAR Register ExtractTemp0        = InvalidReg;
static MOZ_CONSTEXPR_VAR Register ExtractTemp1        = InvalidReg;


static MOZ_CONSTEXPR_VAR Register BaselineSecondScratchReg = r6;








static MOZ_CONSTEXPR_VAR FloatRegister FloatReg0      = d0;
static MOZ_CONSTEXPR_VAR FloatRegister FloatReg1      = d2;

} 
} 

#endif 

#endif 
