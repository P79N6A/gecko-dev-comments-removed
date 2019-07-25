






































#ifndef __nanojit_h__
#define __nanojit_h__

#include "avmplus.h"

#ifdef FEATURE_NANOJIT

#if defined AVMPLUS_IA32
    #define NANOJIT_IA32
#elif defined AVMPLUS_ARM
    #define NANOJIT_ARM
#elif defined AVMPLUS_PPC
    #define NANOJIT_PPC
#elif defined AVMPLUS_SPARC
    #define NANOJIT_SPARC
#elif defined AVMPLUS_AMD64
    #define NANOJIT_X64
#elif defined VMCFG_SH4
    #define NANOJIT_SH4
#elif defined AVMPLUS_MIPS
    #define NANOJIT_MIPS
#else
    #error "unknown nanojit architecture"
#endif

#ifdef AVMPLUS_64BIT
#define NANOJIT_64BIT
#endif

#if defined NANOJIT_64BIT
    #define IF_64BIT(...) __VA_ARGS__
    #define UNLESS_64BIT(...)
    #define CASE32(x)
    #define CASE64(x)   case x
#else
    #define IF_64BIT(...)
    #define UNLESS_64BIT(...) __VA_ARGS__
    #define CASE32(x)   case x
    #define CASE64(x)
#endif

#if defined NANOJIT_IA32 || defined NANOJIT_X64
    #define CASE86(x)   case x
#else
    #define CASE86(x)
#endif


#ifdef MOZ_VALGRIND
#  define JS_VALGRIND
#endif
#ifdef JS_VALGRIND
#  include <valgrind/valgrind.h>
#elif !defined(VALGRIND_DISCARD_TRANSLATIONS)
#  define VALGRIND_DISCARD_TRANSLATIONS(addr, szB)
#endif

namespace nanojit
{
    




    typedef avmplus::AvmCore AvmCore;

    const uint32_t MAXARGS = 8;

    #if defined(_DEBUG)

        #define __NanoAssertMsgf(a, file_, line_, f, ...)  \
            if (!(a)) { \
                avmplus::AvmLog("Assertion failure: " f "%s (%s:%d)\n", __VA_ARGS__, #a, file_, line_); \
                avmplus::AvmAssertFail(""); \
            }

        #define _NanoAssertMsgf(a, file_, line_, f, ...)   __NanoAssertMsgf(a, file_, line_, f, __VA_ARGS__)

        #define NanoAssertMsgf(a,f,...)   do { __NanoAssertMsgf(a, __FILE__, __LINE__, f ": ", __VA_ARGS__); } while (0)
        #define NanoAssertMsg(a,m)        do { __NanoAssertMsgf(a, __FILE__, __LINE__, "\"%s\": ", m); } while (0)
        #define NanoAssert(a)             do { __NanoAssertMsgf(a, __FILE__, __LINE__, "%s", ""); } while (0)
    #else
        #define NanoAssertMsgf(a,f,...)   do { } while (0) /* no semi */
        #define NanoAssertMsg(a,m)        do { } while (0) /* no semi */
        #define NanoAssert(a)             do { } while (0) /* no semi */
    #endif

    





    #ifdef __SUNPRO_CC
        #define NanoStaticAssert(condition)
    #else
        #define NanoStaticAssert(condition) \
            extern void nano_static_assert(int arg[(condition) ? 1 : -1])
    #endif


    




}

#ifdef AVMPLUS_VERBOSE
    #define NJ_VERBOSE 1
#endif

#if defined(NJ_VERBOSE)
    #include <stdio.h>
    #define verbose_outputf            if (_logc->lcbits & LC_Native) \
                                        Assembler::outputf
    #define verbose_only(...)        __VA_ARGS__
#else
    #define verbose_outputf
    #define verbose_only(...)
#endif 

#ifdef _DEBUG
    #define debug_only(x)           x
#else
    #define debug_only(x)
#endif 

#define isS8(i)  ( int32_t(i) == int8_t(i) )
#define isU8(i)  ( int32_t(i) == uint8_t(i) )
#define isS16(i) ( int32_t(i) == int16_t(i) )
#define isU16(i) ( int32_t(i) == uint16_t(i) )
#define isS24(i) ( (int32_t((i)<<8)>>8) == (i) )

static inline bool isS32(intptr_t i) {
    return int32_t(i) == i;
}

static inline bool isU32(uintptr_t i) {
    return uint32_t(i) == i;
}

#define alignTo(x,s)        ((((uintptr_t)(x)))&~(((uintptr_t)s)-1))
#define alignUp(x,s)        ((((uintptr_t)(x))+(((uintptr_t)s)-1))&~(((uintptr_t)s)-1))

#define NJ_MIN(x, y) ((x) < (y) ? (x) : (y))
#define NJ_MAX(x, y) ((x) > (y) ? (x) : (y))

namespace nanojit
{



#if defined(_WIN32) && (_MSC_VER >= 1300) && (defined(_M_IX86) || defined(_M_AMD64) || defined(_M_X64))

    extern "C" unsigned char _BitScanForward(unsigned long * Index, unsigned long Mask);
    extern "C" unsigned char _BitScanReverse(unsigned long * Index, unsigned long Mask);
    # pragma intrinsic(_BitScanForward)
    # pragma intrinsic(_BitScanReverse)

    
    static inline int msbSet32(uint32_t x) {
        unsigned long idx;
        _BitScanReverse(&idx, (unsigned long)(x | 1)); 
        return idx;
    }

    
    static inline int lsbSet32(uint32_t x) {
        unsigned long idx;
        _BitScanForward(&idx, (unsigned long)(x | 0x80000000)); 
        return idx;
    }

#if defined(_M_AMD64) || defined(_M_X64)
    extern "C" unsigned char _BitScanForward64(unsigned long * Index, unsigned __int64 Mask);
    extern "C" unsigned char _BitScanReverse64(unsigned long * Index, unsigned __int64 Mask);
    # pragma intrinsic(_BitScanForward64)
    # pragma intrinsic(_BitScanReverse64)

    
    static inline int msbSet64(uint64_t x) {
        unsigned long idx;
        _BitScanReverse64(&idx, (unsigned __int64)(x | 1)); 
        return idx;
    }

    
    static inline int lsbSet64(uint64_t x) {
        unsigned long idx;
        _BitScanForward64(&idx, (unsigned __int64)(x | 0x8000000000000000LL)); 
        return idx;
    }
#else
    
    static int msbSet64(uint64_t x) {
        return (x & 0xffffffff00000000LL) ? msbSet32(uint32_t(x >> 32)) + 32 : msbSet32(uint32_t(x));
    }
    
    static int lsbSet64(uint64_t x) {
        return (x & 0x00000000ffffffffLL) ? lsbSet32(uint32_t(x)) : lsbSet32(uint32_t(x >> 32)) + 32;
    }
#endif

#elif (__GNUC__ >= 4) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)

    
    static inline int msbSet32(uint32_t x) {
        return 31 - __builtin_clz(x | 1);
    }

    
    static inline int lsbSet32(uint32_t x) {
        return __builtin_ctz(x | 0x80000000);
    }

    
    static inline int msbSet64(uint64_t x) {
        return 63 - __builtin_clzll(x | 1);
    }

    
    static inline int lsbSet64(uint64_t x) {
        return __builtin_ctzll(x | 0x8000000000000000LL);
    }

#else

    
    static int msbSet32(uint32_t x) {
        for (int i = 31; i >= 0; i--)
            if ((1 << i) & x)
                return i;
        return 0;
    }

    
    static int lsbSet32(uint32_t x) {
        for (int i = 0; i < 32; i++)
            if ((1 << i) & x)
                return i;
        return 31;
    }

    
    static int msbSet64(uint64_t x) {
        for (int i = 63; i >= 0; i--)
            if ((1LL << i) & x)
                return i;
        return 0;
    }

    
    static int lsbSet64(uint64_t x) {
        for (int i = 0; i < 64; i++)
            if ((1LL << i) & x)
                return i;
        return 63;
    }

#endif 
} 














# if defined(__GNUC__)
# define PRINTF_CHECK(x, y) __attribute__((format(__printf__, x, y)))
# else
# define PRINTF_CHECK(x, y)
# endif

namespace nanojit {

    

    enum LC_Bits {
        


        
        LC_FragProfile      = 1<<8, 
        LC_Liveness         = 1<<7, 
        LC_ReadLIR          = 1<<6, 
        LC_AfterSF          = 1<<5, 
        LC_AfterDCE         = 1<<4, 
        LC_Bytes            = 1<<3, 
        LC_Native           = 1<<2, 
        LC_RegAlloc         = 1<<1, 
        LC_Activation       = 1<<0  
    };

    class LogControl
    {
    public:
        
        
        virtual ~LogControl() {}
        #ifdef NJ_VERBOSE
        virtual void printf( const char* format, ... ) PRINTF_CHECK(2,3);
        #endif

        
        uint32_t lcbits;
    };
}






#include "njconfig.h"
#include "Allocator.h"
#include "Containers.h"
#include "Native.h"
#include "CodeAlloc.h"
#include "LIR.h"
#include "RegAlloc.h"
#include "Fragmento.h"
#include "Assembler.h"

#endif 
#endif 
