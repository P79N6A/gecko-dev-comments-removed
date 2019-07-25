










































#ifndef jsutil_h___
#define jsutil_h___

#include "jstypes.h"
#include <stdlib.h>
#include <string.h>

JS_BEGIN_EXTERN_C





extern JS_PUBLIC_API(void)
JS_Assert(const char *s, const char *file, JSIntn ln);

#define JS_CRASH_UNLESS(__cond)                                                 \
    JS_BEGIN_MACRO                                                              \
        if (!(__cond)) {                                                        \
            *(int *)(uintptr_t)0xccadbeef = 0;                                  \
            ((void(*)())0)(); /* More reliable, but doesn't say CCADBEEF */     \
        }                                                                       \
    JS_END_MACRO

#ifdef DEBUG

#define JS_ASSERT(expr)                                                       \
    ((expr) ? (void)0 : JS_Assert(#expr, __FILE__, __LINE__))

#define JS_ASSERT_IF(cond, expr)                                              \
    ((!(cond) || (expr)) ? (void)0 : JS_Assert(#expr, __FILE__, __LINE__))

#define JS_NOT_REACHED(reason)                                                \
    JS_Assert(reason, __FILE__, __LINE__)

#define JS_ALWAYS_TRUE(expr) JS_ASSERT(expr)

#define JS_ALWAYS_FALSE(expr) JS_ASSERT(!(expr))

# ifdef JS_THREADSAFE
# define JS_THREADSAFE_ASSERT(expr) JS_ASSERT(expr) 
# else
# define JS_THREADSAFE_ASSERT(expr) ((void) 0)
# endif

#else

#define JS_ASSERT(expr)         ((void) 0)
#define JS_ASSERT_IF(cond,expr) ((void) 0)
#define JS_NOT_REACHED(reason)
#define JS_ALWAYS_TRUE(expr)    ((void) (expr))
#define JS_ALWAYS_FALSE(expr)    ((void) (expr))
#define JS_THREADSAFE_ASSERT(expr) ((void) 0)

#endif 







#ifdef __SUNPRO_CC







#define JS_STATIC_ASSERT(cond) extern char js_static_assert[(cond) ? 1 : -1]
#else
#ifdef __COUNTER__
    #define JS_STATIC_ASSERT_GLUE1(x,y) x##y
    #define JS_STATIC_ASSERT_GLUE(x,y) JS_STATIC_ASSERT_GLUE1(x,y)
    #define JS_STATIC_ASSERT(cond)                                            \
        typedef int JS_STATIC_ASSERT_GLUE(js_static_assert, __COUNTER__)[(cond) ? 1 : -1]
#else
    #define JS_STATIC_ASSERT(cond) extern void js_static_assert(int arg[(cond) ? 1 : -1])
#endif
#endif

#define JS_STATIC_ASSERT_IF(cond, expr) JS_STATIC_ASSERT(!(cond) || (expr))






extern JS_PUBLIC_API(void) JS_Abort(void);

#ifdef DEBUG
# define JS_BASIC_STATS 1
#endif

#ifdef DEBUG_brendan
# define JS_SCOPE_DEPTH_METER 1
#endif

#ifdef JS_BASIC_STATS

#include <stdio.h>

typedef struct JSBasicStats {
    uint32      num;
    uint32      max;
    double      sum;
    double      sqsum;
    uint32      logscale;           
    uint32      hist[11];
} JSBasicStats;

#define JS_INIT_STATIC_BASIC_STATS  {0,0,0,0,0,{0,0,0,0,0,0,0,0,0,0,0}}
#define JS_BASIC_STATS_INIT(bs)     memset((bs), 0, sizeof(JSBasicStats))

#define JS_BASIC_STATS_ACCUM(bs,val)                                          \
    JS_BasicStatsAccum(bs, val)

#define JS_MeanAndStdDevBS(bs,sigma)                                          \
    JS_MeanAndStdDev((bs)->num, (bs)->sum, (bs)->sqsum, sigma)

extern void
JS_BasicStatsAccum(JSBasicStats *bs, uint32 val);

extern double
JS_MeanAndStdDev(uint32 num, double sum, double sqsum, double *sigma);

extern void
JS_DumpBasicStats(JSBasicStats *bs, const char *title, FILE *fp);

extern void
JS_DumpHistogram(JSBasicStats *bs, FILE *fp);

#else

#define JS_BASIC_STATS_ACCUM(bs,val)

#endif 


#if defined(DEBUG_notme) && defined(XP_UNIX)

typedef struct JSCallsite JSCallsite;

struct JSCallsite {
    uint32      pc;
    char        *name;
    const char  *library;
    int         offset;
    JSCallsite  *parent;
    JSCallsite  *siblings;
    JSCallsite  *kids;
    void        *handy;
};

extern JS_FRIEND_API(JSCallsite *)
JS_Backtrace(int skip);

extern JS_FRIEND_API(void)
JS_DumpBacktrace(JSCallsite *trace);
#endif

#if defined JS_USE_CUSTOM_ALLOCATOR

#include "jscustomallocator.h"

#else

static JS_INLINE void* js_malloc(size_t bytes) {
    return malloc(bytes);
}

static JS_INLINE void* js_calloc(size_t bytes) {
    return calloc(bytes, 1);
}

static JS_INLINE void* js_realloc(void* p, size_t bytes) {
    return realloc(p, bytes);
}

static JS_INLINE void js_free(void* p) {
    free(p);
}
#endif

JS_END_EXTERN_C

#ifdef __cplusplus

















#define JS_NEW_BODY(t, parms)                                                 \
    void *memory = js_malloc(sizeof(t));                                      \
    return memory ? new(memory) t parms : NULL;

template <class T>
JS_ALWAYS_INLINE T *js_new() {
    JS_NEW_BODY(T, ())
}

template <class T, class P1>
JS_ALWAYS_INLINE T *js_new(const P1 &p1) {
    JS_NEW_BODY(T, (p1))
}

template <class T, class P1, class P2>
JS_ALWAYS_INLINE T *js_new(const P1 &p1, const P2 &p2) {
    JS_NEW_BODY(T, (p1, p2))
}

template <class T, class P1, class P2, class P3>
JS_ALWAYS_INLINE T *js_new(const P1 &p1, const P2 &p2, const P3 &p3) {
    JS_NEW_BODY(T, (p1, p2, p3))
}

template <class T, class P1, class P2, class P3, class P4>
JS_ALWAYS_INLINE T *js_new(const P1 &p1, const P2 &p2, const P3 &p3, const P4 &p4) {
    JS_NEW_BODY(T, (p1, p2, p3, p4))
}



#undef JS_NEW_BODY

template <class T>
JS_ALWAYS_INLINE void js_delete(T *p) {
    if (p) {
        p->~T();
        js_free(p);
    }
}

static const int JSMinAlignment = 8;

template <class T>
JS_ALWAYS_INLINE T *js_array_new(size_t n) {
	
    uint64 numBytes64 = uint64(JSMinAlignment) + uint64(sizeof(T)) * uint64(n);
    size_t numBytes = size_t(numBytes64);
    if (numBytes64 != numBytes) {
        JS_ASSERT(0);   
        return NULL;
    }
    void *memory = js_malloc(numBytes);
    if (!memory)
        return NULL;
	*(size_t *)memory = n;
	memory = (void*)(uintptr_t(memory) + JSMinAlignment);
    return new(memory) T[n];
}

template <class T>
JS_ALWAYS_INLINE void js_array_delete(T *p) {
    if (p) {
		void* p0 = (void *)(uintptr_t(p) - JSMinAlignment);
		size_t n = *(size_t *)p0;
		for (size_t i = 0; i < n; i++)
			(p + i)->~T();
		js_free(p0);
    }
}



































#ifdef DEBUG
class JSGuardObjectNotifier
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

class JSGuardObjectNotificationReceiver
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
#define JS_GUARD_OBJECT_NOTIFIER_PARAM0 \
    const JSGuardObjectNotifier& _notifier = JSGuardObjectNotifier()
#define JS_GUARD_OBJECT_NOTIFIER_INIT \
    JS_BEGIN_MACRO _mCheckNotUsedAsTemporary.Init(_notifier); JS_END_MACRO

#else 

#define JS_DECL_USE_GUARD_OBJECT_NOTIFIER
#define JS_GUARD_OBJECT_NOTIFIER_PARAM
#define JS_GUARD_OBJECT_NOTIFIER_PARAM0
#define JS_GUARD_OBJECT_NOTIFIER_INIT JS_BEGIN_MACRO JS_END_MACRO

#endif 

namespace js {

template <class T>
JS_ALWAYS_INLINE static void
PodZero(T *t)
{
    memset(t, 0, sizeof(T));
}

template <class T>
JS_ALWAYS_INLINE static void
PodZero(T *t, size_t nelem)
{
    memset(t, 0, nelem * sizeof(T));
}








template <class T, size_t N> static void PodZero(T (&)[N]);          
template <class T, size_t N> static void PodZero(T (&)[N], size_t);  

template <class T, size_t N>
JS_ALWAYS_INLINE static void
PodArrayZero(T (&t)[N])
{
    memset(t, 0, N * sizeof(T));
}

template <class T>
JS_ALWAYS_INLINE static void
PodCopy(T *dst, const T *src, size_t nelem)
{
    
    JS_ASSERT_IF(dst >= src, size_t(dst - src) >= nelem);
    JS_ASSERT_IF(src >= dst, size_t(src - dst) >= nelem);

    if (nelem < 128) {
        for (const T *srcend = src + nelem; src != srcend; ++src, ++dst)
            *dst = *src;
    } else {
        memcpy(dst, src, nelem * sizeof(T));
    }
}

} 

#endif 

#endif 
