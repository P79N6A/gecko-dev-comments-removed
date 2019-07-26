





#ifndef ion_x86_BaselineRegisters_x86_h
#define ion_x86_BaselineRegisters_x86_h

#ifdef JS_ION

#include "ion/IonMacroAssembler.h"

namespace js {
namespace ion {

static MOZ_CONSTEXPR_VAR Register BaselineFrameReg = ebp;
static MOZ_CONSTEXPR_VAR Register BaselineStackReg = esp;


static MOZ_CONSTEXPR_VAR ValueOperand R0(ecx, edx);
static MOZ_CONSTEXPR_VAR ValueOperand R1(eax, ebx);
static MOZ_CONSTEXPR_VAR ValueOperand R2(esi, edi);



static MOZ_CONSTEXPR_VAR Register BaselineTailCallReg = esi;
static MOZ_CONSTEXPR_VAR Register BaselineStubReg     = edi;

static MOZ_CONSTEXPR_VAR Register ExtractTemp0        = InvalidReg;
static MOZ_CONSTEXPR_VAR Register ExtractTemp1        = InvalidReg;


static MOZ_CONSTEXPR_VAR FloatRegister FloatReg0      = xmm0;
static MOZ_CONSTEXPR_VAR FloatRegister FloatReg1      = xmm1;

} 
} 

#endif 

#endif 
