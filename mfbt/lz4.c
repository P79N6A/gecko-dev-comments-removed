












































#define MEMORY_USAGE 14






#define HEAPMODE 0













#if (defined(__x86_64__) || defined(_M_X64) || defined(_WIN64) \
  || defined(__powerpc64__) || defined(__ppc64__) || defined(__PPC64__) \
  || defined(__64BIT__) || defined(_LP64) || defined(__LP64__) \
  || defined(__ia64) || defined(__itanium__) || defined(_M_IA64) )   
#  define LZ4_ARCH64 1
#else
#  define LZ4_ARCH64 0
#endif



#if defined (__GLIBC__)
#  include <endian.h>
#  if (__BYTE_ORDER == __BIG_ENDIAN)
#     define LZ4_BIG_ENDIAN 1
#  endif
#elif (defined(__BIG_ENDIAN__) || defined(__BIG_ENDIAN) || defined(_BIG_ENDIAN)) && !(defined(__LITTLE_ENDIAN__) || defined(__LITTLE_ENDIAN) || defined(_LITTLE_ENDIAN))
#  define LZ4_BIG_ENDIAN 1
#elif defined(__sparc) || defined(__sparc__) \
   || defined(__powerpc__) || defined(__ppc__) || defined(__PPC__) \
   || defined(__hpux)  || defined(__hppa) \
   || defined(_MIPSEB) || defined(__s390__)
#  define LZ4_BIG_ENDIAN 1
#else

#endif




#if defined(__ARM_FEATURE_UNALIGNED)
#  define LZ4_FORCE_UNALIGNED_ACCESS 1
#endif


#if defined(_MSC_VER) && defined(_WIN32_WCE)            
#  define LZ4_FORCE_SW_BITCOUNT
#endif





#if defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L   

#else
#  define restrict
#endif

#define GCC_VERSION (__GNUC__ * 100 + __GNUC_MINOR__)

#ifdef _MSC_VER    
#  include <intrin.h>   
#  if LZ4_ARCH64   
#    pragma intrinsic(_BitScanForward64) // For Visual 2005
#    pragma intrinsic(_BitScanReverse64) // For Visual 2005
#  else
#    pragma intrinsic(_BitScanForward)   // For Visual 2005
#    pragma intrinsic(_BitScanReverse)   // For Visual 2005
#  endif
#  pragma warning(disable : 4127)        // disable: C4127: conditional expression is constant
#endif

#ifdef _MSC_VER
#  define lz4_bswap16(x) _byteswap_ushort(x)
#else
#  define lz4_bswap16(x) ((unsigned short int) ((((x) >> 8) & 0xffu) | (((x) & 0xffu) << 8)))
#endif

#if (GCC_VERSION >= 302) || (__INTEL_COMPILER >= 800) || defined(__clang__)
#  define expect(expr,value)    (__builtin_expect ((expr),(value)) )
#else
#  define expect(expr,value)    (expr)
#endif

#define likely(expr)     expect((expr) != 0, 1)
#define unlikely(expr)   expect((expr) != 0, 0)





#include <stdlib.h>   
#include <string.h>   
#include "lz4.h"





#if defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L   
# include <stdint.h>
  typedef uint8_t  BYTE;
  typedef uint16_t U16;
  typedef uint32_t U32;
  typedef  int32_t S32;
  typedef uint64_t U64;
#else
  typedef unsigned char       BYTE;
  typedef unsigned short      U16;
  typedef unsigned int        U32;
  typedef   signed int        S32;
  typedef unsigned long long  U64;
#endif

#if defined(__GNUC__)  && !defined(LZ4_FORCE_UNALIGNED_ACCESS)
#  define _PACKED __attribute__ ((packed))
#else
#  define _PACKED
#endif

#if !defined(LZ4_FORCE_UNALIGNED_ACCESS) && !defined(__GNUC__)
#  pragma pack(push, 1)
#endif

typedef struct _U16_S { U16 v; } _PACKED U16_S;
typedef struct _U32_S { U32 v; } _PACKED U32_S;
typedef struct _U64_S { U64 v; } _PACKED U64_S;

#if !defined(LZ4_FORCE_UNALIGNED_ACCESS) && !defined(__GNUC__)
#  pragma pack(pop)
#endif

#define A64(x) (((U64_S *)(x))->v)
#define A32(x) (((U32_S *)(x))->v)
#define A16(x) (((U16_S *)(x))->v)





#define HASHTABLESIZE (1 << MEMORY_USAGE)

#define MINMATCH 4

#define COPYLENGTH 8
#define LASTLITERALS 5
#define MFLIMIT (COPYLENGTH+MINMATCH)
#define MINLENGTH (MFLIMIT+1)

#define LZ4_64KLIMIT ((1<<16) + (MFLIMIT-1))
#define SKIPSTRENGTH 6     // Increasing this value will make the compression run slower on incompressible data

#define MAXD_LOG 16
#define MAX_DISTANCE ((1 << MAXD_LOG) - 1)

#define ML_BITS  4
#define ML_MASK  ((1U<<ML_BITS)-1)
#define RUN_BITS (8-ML_BITS)
#define RUN_MASK ((1U<<RUN_BITS)-1)





#if LZ4_ARCH64   
#  define STEPSIZE 8
#  define UARCH U64
#  define AARCH A64
#  define LZ4_COPYSTEP(s,d)       A64(d) = A64(s); d+=8; s+=8;
#  define LZ4_COPYPACKET(s,d)     LZ4_COPYSTEP(s,d)
#  define LZ4_SECURECOPY(s,d,e)   if (d<e) LZ4_WILDCOPY(s,d,e)
#  define HTYPE                   U32
#  define INITBASE(base)          const BYTE* const base = ip
#else      
#  define STEPSIZE 4
#  define UARCH U32
#  define AARCH A32
#  define LZ4_COPYSTEP(s,d)       A32(d) = A32(s); d+=4; s+=4;
#  define LZ4_COPYPACKET(s,d)     LZ4_COPYSTEP(s,d); LZ4_COPYSTEP(s,d);
#  define LZ4_SECURECOPY          LZ4_WILDCOPY
#  define HTYPE                   const BYTE*
#  define INITBASE(base)          const int base = 0
#endif

#if (defined(LZ4_BIG_ENDIAN) && !defined(BIG_ENDIAN_NATIVE_BUT_INCOMPATIBLE))
#  define LZ4_READ_LITTLEENDIAN_16(d,s,p) { U16 v = A16(p); v = lz4_bswap16(v); d = (s) - v; }
#  define LZ4_WRITE_LITTLEENDIAN_16(p,i)  { U16 v = (U16)(i); v = lz4_bswap16(v); A16(p) = v; p+=2; }
#else      
#  define LZ4_READ_LITTLEENDIAN_16(d,s,p) { d = (s) - A16(p); }
#  define LZ4_WRITE_LITTLEENDIAN_16(p,v)  { A16(p) = v; p+=2; }
#endif





#define LZ4_WILDCOPY(s,d,e)     do { LZ4_COPYPACKET(s,d) } while (d<e);
#define LZ4_BLINDCOPY(s,d,l)    { BYTE* e=(d)+(l); LZ4_WILDCOPY(s,d,e); d=e; }





#if LZ4_ARCH64

static inline int LZ4_NbCommonBytes (register U64 val)
{
#if defined(LZ4_BIG_ENDIAN)
    #if defined(_MSC_VER) && !defined(LZ4_FORCE_SW_BITCOUNT)
    unsigned long r = 0;
    _BitScanReverse64( &r, val );
    return (int)(r>>3);
    #elif defined(__GNUC__) && (GCC_VERSION >= 304) && !defined(LZ4_FORCE_SW_BITCOUNT)
    return (__builtin_clzll(val) >> 3);
    #else
    int r;
    if (!(val>>32)) { r=4; } else { r=0; val>>=32; }
    if (!(val>>16)) { r+=2; val>>=8; } else { val>>=24; }
    r += (!val);
    return r;
    #endif
#else
    #if defined(_MSC_VER) && !defined(LZ4_FORCE_SW_BITCOUNT)
    unsigned long r = 0;
    _BitScanForward64( &r, val );
    return (int)(r>>3);
    #elif defined(__GNUC__) && (GCC_VERSION >= 304) && !defined(LZ4_FORCE_SW_BITCOUNT)
    return (__builtin_ctzll(val) >> 3);
    #else
    static const int DeBruijnBytePos[64] = { 0, 0, 0, 0, 0, 1, 1, 2, 0, 3, 1, 3, 1, 4, 2, 7, 0, 2, 3, 6, 1, 5, 3, 5, 1, 3, 4, 4, 2, 5, 6, 7, 7, 0, 1, 2, 3, 3, 4, 6, 2, 6, 5, 5, 3, 4, 5, 6, 7, 1, 2, 4, 6, 4, 4, 5, 7, 2, 6, 5, 7, 6, 7, 7 };
    return DeBruijnBytePos[((U64)((val & -val) * 0x0218A392CDABBD3F)) >> 58];
    #endif
#endif
}

#else

static inline int LZ4_NbCommonBytes (register U32 val)
{
#if defined(LZ4_BIG_ENDIAN)
#  if defined(_MSC_VER) && !defined(LZ4_FORCE_SW_BITCOUNT)
    unsigned long r = 0;
    _BitScanReverse( &r, val );
    return (int)(r>>3);
#  elif defined(__GNUC__) && (GCC_VERSION >= 304) && !defined(LZ4_FORCE_SW_BITCOUNT)
    return (__builtin_clz(val) >> 3);
#  else
    int r;
    if (!(val>>16)) { r=2; val>>=8; } else { r=0; val>>=24; }
    r += (!val);
    return r;
#  endif
#else
#  if defined(_MSC_VER) && !defined(LZ4_FORCE_SW_BITCOUNT)
    unsigned long r;
    _BitScanForward( &r, val );
    return (int)(r>>3);
#  elif defined(__GNUC__) && (GCC_VERSION >= 304) && !defined(LZ4_FORCE_SW_BITCOUNT)
    return (__builtin_ctz(val) >> 3);
#  else
    static const int DeBruijnBytePos[32] = { 0, 0, 3, 0, 3, 1, 3, 0, 3, 2, 2, 1, 3, 2, 0, 1, 3, 3, 1, 2, 2, 2, 2, 0, 3, 1, 2, 0, 1, 0, 1, 1 };
    return DeBruijnBytePos[((U32)((val & -(S32)val) * 0x077CB531U)) >> 27];
#  endif
#endif
}

#endif

















#define FUNCTION_NAME LZ4_compress_stack
#include "lz4_encoder.h"













#define FUNCTION_NAME LZ4_compress_stack_limitedOutput
#define LIMITED_OUTPUT
#include "lz4_encoder.h"














#define FUNCTION_NAME LZ4_compress64k_stack
#define COMPRESS_64K
#include "lz4_encoder.h"















#define FUNCTION_NAME LZ4_compress64k_stack_limitedOutput
#define COMPRESS_64K
#define LIMITED_OUTPUT
#include "lz4_encoder.h"










void* LZ4_create() { return malloc(HASHTABLESIZE); }
int   LZ4_free(void* ctx) { free(ctx); return 0; }














#define FUNCTION_NAME LZ4_compress_heap
#define USE_HEAPMEMORY
#include "lz4_encoder.h"















#define FUNCTION_NAME LZ4_compress_heap_limitedOutput
#define LIMITED_OUTPUT
#define USE_HEAPMEMORY
#include "lz4_encoder.h"















#define FUNCTION_NAME LZ4_compress64k_heap
#define COMPRESS_64K
#define USE_HEAPMEMORY
#include "lz4_encoder.h"
















#define FUNCTION_NAME LZ4_compress64k_heap_limitedOutput
#define COMPRESS_64K
#define LIMITED_OUTPUT
#define USE_HEAPMEMORY
#include "lz4_encoder.h"


int LZ4_compress(const char* source, char* dest, int inputSize)
{
#if HEAPMODE
    void* ctx = LZ4_create();
    int result;
    if (ctx == NULL) return 0;    
    if (inputSize < LZ4_64KLIMIT)
        result = LZ4_compress64k_heap(ctx, source, dest, inputSize);
    else result = LZ4_compress_heap(ctx, source, dest, inputSize);
    LZ4_free(ctx);
    return result;
#else
    if (inputSize < (int)LZ4_64KLIMIT) return LZ4_compress64k_stack(source, dest, inputSize);
    return LZ4_compress_stack(source, dest, inputSize);
#endif
}


int LZ4_compress_limitedOutput(const char* source, char* dest, int inputSize, int maxOutputSize)
{
#if HEAPMODE
    void* ctx = LZ4_create();
    int result;
    if (ctx == NULL) return 0;    
    if (inputSize < LZ4_64KLIMIT)
        result = LZ4_compress64k_heap_limitedOutput(ctx, source, dest, inputSize, maxOutputSize);
    else result = LZ4_compress_heap_limitedOutput(ctx, source, dest, inputSize, maxOutputSize);
    LZ4_free(ctx);
    return result;
#else
    if (inputSize < (int)LZ4_64KLIMIT) return LZ4_compress64k_stack_limitedOutput(source, dest, inputSize, maxOutputSize);
    return LZ4_compress_stack_limitedOutput(source, dest, inputSize, maxOutputSize);
#endif
}






typedef enum { noPrefix = 0, withPrefix = 1 } prefix64k_directive;
typedef enum { endOnOutputSize = 0, endOnInputSize = 1 } end_directive;
typedef enum { full = 0, partial = 1 } exit_directive;






static inline int LZ4_decompress_generic(
                 const char* source,
                 char* dest,
                 int inputSize,          
                 int outputSize,         

                 int endOnInput,         
                 int prefix64k,          
                 int partialDecoding,    
                 int targetOutputSize    
                 )
{
    
    const BYTE* restrict ip = (const BYTE*) source;
    const BYTE* ref;
    const BYTE* const iend = ip + inputSize;

    BYTE* op = (BYTE*) dest;
    BYTE* const oend = op + outputSize;
    BYTE* cpy;
    BYTE* oexit = op + targetOutputSize;

    size_t dec32table[] = {0, 3, 2, 3, 0, 0, 0, 0};
#if LZ4_ARCH64
    size_t dec64table[] = {0, 0, 0, (size_t)-1, 0, 1, 2, 3};
#endif


    
    if ((partialDecoding) && (oexit> oend-MFLIMIT)) oexit = oend-MFLIMIT;   
    if unlikely(outputSize==0) goto _output_error;                          


    
    while (1)
    {
        unsigned token;
        size_t length;

        
        token = *ip++;
        if ((length=(token>>ML_BITS)) == RUN_MASK)  
        { 
            unsigned s=255; 
            while (((endOnInput)?ip<iend:1) && (s==255)) 
            { 
                s = *ip++; 
                length += s; 
            } 
        }

        
        cpy = op+length;
        if (((endOnInput) && ((cpy>(partialDecoding?oexit:oend-MFLIMIT)) || (ip+length>iend-(2+1+LASTLITERALS))) )
            || ((!endOnInput) && (cpy>oend-COPYLENGTH)))
        {
            if (partialDecoding)
            {
                if (cpy > oend) goto _output_error;                            
                if ((endOnInput) && (ip+length > iend)) goto _output_error;    
            }
            else
            {
                if ((!endOnInput) && (cpy != oend)) goto _output_error;        
                if ((endOnInput) && ((ip+length != iend) || (cpy > oend))) goto _output_error;   
            }
            memcpy(op, ip, length);
            ip += length;
            op += length;
            break;                                       
        }
        LZ4_WILDCOPY(ip, op, cpy); ip -= (op-cpy); op = cpy;

        
        LZ4_READ_LITTLEENDIAN_16(ref,cpy,ip); ip+=2;
        if ((prefix64k==noPrefix) && unlikely(ref < (BYTE* const)dest)) goto _output_error;   

        
        if ((length=(token&ML_MASK)) == ML_MASK) 
        { 
            while (endOnInput ? ip<iend-(LASTLITERALS+1) : 1)    
            { 
                unsigned s = *ip++; 
                length += s; 
                if (s==255) continue; 
                break; 
            } 
        }

        
        if unlikely((op-ref)<STEPSIZE)
        {
#if LZ4_ARCH64
            size_t dec64 = dec64table[op-ref];
#else
            const size_t dec64 = 0;
#endif
            op[0] = ref[0];
            op[1] = ref[1];
            op[2] = ref[2];
            op[3] = ref[3];
            op += 4, ref += 4; ref -= dec32table[op-ref];
            A32(op) = A32(ref); 
            op += STEPSIZE-4; ref -= dec64;
        } else { LZ4_COPYSTEP(ref,op); }
        cpy = op + length - (STEPSIZE-4);

        if unlikely(cpy>oend-(COPYLENGTH)-(STEPSIZE-4))
        {
            if (cpy > oend-LASTLITERALS) goto _output_error;    
            LZ4_SECURECOPY(ref, op, (oend-COPYLENGTH));
            while(op<cpy) *op++=*ref++;
            op=cpy;
            continue;
        }
        LZ4_WILDCOPY(ref, op, cpy);
        op=cpy;   
    }

    
    if (endOnInput)
       return (int) (((char*)op)-dest);     
    else
       return (int) (((char*)ip)-source);   

    
_output_error:
    return (int) (-(((char*)ip)-source))-1;
}


int LZ4_decompress_safe(const char* source, char* dest, int inputSize, int maxOutputSize)
{
    return LZ4_decompress_generic(source, dest, inputSize, maxOutputSize, endOnInputSize, noPrefix, full, 0);
}

int LZ4_decompress_fast(const char* source, char* dest, int outputSize)
{
    return LZ4_decompress_generic(source, dest, 0, outputSize, endOnOutputSize, noPrefix, full, 0);
}

int LZ4_decompress_safe_withPrefix64k(const char* source, char* dest, int inputSize, int maxOutputSize)
{
    return LZ4_decompress_generic(source, dest, inputSize, maxOutputSize, endOnInputSize, withPrefix, full, 0);
}

int LZ4_decompress_fast_withPrefix64k(const char* source, char* dest, int outputSize)
{
    return LZ4_decompress_generic(source, dest, 0, outputSize, endOnOutputSize, withPrefix, full, 0);
}

int LZ4_decompress_safe_partial(const char* source, char* dest, int inputSize, int targetOutputSize, int maxOutputSize)
{
    return LZ4_decompress_generic(source, dest, inputSize, maxOutputSize, endOnInputSize, noPrefix, partial, targetOutputSize);
}

