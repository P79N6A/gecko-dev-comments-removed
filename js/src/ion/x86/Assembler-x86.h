








































#ifndef jsion_cpu_x86_assembler_h__
#define jsion_cpu_x86_assembler_h__

#include "ion/IonAssembler.h"

namespace js {
namespace ion {

static const Register eax = { RegisterCodes::EAX };
static const Register ecx = { RegisterCodes::ECX };
static const Register edx = { RegisterCodes::EDX };
static const Register ebx = { RegisterCodes::EBX };
static const Register esp = { RegisterCodes::ESP };
static const Register ebp = { RegisterCodes::EBP };
static const Register esi = { RegisterCodes::ESI };
static const Register edi = { RegisterCodes::EDI };

static const Register JSReturnReg_Type = ecx;
static const Register JSReturnReg_Data = edx;

} 
} 

#endif 

