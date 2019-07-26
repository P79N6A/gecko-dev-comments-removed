






#if !defined(jsion_baseline_registers_x86_h__) && defined(JS_ION)
#define jsion_baseline_registers_x86_h__

#include "ion/IonMacroAssembler.h"

namespace js {
namespace ion {

static const Register BaselineFrameReg    = ebp;
static const Register BaselineStackReg    = esp;

static const ValueOperand R0(ecx, edx);
static const ValueOperand R1(eax, ebx);
static const ValueOperand R2(esi, edi);

} 
} 

#endif

