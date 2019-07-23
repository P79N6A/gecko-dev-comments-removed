







































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

namespace nanojit {
    enum LOpcode
#if defined(_MSC_VER) && _MSC_VER >= 1400
#pragma warning(disable:4480) // nonstandard extension used: specifying underlying type for enum
          : unsigned
#endif
    {
        
        LIR64    = 0x40,            

#define OPDEF(op, number, repkind) \
        LIR_##op = (number),
#define OPD64(op, number, repkind) \
        LIR_##op = ((number) | LIR64),
#include "LIRopcode.tbl"
        LIR_sentinel,
#undef OPDEF
#undef OPD64

#ifdef NANOJIT_64BIT
#  define PTR_SIZE(a,b)  b
#else
#  define PTR_SIZE(a,b)  a
#endif

        
        LIR_ldp     = PTR_SIZE(LIR_ld,     LIR_ldq),
        LIR_ldcp    = PTR_SIZE(LIR_ldc,    LIR_ldqc),
        LIR_stpi    = PTR_SIZE(LIR_sti,    LIR_stqi),
        LIR_piadd   = PTR_SIZE(LIR_add,    LIR_qiadd),
        LIR_piand   = PTR_SIZE(LIR_and,    LIR_qiand),
        LIR_pilsh   = PTR_SIZE(LIR_lsh,    LIR_qilsh),
        LIR_pirsh   = PTR_SIZE(LIR_rsh,    LIR_qirsh),
        LIR_pursh   = PTR_SIZE(LIR_ush,    LIR_qursh),
        LIR_pcmov   = PTR_SIZE(LIR_cmov,   LIR_qcmov),
        LIR_pior    = PTR_SIZE(LIR_or,     LIR_qior),
        LIR_pxor    = PTR_SIZE(LIR_xor,    LIR_qxor),
        LIR_addp    = PTR_SIZE(LIR_iaddp,  LIR_qaddp),
        LIR_peq     = PTR_SIZE(LIR_eq,     LIR_qeq),
        LIR_plt     = PTR_SIZE(LIR_lt,     LIR_qlt),
        LIR_pgt     = PTR_SIZE(LIR_gt,     LIR_qgt),
        LIR_ple     = PTR_SIZE(LIR_le,     LIR_qle),
        LIR_pge     = PTR_SIZE(LIR_ge,     LIR_qge),
        LIR_pult    = PTR_SIZE(LIR_ult,    LIR_qult),
        LIR_pugt    = PTR_SIZE(LIR_ugt,    LIR_qugt),
        LIR_pule    = PTR_SIZE(LIR_ule,    LIR_qule),
        LIR_puge    = PTR_SIZE(LIR_uge,    LIR_quge),
        LIR_alloc   = PTR_SIZE(LIR_ialloc, LIR_qalloc),
        LIR_pcall   = PTR_SIZE(LIR_icall,  LIR_qcall),
        LIR_param   = PTR_SIZE(LIR_iparam, LIR_qparam)
    };
}

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
                if (outputAddr) \
                   VMPI_sprintf(outline, "%010lx   ", (unsigned long)_nIns); \
                else \
                   VMPI_memset(outline, (int)' ', 10+3); \
                sprintf(&outline[13], ##__VA_ARGS__); \
                output(); \
                outputAddr=(_logc->lcbits & LC_NoCodeAddrs) ? false : true;    \
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
