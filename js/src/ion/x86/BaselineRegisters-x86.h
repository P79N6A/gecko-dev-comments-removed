





#ifndef jsion_baseline_registers_x86_h__
#define jsion_baseline_registers_x86_h__

#ifdef JS_ION

#include "ion/IonMacroAssembler.h"

namespace js {
namespace ion {

static const Register BaselineFrameReg = ebp;
static const Register BaselineStackReg = esp;


static const ValueOperand R0(ecx, edx);
static const ValueOperand R1(eax, ebx);
static const ValueOperand R2(esi, edi);



static const Register BaselineTailCallReg = esi;
static const Register BaselineStubReg     = edi;

static const Register ExtractTemp0        = InvalidReg;
static const Register ExtractTemp1        = InvalidReg;


static const FloatRegister FloatReg0      = xmm0;
static const FloatRegister FloatReg1      = xmm1;

} 
} 

#endif 

#endif 

