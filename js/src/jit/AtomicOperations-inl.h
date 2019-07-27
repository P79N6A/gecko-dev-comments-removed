





#ifndef jit_AtomicOperations_inl_h
#define jit_AtomicOperations_inl_h

#if defined(JS_CODEGEN_ARM)
# include "jit/arm/AtomicOperations-arm.h"
#elif defined(JS_CODEGEN_ARM64)
# include "jit/arm64/AtomicOperations-arm64.h"
#elif defined(JS_CODEGEN_MIPS)
# include "jit/mips/AtomicOperations-mips.h"
#elif defined(JS_CODEGEN_NONE)
# include "jit/none/AtomicOperations-none.h"
#elif defined(JS_CODEGEN_X86) || defined(JS_CODEGEN_X64)
# include "jit/x86-shared/AtomicOperations-x86-shared.h"
#else
# error "Atomic operations must be defined for this platform"
#endif

inline bool
js::jit::AtomicOperations::isLockfree(int32_t size)
{
    

    switch (size) {
      case 1:
      case 2:
      case 4:
        return true;
      case 8:
        return AtomicOperations::isLockfree8();
      default:
        return false;
    }
}

#endif 
