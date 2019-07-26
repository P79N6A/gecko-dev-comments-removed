






#if !defined(jsion_baseline_registers_x64_h__) && defined(JS_ION)
#define jsion_baseline_registers_x64_h__

#include "ion/IonMacroAssembler.h"

namespace js {
namespace ion {

static const Register BaselineFrameReg    = rbp;
static const Register BaselineStackReg    = rsp;

static const ValueOperand R0(r12);
static const ValueOperand R1(r13);
static const ValueOperand R2(r14);

} 
} 

#endif

