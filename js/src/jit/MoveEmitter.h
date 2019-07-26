





#ifndef jit_MoveEmitter_h
#define jit_MoveEmitter_h

#if defined(JS_CODEGEN_X86) || defined(JS_CODEGEN_X64)
# include "jit/shared/MoveEmitter-x86-shared.h"
#elif defined(JS_CODEGEN_ARM)
# include "jit/arm/MoveEmitter-arm.h"
#elif defined(JS_CODEGEN_MIPS)
# include "jit/mips/MoveEmitter-mips.h"
#else
# error "Unknown architecture!"
#endif

#endif 
