






#ifndef jsion_move_emitter_h__
#define jsion_move_emitter_h__

#if defined(JS_CPU_X86) || defined(JS_CPU_X64)
# include "ion/shared/MoveEmitter-x86-shared.h"
#elif defined(JS_CPU_ARM)
# include "ion/arm/MoveEmitter-arm.h"
#else
# error "CPU Not Supported"
#endif

#endif 

