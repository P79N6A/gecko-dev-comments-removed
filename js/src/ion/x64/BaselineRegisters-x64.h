





#if !defined(jsion_baseline_registers_x64_h__) && defined(JS_ION)
#define jsion_baseline_registers_x64_h__

#include "ion/IonMacroAssembler.h"

namespace js {
namespace ion {

static const Register BaselineFrameReg    = rbp;
static const Register BaselineStackReg    = rsp;

static const ValueOperand R0(rcx);
static const ValueOperand R1(rbx);
static const ValueOperand R2(rax);



static const Register BaselineTailCallReg = rsi;
static const Register BaselineStubReg     = rdi;

static const Register ExtractTemp0        = r14;
static const Register ExtractTemp1        = r15;


static const FloatRegister FloatReg0      = xmm0;
static const FloatRegister FloatReg1      = xmm1;

} 
} 

#endif

