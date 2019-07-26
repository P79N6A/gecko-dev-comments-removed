






#if !defined(jsion_baseline_registers_arm_h__) && defined(JS_ION)
#define jsion_baseline_registers_arm_h__

#include "ion/IonMacroAssembler.h"

namespace js {
namespace ion {






static const Register BaselineFrameReg = r11;
static const Register BaselineStackReg = sp;





static const ValueOperand R0(r3, r2);
static const ValueOperand R1(r5, r4);
static const ValueOperand R2(r1, r0);




static const Register BaselineTailCallReg = r14;
static const Register BaselineStubReg     = r9;

static const Register ExtractTemp0        = InvalidReg;
static const Register ExtractTemp1        = InvalidReg;


static const Register BaselineSecondScratchReg = r6;







} 
} 

#endif

