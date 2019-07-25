







































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
#elif defined(NANOJIT_SH4)
#include "NativeSH4.h"
#elif defined(NANOJIT_MIPS)
#include "NativeMIPS.h"
#else
#error "unknown nanojit architecture"
#endif

#ifndef NJ_USES_IMMD_POOL
#  define NJ_USES_IMMD_POOL 0
#endif

#ifndef NJ_JTBL_SUPPORTED
#  define NJ_JTBL_SUPPORTED 0
#endif

#ifndef NJ_EXPANDED_LOADSTORE_SUPPORTED
#  define NJ_EXPANDED_LOADSTORE_SUPPORTED 0
#endif

#ifndef NJ_F2I_SUPPORTED
#  define NJ_F2I_SUPPORTED 0
#endif

#ifndef NJ_SOFTFLOAT_SUPPORTED
#  define NJ_SOFTFLOAT_SUPPORTED 0
#endif

#ifndef NJ_DIVI_SUPPORTED
#  define NJ_DIVI_SUPPORTED 0
#endif

#if NJ_SOFTFLOAT_SUPPORTED
    #define CASESF(x)   case x
#else
    #define CASESF(x)
#endif

namespace nanojit {

    class Fragment;
    struct SideExit;

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

        void addGuard(GuardRecord* gr)
        {
            NanoAssert(gr->next == NULL);
            NanoAssert(guards != gr);
            gr->next = guards;
            guards = gr;
        }
    };
}

    #define isSPorFP(r)     ( (r)==SP || (r)==FP )

    #if defined(NJ_VERBOSE)
        inline char cvaltoa(unsigned char u) {
            return u<10 ? u+'0' : u+'a'-10;
        }

        inline char* appendHexVals(char* str, char* valFrom, char* valTo) {
            NanoAssert(valFrom <= valTo);
            str += VMPI_strlen(str);
            for(char* ch = valFrom; ch < valTo; ch++) {
                unsigned char u = (unsigned char)*ch;
                *str++ = cvaltoa(u >> 4);
                *str++ = cvaltoa(u &  0xf);
                *str++ = ' ';
            }
            *str = '\0';
            return str;
        }

        inline char* padTo(char* str, int n, char c=' ') {
            char* start = str + VMPI_strlen(str);
            char* end = &str[n];
            while(start < end)
                *start++ = c;
            *end = '\0';
            return end;
        }

        
        
        
        #define asm_output(...) do {                                            \
            if (_logc->lcbits & LC_Native) {                                    \
                outline[0]='\0';                                                \
                VMPI_sprintf(outline, "%p  ", _nIns);                           \
                if (_logc->lcbits & LC_Bytes) {                                 \
                    appendHexVals(outline, (char*)_nIns, (char*)_nInsAfter);    \
                    padTo(outline, 3*15);                                       \
                }                                                               \
                VMPI_sprintf(outline + VMPI_strlen(outline), ##__VA_ARGS__);    \
                output();                                                       \
                _nInsAfter = _nIns;                                             \
            }                                                                   \
        } while (0) /* no semi */
        #define gpn(r)                  regNames[(REGNUM(r))]
    #else
        #define asm_output(...)
        #define gpn(r)
    #endif 

#endif 
