





#ifndef jit_MoveEmitter_h
#define jit_MoveEmitter_h

#if defined(JS_CPU_X86) || defined(JS_CPU_X64)
# include "jit/shared/MoveEmitter-x86-shared.h"
#elif defined(JS_CPU_ARM)
# include "jit/arm/MoveEmitter-arm.h"
#else
# error "CPU Not Supported"
#endif

#endif 
