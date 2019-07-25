






































#ifndef js_utility_h__
#define js_utility_h__

#include <stdlib.h>
#include <string.h>

#include "mozilla/Util.h"

#ifdef __cplusplus


namespace JS {}


namespace mozilla {}


namespace js {


using namespace JS;
using namespace mozilla;

}  
#endif  

JS_BEGIN_EXTERN_C





#define JS_FREE_PATTERN 0xDA


#ifdef DEBUG
# define JS_ASSERT(expr)                                                      \
    ((expr) ? (void)0 : JS_Assert(#expr, __FILE__, __LINE__))
# define JS_ASSERT_IF(cond, expr)                                             \
    ((!(cond) || (expr)) ? (void)0 : JS_Assert(#expr, __FILE__, __LINE__))
# define JS_NOT_REACHED(reason)                                               \
    JS_Assert(reason, __FILE__, __LINE__)
# define JS_ALWAYS_TRUE(expr) JS_ASSERT(expr)
# define JS_ALWAYS_FALSE(expr) JS_ASSERT(!(expr))
# ifdef JS_THREADSAFE
#  define JS_THREADSAFE_ASSERT(expr) JS_ASSERT(expr)
# else
#  define JS_THREADSAFE_ASSERT(expr) ((void) 0)
# endif
#else
# define JS_ASSERT(expr)         ((void) 0)
# define JS_ASSERT_IF(cond,expr) ((void) 0)
# define JS_NOT_REACHED(reason)
# define JS_ALWAYS_TRUE(expr)    ((void) (expr))
# define JS_ALWAYS_FALSE(expr)    ((void) (expr))
# define JS_THREADSAFE_ASSERT(expr) ((void) 0)
#endif







#ifdef __SUNPRO_CC







# define JS_STATIC_ASSERT(cond) extern char js_static_assert[(cond) ? 1 : -1]
#else
# ifdef __COUNTER__
#  define JS_STATIC_ASSERT_GLUE1(x,y) x##y
#  define JS_STATIC_ASSERT_GLUE(x,y) JS_STATIC_ASSERT_GLUE1(x,y)
#  define JS_STATIC_ASSERT(cond)                                            \
        typedef int JS_STATIC_ASSERT_GLUE(js_static_assert, __COUNTER__)[(cond) ? 1 : -1]
# else
#  define JS_STATIC_ASSERT(cond) extern void js_static_assert(int arg[(cond) ? 1 : -1])
# endif
#endif

#define JS_STATIC_ASSERT_IF(cond, expr) JS_STATIC_ASSERT(!(cond) || (expr))






extern JS_PUBLIC_API(void) JS_Abort(void);




#if defined JS_USE_CUSTOM_ALLOCATOR
# include "jscustomallocator.h"
#else
# ifdef DEBUG




extern JS_PUBLIC_DATA(JSUint32) OOM_maxAllocations; 
extern JS_PUBLIC_DATA(JSUint32) OOM_counter; 
#  define JS_OOM_POSSIBLY_FAIL() \
    do \
    { \
        if (OOM_counter++ >= OOM_maxAllocations) { \
            return NULL; \
        } \
    } while (0)

# else
#  define JS_OOM_POSSIBLY_FAIL() do {} while(0)
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
        JSUint32 j_ = (JSUint32)(_n);                                         \
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
        JSUint32 j_ = (JSUint32)(_n);                                         \
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







#define JS_CEILING_LOG2W(n) ((n) <= 1 ? 0 : 1 + JS_FLOOR_LOG2W((n) - 1))







#define JS_FLOOR_LOG2W(n) (JS_ASSERT((n) != 0), js_FloorLog2wImpl(n))

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

#ifdef __cplusplus
#include <new>
















































































#define JS_NEW_BODY(allocator, t, parms)                                       \
    void *memory = allocator(sizeof(t));                                       \
    return memory ? new(memory) t parms : NULL;










#define JS_DECLARE_NEW_METHODS(ALLOCATOR, QUALIFIERS)\
    template <class T>\
    QUALIFIERS T *new_() {\
        JS_NEW_BODY(ALLOCATOR, T, ())\
    }\
\
    template <class T, class P1>\
    QUALIFIERS T *new_(P1 p1) {\
        JS_NEW_BODY(ALLOCATOR, T, (p1))\
    }\
\
    template <class T, class P1, class P2>\
    QUALIFIERS T *new_(P1 p1, P2 p2) {\
        JS_NEW_BODY(ALLOCATOR, T, (p1, p2))\
    }\
\
    template <class T, class P1, class P2, class P3>\
    QUALIFIERS T *new_(P1 p1, P2 p2, P3 p3) {\
        JS_NEW_BODY(ALLOCATOR, T, (p1, p2, p3))\
    }\
\
    template <class T, class P1, class P2, class P3, class P4>\
    QUALIFIERS T *new_(P1 p1, P2 p2, P3 p3, P4 p4) {\
        JS_NEW_BODY(ALLOCATOR, T, (p1, p2, p3, p4))\
    }\
\
    template <class T, class P1, class P2, class P3, class P4, class P5>\
    QUALIFIERS T *new_(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5) {\
        JS_NEW_BODY(ALLOCATOR, T, (p1, p2, p3, p4, p5))\
    }\
\
    template <class T, class P1, class P2, class P3, class P4, class P5, class P6>\
    QUALIFIERS T *new_(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6) {\
        JS_NEW_BODY(ALLOCATOR, T, (p1, p2, p3, p4, p5, p6))\
    }\
\
    template <class T, class P1, class P2, class P3, class P4, class P5, class P6, class P7>\
    QUALIFIERS T *new_(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7) {\
        JS_NEW_BODY(ALLOCATOR, T, (p1, p2, p3, p4, p5, p6, p7))\
    }\
\
    template <class T, class P1, class P2, class P3, class P4, class P5, class P6, class P7, class P8>\
    QUALIFIERS T *new_(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8) {\
        JS_NEW_BODY(ALLOCATOR, T, (p1, p2, p3, p4, p5, p6, p7, p8))\
    }\
\
    template <class T, class P1, class P2, class P3, class P4, class P5, class P6, class P7, class P8, class P9>\
    QUALIFIERS T *new_(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8, P9 p9) {\
        JS_NEW_BODY(ALLOCATOR, T, (p1, p2, p3, p4, p5, p6, p7, p8, p9))\
    }\
\
    template <class T, class P1, class P2, class P3, class P4, class P5, class P6, class P7, class P8, class P9, class P10>\
    QUALIFIERS T *new_(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8, P9 p9, P10 p10) {\
        JS_NEW_BODY(ALLOCATOR, T, (p1, p2, p3, p4, p5, p6, p7, p8, p9, p10))\
    }\
\
    template <class T, class P1, class P2, class P3, class P4, class P5, class P6, class P7, class P8, class P9, class P10, class P11>\
    QUALIFIERS T *new_(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8, P9 p9, P10 p10, P11 p11) {\
        JS_NEW_BODY(ALLOCATOR, T, (p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11))\
    }\
\
    template <class T, class P1, class P2, class P3, class P4, class P5, class P6, class P7, class P8, class P9, class P10, class P11, class P12>\
    QUALIFIERS T *new_(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7, P8 p8, P9 p9, P10 p10, P11 p11, P12 p12) {\
        JS_NEW_BODY(ALLOCATOR, T, (p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12))\
    }\
    static const int JSMinAlignment = 8;\
    template <class T>\
    QUALIFIERS T *array_new(size_t n) {\
        /* The length is stored just before the vector memory. */\
        uint64 numBytes64 = uint64(JSMinAlignment) + uint64(sizeof(T)) * uint64(n);\
        size_t numBytes = size_t(numBytes64);\
        if (numBytes64 != numBytes) {\
            JS_ASSERT(0);   /* we want to know if this happens in debug builds */\
            return NULL;\
        }\
        void *memory = ALLOCATOR(numBytes);\
        if (!memory)\
            return NULL;\
        *(size_t *)memory = n;\
        memory = (void*)(uintptr_t(memory) + JSMinAlignment);\
        return new(memory) T[n];\
    }\


#define JS_DECLARE_DELETE_METHODS(DEALLOCATOR, QUALIFIERS)\
    template <class T>\
    QUALIFIERS void delete_(T *p) {\
        if (p) {\
            p->~T();\
            DEALLOCATOR(p);\
        }\
    }\
\
    template <class T>\
    QUALIFIERS void array_delete(T *p) {\
        if (p) {\
            void* p0 = (void *)(uintptr_t(p) - js::OffTheBooks::JSMinAlignment);\
            size_t n = *(size_t *)p0;\
            for (size_t i = 0; i < n; i++)\
                (p + i)->~T();\
            DEALLOCATOR(p0);\
        }\
    }








namespace js {

class OffTheBooks {
public:
    JS_DECLARE_NEW_METHODS(::js_malloc, JS_ALWAYS_INLINE static)

    static JS_INLINE void* malloc_(size_t bytes) {
        return ::js_malloc(bytes);
    }

    static JS_INLINE void* calloc_(size_t bytes) {
        return ::js_calloc(bytes);
    }

    static JS_INLINE void* realloc_(void* p, size_t bytes) {
        return ::js_realloc(p, bytes);
    }
};





class Foreground {
public:
    
    static JS_ALWAYS_INLINE void free_(void* p) {
        ::js_free(p);
    }

    JS_DECLARE_DELETE_METHODS(::js_free, JS_ALWAYS_INLINE static)
};

class UnwantedForeground : public Foreground {
};

} 





#define JS_DECLARE_ALLOCATION_FRIENDS_FOR_PRIVATE_CONSTRUCTOR \
    friend class js::OffTheBooks;\
    friend class js::Foreground;\
    friend class js::UnwantedForeground;\
    friend struct ::JSContext;\
    friend struct ::JSRuntime



































#ifdef DEBUG
class JS_FRIEND_API(JSGuardObjectNotifier)
{
private:
    bool* mStatementDone;
public:
    JSGuardObjectNotifier() : mStatementDone(NULL) {}

    ~JSGuardObjectNotifier() {
        *mStatementDone = true;
    }

    void setStatementDone(bool *aStatementDone) {
        mStatementDone = aStatementDone;
    }
};

class JS_FRIEND_API(JSGuardObjectNotificationReceiver)
{
private:
    bool mStatementDone;
public:
    JSGuardObjectNotificationReceiver() : mStatementDone(false) {}

    ~JSGuardObjectNotificationReceiver() {
        





        JS_ASSERT(mStatementDone);
    }

    void Init(const JSGuardObjectNotifier &aNotifier) {
        



        const_cast<JSGuardObjectNotifier&>(aNotifier).
            setStatementDone(&mStatementDone);
    }
};

#define JS_DECL_USE_GUARD_OBJECT_NOTIFIER \
    JSGuardObjectNotificationReceiver _mCheckNotUsedAsTemporary;
#define JS_GUARD_OBJECT_NOTIFIER_PARAM \
    , const JSGuardObjectNotifier& _notifier = JSGuardObjectNotifier()
#define JS_GUARD_OBJECT_NOTIFIER_PARAM_NO_INIT \
    , const JSGuardObjectNotifier& _notifier
#define JS_GUARD_OBJECT_NOTIFIER_PARAM0 \
    const JSGuardObjectNotifier& _notifier = JSGuardObjectNotifier()
#define JS_GUARD_OBJECT_NOTIFIER_INIT \
    JS_BEGIN_MACRO _mCheckNotUsedAsTemporary.Init(_notifier); JS_END_MACRO

#else 

#define JS_DECL_USE_GUARD_OBJECT_NOTIFIER
#define JS_GUARD_OBJECT_NOTIFIER_PARAM
#define JS_GUARD_OBJECT_NOTIFIER_PARAM_NO_INIT
#define JS_GUARD_OBJECT_NOTIFIER_PARAM0
#define JS_GUARD_OBJECT_NOTIFIER_INIT JS_BEGIN_MACRO JS_END_MACRO

#endif 

namespace js {





































































































template<typename T>
class MoveRef {
  public:
    typedef T Referent;
    explicit MoveRef(T &t) : pointer(&t) { }
    T &operator*()  const { return *pointer; }
    T *operator->() const { return  pointer; }
#ifdef __GXX_EXPERIMENTAL_CXX0X__
    








    operator T&& ()  const { return static_cast<T&&>(*pointer); }
#else
    operator T& ()   const { return *pointer; }
#endif
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





JS_ALWAYS_INLINE size_t
RoundUpPow2(size_t x)
{
    return size_t(1) << JS_CEILING_LOG2W(x);
}

} 

#endif 




typedef size_t(*JSMallocSizeOfFun)(const void *p, size_t computedSize);


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
