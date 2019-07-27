





#ifndef js_Utility_h
#define js_Utility_h

#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"
#include "mozilla/Compiler.h"
#include "mozilla/Move.h"
#include "mozilla/NullPtr.h"
#include "mozilla/Scoped.h"
#include "mozilla/TemplateLib.h"
#include "mozilla/UniquePtr.h"

#include <stdlib.h>
#include <string.h>

#ifdef JS_OOM_DO_BACKTRACES
#include <execinfo.h>
#include <stdio.h>
#endif

#include "jstypes.h"


namespace JS {}


namespace mozilla {}


namespace js {}






#define JS_FRESH_NURSERY_PATTERN 0x2F
#define JS_SWEPT_NURSERY_PATTERN 0x2B
#define JS_ALLOCATED_NURSERY_PATTERN 0x2D
#define JS_FRESH_TENURED_PATTERN 0x4F
#define JS_MOVED_TENURED_PATTERN 0x49
#define JS_SWEPT_TENURED_PATTERN 0x4B
#define JS_ALLOCATED_TENURED_PATTERN 0x4D
#define JS_SWEPT_CODE_PATTERN 0x3b
#define JS_SWEPT_FRAME_PATTERN 0x5b
#define JS_POISONED_FORKJOIN_CHUNK 0xBD

#define JS_ASSERT(expr)           MOZ_ASSERT(expr)
#define JS_ASSERT_IF(cond, expr)  MOZ_ASSERT_IF(cond, expr)

#define JS_STATIC_ASSERT(cond)           static_assert(cond, "JS_STATIC_ASSERT")
#define JS_STATIC_ASSERT_IF(cond, expr)  MOZ_STATIC_ASSERT_IF(cond, expr, "JS_STATIC_ASSERT_IF")

extern MOZ_NORETURN JS_PUBLIC_API(void)
JS_Assert(const char *s, const char *file, int ln);




#if defined JS_USE_CUSTOM_ALLOCATOR
# include "jscustomallocator.h"
#else
# if defined(DEBUG) || defined(JS_OOM_BREAKPOINT)





extern JS_PUBLIC_DATA(uint32_t) OOM_maxAllocations; 
extern JS_PUBLIC_DATA(uint32_t) OOM_counter; 

#ifdef JS_OOM_BREAKPOINT
static MOZ_NEVER_INLINE void js_failedAllocBreakpoint() { asm(""); }
#define JS_OOM_CALL_BP_FUNC() js_failedAllocBreakpoint()
#else
#define JS_OOM_CALL_BP_FUNC() do {} while(0)
#endif

#  define JS_OOM_POSSIBLY_FAIL() \
    do \
    { \
        if (++OOM_counter > OOM_maxAllocations) { \
            JS_OOM_CALL_BP_FUNC();\
            return nullptr; \
        } \
    } while (0)

# else
#  define JS_OOM_POSSIBLY_FAIL() do {} while(0)
# endif 

static inline void* js_malloc(size_t bytes)
{
    JS_OOM_POSSIBLY_FAIL();
    return malloc(bytes);
}

static inline void* js_calloc(size_t bytes)
{
    JS_OOM_POSSIBLY_FAIL();
    return calloc(bytes, 1);
}

static inline void* js_calloc(size_t nmemb, size_t size)
{
    JS_OOM_POSSIBLY_FAIL();
    return calloc(nmemb, size);
}

static inline void* js_realloc(void* p, size_t bytes)
{
    JS_OOM_POSSIBLY_FAIL();
    return realloc(p, bytes);
}

static inline void js_free(void* p)
{
    free(p);
}

static inline char* js_strdup(const char* s)
{
    JS_OOM_POSSIBLY_FAIL();
    return strdup(s);
}
#endif

#include <new>











































#define JS_NEW_BODY(allocator, t, parms)                                       \
    void *memory = allocator(sizeof(t));                                       \
    return memory ? new(memory) t parms : nullptr;
#define JS_MAKE_BODY(newname, T, parms)                                        \
    T *ptr = newname<T> parms;                                                 \
    return mozilla::UniquePtr<T, JS::DeletePolicy<T>>(ptr);










#define JS_DECLARE_NEW_METHODS(NEWNAME, ALLOCATOR, QUALIFIERS)\
    template <class T>\
    QUALIFIERS T *NEWNAME() MOZ_HEAP_ALLOCATOR {\
        JS_NEW_BODY(ALLOCATOR, T, ())\
    }\
\
    template <class T, class P1>\
    QUALIFIERS T *NEWNAME(P1 &&p1) MOZ_HEAP_ALLOCATOR {\
        JS_NEW_BODY(ALLOCATOR, T,\
                    (mozilla::Forward<P1>(p1)))\
    }\
\
    template <class T, class P1, class P2>\
    QUALIFIERS T *NEWNAME(P1 &&p1, P2 &&p2) MOZ_HEAP_ALLOCATOR {\
        JS_NEW_BODY(ALLOCATOR, T,\
                    (mozilla::Forward<P1>(p1),\
                     mozilla::Forward<P2>(p2)))\
    }\
\
    template <class T, class P1, class P2, class P3>\
    QUALIFIERS T *NEWNAME(P1 &&p1, P2 &&p2, P3 &&p3) MOZ_HEAP_ALLOCATOR {\
        JS_NEW_BODY(ALLOCATOR, T,\
                    (mozilla::Forward<P1>(p1),\
                     mozilla::Forward<P2>(p2),\
                     mozilla::Forward<P3>(p3)))\
    }\
\
    template <class T, class P1, class P2, class P3, class P4>\
    QUALIFIERS T *NEWNAME(P1 &&p1, P2 &&p2, P3 &&p3, P4 &&p4) MOZ_HEAP_ALLOCATOR {\
        JS_NEW_BODY(ALLOCATOR, T,\
                    (mozilla::Forward<P1>(p1),\
                     mozilla::Forward<P2>(p2),\
                     mozilla::Forward<P3>(p3),\
                     mozilla::Forward<P4>(p4)))\
    }\
\
    template <class T, class P1, class P2, class P3, class P4, class P5>\
    QUALIFIERS T *NEWNAME(P1 &&p1, P2 &&p2, P3 &&p3, P4 &&p4, P5 &&p5) MOZ_HEAP_ALLOCATOR {\
        JS_NEW_BODY(ALLOCATOR, T,\
                    (mozilla::Forward<P1>(p1),\
                     mozilla::Forward<P2>(p2),\
                     mozilla::Forward<P3>(p3),\
                     mozilla::Forward<P4>(p4),\
                     mozilla::Forward<P5>(p5)))\
    }\
\
    template <class T, class P1, class P2, class P3, class P4, class P5, class P6>\
    QUALIFIERS T *NEWNAME(P1 &&p1, P2 &&p2, P3 &&p3, P4 &&p4, P5 &&p5, P6 &&p6) MOZ_HEAP_ALLOCATOR {\
        JS_NEW_BODY(ALLOCATOR, T,\
                    (mozilla::Forward<P1>(p1),\
                     mozilla::Forward<P2>(p2),\
                     mozilla::Forward<P3>(p3),\
                     mozilla::Forward<P4>(p4),\
                     mozilla::Forward<P5>(p5),\
                     mozilla::Forward<P6>(p6)))\
    }\
\
    template <class T, class P1, class P2, class P3, class P4, class P5, class P6, class P7>\
    QUALIFIERS T *NEWNAME(P1 &&p1, P2 &&p2, P3 &&p3, P4 &&p4, P5 &&p5, P6 &&p6, P7 &&p7) MOZ_HEAP_ALLOCATOR {\
        JS_NEW_BODY(ALLOCATOR, T,\
                    (mozilla::Forward<P1>(p1),\
                     mozilla::Forward<P2>(p2),\
                     mozilla::Forward<P3>(p3),\
                     mozilla::Forward<P4>(p4),\
                     mozilla::Forward<P5>(p5),\
                     mozilla::Forward<P6>(p6),\
                     mozilla::Forward<P7>(p7)))\
    }\
\
    template <class T, class P1, class P2, class P3, class P4, class P5, class P6, class P7, class P8>\
    QUALIFIERS T *NEWNAME(P1 &&p1, P2 &&p2, P3 &&p3, P4 &&p4, P5 &&p5, P6 &&p6, P7 &&p7, P8 &&p8) MOZ_HEAP_ALLOCATOR {\
        JS_NEW_BODY(ALLOCATOR, T,\
                    (mozilla::Forward<P1>(p1),\
                     mozilla::Forward<P2>(p2),\
                     mozilla::Forward<P3>(p3),\
                     mozilla::Forward<P4>(p4),\
                     mozilla::Forward<P5>(p5),\
                     mozilla::Forward<P6>(p6),\
                     mozilla::Forward<P7>(p7),\
                     mozilla::Forward<P8>(p8)))\
    }\
\
    template <class T, class P1, class P2, class P3, class P4, class P5, class P6, class P7, class P8, class P9>\
    QUALIFIERS T *NEWNAME(P1 &&p1, P2 &&p2, P3 &&p3, P4 &&p4, P5 &&p5, P6 &&p6, P7 &&p7, P8 &&p8, P9 &&p9) MOZ_HEAP_ALLOCATOR {\
        JS_NEW_BODY(ALLOCATOR, T,\
                    (mozilla::Forward<P1>(p1),\
                     mozilla::Forward<P2>(p2),\
                     mozilla::Forward<P3>(p3),\
                     mozilla::Forward<P4>(p4),\
                     mozilla::Forward<P5>(p5),\
                     mozilla::Forward<P6>(p6),\
                     mozilla::Forward<P7>(p7),\
                     mozilla::Forward<P8>(p8),\
                     mozilla::Forward<P9>(p9)))\
    }\
\
    template <class T, class P1, class P2, class P3, class P4, class P5, class P6, class P7, class P8, class P9, class P10>\
    QUALIFIERS T *NEWNAME(P1 &&p1, P2 &&p2, P3 &&p3, P4 &&p4, P5 &&p5, P6 &&p6, P7 &&p7, P8 &&p8, P9 &&p9, P10 &&p10) MOZ_HEAP_ALLOCATOR {\
        JS_NEW_BODY(ALLOCATOR, T,\
                    (mozilla::Forward<P1>(p1),\
                     mozilla::Forward<P2>(p2),\
                     mozilla::Forward<P3>(p3),\
                     mozilla::Forward<P4>(p4),\
                     mozilla::Forward<P5>(p5),\
                     mozilla::Forward<P6>(p6),\
                     mozilla::Forward<P7>(p7),\
                     mozilla::Forward<P8>(p8),\
                     mozilla::Forward<P9>(p9),\
                     mozilla::Forward<P10>(p10)))\
    }\
\
    template <class T, class P1, class P2, class P3, class P4, class P5, class P6, class P7, class P8, class P9, class P10, class P11>\
    QUALIFIERS T *NEWNAME(P1 &&p1, P2 &&p2, P3 &&p3, P4 &&p4, P5 &&p5, P6 &&p6, P7 &&p7, P8 &&p8, P9 &&p9, P10 &&p10, P11 &&p11) MOZ_HEAP_ALLOCATOR {\
        JS_NEW_BODY(ALLOCATOR, T,\
                    (mozilla::Forward<P1>(p1),\
                     mozilla::Forward<P2>(p2),\
                     mozilla::Forward<P3>(p3),\
                     mozilla::Forward<P4>(p4),\
                     mozilla::Forward<P5>(p5),\
                     mozilla::Forward<P6>(p6),\
                     mozilla::Forward<P7>(p7),\
                     mozilla::Forward<P8>(p8),\
                     mozilla::Forward<P9>(p9),\
                     mozilla::Forward<P10>(p10),\
                     mozilla::Forward<P11>(p11)))\
    }\
\
    template <class T, class P1, class P2, class P3, class P4, class P5, class P6, class P7, class P8, class P9, class P10, class P11, class P12>\
    QUALIFIERS T *NEWNAME(P1 &&p1, P2 &&p2, P3 &&p3, P4 &&p4, P5 &&p5, P6 &&p6, P7 &&p7, P8 &&p8, P9 &&p9, P10 &&p10, P11 &&p11, P12 &&p12) MOZ_HEAP_ALLOCATOR {\
        JS_NEW_BODY(ALLOCATOR, T,\
                    (mozilla::Forward<P1>(p1),\
                     mozilla::Forward<P2>(p2),\
                     mozilla::Forward<P3>(p3),\
                     mozilla::Forward<P4>(p4),\
                     mozilla::Forward<P5>(p5),\
                     mozilla::Forward<P6>(p6),\
                     mozilla::Forward<P7>(p7),\
                     mozilla::Forward<P8>(p8),\
                     mozilla::Forward<P9>(p9),\
                     mozilla::Forward<P10>(p10),\
                     mozilla::Forward<P11>(p11),\
                     mozilla::Forward<P12>(p12)))\
    }\












#define JS_DECLARE_MAKE_METHODS(MAKENAME, NEWNAME, QUALIFIERS)\
    template <class T> \
    QUALIFIERS \
    mozilla::UniquePtr<T, JS::DeletePolicy<T>> \
    MAKENAME() MOZ_HEAP_ALLOCATOR {\
        JS_MAKE_BODY(NEWNAME, T, ())\
    }\
\
    template <class T, class P1> \
    QUALIFIERS \
    mozilla::UniquePtr<T, JS::DeletePolicy<T>> \
    MAKENAME(P1 &&p1) MOZ_HEAP_ALLOCATOR {\
        JS_MAKE_BODY(NEWNAME, T,\
                    (mozilla::Forward<P1>(p1)))\
    }\
\
    template <class T, class P1, class P2> \
    QUALIFIERS \
mozilla::UniquePtr<T, JS::DeletePolicy<T>> \
    MAKENAME(P1 &&p1, P2 &&p2) MOZ_HEAP_ALLOCATOR {\
        JS_MAKE_BODY(NEWNAME, T,\
                    (mozilla::Forward<P1>(p1),\
                     mozilla::Forward<P2>(p2)))\
    }\
\
    template <class T, class P1, class P2, class P3> \
    QUALIFIERS \
    mozilla::UniquePtr<T, JS::DeletePolicy<T>> \
    MAKENAME(P1 &&p1, P2 &&p2, P3 &&p3) MOZ_HEAP_ALLOCATOR {\
        JS_MAKE_BODY(NEWNAME, T,\
                    (mozilla::Forward<P1>(p1),\
                     mozilla::Forward<P2>(p2),\
                     mozilla::Forward<P3>(p3)))\
    }\
\
    template <class T, class P1, class P2, class P3, class P4> \
    QUALIFIERS \
    mozilla::UniquePtr<T, JS::DeletePolicy<T>> \
    MAKENAME(P1 &&p1, P2 &&p2, P3 &&p3, P4 &&p4) MOZ_HEAP_ALLOCATOR {\
        JS_MAKE_BODY(NEWNAME, T,\
                    (mozilla::Forward<P1>(p1),\
                     mozilla::Forward<P2>(p2),\
                     mozilla::Forward<P3>(p3),\
                     mozilla::Forward<P4>(p4)))\
    }\
\
    template <class T, class P1, class P2, class P3, class P4, class P5> \
    QUALIFIERS \
    mozilla::UniquePtr<T, JS::DeletePolicy<T>> \
    MAKENAME(P1 &&p1, P2 &&p2, P3 &&p3, P4 &&p4, P5 &&p5) MOZ_HEAP_ALLOCATOR {\
        JS_MAKE_BODY(NEWNAME, T,\
                    (mozilla::Forward<P1>(p1),\
                     mozilla::Forward<P2>(p2),\
                     mozilla::Forward<P3>(p3),\
                     mozilla::Forward<P4>(p4),\
                     mozilla::Forward<P5>(p5)))\
    }\
\
    template <class T, class P1, class P2, class P3, class P4, class P5, class P6> \
    QUALIFIERS \
    mozilla::UniquePtr<T, JS::DeletePolicy<T>> \
    MAKENAME(P1 &&p1, P2 &&p2, P3 &&p3, P4 &&p4, P5 &&p5, P6 &&p6) MOZ_HEAP_ALLOCATOR {\
        JS_MAKE_BODY(NEWNAME, T,\
                    (mozilla::Forward<P1>(p1),\
                     mozilla::Forward<P2>(p2),\
                     mozilla::Forward<P3>(p3),\
                     mozilla::Forward<P4>(p4),\
                     mozilla::Forward<P5>(p5),\
                     mozilla::Forward<P6>(p6)))\
    }\
\
    template <class T, class P1, class P2, class P3, class P4, class P5, class P6, class P7> \
    QUALIFIERS \
    mozilla::UniquePtr<T, JS::DeletePolicy<T>> \
    MAKENAME(P1 &&p1, P2 &&p2, P3 &&p3, P4 &&p4, P5 &&p5, P6 &&p6, P7 &&p7) MOZ_HEAP_ALLOCATOR {\
        JS_MAKE_BODY(NEWNAME, T,\
                    (mozilla::Forward<P1>(p1),\
                     mozilla::Forward<P2>(p2),\
                     mozilla::Forward<P3>(p3),\
                     mozilla::Forward<P4>(p4),\
                     mozilla::Forward<P5>(p5),\
                     mozilla::Forward<P6>(p6),\
                     mozilla::Forward<P7>(p7)))\
    }\
\
    template <class T, class P1, class P2, class P3, class P4, class P5, class P6, class P7, class P8> \
    QUALIFIERS \
    mozilla::UniquePtr<T, JS::DeletePolicy<T>> \
    MAKENAME(P1 &&p1, P2 &&p2, P3 &&p3, P4 &&p4, P5 &&p5, P6 &&p6, P7 &&p7, P8 &&p8) MOZ_HEAP_ALLOCATOR {\
        JS_MAKE_BODY(NEWNAME, T,\
                    (mozilla::Forward<P1>(p1),\
                     mozilla::Forward<P2>(p2),\
                     mozilla::Forward<P3>(p3),\
                     mozilla::Forward<P4>(p4),\
                     mozilla::Forward<P5>(p5),\
                     mozilla::Forward<P6>(p6),\
                     mozilla::Forward<P7>(p7),\
                     mozilla::Forward<P8>(p8)))\
    }\
\
    template <class T, class P1, class P2, class P3, class P4, class P5, class P6, class P7, class P8, class P9> \
    QUALIFIERS \
    mozilla::UniquePtr<T, JS::DeletePolicy<T>> \
    MAKENAME(P1 &&p1, P2 &&p2, P3 &&p3, P4 &&p4, P5 &&p5, P6 &&p6, P7 &&p7, P8 &&p8, P9 &&p9) MOZ_HEAP_ALLOCATOR {\
        JS_MAKE_BODY(NEWNAME, T,\
                    (mozilla::Forward<P1>(p1),\
                     mozilla::Forward<P2>(p2),\
                     mozilla::Forward<P3>(p3),\
                     mozilla::Forward<P4>(p4),\
                     mozilla::Forward<P5>(p5),\
                     mozilla::Forward<P6>(p6),\
                     mozilla::Forward<P7>(p7),\
                     mozilla::Forward<P8>(p8),\
                     mozilla::Forward<P9>(p9)))\
    }\
\
    template <class T, class P1, class P2, class P3, class P4, class P5, class P6, class P7, class P8, class P9, class P10> \
    QUALIFIERS \
    mozilla::UniquePtr<T, JS::DeletePolicy<T>> \
    MAKENAME(P1 &&p1, P2 &&p2, P3 &&p3, P4 &&p4, P5 &&p5, P6 &&p6, P7 &&p7, P8 &&p8, P9 &&p9, P10 &&p10) MOZ_HEAP_ALLOCATOR {\
        JS_MAKE_BODY(NEWNAME, T,\
                    (mozilla::Forward<P1>(p1),\
                     mozilla::Forward<P2>(p2),\
                     mozilla::Forward<P3>(p3),\
                     mozilla::Forward<P4>(p4),\
                     mozilla::Forward<P5>(p5),\
                     mozilla::Forward<P6>(p6),\
                     mozilla::Forward<P7>(p7),\
                     mozilla::Forward<P8>(p8),\
                     mozilla::Forward<P9>(p9),\
                     mozilla::Forward<P10>(p10)))\
    }\
\
    template <class T, class P1, class P2, class P3, class P4, class P5, class P6, class P7, class P8, class P9, class P10, class P11> \
    QUALIFIERS \
    mozilla::UniquePtr<T, JS::DeletePolicy<T>> \
    MAKENAME(P1 &&p1, P2 &&p2, P3 &&p3, P4 &&p4, P5 &&p5, P6 &&p6, P7 &&p7, P8 &&p8, P9 &&p9, P10 &&p10, P11 &&p11) MOZ_HEAP_ALLOCATOR {\
        JS_MAKE_BODY(NEWNAME, T,\
                    (mozilla::Forward<P1>(p1),\
                     mozilla::Forward<P2>(p2),\
                     mozilla::Forward<P3>(p3),\
                     mozilla::Forward<P4>(p4),\
                     mozilla::Forward<P5>(p5),\
                     mozilla::Forward<P6>(p6),\
                     mozilla::Forward<P7>(p7),\
                     mozilla::Forward<P8>(p8),\
                     mozilla::Forward<P9>(p9),\
                     mozilla::Forward<P10>(p10),\
                     mozilla::Forward<P11>(p11)))\
    }\
\
    template <class T, class P1, class P2, class P3, class P4, class P5, class P6, class P7, class P8, class P9, class P10, class P11, class P12> \
    QUALIFIERS \
    mozilla::UniquePtr<T, JS::DeletePolicy<T>> \
    MAKENAME(P1 &&p1, P2 &&p2, P3 &&p3, P4 &&p4, P5 &&p5, P6 &&p6, P7 &&p7, P8 &&p8, P9 &&p9, P10 &&p10, P11 &&p11, P12 &&p12) MOZ_HEAP_ALLOCATOR {\
        JS_MAKE_BODY(NEWNAME, T,\
                    (mozilla::Forward<P1>(p1),\
                     mozilla::Forward<P2>(p2),\
                     mozilla::Forward<P3>(p3),\
                     mozilla::Forward<P4>(p4),\
                     mozilla::Forward<P5>(p5),\
                     mozilla::Forward<P6>(p6),\
                     mozilla::Forward<P7>(p7),\
                     mozilla::Forward<P8>(p8),\
                     mozilla::Forward<P9>(p9),\
                     mozilla::Forward<P10>(p10),\
                     mozilla::Forward<P11>(p11),\
                     mozilla::Forward<P12>(p12)))\
    }\

JS_DECLARE_NEW_METHODS(js_new, js_malloc, static MOZ_ALWAYS_INLINE)

template <class T>
static MOZ_ALWAYS_INLINE void
js_delete(T *p)
{
    if (p) {
        p->~T();
        js_free(p);
    }
}

template<class T>
static MOZ_ALWAYS_INLINE void
js_delete_poison(T *p)
{
    if (p) {
        p->~T();
        memset(p, 0x3B, sizeof(T));
        js_free(p);
    }
}

template <class T>
static MOZ_ALWAYS_INLINE T *
js_pod_malloc()
{
    return (T *)js_malloc(sizeof(T));
}

template <class T>
static MOZ_ALWAYS_INLINE T *
js_pod_calloc()
{
    return (T *)js_calloc(sizeof(T));
}

template <class T>
static MOZ_ALWAYS_INLINE T *
js_pod_malloc(size_t numElems)
{
    if (numElems & mozilla::tl::MulOverflowMask<sizeof(T)>::value)
        return nullptr;
    return (T *)js_malloc(numElems * sizeof(T));
}

template <class T>
static MOZ_ALWAYS_INLINE T *
js_pod_calloc(size_t numElems)
{
    if (numElems & mozilla::tl::MulOverflowMask<sizeof(T)>::value)
        return nullptr;
    return (T *)js_calloc(numElems * sizeof(T));
}

template <class T>
static MOZ_ALWAYS_INLINE T *
js_pod_realloc(T *prior, size_t oldSize, size_t newSize)
{
    MOZ_ASSERT(!(oldSize & mozilla::tl::MulOverflowMask<sizeof(T)>::value));
    if (newSize & mozilla::tl::MulOverflowMask<sizeof(T)>::value)
        return nullptr;
    return (T *)js_realloc(prior, newSize * sizeof(T));
}

namespace js {

template<typename T>
struct ScopedFreePtrTraits
{
    typedef T* type;
    static T* empty() { return nullptr; }
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

namespace JS {

template<typename T>
struct DeletePolicy
{
    void operator()(T* ptr) {
        js_delete(ptr);
    }
};

struct FreePolicy
{
    void operator()(void* ptr) {
        js_free(ptr);
    }
};

} 

namespace js {


typedef uint32_t HashNumber;
const unsigned HashNumberSizeBits = 32;

typedef mozilla::UniquePtr<char, JS::FreePolicy> UniqueChars;

static inline UniqueChars make_string_copy(const char* str)
{
    return UniqueChars(js_strdup(str));
}

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
#if defined(JSGC_ROOT_ANALYSIS) && defined(JS_DEBUG)
    uint8_t *ptr = (uint8_t *) v + 3;
    *ptr = JS_FREE_PATTERN;
#endif
}

template <typename T>
inline bool IsPoisonedPtr(T *v)
{
#if defined(JSGC_ROOT_ANALYSIS) && defined(JS_DEBUG)
    uint32_t mask = uintptr_t(v) & 0xff000000;
    return mask == uint32_t(JS_FREE_PATTERN << 24);
#else
    return false;
#endif
}

}


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
