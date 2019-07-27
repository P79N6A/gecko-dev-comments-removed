





#ifndef jit_x64_SharedICRegisters_x64_h
#define jit_x64_SharedICRegisters_x64_h

#include "jit/MacroAssembler.h"

namespace js {
namespace jit {

static MOZ_CONSTEXPR_VAR Register BaselineFrameReg    = rbp;
static MOZ_CONSTEXPR_VAR Register BaselineStackReg    = rsp;

static MOZ_CONSTEXPR_VAR ValueOperand R0(rcx);
static MOZ_CONSTEXPR_VAR ValueOperand R1(rbx);
static MOZ_CONSTEXPR_VAR ValueOperand R2(rax);

static MOZ_CONSTEXPR_VAR Register BaselineTailCallReg = rsi;
static MOZ_CONSTEXPR_VAR Register BaselineStubReg     = rdi;

static MOZ_CONSTEXPR_VAR Register ExtractTemp0        = r14;
static MOZ_CONSTEXPR_VAR Register ExtractTemp1        = r15;


static MOZ_CONSTEXPR_VAR FloatRegister FloatReg0      = xmm0;
static MOZ_CONSTEXPR_VAR FloatRegister FloatReg1      = xmm1;

} 
} 

#endif 
