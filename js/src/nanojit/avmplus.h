


































#ifndef avm_h___
#define avm_h___

#include "VMPI.h"

#ifdef AVMPLUS_ARM
#define ARM_ARCH   AvmCore::config.arch
#define ARM_VFP    AvmCore::config.vfp
#define ARM_THUMB2 AvmCore::config.thumb2
#else
#define ARM_VFP    1
#define ARM_THUMB2 1
#endif

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

#if defined(DEBUG) || defined(NJ_NO_VARIADIC_MACROS)
#if !defined _DEBUG
#define _DEBUG
#endif
#define NJ_VERBOSE 1
#define NJ_PROFILE 1
#include <stdarg.h>
#endif

#ifdef _DEBUG
void NanoAssertFail();
#endif

#define AvmAssert(x) assert(x)
#define AvmAssertMsg(x, y)
#define AvmDebugLog(x) printf x

#if defined(AVMPLUS_IA32)
#if defined(_MSC_VER)
__declspec(naked) static inline __int64 rdtsc()
{
    __asm
    {
        rdtsc;
        ret;
    }
}
#elif defined(SOLARIS)
static inline unsigned long long rdtsc(void)
{
    unsigned long long int x;
    asm volatile (".byte 0x0f, 0x31" : "=A" (x));
    return x;
}
#elif defined(__i386__)
static __inline__ unsigned long long rdtsc(void)
{
  unsigned long long int x;
     __asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
     return x;
}
#endif 

#elif defined(__x86_64__)

static __inline__ uint64_t rdtsc(void)
{
  unsigned hi, lo;
  __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
  return ( (uint64_t)lo)|( ((uint64_t)hi)<<32 );
}

#elif defined(_MSC_VER) && defined(_M_AMD64)

#include <intrin.h>
#pragma intrinsic(__rdtsc)

static inline unsigned __int64 rdtsc(void)
{
    return __rdtsc();
}

#elif defined(__powerpc__)

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

struct JSContext;

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

    typedef int FunctionID;

    extern void AvmLog(char const *msg, ...);

    class Config
    {
    public:
        Config() {
            memset(this, 0, sizeof(Config));
#ifdef DEBUG
            verbose = false;
            verbose_addrs = 1;
            verbose_exits = 1;
            verbose_live = 1;
            show_stats = 1;
#endif
        }

        uint32_t tree_opt:1;
        uint32_t quiet_opt:1;
        uint32_t verbose:1;
        uint32_t verbose_addrs:1;
        uint32_t verbose_live:1;
        uint32_t verbose_exits:1;
        uint32_t show_stats:1;

#if defined (AVMPLUS_IA32)
    
        bool sse2;
        bool use_cmov;
        
        bool fixed_esp;
#endif

#if defined (AVMPLUS_ARM)
        
# if defined (NJ_FORCE_SOFTFLOAT)
        static const bool vfp = false;
# else
        bool vfp;
# endif

        
# if defined (NJ_FORCE_ARM_ARCH_VERSION)
        static const unsigned int arch = NJ_FORCE_ARM_ARCH_VERSION;
# else
        unsigned int arch;
# endif

        
        
# if defined (NJ_FORCE_NO_ARM_THUMB)
        static const bool thumb = false;
# else
        bool thumb;
# endif

        
        
# if defined (NJ_FORCE_NO_ARM_THUMB2)
        static const bool thumb2 = false;
# else
        bool thumb2;
# endif

#endif

#if defined (NJ_FORCE_SOFTFLOAT)
        static const bool soft_float = true;
#else
        bool soft_float;
#endif
    };

    static const int kstrconst_emptyString = 0;

    class AvmInterpreter
    {
        class Labels {
        public:
            const char* format(const void* ip)
            {
                static char buf[33];
                sprintf(buf, "%p", ip);
                return buf;
            }
        };

        Labels _labels;
    public:
        Labels* labels;

        AvmInterpreter()
        {
            labels = &_labels;
        }

    };

    class AvmConsole
    {
    public:
        AvmConsole& operator<<(const char* s)
        {
            fprintf(stdout, "%s", s);
            return *this;
        }
    };

    class AvmCore
    {
    public:
        AvmInterpreter interp;
        AvmConsole console;

        static Config config;

#ifdef AVMPLUS_IA32
        static inline bool
        use_sse2()
        {
            return config.sse2;
        }
#endif

        static inline bool
        use_cmov()
        {
#ifdef AVMPLUS_IA32
            return config.use_cmov;
#else
        return true;
#endif
        }

        static inline bool
        quiet_opt()
        {
            return config.quiet_opt;
        }

        static inline bool
        verbose()
        {
            return config.verbose;
        }

    };

    











    class BitSet
    {
        public:
            enum {  kUnit = 8*sizeof(long),
                    kDefaultCapacity = 4   };

            BitSet()
            {
                capacity = kDefaultCapacity;
                reset();
            }

            ~BitSet()
            {
                if (capacity > kDefaultCapacity)
                    free(bits.ptr);
            }

            void reset()
            {
                if (capacity > kDefaultCapacity)
                    for(int i=0; i<capacity; i++)
                        bits.ptr[i] = 0;
                else
                    for(int i=0; i<capacity; i++)
                        bits.ar[i] = 0;
            }

            void set(int bitNbr)
            {
                int index = bitNbr / kUnit;
                int bit = bitNbr % kUnit;
                if (index >= capacity)
                    grow(index+1);

                if (capacity > kDefaultCapacity)
                    bits.ptr[index] |= (1<<bit);
                else
                    bits.ar[index] |= (1<<bit);
            }

            void clear(int bitNbr)
            {
                int index = bitNbr / kUnit;
                int bit = bitNbr % kUnit;
                if (index < capacity)
                {
                    if (capacity > kDefaultCapacity)
                        bits.ptr[index] &= ~(1<<bit);
                    else
                        bits.ar[index] &= ~(1<<bit);
                }
            }

            bool get(int bitNbr) const
            {
                int index = bitNbr / kUnit;
                int bit = bitNbr % kUnit;
                bool value = false;
                if (index < capacity)
                {
                    if (capacity > kDefaultCapacity)
                        value = ( bits.ptr[index] & (1<<bit) ) ? true : false;
                    else
                        value = ( bits.ar[index] & (1<<bit) ) ? true : false;
                }
                return value;
            }

        private:
            
            void grow(int newCapacity)
            {
                
                newCapacity *= 2;
                
                long* newBits = (long*)calloc(1, newCapacity * sizeof(long));
                

                
                if (capacity > kDefaultCapacity)
                    for(int i=0; i<capacity; i++)
                        newBits[i] = bits.ptr[i];
                else
                    for(int i=0; i<capacity; i++)
                        newBits[i] = bits.ar[i];

                
                if (capacity > kDefaultCapacity)
                    free(bits.ptr);

                bits.ptr = newBits;
                capacity = newCapacity;
            }

            
            
            
            int capacity;
            union
            {
                long ar[kDefaultCapacity];
                long*  ptr;
            }
            bits;
    };
}

#endif
