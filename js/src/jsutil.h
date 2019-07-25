










































#ifndef jsutil_h___
#define jsutil_h___

#include "jstypes.h"
#include <stdlib.h>

JS_BEGIN_EXTERN_C





extern JS_PUBLIC_API(void)
JS_Assert(const char *s, const char *file, JSIntn ln);

#ifdef DEBUG

#define JS_ASSERT(expr)                                                       \
    ((expr) ? (void)0 : JS_Assert(#expr, __FILE__, __LINE__))

#define JS_ASSERT_IF(cond, expr)                                              \
    ((!(cond) || (expr)) ? (void)0 : JS_Assert(#expr, __FILE__, __LINE__))

#define JS_NOT_REACHED(reason)                                                \
    JS_Assert(reason, __FILE__, __LINE__)

#define JS_ALWAYS_TRUE(expr) JS_ASSERT(expr)

#else

#define JS_ASSERT(expr)         ((void) 0)
#define JS_ASSERT_IF(cond,expr) ((void) 0)
#define JS_NOT_REACHED(reason)
#define JS_ALWAYS_TRUE(expr)    ((void) (expr))

#endif 













#ifdef __SUNPRO_CC
#define JS_STATIC_ASSERT(cond)
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

    void SetStatementDone(bool *aStatementDone) {
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
            SetStatementDone(&mStatementDone);
    }
};

#define JS_DECL_USE_GUARD_OBJECT_NOTIFIER \
    JSGuardObjectNotificationReceiver _mCheckNotUsedAsTemporary;
#define JS_GUARD_OBJECT_NOTIFIER_PARAM \
    , const JSGuardObjectNotifier& _notifier = JSGuardObjectNotifier()
#define JS_GUARD_OBJECT_NOTIFIER_INIT \
    JS_BEGIN_MACRO _mCheckNotUsedAsTemporary.Init(_notifier); JS_END_MACRO

#else 

#define JS_DECL_USE_GUARD_OBJECT_NOTIFIER
#define JS_GUARD_OBJECT_NOTIFIER_PARAM
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

} 

#endif 

#endif 
