





#ifndef jit_none_BaselineRegisters_none_h
#define jit_none_BaselineRegisters_none_h

#include "jit/IonMacroAssembler.h"

namespace js {
namespace jit {

static MOZ_CONSTEXPR_VAR Register BaselineFrameReg = { 0 };
static MOZ_CONSTEXPR_VAR Register BaselineStackReg = { 0 };

static MOZ_CONSTEXPR_VAR ValueOperand R0 = JSReturnOperand;
static MOZ_CONSTEXPR_VAR ValueOperand R1 = JSReturnOperand;
static MOZ_CONSTEXPR_VAR ValueOperand R2 = JSReturnOperand;

static MOZ_CONSTEXPR_VAR Register BaselineTailCallReg = { 0 };
static MOZ_CONSTEXPR_VAR Register BaselineStubReg = { 0 };

static MOZ_CONSTEXPR_VAR Register ExtractTemp0 = { 0 };
static MOZ_CONSTEXPR_VAR Register ExtractTemp1 = { 0 };

static MOZ_CONSTEXPR_VAR FloatRegister FloatReg0 = { 0 };
static MOZ_CONSTEXPR_VAR FloatRegister FloatReg1 = { 0 };

} 
} 

#endif 

