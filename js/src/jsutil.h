










































#ifndef jsutil_h___
#define jsutil_h___

JS_BEGIN_EXTERN_C

#ifdef DEBUG

extern JS_PUBLIC_API(void)
JS_Assert(const char *s, const char *file, JSIntn ln);
#define JS_ASSERT(_expr) \
    ((_expr)?((void)0):JS_Assert(# _expr,__FILE__,__LINE__))

#define JS_ASSERT_IF(_cond, _expr) \
    (!(_cond) || (_expr)?((void)0):JS_Assert(# _expr,__FILE__,__LINE__))

#define JS_NOT_REACHED(_reasonStr) \
    JS_Assert(_reasonStr,__FILE__,__LINE__)

#else

#define JS_ASSERT(expr) ((void) 0)
#define JS_ASSERT_IF(cond,expr) ((void) 0)
#define JS_NOT_REACHED(reasonStr)

#endif 






#define JS_STATIC_ASSERT(condition)                                           \
    extern void js_static_assert(int arg[(condition) ? 1 : -1])






extern JS_PUBLIC_API(void) JS_Abort(void);

#ifdef XP_UNIX

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

extern JSCallsite *JS_Backtrace(int skip);

#endif

JS_END_EXTERN_C

#endif 
