






#ifndef js_utility_h__
#define js_utility_h__

#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"
#include "mozilla/Scoped.h"

#include <stdlib.h>
#include <string.h>

#ifdef JS_OOM_DO_BACKTRACES
#include <stdio.h>
#include <execinfo.h>
#endif

#include "jstypes.h"

#include "js/TemplateLib.h"


namespace JS {}


namespace mozilla {}


namespace js {}





#define JS_FREE_PATTERN 0xDA

#define JS_ASSERT(expr)           MOZ_ASSERT(expr)
#define JS_ASSERT_IF(cond, expr)  MOZ_ASSERT_IF(cond, expr)
#define JS_NOT_REACHED(reason)    MOZ_NOT_REACHED(reason)
#define JS_ALWAYS_TRUE(expr)      MOZ_ALWAYS_TRUE(expr)
#define JS_ALWAYS_FALSE(expr)     MOZ_ALWAYS_FALSE(expr)

#ifdef DEBUG
# ifdef JS_THREADSAFE
#  define JS_THREADSAFE_ASSERT(expr) JS_ASSERT(expr)
# else
#  define JS_THREADSAFE_ASSERT(expr) ((void) 0)
# endif
#else
# define JS_THREADSAFE_ASSERT(expr) ((void) 0)
#endif

#if defined(DEBUG)
# define JS_DIAGNOSTICS_ASSERT(expr) MOZ_ASSERT(expr)
#elif defined(JS_CRASH_DIAGNOSTICS)
# define JS_DIAGNOSTICS_ASSERT(expr) do { if (!(expr)) MOZ_CRASH(); } while(0)
#else
# define JS_DIAGNOSTICS_ASSERT(expr) ((void) 0)
#endif

#define JS_STATIC_ASSERT(cond)           MOZ_STATIC_ASSERT(cond, "JS_STATIC_ASSERT")
#define JS_STATIC_ASSERT_IF(cond, expr)  MOZ_STATIC_ASSERT_IF(cond, expr, "JS_STATIC_ASSERT_IF")

extern MOZ_NORETURN JS_PUBLIC_API(void)
JS_Assert(const char *s, const char *file, int ln);






extern JS_PUBLIC_API(void) JS_Abort(void);




#if defined JS_USE_CUSTOM_ALLOCATOR
# include "jscustomallocator.h"
#else
# ifdef DEBUG




extern JS_PUBLIC_DATA(uint32_t) OOM_maxAllocations; 
extern JS_PUBLIC_DATA(uint32_t) OOM_counter; 

#ifdef JS_OOM_DO_BACKTRACES
#define JS_OOM_BACKTRACE_SIZE 32
static JS_ALWAYS_INLINE void
PrintBacktrace()
{
    void* OOM_trace[JS_OOM_BACKTRACE_SIZE];
    char** OOM_traceSymbols = NULL;
    int32_t OOM_traceSize = 0;
    int32_t OOM_traceIdx = 0;
    OOM_traceSize = backtrace(OOM_trace, JS_OOM_BACKTRACE_SIZE);
    OOM_traceSymbols = backtrace_symbols(OOM_trace, OOM_traceSize);

    if (!OOM_traceSymbols)
        return;

    for (OOM_traceIdx = 0; OOM_traceIdx < OOM_traceSize; ++OOM_traceIdx) {
        fprintf(stderr, "#%d %s\n", OOM_traceIdx, OOM_traceSymbols[OOM_traceIdx]);
    }

    free(OOM_traceSymbols);
}

#define JS_OOM_EMIT_BACKTRACE() \
    do {\
        fprintf(stderr, "Forcing artificial memory allocation function failure:\n");\
	PrintBacktrace();\
    } while (0)
# else
#  define JS_OOM_EMIT_BACKTRACE() do {} while(0)
#endif 

#  define JS_OOM_POSSIBLY_FAIL() \
    do \
    { \
        if (++OOM_counter > OOM_maxAllocations) { \
            JS_OOM_EMIT_BACKTRACE();\
            return NULL; \
        } \
    } while (0)

#  define JS_OOM_POSSIBLY_FAIL_REPORT(cx) \
    do \
    { \
        if (++OOM_counter > OOM_maxAllocations) { \
            JS_OOM_EMIT_BACKTRACE();\
            js_ReportOutOfMemory(cx);\
            return NULL; \
        } \
    } while (0)

# else
#  define JS_OOM_POSSIBLY_FAIL() do {} while(0)
#  define JS_OOM_POSSIBLY_FAIL_REPORT(cx) do {} while(0)
# endif 

static JS_INLINE void* js_malloc(size_t bytes)
{
    JS_OOM_POSSIBLY_FAIL();
    return malloc(bytes);
}

static JS_INLINE void* js_calloc(size_t bytes)
{
    JS_OOM_POSSIBLY_FAIL();
    return calloc(bytes, 1);
}

static JS_INLINE void* js_realloc(void* p, size_t bytes)
{
    JS_OOM_POSSIBLY_FAIL();
    return realloc(p, bytes);
}

static JS_INLINE void js_free(void* p)
{
    free(p);
}
#endif

JS_BEGIN_EXTERN_C









#if defined(_WIN32) && (_MSC_VER >= 1300) && (defined(_M_IX86) || defined(_M_AMD64) || defined(_M_X64))

unsigned char _BitScanForward(unsigned long * Index, unsigned long Mask);
unsigned char _BitScanReverse(unsigned long * Index, unsigned long Mask);
# pragma intrinsic(_BitScanForward,_BitScanReverse)

__forceinline static int
__BitScanForward32(unsigned int val)
{
    unsigned long idx;

    _BitScanForward(&idx, (unsigned long)val);
    return (int)idx;
}
__forceinline static int
__BitScanReverse32(unsigned int val)
{
    unsigned long idx;

    _BitScanReverse(&idx, (unsigned long)val);
    return (int)(31-idx);
}
# define js_bitscan_ctz32(val)  __BitScanForward32(val)
# define js_bitscan_clz32(val)  __BitScanReverse32(val)
# define JS_HAS_BUILTIN_BITSCAN32

#if defined(_M_AMD64) || defined(_M_X64)
unsigned char _BitScanForward64(unsigned long * Index, unsigned __int64 Mask);
unsigned char _BitScanReverse64(unsigned long * Index, unsigned __int64 Mask);
# pragma intrinsic(_BitScanForward64,_BitScanReverse64)

__forceinline static int
__BitScanForward64(unsigned __int64 val)
{
    unsigned long idx;

    _BitScanForward64(&idx, val);
    return (int)idx;
}
__forceinline static int
__BitScanReverse64(unsigned __int64 val)
{
    unsigned long idx;

    _BitScanReverse64(&idx, val);
    return (int)(63-idx);
}
# define js_bitscan_ctz64(val)  __BitScanForward64(val)
# define js_bitscan_clz64(val)  __BitScanReverse64(val)
# define JS_HAS_BUILTIN_BITSCAN64
#endif
#elif (__GNUC__ >= 4) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)

# define js_bitscan_ctz32(val)  __builtin_ctz(val)
# define js_bitscan_clz32(val)  __builtin_clz(val)
# define JS_HAS_BUILTIN_BITSCAN32
# if (JS_BYTES_PER_WORD == 8)
#  define js_bitscan_ctz64(val)  __builtin_ctzll(val)
#  define js_bitscan_clz64(val)  __builtin_clzll(val)
#  define JS_HAS_BUILTIN_BITSCAN64
# endif

#endif





#ifdef JS_HAS_BUILTIN_BITSCAN32





# define JS_CEILING_LOG2(_log2,_n)                                            \
    JS_BEGIN_MACRO                                                            \
        unsigned int j_ = (unsigned int)(_n);                                 \
        (_log2) = (j_ <= 1 ? 0 : 32 - js_bitscan_clz32(j_ - 1));              \
    JS_END_MACRO
#else
# define JS_CEILING_LOG2(_log2,_n)                                            \
    JS_BEGIN_MACRO                                                            \
        uint32_t j_ = (uint32_t)(_n);                                         \
        (_log2) = 0;                                                          \
        if ((j_) & ((j_)-1))                                                  \
            (_log2) += 1;                                                     \
        if ((j_) >> 16)                                                       \
            (_log2) += 16, (j_) >>= 16;                                       \
        if ((j_) >> 8)                                                        \
            (_log2) += 8, (j_) >>= 8;                                         \
        if ((j_) >> 4)                                                        \
            (_log2) += 4, (j_) >>= 4;                                         \
        if ((j_) >> 2)                                                        \
            (_log2) += 2, (j_) >>= 2;                                         \
        if ((j_) >> 1)                                                        \
            (_log2) += 1;                                                     \
    JS_END_MACRO
#endif







#ifdef JS_HAS_BUILTIN_BITSCAN32





# define JS_FLOOR_LOG2(_log2,_n)                                              \
    JS_BEGIN_MACRO                                                            \
        (_log2) = 31 - js_bitscan_clz32(((unsigned int)(_n)) | 1);            \
    JS_END_MACRO
#else
# define JS_FLOOR_LOG2(_log2,_n)                                              \
    JS_BEGIN_MACRO                                                            \
        uint32_t j_ = (uint32_t)(_n);                                         \
        (_log2) = 0;                                                          \
        if ((j_) >> 16)                                                       \
            (_log2) += 16, (j_) >>= 16;                                       \
        if ((j_) >> 8)                                                        \
            (_log2) += 8, (j_) >>= 8;                                         \
        if ((j_) >> 4)                                                        \
            (_log2) += 4, (j_) >>= 4;                                         \
        if ((j_) >> 2)                                                        \
            (_log2) += 2, (j_) >>= 2;                                         \
        if ((j_) >> 1)                                                        \
            (_log2) += 1;                                                     \
    JS_END_MACRO
#endif

#if JS_BYTES_PER_WORD == 4
# ifdef JS_HAS_BUILTIN_BITSCAN32
#  define js_FloorLog2wImpl(n)                                                \
    ((size_t)(JS_BITS_PER_WORD - 1 - js_bitscan_clz32(n)))
# else
JS_PUBLIC_API(size_t) js_FloorLog2wImpl(size_t n);
# endif
#elif JS_BYTES_PER_WORD == 8
# ifdef JS_HAS_BUILTIN_BITSCAN64
#  define js_FloorLog2wImpl(n)                                                \
    ((size_t)(JS_BITS_PER_WORD - 1 - js_bitscan_clz64(n)))
# else
JS_PUBLIC_API(size_t) js_FloorLog2wImpl(size_t n);
# endif
#else
# error "NOT SUPPORTED"
#endif

JS_END_EXTERN_C







#define JS_CEILING_LOG2W(n) ((n) <= 1 ? 0 : 1 + JS_FLOOR_LOG2W((n) - 1))







static MOZ_ALWAYS_INLINE size_t
JS_FLOOR_LOG2W(size_t n)
{
    JS_ASSERT(n != 0);
    return js_FloorLog2wImpl(n);
}












#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_AMD64) || \
    defined(_M_X64))
#include <stdlib.h>
#pragma intrinsic(_rotl)
#define JS_ROTATE_LEFT32(a, bits) _rotl(a, bits)
#else
#define JS_ROTATE_LEFT32(a, bits) (((a) << (bits)) | ((a) >> (32 - (bits))))
#endif

#include <new>











































#define JS_NEW_BODY(allocator, t, parms)                                       \
    void *memory = allocator(sizeof(t));                                       \
    return memory ? new(memory) t parms : NULL;










#define JS_DECLARE_NEW_METHODS(NEWNAME, ALLOCATOR, QUALIFIERS)\
    template <class T>\
    QUALIFIERS T *NEWNAME() {\
        JS_NEW_BODY(ALLOCATOR, T, ())\
    }\
\
    template <class T, class P1>\
    QUALIFIERS T *NEWNAME(P1 p1) {\
        JS_NEW_BODY(ALLOCATOR, T, (p1))\
    }\
\
    template <class T, class P1, class P2>\
    QUALIFIERS T *NEWNAME(P1 p1, P2 p2) {\
        JS_NEW_BODY(ALLOCATOR, T, (p1, p2))\
    }\
\
    template <class T, class P1, class P2, class P3>\
    QUALIFIERS T *NEWNAME(P1 p1, P2 p2, P3 p3) {\
        JS_NEW_BODY(ALLOCATOR, T, (p1, p2, p3))\
    }\
\
    template <class T, class P1, class P2, class P3, class P4>\
    QUALIFIERS T *NEWNAME(P1 p1, P2 p2, P3 p3, P4 p4) {\
        JS_NEW_BODY(ALLOCATOR, T, (p1, p2, p3, p4))\
    }\
\
    template <class T, class P1, class P2, class P3, class P4, class P5>\
    QUALIFIERS T *NEWNAME(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5) {\
        JS_NEW_BODY(ALLOCATOR, T, (p1, p2, p3, p4, p5))\
    }\
\
    template <class T, class P1, class P2, class P3, class P4, class P5, class P6>\
    QUALIFIERS T *NEWNAME(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6) {\
        JS_NEW_BODY(ALLOCATOR, T, (p1, p2, p3, p4, p5, p6))\
    }\
\
    template <class T, class P1, class P2, class P3, class P4, class P5, class P6, class P7>\
    QUALIFIERS T *NEWNAME(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7) {\
        JS_NEW_BODY(ALLOCATOR, T, (p1, p2, p3, p4, p5, p6, p7))\
    }\
\
    template <class T, class P1, class P2, class P3, class P4, class P5, class P6, class P7, class P8>\
    QUALIFIERS T *NEWNAME(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8) {\
        JS_NEW_BODY(ALLOCATOR, T, (p1, p2, p3, p4, p5, p6, p7, p8))\
    }\
\
    template <class T, class P1, class P2, class P3, class P4, class P5, class P6, class P7, class P8, class P9>\
    QUALIFIERS T *NEWNAME(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8, P9 p9) {\
        JS_NEW_BODY(ALLOCATOR, T, (p1, p2, p3, p4, p5, p6, p7, p8, p9))\
    }\
\
    template <class T, class P1, class P2, class P3, class P4, class P5, class P6, class P7, class P8, class P9, class P10>\
    QUALIFIERS T *NEWNAME(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8, P9 p9, P10 p10) {\
        JS_NEW_BODY(ALLOCATOR, T, (p1, p2, p3, p4, p5, p6, p7, p8, p9, p10))\
    }\
\
    template <class T, class P1, class P2, class P3, class P4, class P5, class P6, class P7, class P8, class P9, class P10, class P11>\
    QUALIFIERS T *NEWNAME(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8, P9 p9, P10 p10, P11 p11) {\
        JS_NEW_BODY(ALLOCATOR, T, (p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11))\
    }\
\
    template <class T, class P1, class P2, class P3, class P4, class P5, class P6, class P7, class P8, class P9, class P10, class P11, class P12>\
    QUALIFIERS T *NEWNAME(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8, P9 p9, P10 p10, P11 p11, P12 p12) {\
        JS_NEW_BODY(ALLOCATOR, T, (p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12))\
    }\

JS_DECLARE_NEW_METHODS(js_new, js_malloc, static JS_ALWAYS_INLINE)

template <class T>
static JS_ALWAYS_INLINE void
js_delete(T *p)
{
    if (p) {
        p->~T();
        js_free(p);
    }
}

template <class T>
static JS_ALWAYS_INLINE T *
js_pod_malloc()
{
    return (T *)js_malloc(sizeof(T));
}

template <class T>
static JS_ALWAYS_INLINE T *
js_pod_calloc()
{
    return (T *)js_calloc(sizeof(T));
}

template <class T>
static JS_ALWAYS_INLINE T *
js_pod_malloc(size_t numElems)
{
    if (numElems & js::tl::MulOverflowMask<sizeof(T)>::result)
        return NULL;
    return (T *)js_malloc(numElems * sizeof(T));
}

template <class T>
static JS_ALWAYS_INLINE T *
js_pod_calloc(size_t numElems)
{
    if (numElems & js::tl::MulOverflowMask<sizeof(T)>::result)
        return NULL;
    return (T *)js_calloc(numElems * sizeof(T));
}

namespace js {

template<typename T>
struct ScopedFreePtrTraits
{
    typedef T* type;
    static T* empty() { return NULL; }
    static void release(T* ptr) { js_free(ptr); }
};
SCOPED_TEMPLATE(ScopedJSFreePtr, ScopedFreePtrTraits)

template <typename T>
struct ScopedDeletePtrTraits : public ScopedFreePtrTraits<T>
{
    static void release(T *ptr) { js_delete(ptr); }
};
SCOPED_TEMPLATE(ScopedJSDeletePtr, ScopedDeletePtrTraits)

template <typename T>
struct ScopedReleasePtrTraits : public ScopedFreePtrTraits<T>
{
    static void release(T *ptr) { if (ptr) ptr->release(); }
};
SCOPED_TEMPLATE(ScopedReleasePtr, ScopedReleasePtrTraits)

} 

namespace js {





































































































template<typename T>
class MoveRef {
  public:
    typedef T Referent;
    explicit MoveRef(T &t) : pointer(&t) { }
    T &operator*()  const { return *pointer; }
    T *operator->() const { return  pointer; }
    operator T& ()   const { return *pointer; }
  private:
    T *pointer;
};

template<typename T>
MoveRef<T> Move(T &t) { return MoveRef<T>(t); }

template<typename T>
MoveRef<T> Move(const T &t) { return MoveRef<T>(const_cast<T &>(t)); }


class ReentrancyGuard
{
    
    ReentrancyGuard(const ReentrancyGuard &);
    void operator=(const ReentrancyGuard &);

#ifdef DEBUG
    bool &entered;
#endif
  public:
    template <class T>
#ifdef DEBUG
    ReentrancyGuard(T &obj)
      : entered(obj.entered)
#else
    ReentrancyGuard(T &/*obj*/)
#endif
    {
#ifdef DEBUG
        JS_ASSERT(!entered);
        entered = true;
#endif
    }
    ~ReentrancyGuard()
    {
#ifdef DEBUG
        entered = false;
#endif
    }
};

template <class T>
JS_ALWAYS_INLINE static void
Swap(T &t, T &u)
{
    T tmp(Move(t));
    t = Move(u);
    u = Move(tmp);
}





JS_ALWAYS_INLINE size_t
RoundUpPow2(size_t x)
{
    return size_t(1) << JS_CEILING_LOG2W(x);
}


typedef uint32_t HashNumber;
const unsigned HashNumberSizeBits = 32;

namespace detail {
















inline HashNumber
ScrambleHashCode(HashNumber h)
{
    














    static const HashNumber goldenRatio = 0x9E3779B9U;
    return h * goldenRatio;
}

} 

} 

namespace JS {












inline void PoisonPtr(void *v)
{
#if defined(JSGC_ROOT_ANALYSIS) && defined(DEBUG)
    uint8_t *ptr = (uint8_t *) v + 3;
    *ptr = JS_FREE_PATTERN;
#endif
}

template <typename T>
inline bool IsPoisonedPtr(T *v)
{
#if defined(JSGC_ROOT_ANALYSIS) && defined(DEBUG)
    uint32_t mask = uintptr_t(v) & 0xff000000;
    return mask == uint32_t(JS_FREE_PATTERN << 24);
#else
    return false;
#endif
}

}




typedef size_t(*JSMallocSizeOfFun)(const void *p);


#ifndef HAVE_STATIC_ANNOTATIONS
# define HAVE_STATIC_ANNOTATIONS
# ifdef XGILL_PLUGIN
#  define STATIC_PRECONDITION(COND)         __attribute__((precondition(#COND)))
#  define STATIC_PRECONDITION_ASSUME(COND)  __attribute__((precondition_assume(#COND)))
#  define STATIC_POSTCONDITION(COND)        __attribute__((postcondition(#COND)))
#  define STATIC_POSTCONDITION_ASSUME(COND) __attribute__((postcondition_assume(#COND)))
#  define STATIC_INVARIANT(COND)            __attribute__((invariant(#COND)))
#  define STATIC_INVARIANT_ASSUME(COND)     __attribute__((invariant_assume(#COND)))
#  define STATIC_PASTE2(X,Y) X ## Y
#  define STATIC_PASTE1(X,Y) STATIC_PASTE2(X,Y)
#  define STATIC_ASSERT(COND)                        \
  JS_BEGIN_MACRO                                     \
    __attribute__((assert_static(#COND), unused))    \
    int STATIC_PASTE1(assert_static_, __COUNTER__);  \
  JS_END_MACRO
#  define STATIC_ASSUME(COND)                        \
  JS_BEGIN_MACRO                                     \
    __attribute__((assume_static(#COND), unused))    \
    int STATIC_PASTE1(assume_static_, __COUNTER__);  \
  JS_END_MACRO
#  define STATIC_ASSERT_RUNTIME(COND)                       \
  JS_BEGIN_MACRO                                            \
    __attribute__((assert_static_runtime(#COND), unused))   \
    int STATIC_PASTE1(assert_static_runtime_, __COUNTER__); \
  JS_END_MACRO
# else 
#  define STATIC_PRECONDITION(COND)
#  define STATIC_PRECONDITION_ASSUME(COND)
#  define STATIC_POSTCONDITION(COND)
#  define STATIC_POSTCONDITION_ASSUME(COND)
#  define STATIC_INVARIANT(COND)
#  define STATIC_INVARIANT_ASSUME(COND)
#  define STATIC_ASSERT(COND)          JS_BEGIN_MACRO /* nothing */ JS_END_MACRO
#  define STATIC_ASSUME(COND)          JS_BEGIN_MACRO /* nothing */ JS_END_MACRO
#  define STATIC_ASSERT_RUNTIME(COND)  JS_BEGIN_MACRO /* nothing */ JS_END_MACRO
# endif 
# define STATIC_SKIP_INFERENCE STATIC_INVARIANT(skip_inference())
#endif 

#endif 
