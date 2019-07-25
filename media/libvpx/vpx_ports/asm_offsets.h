










#ifndef VPX_PORTS_ASM_OFFSETS_H
#define VPX_PORTS_ASM_OFFSETS_H

#include <stddef.h>

#define ct_assert(name,cond) \
    static void assert_##name(void) UNUSED;\
    static void assert_##name(void) {switch(0){case 0:case !!(cond):;}}

#if INLINE_ASM
#define DEFINE(sym, val) asm("\n" #sym " EQU %0" : : "i" (val));
#define BEGIN int main(void) {
#define END return 0; }
#else
#define DEFINE(sym, val) int sym = val;
#define BEGIN
#define END
#endif

#endif 
