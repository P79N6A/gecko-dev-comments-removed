


































#ifndef avm_h___
#define avm_h___

#include "VMPI.h"
#include "njcpudetect.h"
#include "njconfig.h"

#if !defined(AVMPLUS_LITTLE_ENDIAN) && !defined(AVMPLUS_BIG_ENDIAN)
#ifdef IS_BIG_ENDIAN
#define AVMPLUS_BIG_ENDIAN
#else
#define AVMPLUS_LITTLE_ENDIAN
#endif
#endif

#if defined(_MSC_VER) && defined(_M_IX86)
#define FASTCALL __fastcall
#elif defined(__GNUC__) && defined(__i386__) &&                 \
    ((__GNUC__ >= 4) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4))
#define FASTCALL __attribute__((fastcall))
#else
#define FASTCALL
#define NO_FASTCALL
#endif

#if defined(NO_FASTCALL)
#if defined(AVMPLUS_IA32)
#define SIMULATE_FASTCALL(lr, state_ptr, frag_ptr, func_addr)   \
    asm volatile(                                               \
        "call *%%esi"                                           \
        : "=a" (lr)                                             \
        : "c" (state_ptr), "d" (frag_ptr), "S" (func_addr)      \
        : "memory", "cc"                                        \
    );
#endif 
#endif 

#ifdef WIN32
#include <windows.h>
#elif defined(AVMPLUS_OS2)
#define INCL_DOSMEMMGR
#include <os2.h>
#endif

#if defined(__SUNPRO_CC)
#define __asm__ asm
#define __volatile__ volatile
#define __inline__ inline
#endif

#if defined(DEBUG)
#if !defined _DEBUG
#define _DEBUG
#endif
#define NJ_VERBOSE 1
#include <stdarg.h>
#endif

#ifdef _DEBUG
namespace avmplus {
    void AvmAssertFail(const char* msg);
}
#endif

#if defined(AVMPLUS_IA32)
#if defined(_MSC_VER)

# define AVMPLUS_HAS_RDTSC 1

__declspec(naked) static inline __int64 rdtsc()
{
    __asm
    {
        rdtsc;
        ret;
    }
}

#elif defined(__i386__) || defined(__i386)

# define AVMPLUS_HAS_RDTSC 1

static __inline__ unsigned long long rdtsc(void)
{
  unsigned long long int x;
  __asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
  return x;
}

#endif 

#elif defined(__x86_64__)

# define AVMPLUS_HAS_RDTSC 1

static __inline__ uint64_t rdtsc(void)
{
  unsigned hi, lo;
  __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
  return ( (uint64_t)lo)|( ((uint64_t)hi)<<32 );
}

#elif defined(_MSC_VER) && defined(_M_AMD64)

# define AVMPLUS_HAS_RDTSC 1

#include <intrin.h>
#pragma intrinsic(__rdtsc)

static inline unsigned __int64 rdtsc(void)
{
    return __rdtsc();
}

#elif defined(__powerpc__)

# define AVMPLUS_HAS_RDTSC 1

typedef unsigned long long int unsigned long long;

static __inline__ unsigned long long rdtsc(void)
{
  unsigned long long int result=0;
  unsigned long int upper, lower,tmp;
  __asm__ volatile(
                "0:                  \n"
                "\tmftbu   %0           \n"
                "\tmftb    %1           \n"
                "\tmftbu   %2           \n"
                "\tcmpw    %2,%0        \n"
                "\tbne     0b         \n"
                : "=r"(upper),"=r"(lower),"=r"(tmp)
                );
  result = upper;
  result = result<<32;
  result = result|lower;

  return(result);
}

#endif 

#ifndef AVMPLUS_HAS_RDTSC
# define AVMPLUS_HAS_RDTSC 0
#endif

#ifdef PERFM
# define PERFM_NVPROF(n,v) _nvprof(n,v)
# define PERFM_NTPROF(n) _ntprof(n)
# define PERFM_TPROF_END() _tprof_end()
#else
# define PERFM_NVPROF(n,v)
# define PERFM_NTPROF(n)
# define PERFM_TPROF_END()
#endif

namespace avmplus {

    extern void AvmLog(char const *msg, ...);

    







    class BitSet
    {
        public:
            enum {  kUnit = 8*sizeof(long),
                    kDefaultCapacity = 4   };

            BitSet()
            {
                capacity = kDefaultCapacity;
                ar = (long*)calloc(capacity, sizeof(long));
                reset();
            }

            ~BitSet()
            {
                free(ar);
            }

            void reset()
            {
                for (int i = 0; i < capacity; i++)
                    ar[i] = 0;
            }

            void set(int bitNbr)
            {
                int index = bitNbr / kUnit;
                int bit = bitNbr % kUnit;
                if (index >= capacity)
                    grow(index+1);

                ar[index] |= (1<<bit);
            }

            void clear(int bitNbr)
            {
                int index = bitNbr / kUnit;
                int bit = bitNbr % kUnit;
                if (index < capacity)
                    ar[index] &= ~(1<<bit);
            }

            bool get(int bitNbr) const
            {
                int index = bitNbr / kUnit;
                int bit = bitNbr % kUnit;
                bool value = false;
                if (index < capacity)
                    value = ( ar[index] & (1<<bit) ) ? true : false;
                return value;
            }

        private:
            
            void grow(int newCapacity)
            {
                
                newCapacity *= 2;
                
                long* newAr = (long*)calloc(newCapacity, sizeof(long));

                
                for (int i = 0; i < capacity; i++)
                    newAr[i] = ar[i];

                
                free(ar);

                ar = newAr;
                capacity = newCapacity;
            }

            int capacity;
            long* ar;
    };
}

#endif
