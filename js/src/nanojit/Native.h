







































#ifndef __nanojit_Native__
#define __nanojit_Native__





#define PEDANTIC 0
#if PEDANTIC
#  define UNLESS_PEDANTIC(...)
#  define IF_PEDANTIC(...) __VA_ARGS__
#else
#  define UNLESS_PEDANTIC(...) __VA_ARGS__
#  define IF_PEDANTIC(...)
#endif

#ifdef NANOJIT_IA32
#include "Nativei386.h"
#elif defined(NANOJIT_ARM)
#include "NativeARM.h"
#elif defined(NANOJIT_PPC)
#include "NativePPC.h"
#elif defined(NANOJIT_SPARC)
#include "NativeSparc.h"
#elif defined(NANOJIT_X64)
#include "NativeX64.h"
#else
#error "unknown nanojit architecture"
#endif

#ifndef NJ_JTBL_SUPPORTED
#  define NJ_JTBL_SUPPORTED 0
#endif

#ifndef NJ_EXPANDED_LOADSTORE_SUPPORTED
#  define NJ_EXPANDED_LOADSTORE_SUPPORTED 0
#endif

namespace nanojit {

    inline Register nextreg(Register r) {
        return Register(r+1);
    }

    inline Register prevreg(Register r) {
        return Register(r-1);
    }


    class Fragment;
    struct SideExit;
    struct SwitchInfo;

    struct GuardRecord
    {
        void* jmp;
        GuardRecord* next;
        SideExit* exit;
        
        verbose_only( uint32_t profCount; )
        verbose_only( uint32_t profGuardID; )
        verbose_only( GuardRecord* nextInFrag; )
    };

    struct SideExit
    {
        GuardRecord* guards;
        Fragment* from;
        Fragment* target;
        SwitchInfo* switchInfo;

        void addGuard(GuardRecord* gr)
        {
            NanoAssert(gr->next == NULL);
            NanoAssert(guards != gr);
            gr->next = guards;
            guards = gr;
        }
    };
}

    #ifdef NJ_STACK_GROWTH_UP
        #define stack_direction(n)   n
    #else
        #define stack_direction(n)  -n
    #endif

    #define isSPorFP(r)     ( (r)==SP || (r)==FP )

    #ifdef NJ_NO_VARIADIC_MACROS
        static void asm_output(const char *f, ...) {}
        #define gpn(r)                    regNames[(r)]
        #define fpn(r)                    regNames[(r)]
    #elif defined(NJ_VERBOSE)
        
        
        
        #define asm_output(...) do { \
            counter_increment(native); \
            if (_logc->lcbits & LC_Assembly) { \
                outline[0]='\0'; \
               VMPI_sprintf(outline, "%010lx   ", (unsigned long)_nIns); \
                sprintf(&outline[13], ##__VA_ARGS__); \
                output(); \
            } \
        } while (0) /* no semi */
        #define gpn(r)                  regNames[(r)]
        #define fpn(r)                  regNames[(r)]
    #else
        #define asm_output(...)
        #define gpn(r)
        #define fpn(r)
    #endif /* NJ_VERBOSE */

#endif 
