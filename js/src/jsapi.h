






































#ifndef jsapi_h___
#define jsapi_h___



#include <stddef.h>
#include <stdio.h>
#include "jspubtd.h"
#include "jsutil.h"

JS_BEGIN_EXTERN_C




#define JSVAL_OBJECT            0x0     /* untagged reference to object */
#define JSVAL_INT               0x1     /* tagged 31-bit integer value */
#define JSVAL_DOUBLE            0x2     /* tagged reference to double */
#define JSVAL_STRING            0x4     /* tagged reference to string */
#define JSVAL_BOOLEAN           0x6     /* tagged boolean value */


#define JSVAL_TAGBITS           3
#define JSVAL_TAGMASK           JS_BITMASK(JSVAL_TAGBITS)
#define JSVAL_TAG(v)            ((v) & JSVAL_TAGMASK)
#define JSVAL_SETTAG(v,t)       ((v) | (t))
#define JSVAL_CLRTAG(v)         ((v) & ~(jsval)JSVAL_TAGMASK)
#define JSVAL_ALIGN             JS_BIT(JSVAL_TAGBITS)


#define JSVAL_IS_OBJECT(v)      (JSVAL_TAG(v) == JSVAL_OBJECT)
#define JSVAL_IS_NUMBER(v)      (JSVAL_IS_INT(v) || JSVAL_IS_DOUBLE(v))
#define JSVAL_IS_INT(v)         (((v) & JSVAL_INT) && (v) != JSVAL_VOID)
#define JSVAL_IS_DOUBLE(v)      (JSVAL_TAG(v) == JSVAL_DOUBLE)
#define JSVAL_IS_STRING(v)      (JSVAL_TAG(v) == JSVAL_STRING)
#define JSVAL_IS_BOOLEAN(v)     (JSVAL_TAG(v) == JSVAL_BOOLEAN)
#define JSVAL_IS_NULL(v)        ((v) == JSVAL_NULL)
#define JSVAL_IS_VOID(v)        ((v) == JSVAL_VOID)
#define JSVAL_IS_PRIMITIVE(v)   (!JSVAL_IS_OBJECT(v) || JSVAL_IS_NULL(v))


#define JSVAL_IS_GCTHING(v)     (!((v) & JSVAL_INT) && !JSVAL_IS_BOOLEAN(v))
#define JSVAL_TO_GCTHING(v)     ((void *)JSVAL_CLRTAG(v))
#define JSVAL_TO_OBJECT(v)      ((JSObject *)JSVAL_TO_GCTHING(v))
#define JSVAL_TO_DOUBLE(v)      ((jsdouble *)JSVAL_TO_GCTHING(v))
#define JSVAL_TO_STRING(v)      ((JSString *)JSVAL_TO_GCTHING(v))
#define OBJECT_TO_JSVAL(obj)    ((jsval)(obj))
#define DOUBLE_TO_JSVAL(dp)     JSVAL_SETTAG((jsval)(dp), JSVAL_DOUBLE)
#define STRING_TO_JSVAL(str)    JSVAL_SETTAG((jsval)(str), JSVAL_STRING)


#define JSVAL_LOCK(cx,v)        (JSVAL_IS_GCTHING(v)                          \
                                 ? JS_LockGCThing(cx, JSVAL_TO_GCTHING(v))    \
                                 : JS_TRUE)
#define JSVAL_UNLOCK(cx,v)      (JSVAL_IS_GCTHING(v)                          \
                                 ? JS_UnlockGCThing(cx, JSVAL_TO_GCTHING(v))  \
                                 : JS_TRUE)


#define JSVAL_INT_BITS          31
#define JSVAL_INT_POW2(n)       ((jsval)1 << (n))
#define JSVAL_INT_MIN           ((jsval)1 - JSVAL_INT_POW2(30))
#define JSVAL_INT_MAX           (JSVAL_INT_POW2(30) - 1)
#define INT_FITS_IN_JSVAL(i)    ((jsuint)((i)+JSVAL_INT_MAX) <= 2*JSVAL_INT_MAX)
#define JSVAL_TO_INT(v)         ((jsint)(v) >> 1)
#define INT_TO_JSVAL(i)         (((jsval)(i) << 1) | JSVAL_INT)


#define JSVAL_TO_BOOLEAN(v)     ((JSBool)((v) >> JSVAL_TAGBITS))
#define BOOLEAN_TO_JSVAL(b)     JSVAL_SETTAG((jsval)(b) << JSVAL_TAGBITS,     \
                                             JSVAL_BOOLEAN)


#define JSVAL_TO_PRIVATE(v)     ((void *)((v) & ~JSVAL_INT))
#define PRIVATE_TO_JSVAL(p)     ((jsval)(p) | JSVAL_INT)


#define JSPROP_ENUMERATE        0x01    /* property is visible to for/in loop */
#define JSPROP_READONLY         0x02    /* not settable: assignment is no-op */
#define JSPROP_PERMANENT        0x04    /* property cannot be deleted */
#define JSPROP_EXPORTED         0x08    /* property is exported from object */
#define JSPROP_GETTER           0x10    /* property holds getter function */
#define JSPROP_SETTER           0x20    /* property holds setter function */
#define JSPROP_SHARED           0x40    /* don't allocate a value slot for this
                                           property; don't copy the property on
                                           set of the same-named property in an
                                           object that delegates to a prototype
                                           containing this property */
#define JSPROP_INDEX            0x80    /* name is actually (jsint) index */


#define JSFUN_LAMBDA            0x08    /* expressed, not declared, function */
#define JSFUN_GETTER            JSPROP_GETTER
#define JSFUN_SETTER            JSPROP_SETTER
#define JSFUN_BOUND_METHOD      0x40    /* bind this to fun->object's parent */
#define JSFUN_HEAVYWEIGHT       0x80    /* activation requires a Call object */

#define JSFUN_DISJOINT_FLAGS(f) ((f) & 0x0f)
#define JSFUN_GSFLAGS(f)        ((f) & (JSFUN_GETTER | JSFUN_SETTER))

#ifdef MOZILLA_1_8_BRANCH





#define JSFUN_GETTER_TEST(f)       (JSFUN_GSFLAGS(f) == JSFUN_GETTER)
#define JSFUN_SETTER_TEST(f)       (JSFUN_GSFLAGS(f) == JSFUN_SETTER)
#define JSFUN_FLAGS_TEST(f,t)      (JSFUN_GSFLAGS(~(f)) ? (f) & (t) : 0)
#define JSFUN_BOUND_METHOD_TEST(f) JSFUN_FLAGS_TEST(f, JSFUN_BOUND_METHOD)
#define JSFUN_HEAVYWEIGHT_TEST(f)  JSFUN_FLAGS_TEST(f, JSFUN_HEAVYWEIGHT)

#define JSFUN_GSFLAG2ATTR(f)       (JSFUN_GETTER_TEST(f) ? JSPROP_GETTER :    \
                                    JSFUN_SETTER_TEST(f) ? JSPROP_SETTER : 0)

#define JSFUN_THISP_FLAGS(f)    (JSFUN_GSFLAGS(~(f)) ? 0 :                    \
                                 (f) & JSFUN_THISP_PRIMITIVE)
#define JSFUN_THISP_TEST(f,t)   ((f) == (t) || (f) == JSFUN_THISP_PRIMITIVE)

#define JSFUN_THISP_STRING      0x30    /* |this| may be a primitive string */
#define JSFUN_THISP_NUMBER      0x70    /* |this| may be a primitive number */
#define JSFUN_THISP_BOOLEAN     0xb0    /* |this| may be a primitive boolean */
#define JSFUN_THISP_PRIMITIVE   0xf0    /* |this| may be any primitive value */

#define JSFUN_FLAGS_MASK        0xf8    /* overlay JSFUN_* attributes */

#else

#define JSFUN_GETTER_TEST(f)       ((f) & JSFUN_GETTER)
#define JSFUN_SETTER_TEST(f)       ((f) & JSFUN_SETTER)
#define JSFUN_BOUND_METHOD_TEST(f) ((f) & JSFUN_BOUND_METHOD)
#define JSFUN_HEAVYWEIGHT_TEST(f)  ((f) & JSFUN_HEAVYWEIGHT)

#define JSFUN_GSFLAG2ATTR(f)       JSFUN_GSFLAGS(f)

#define JSFUN_THISP_FLAGS(f)  (f)
#define JSFUN_THISP_TEST(f,t) ((f) & t)

#define JSFUN_THISP_STRING    0x0100    /* |this| may be a primitive string */
#define JSFUN_THISP_NUMBER    0x0200    /* |this| may be a primitive number */
#define JSFUN_THISP_BOOLEAN   0x0400    /* |this| may be a primitive boolean */
#define JSFUN_THISP_PRIMITIVE 0x0700    /* |this| may be any primitive value */

#define JSFUN_FLAGS_MASK      0x07f8    /* overlay JSFUN_* attributes --
                                           note that bit #15 is used internally
                                           to flag interpreted functions */

#endif












#define JSFUN_GENERIC_NATIVE    JSFUN_LAMBDA





#define JSVAL_VOID              INT_TO_JSVAL(0 - JSVAL_INT_POW2(30))
#define JSVAL_NULL              OBJECT_TO_JSVAL(0)
#define JSVAL_ZERO              INT_TO_JSVAL(0)
#define JSVAL_ONE               INT_TO_JSVAL(1)
#define JSVAL_FALSE             BOOLEAN_TO_JSVAL(JS_FALSE)
#define JSVAL_TRUE              BOOLEAN_TO_JSVAL(JS_TRUE)





extern JS_PUBLIC_API(int64)
JS_Now();


extern JS_PUBLIC_API(jsval)
JS_GetNaNValue(JSContext *cx);

extern JS_PUBLIC_API(jsval)
JS_GetNegativeInfinityValue(JSContext *cx);

extern JS_PUBLIC_API(jsval)
JS_GetPositiveInfinityValue(JSContext *cx);

extern JS_PUBLIC_API(jsval)
JS_GetEmptyStringValue(JSContext *cx);


































extern JS_PUBLIC_API(JSBool)
JS_ConvertArguments(JSContext *cx, uintN argc, jsval *argv, const char *format,
                    ...);

#ifdef va_start
extern JS_PUBLIC_API(JSBool)
JS_ConvertArgumentsVA(JSContext *cx, uintN argc, jsval *argv,
                      const char *format, va_list ap);
#endif











extern JS_PUBLIC_API(jsval *)
JS_PushArguments(JSContext *cx, void **markp, const char *format, ...);

#ifdef va_start
extern JS_PUBLIC_API(jsval *)
JS_PushArgumentsVA(JSContext *cx, void **markp, const char *format, va_list ap);
#endif

extern JS_PUBLIC_API(void)
JS_PopArguments(JSContext *cx, void *mark);

#ifdef JS_ARGUMENT_FORMATTER_DEFINED








































extern JS_PUBLIC_API(JSBool)
JS_AddArgumentFormatter(JSContext *cx, const char *format,
                        JSArgumentFormatter formatter);

extern JS_PUBLIC_API(void)
JS_RemoveArgumentFormatter(JSContext *cx, const char *format);

#endif 

extern JS_PUBLIC_API(JSBool)
JS_ConvertValue(JSContext *cx, jsval v, JSType type, jsval *vp);

extern JS_PUBLIC_API(JSBool)
JS_ValueToObject(JSContext *cx, jsval v, JSObject **objp);

extern JS_PUBLIC_API(JSFunction *)
JS_ValueToFunction(JSContext *cx, jsval v);

extern JS_PUBLIC_API(JSFunction *)
JS_ValueToConstructor(JSContext *cx, jsval v);

extern JS_PUBLIC_API(JSString *)
JS_ValueToString(JSContext *cx, jsval v);

extern JS_PUBLIC_API(JSBool)
JS_ValueToNumber(JSContext *cx, jsval v, jsdouble *dp);





extern JS_PUBLIC_API(JSBool)
JS_ValueToECMAInt32(JSContext *cx, jsval v, int32 *ip);





extern JS_PUBLIC_API(JSBool)
JS_ValueToECMAUint32(JSContext *cx, jsval v, uint32 *ip);






extern JS_PUBLIC_API(JSBool)
JS_ValueToInt32(JSContext *cx, jsval v, int32 *ip);




extern JS_PUBLIC_API(JSBool)
JS_ValueToUint16(JSContext *cx, jsval v, uint16 *ip);

extern JS_PUBLIC_API(JSBool)
JS_ValueToBoolean(JSContext *cx, jsval v, JSBool *bp);

extern JS_PUBLIC_API(JSType)
JS_TypeOfValue(JSContext *cx, jsval v);

extern JS_PUBLIC_API(const char *)
JS_GetTypeName(JSContext *cx, JSType type);










#define JS_NewRuntime       JS_Init
#define JS_DestroyRuntime   JS_Finish
#define JS_LockRuntime      JS_Lock
#define JS_UnlockRuntime    JS_Unlock

extern JS_PUBLIC_API(JSRuntime *)
JS_NewRuntime(uint32 maxbytes);

extern JS_PUBLIC_API(void)
JS_DestroyRuntime(JSRuntime *rt);

extern JS_PUBLIC_API(void)
JS_ShutDown(void);

JS_PUBLIC_API(void *)
JS_GetRuntimePrivate(JSRuntime *rt);

JS_PUBLIC_API(void)
JS_SetRuntimePrivate(JSRuntime *rt, void *data);

#ifdef JS_THREADSAFE

extern JS_PUBLIC_API(void)
JS_BeginRequest(JSContext *cx);

extern JS_PUBLIC_API(void)
JS_EndRequest(JSContext *cx);


extern JS_PUBLIC_API(void)
JS_YieldRequest(JSContext *cx);

extern JS_PUBLIC_API(jsrefcount)
JS_SuspendRequest(JSContext *cx);

extern JS_PUBLIC_API(void)
JS_ResumeRequest(JSContext *cx, jsrefcount saveDepth);

#ifdef __cplusplus
JS_END_EXTERN_C

class JSAutoRequest {
  public:
    JSAutoRequest(JSContext *cx) : mContext(cx), mSaveDepth(0) {
        JS_BeginRequest(mContext);
    }
    ~JSAutoRequest() {
        JS_EndRequest(mContext);
    }

    void suspend() {
        mSaveDepth = JS_SuspendRequest(mContext);
    }
    void resume() {
        JS_ResumeRequest(mContext, mSaveDepth);
    }

  protected:
    JSContext *mContext;
    jsrefcount mSaveDepth;

#if 0
  private:
    static void *operator new(size_t) CPP_THROW_NEW { return 0; };
    static void operator delete(void *, size_t) { };
#endif
};

JS_BEGIN_EXTERN_C
#endif

#endif 

extern JS_PUBLIC_API(void)
JS_Lock(JSRuntime *rt);

extern JS_PUBLIC_API(void)
JS_Unlock(JSRuntime *rt);

extern JS_PUBLIC_API(JSContextCallback)
JS_SetContextCallback(JSRuntime *rt, JSContextCallback cxCallback);

extern JS_PUBLIC_API(JSContext *)
JS_NewContext(JSRuntime *rt, size_t stackChunkSize);

extern JS_PUBLIC_API(void)
JS_DestroyContext(JSContext *cx);

extern JS_PUBLIC_API(void)
JS_DestroyContextNoGC(JSContext *cx);

extern JS_PUBLIC_API(void)
JS_DestroyContextMaybeGC(JSContext *cx);

extern JS_PUBLIC_API(void *)
JS_GetContextPrivate(JSContext *cx);

extern JS_PUBLIC_API(void)
JS_SetContextPrivate(JSContext *cx, void *data);

extern JS_PUBLIC_API(JSRuntime *)
JS_GetRuntime(JSContext *cx);

extern JS_PUBLIC_API(JSContext *)
JS_ContextIterator(JSRuntime *rt, JSContext **iterp);

extern JS_PUBLIC_API(JSVersion)
JS_GetVersion(JSContext *cx);

extern JS_PUBLIC_API(JSVersion)
JS_SetVersion(JSContext *cx, JSVersion version);

extern JS_PUBLIC_API(const char *)
JS_VersionToString(JSVersion version);

extern JS_PUBLIC_API(JSVersion)
JS_StringToVersion(const char *string);








#define JSOPTION_STRICT         JS_BIT(0)       /* warn on dubious practice */
#define JSOPTION_WERROR         JS_BIT(1)       /* convert warning to error */
#define JSOPTION_VAROBJFIX      JS_BIT(2)       /* make JS_EvaluateScript use
                                                   the last object on its 'obj'
                                                   param's scope chain as the
                                                   ECMA 'variables object' */
#define JSOPTION_PRIVATE_IS_NSISUPPORTS \
                                JS_BIT(3)       /* context private data points
                                                   to an nsISupports subclass */
#define JSOPTION_COMPILE_N_GO   JS_BIT(4)       /* caller of JS_Compile*Script
                                                   promises to execute compiled
                                                   script once only; enables
                                                   compile-time scope chain
                                                   resolution of consts. */
#define JSOPTION_ATLINE         JS_BIT(5)       /* //@line number ["filename"]
                                                   option supported for the
                                                   XUL preprocessor and kindred
                                                   beasts. */
#define JSOPTION_XML            JS_BIT(6)       /* EMCAScript for XML support:
                                                   parse <!-- --> as a token,
                                                   not backward compatible with
                                                   the comment-hiding hack used
                                                   in HTML script tags. */
#define JSOPTION_NATIVE_BRANCH_CALLBACK \
                                JS_BIT(7)       /* the branch callback set by
                                                   JS_SetBranchCallback may be
                                                   called with a null script
                                                   parameter, by native code
                                                   that loops intensively */
#define JSOPTION_DONT_REPORT_UNCAUGHT \
                                JS_BIT(8)       /* When returning from the
                                                   outermost API call, prevent
                                                   uncaught exceptions from
                                                   being converted to error
                                                   reports */

#define JSOPTION_RELIMIT        JS_BIT(9)       /* Throw exception on any
                                                   regular expression which
                                                   backtracks more than n^3
                                                   times, where n is length
                                                   of the input string */
#define JSOPTION_ANONFUNFIX     JS_BIT(10)      /* Disallow function () {} in
                                                   statement context per
                                                   ECMA-262 Edition 3. */

extern JS_PUBLIC_API(uint32)
JS_GetOptions(JSContext *cx);

extern JS_PUBLIC_API(uint32)
JS_SetOptions(JSContext *cx, uint32 options);

extern JS_PUBLIC_API(uint32)
JS_ToggleOptions(JSContext *cx, uint32 options);

extern JS_PUBLIC_API(const char *)
JS_GetImplementationVersion(void);

extern JS_PUBLIC_API(JSObject *)
JS_GetGlobalObject(JSContext *cx);

extern JS_PUBLIC_API(void)
JS_SetGlobalObject(JSContext *cx, JSObject *obj);








extern JS_PUBLIC_API(JSBool)
JS_InitStandardClasses(JSContext *cx, JSObject *obj);














extern JS_PUBLIC_API(JSBool)
JS_ResolveStandardClass(JSContext *cx, JSObject *obj, jsval id,
                        JSBool *resolved);

extern JS_PUBLIC_API(JSBool)
JS_EnumerateStandardClasses(JSContext *cx, JSObject *obj);






extern JS_PUBLIC_API(JSIdArray *)
JS_EnumerateResolvedStandardClasses(JSContext *cx, JSObject *obj,
                                    JSIdArray *ida);

extern JS_PUBLIC_API(JSBool)
JS_GetClassObject(JSContext *cx, JSObject *obj, JSProtoKey key,
                  JSObject **objp);

extern JS_PUBLIC_API(JSObject *)
JS_GetScopeChain(JSContext *cx);

extern JS_PUBLIC_API(void *)
JS_malloc(JSContext *cx, size_t nbytes);

extern JS_PUBLIC_API(void *)
JS_realloc(JSContext *cx, void *p, size_t nbytes);

extern JS_PUBLIC_API(void)
JS_free(JSContext *cx, void *p);

extern JS_PUBLIC_API(char *)
JS_strdup(JSContext *cx, const char *s);

extern JS_PUBLIC_API(jsdouble *)
JS_NewDouble(JSContext *cx, jsdouble d);

extern JS_PUBLIC_API(JSBool)
JS_NewDoubleValue(JSContext *cx, jsdouble d, jsval *rval);

extern JS_PUBLIC_API(JSBool)
JS_NewNumberValue(JSContext *cx, jsdouble d, jsval *rval);

















extern JS_PUBLIC_API(JSBool)
JS_AddRoot(JSContext *cx, void *rp);

#ifdef NAME_ALL_GC_ROOTS
#define JS_DEFINE_TO_TOKEN(def) #def
#define JS_DEFINE_TO_STRING(def) JS_DEFINE_TO_TOKEN(def)
#define JS_AddRoot(cx,rp) JS_AddNamedRoot((cx), (rp), (__FILE__ ":" JS_TOKEN_TO_STRING(__LINE__))
#endif

extern JS_PUBLIC_API(JSBool)
JS_AddNamedRoot(JSContext *cx, void *rp, const char *name);

extern JS_PUBLIC_API(JSBool)
JS_AddNamedRootRT(JSRuntime *rt, void *rp, const char *name);

extern JS_PUBLIC_API(JSBool)
JS_RemoveRoot(JSContext *cx, void *rp);

extern JS_PUBLIC_API(JSBool)
JS_RemoveRootRT(JSRuntime *rt, void *rp);




















extern JS_PUBLIC_API(void)
JS_ClearNewbornRoots(JSContext *cx);














































extern JS_PUBLIC_API(JSBool)
JS_EnterLocalRootScope(JSContext *cx);

extern JS_PUBLIC_API(void)
JS_LeaveLocalRootScope(JSContext *cx);

extern JS_PUBLIC_API(void)
JS_LeaveLocalRootScopeWithResult(JSContext *cx, jsval rval);

extern JS_PUBLIC_API(void)
JS_ForgetLocalRoot(JSContext *cx, void *thing);

#ifdef __cplusplus
JS_END_EXTERN_C

class JSAutoLocalRootScope {
  public:
    JSAutoLocalRootScope(JSContext *cx) : mContext(cx) {
        JS_EnterLocalRootScope(mContext);
    }
    ~JSAutoLocalRootScope() {
        JS_LeaveLocalRootScope(mContext);
    }

    void forget(void *thing) {
        JS_ForgetLocalRoot(mContext, thing);
    }

  protected:
    JSContext *mContext;

#if 0
  private:
    static void *operator new(size_t) CPP_THROW_NEW { return 0; };
    static void operator delete(void *, size_t) { };
#endif
};

JS_BEGIN_EXTERN_C
#endif

#ifdef DEBUG
extern JS_PUBLIC_API(void)
JS_DumpNamedRoots(JSRuntime *rt,
                  void (*dump)(const char *name, void *rp, void *data),
                  void *data);
#endif



















#define JS_MAP_GCROOT_NEXT      0       /* continue mapping entries */
#define JS_MAP_GCROOT_STOP      1       /* stop mapping entries */
#define JS_MAP_GCROOT_REMOVE    2       /* remove and free the current entry */

typedef intN
(* JS_DLL_CALLBACK JSGCRootMapFun)(void *rp, const char *name, void *data);

extern JS_PUBLIC_API(uint32)
JS_MapGCRoots(JSRuntime *rt, JSGCRootMapFun map, void *data);

extern JS_PUBLIC_API(JSBool)
JS_LockGCThing(JSContext *cx, void *thing);

extern JS_PUBLIC_API(JSBool)
JS_LockGCThingRT(JSRuntime *rt, void *thing);

extern JS_PUBLIC_API(JSBool)
JS_UnlockGCThing(JSContext *cx, void *thing);

extern JS_PUBLIC_API(JSBool)
JS_UnlockGCThingRT(JSRuntime *rt, void *thing);








extern JS_PUBLIC_API(void)
JS_SetExtraGCRoots(JSRuntime *rt, JSTraceDataOp traceOp, void *data);





extern JS_PUBLIC_API(void)
JS_MarkGCThing(JSContext *cx, void *thing, const char *name, void *arg);














#define JSTRACE_OBJECT  0
#define JSTRACE_DOUBLE  1
#define JSTRACE_STRING  2





#define JSVAL_IS_TRACEABLE(v)   (JSVAL_IS_GCTHING(v) && !JSVAL_IS_NULL(v))
#define JSVAL_TO_TRACEABLE(v)   (JSVAL_TO_GCTHING(v))
#define JSVAL_TRACE_KIND(v)     (JSVAL_TAG(v) >> 1)

JS_STATIC_ASSERT(JSVAL_TRACE_KIND(JSVAL_OBJECT) == JSTRACE_OBJECT);
JS_STATIC_ASSERT(JSVAL_TRACE_KIND(JSVAL_DOUBLE) == JSTRACE_DOUBLE);
JS_STATIC_ASSERT(JSVAL_TRACE_KIND(JSVAL_STRING) == JSTRACE_STRING);

struct JSTracer {
    JSContext           *context;
    JSTraceCallback     callback;
#ifdef DEBUG
    JSTraceNamePrinter  debugPrinter;
    const void          *debugPrintArg;
    size_t              debugPrintIndex;
#endif
};







extern JS_PUBLIC_API(void)
JS_CallTracer(JSTracer *trc, void *thing, uint32 kind);

















#ifdef DEBUG
# define JS_SET_TRACING_DETAILS(trc, printer, arg, index)                     \
    JS_BEGIN_MACRO                                                            \
        (trc)->debugPrinter = (printer);                                      \
        (trc)->debugPrintArg = (arg);                                         \
        (trc)->debugPrintIndex = (index);                                     \
    JS_END_MACRO
#else
# define JS_SET_TRACING_DETAILS(trc, printer, arg, index)                     \
    JS_BEGIN_MACRO                                                            \
    JS_END_MACRO
#endif





# define JS_SET_TRACING_INDEX(trc, name, index)                               \
    JS_SET_TRACING_DETAILS(trc, NULL, name, index)




# define JS_SET_TRACING_NAME(trc, name)                                       \
    JS_SET_TRACING_DETAILS(trc, NULL, name, (size_t)-1)





# define JS_CALL_TRACER(trc, thing, kind, name)                               \
    JS_BEGIN_MACRO                                                            \
        JS_SET_TRACING_NAME(trc, name);                                       \
        JS_CallTracer((trc), (thing), (kind));                                \
    JS_END_MACRO





#define JS_CALL_VALUE_TRACER(trc, val, name)                                  \
    JS_BEGIN_MACRO                                                            \
        if (JSVAL_IS_TRACEABLE(val)) {                                        \
            JS_CALL_TRACER((trc), JSVAL_TO_GCTHING(val),                      \
                           JSVAL_TRACE_KIND(val), name);                      \
        }                                                                     \
    JS_END_MACRO

#define JS_CALL_OBJECT_TRACER(trc, object, name)                              \
    JS_BEGIN_MACRO                                                            \
        JSObject *obj_ = (object);                                            \
        JS_ASSERT(obj_);                                                      \
        JS_CALL_TRACER((trc), obj_, JSTRACE_OBJECT, name);                    \
    JS_END_MACRO

#define JS_CALL_STRING_TRACER(trc, string, name)                              \
    JS_BEGIN_MACRO                                                            \
        JSString *str_ = (string);                                            \
        JS_ASSERT(str_);                                                      \
        JS_CALL_TRACER((trc), str_, JSTRACE_STRING, name);                    \
    JS_END_MACRO

#define JS_CALL_DOUBLE_TRACER(trc, number, name)                              \
    JS_BEGIN_MACRO                                                            \
        jsdouble *num_ = (number);                                            \
        JS_ASSERT(num_);                                                      \
        JS_CALL_TRACER((trc), num_, JSTRACE_DOUBLE, name);                    \
    JS_END_MACRO




# define JS_TRACER_INIT(trc, cx_, callback_)                                  \
    JS_BEGIN_MACRO                                                            \
        (trc)->context = (cx_);                                               \
        (trc)->callback = (callback_);                                        \
        JS_SET_TRACING_DETAILS(trc, NULL, NULL, (size_t)-1);                  \
    JS_END_MACRO

extern JS_PUBLIC_API(void)
JS_TraceChildren(JSTracer *trc, void *thing, uint32 kind);

extern JS_PUBLIC_API(void)
JS_TraceRuntime(JSTracer *trc);

#ifdef DEBUG

extern JS_PUBLIC_API(void)
JS_PrintTraceThingInfo(char *buf, size_t bufsize, JSTracer *trc,
                       void *thing, uint32 kind, JSBool includeDetails);
















extern JS_PUBLIC_API(JSBool)
JS_DumpHeap(JSContext *cx, FILE *fp, void* startThing, uint32 startKind,
            void *thingToFind, size_t maxDepth, void *thingToIgnore);

#endif




extern JS_PUBLIC_API(void)
JS_GC(JSContext *cx);

extern JS_PUBLIC_API(void)
JS_MaybeGC(JSContext *cx);

extern JS_PUBLIC_API(JSGCCallback)
JS_SetGCCallback(JSContext *cx, JSGCCallback cb);

extern JS_PUBLIC_API(JSGCCallback)
JS_SetGCCallbackRT(JSRuntime *rt, JSGCCallback cb);

extern JS_PUBLIC_API(JSBool)
JS_IsGCMarkingTracer(JSTracer *trc);

extern JS_PUBLIC_API(JSTracer *)
JS_GetGCMarkingTracer(JSContext *cx);

extern JS_PUBLIC_API(void)
JS_SetGCThingCallback(JSContext *cx, JSGCThingCallback cb, void *closure);

extern JS_PUBLIC_API(JSBool)
JS_IsAboutToBeFinalized(JSContext *cx, void *thing);

typedef enum JSGCParamKey {
    JSGC_MAX_BYTES        = 0,  
    JSGC_MAX_MALLOC_BYTES = 1   
} JSGCParamKey;

extern JS_PUBLIC_API(void)
JS_SetGCParameter(JSRuntime *rt, JSGCParamKey key, uint32 value);













extern JS_PUBLIC_API(intN)
JS_AddExternalStringFinalizer(JSStringFinalizeOp finalizer);














extern JS_PUBLIC_API(intN)
JS_RemoveExternalStringFinalizer(JSStringFinalizeOp finalizer);






extern JS_PUBLIC_API(JSString *)
JS_NewExternalString(JSContext *cx, jschar *chars, size_t length, intN type);





extern JS_PUBLIC_API(intN)
JS_GetExternalStringGCType(JSRuntime *rt, JSString *str);






extern JS_PUBLIC_API(void)
JS_SetThreadStackLimit(JSContext *cx, jsuword limitAddr);








struct JSClass {
    const char          *name;
    uint32              flags;

    
    JSPropertyOp        addProperty;
    JSPropertyOp        delProperty;
    JSPropertyOp        getProperty;
    JSPropertyOp        setProperty;
    JSEnumerateOp       enumerate;
    JSResolveOp         resolve;
    JSConvertOp         convert;
    JSFinalizeOp        finalize;

    
    JSGetObjectOps      getObjectOps;
    JSCheckAccessOp     checkAccess;
    JSNative            call;
    JSNative            construct;
    JSXDRObjectOp       xdrObject;
    JSHasInstanceOp     hasInstance;
    JSMarkOp            mark;
    JSReserveSlotsOp    reserveSlots;
};

struct JSExtendedClass {
    JSClass             base;
    JSEqualityOp        equality;
    JSObjectOp          outerObject;
    JSObjectOp          innerObject;
    void                (*reserved0)();
    void                (*reserved1)();
    void                (*reserved2)();
    void                (*reserved3)();
    void                (*reserved4)();
};

#define JSCLASS_HAS_PRIVATE             (1<<0)  /* objects have private slot */
#define JSCLASS_NEW_ENUMERATE           (1<<1)  /* has JSNewEnumerateOp hook */
#define JSCLASS_NEW_RESOLVE             (1<<2)  /* has JSNewResolveOp hook */
#define JSCLASS_PRIVATE_IS_NSISUPPORTS  (1<<3)  /* private is (nsISupports *) */
#define JSCLASS_SHARE_ALL_PROPERTIES    (1<<4)  /* all properties are SHARED */
#define JSCLASS_NEW_RESOLVE_GETS_START  (1<<5)  /* JSNewResolveOp gets starting
                                                   object in prototype chain
                                                   passed in via *objp in/out
                                                   parameter */
#define JSCLASS_CONSTRUCT_PROTOTYPE     (1<<6)  

#define JSCLASS_DOCUMENT_OBSERVER       (1<<7)  /* DOM document observer */






#define JSCLASS_RESERVED_SLOTS_SHIFT    8       /* room for 8 flags below */
#define JSCLASS_RESERVED_SLOTS_WIDTH    8       /* and 16 above this field */
#define JSCLASS_RESERVED_SLOTS_MASK     JS_BITMASK(JSCLASS_RESERVED_SLOTS_WIDTH)
#define JSCLASS_HAS_RESERVED_SLOTS(n)   (((n) & JSCLASS_RESERVED_SLOTS_MASK)  \
                                         << JSCLASS_RESERVED_SLOTS_SHIFT)
#define JSCLASS_RESERVED_SLOTS(clasp)   (((clasp)->flags                      \
                                          >> JSCLASS_RESERVED_SLOTS_SHIFT)    \
                                         & JSCLASS_RESERVED_SLOTS_MASK)

#define JSCLASS_HIGH_FLAGS_SHIFT        (JSCLASS_RESERVED_SLOTS_SHIFT +       \
                                         JSCLASS_RESERVED_SLOTS_WIDTH)


#define JSCLASS_IS_EXTENDED             (1<<(JSCLASS_HIGH_FLAGS_SHIFT+0))
#define JSCLASS_IS_ANONYMOUS            (1<<(JSCLASS_HIGH_FLAGS_SHIFT+1))
#define JSCLASS_IS_GLOBAL               (1<<(JSCLASS_HIGH_FLAGS_SHIFT+2))


#define JSCLASS_MARK_IS_TRACE           (1<<(JSCLASS_HIGH_FLAGS_SHIFT+3))














#define JSCLASS_GLOBAL_FLAGS \
    (JSCLASS_IS_GLOBAL | JSCLASS_HAS_RESERVED_SLOTS(JSProto_LIMIT))


#define JSCLASS_CACHED_PROTO_SHIFT      (JSCLASS_HIGH_FLAGS_SHIFT + 8)
#define JSCLASS_CACHED_PROTO_WIDTH      8
#define JSCLASS_CACHED_PROTO_MASK       JS_BITMASK(JSCLASS_CACHED_PROTO_WIDTH)
#define JSCLASS_HAS_CACHED_PROTO(key)   ((key) << JSCLASS_CACHED_PROTO_SHIFT)
#define JSCLASS_CACHED_PROTO_KEY(clasp) (((clasp)->flags                      \
                                          >> JSCLASS_CACHED_PROTO_SHIFT)      \
                                         & JSCLASS_CACHED_PROTO_MASK)


#define JSCLASS_NO_OPTIONAL_MEMBERS     0,0,0,0,0,0,0,0
#define JSCLASS_NO_RESERVED_MEMBERS     0,0,0,0,0


struct JSObjectOps {
    
    JSNewObjectMapOp    newObjectMap;
    JSObjectMapOp       destroyObjectMap;
    JSLookupPropOp      lookupProperty;
    JSDefinePropOp      defineProperty;
    JSPropertyIdOp      getProperty;
    JSPropertyIdOp      setProperty;
    JSAttributesOp      getAttributes;
    JSAttributesOp      setAttributes;
    JSPropertyIdOp      deleteProperty;
    JSConvertOp         defaultValue;
    JSNewEnumerateOp    enumerate;
    JSCheckAccessIdOp   checkAccess;

    
    JSObjectOp          thisObject;
    JSPropertyRefOp     dropProperty;
    JSNative            call;
    JSNative            construct;
    JSXDRObjectOp       xdrObject;
    JSHasInstanceOp     hasInstance;
    JSSetObjectSlotOp   setProto;
    JSSetObjectSlotOp   setParent;
    JSTraceOp           trace;
    JSFinalizeOp        clear;
    JSGetRequiredSlotOp getRequiredSlot;
    JSSetRequiredSlotOp setRequiredSlot;
};

struct JSXMLObjectOps {
    JSObjectOps         base;
    JSGetMethodOp       getMethod;
    JSSetMethodOp       setMethod;
    JSEnumerateValuesOp enumerateValues;
    JSEqualityOp        equality;
    JSConcatenateOp     concatenate;
};











struct JSProperty {
    jsid id;
};

struct JSIdArray {
    jsint length;
    jsid  vector[1];    
};

extern JS_PUBLIC_API(void)
JS_DestroyIdArray(JSContext *cx, JSIdArray *ida);

extern JS_PUBLIC_API(JSBool)
JS_ValueToId(JSContext *cx, jsval v, jsid *idp);

extern JS_PUBLIC_API(JSBool)
JS_IdToValue(JSContext *cx, jsid id, jsval *vp);






#define JS_DEFAULT_XML_NAMESPACE_ID ((jsid) JSVAL_VOID)




#define JSRESOLVE_QUALIFIED     0x01    /* resolve a qualified property id */
#define JSRESOLVE_ASSIGNING     0x02    /* resolve on the left of assignment */
#define JSRESOLVE_DETECTING     0x04    /* 'if (o.p)...' or '(o.p) ?...:...' */
#define JSRESOLVE_DECLARING     0x08    /* var, const, or function prolog op */
#define JSRESOLVE_CLASSNAME     0x10    /* class name used when constructing */

extern JS_PUBLIC_API(JSBool)
JS_PropertyStub(JSContext *cx, JSObject *obj, jsval id, jsval *vp);

extern JS_PUBLIC_API(JSBool)
JS_EnumerateStub(JSContext *cx, JSObject *obj);

extern JS_PUBLIC_API(JSBool)
JS_ResolveStub(JSContext *cx, JSObject *obj, jsval id);

extern JS_PUBLIC_API(JSBool)
JS_ConvertStub(JSContext *cx, JSObject *obj, JSType type, jsval *vp);

extern JS_PUBLIC_API(void)
JS_FinalizeStub(JSContext *cx, JSObject *obj);

struct JSConstDoubleSpec {
    jsdouble        dval;
    const char      *name;
    uint8           flags;
    uint8           spare[3];
};






struct JSPropertySpec {
    const char      *name;
    int8            tinyid;
    uint8           flags;
    JSPropertyOp    getter;
    JSPropertyOp    setter;
};

struct JSFunctionSpec {
    const char      *name;
    JSNative        call;
#ifdef MOZILLA_1_8_BRANCH
    uint8           nargs;
    uint8           flags;
    uint16          extra;
#else
    uint16          nargs;
    uint16          flags;
    uint32          extra;      



#endif
};

extern JS_PUBLIC_API(JSObject *)
JS_InitClass(JSContext *cx, JSObject *obj, JSObject *parent_proto,
             JSClass *clasp, JSNative constructor, uintN nargs,
             JSPropertySpec *ps, JSFunctionSpec *fs,
             JSPropertySpec *static_ps, JSFunctionSpec *static_fs);

#ifdef JS_THREADSAFE
extern JS_PUBLIC_API(JSClass *)
JS_GetClass(JSContext *cx, JSObject *obj);

#define JS_GET_CLASS(cx,obj) JS_GetClass(cx, obj)
#else
extern JS_PUBLIC_API(JSClass *)
JS_GetClass(JSObject *obj);

#define JS_GET_CLASS(cx,obj) JS_GetClass(obj)
#endif

extern JS_PUBLIC_API(JSBool)
JS_InstanceOf(JSContext *cx, JSObject *obj, JSClass *clasp, jsval *argv);

extern JS_PUBLIC_API(JSBool)
JS_HasInstance(JSContext *cx, JSObject *obj, jsval v, JSBool *bp);

extern JS_PUBLIC_API(void *)
JS_GetPrivate(JSContext *cx, JSObject *obj);

extern JS_PUBLIC_API(JSBool)
JS_SetPrivate(JSContext *cx, JSObject *obj, void *data);

extern JS_PUBLIC_API(void *)
JS_GetInstancePrivate(JSContext *cx, JSObject *obj, JSClass *clasp,
                      jsval *argv);

extern JS_PUBLIC_API(JSObject *)
JS_GetPrototype(JSContext *cx, JSObject *obj);

extern JS_PUBLIC_API(JSBool)
JS_SetPrototype(JSContext *cx, JSObject *obj, JSObject *proto);

extern JS_PUBLIC_API(JSObject *)
JS_GetParent(JSContext *cx, JSObject *obj);

extern JS_PUBLIC_API(JSBool)
JS_SetParent(JSContext *cx, JSObject *obj, JSObject *parent);

extern JS_PUBLIC_API(JSObject *)
JS_GetConstructor(JSContext *cx, JSObject *proto);






extern JS_PUBLIC_API(JSBool)
JS_GetObjectId(JSContext *cx, JSObject *obj, jsid *idp);

extern JS_PUBLIC_API(JSObject *)
JS_NewObject(JSContext *cx, JSClass *clasp, JSObject *proto, JSObject *parent);

extern JS_PUBLIC_API(JSBool)
JS_SealObject(JSContext *cx, JSObject *obj, JSBool deep);

extern JS_PUBLIC_API(JSObject *)
JS_ConstructObject(JSContext *cx, JSClass *clasp, JSObject *proto,
                   JSObject *parent);

extern JS_PUBLIC_API(JSObject *)
JS_ConstructObjectWithArguments(JSContext *cx, JSClass *clasp, JSObject *proto,
                                JSObject *parent, uintN argc, jsval *argv);

extern JS_PUBLIC_API(JSObject *)
JS_DefineObject(JSContext *cx, JSObject *obj, const char *name, JSClass *clasp,
                JSObject *proto, uintN attrs);

extern JS_PUBLIC_API(JSBool)
JS_DefineConstDoubles(JSContext *cx, JSObject *obj, JSConstDoubleSpec *cds);

extern JS_PUBLIC_API(JSBool)
JS_DefineProperties(JSContext *cx, JSObject *obj, JSPropertySpec *ps);

extern JS_PUBLIC_API(JSBool)
JS_DefineProperty(JSContext *cx, JSObject *obj, const char *name, jsval value,
                  JSPropertyOp getter, JSPropertyOp setter, uintN attrs);







extern JS_PUBLIC_API(JSBool)
JS_GetPropertyAttributes(JSContext *cx, JSObject *obj, const char *name,
                         uintN *attrsp, JSBool *foundp);






extern JS_PUBLIC_API(JSBool)
JS_GetPropertyAttrsGetterAndSetter(JSContext *cx, JSObject *obj,
                                   const char *name,
                                   uintN *attrsp, JSBool *foundp,
                                   JSPropertyOp *getterp,
                                   JSPropertyOp *setterp);







extern JS_PUBLIC_API(JSBool)
JS_SetPropertyAttributes(JSContext *cx, JSObject *obj, const char *name,
                         uintN attrs, JSBool *foundp);

extern JS_PUBLIC_API(JSBool)
JS_DefinePropertyWithTinyId(JSContext *cx, JSObject *obj, const char *name,
                            int8 tinyid, jsval value,
                            JSPropertyOp getter, JSPropertyOp setter,
                            uintN attrs);

extern JS_PUBLIC_API(JSBool)
JS_AliasProperty(JSContext *cx, JSObject *obj, const char *name,
                 const char *alias);

extern JS_PUBLIC_API(JSBool)
JS_HasProperty(JSContext *cx, JSObject *obj, const char *name, JSBool *foundp);

extern JS_PUBLIC_API(JSBool)
JS_LookupProperty(JSContext *cx, JSObject *obj, const char *name, jsval *vp);

extern JS_PUBLIC_API(JSBool)
JS_LookupPropertyWithFlags(JSContext *cx, JSObject *obj, const char *name,
                           uintN flags, jsval *vp);

extern JS_PUBLIC_API(JSBool)
JS_GetProperty(JSContext *cx, JSObject *obj, const char *name, jsval *vp);

extern JS_PUBLIC_API(JSBool)
JS_GetMethodById(JSContext *cx, JSObject *obj, jsid id, JSObject **objp,
                 jsval *vp);

extern JS_PUBLIC_API(JSBool)
JS_GetMethod(JSContext *cx, JSObject *obj, const char *name, JSObject **objp,
             jsval *vp);

extern JS_PUBLIC_API(JSBool)
JS_SetProperty(JSContext *cx, JSObject *obj, const char *name, jsval *vp);

extern JS_PUBLIC_API(JSBool)
JS_DeleteProperty(JSContext *cx, JSObject *obj, const char *name);

extern JS_PUBLIC_API(JSBool)
JS_DeleteProperty2(JSContext *cx, JSObject *obj, const char *name,
                   jsval *rval);

extern JS_PUBLIC_API(JSBool)
JS_DefineUCProperty(JSContext *cx, JSObject *obj,
                    const jschar *name, size_t namelen, jsval value,
                    JSPropertyOp getter, JSPropertyOp setter,
                    uintN attrs);







extern JS_PUBLIC_API(JSBool)
JS_GetUCPropertyAttributes(JSContext *cx, JSObject *obj,
                           const jschar *name, size_t namelen,
                           uintN *attrsp, JSBool *foundp);






extern JS_PUBLIC_API(JSBool)
JS_GetUCPropertyAttrsGetterAndSetter(JSContext *cx, JSObject *obj,
                                     const jschar *name, size_t namelen,
                                     uintN *attrsp, JSBool *foundp,
                                     JSPropertyOp *getterp,
                                     JSPropertyOp *setterp);







extern JS_PUBLIC_API(JSBool)
JS_SetUCPropertyAttributes(JSContext *cx, JSObject *obj,
                           const jschar *name, size_t namelen,
                           uintN attrs, JSBool *foundp);


extern JS_PUBLIC_API(JSBool)
JS_DefineUCPropertyWithTinyId(JSContext *cx, JSObject *obj,
                              const jschar *name, size_t namelen,
                              int8 tinyid, jsval value,
                              JSPropertyOp getter, JSPropertyOp setter,
                              uintN attrs);

extern JS_PUBLIC_API(JSBool)
JS_HasUCProperty(JSContext *cx, JSObject *obj,
                 const jschar *name, size_t namelen,
                 JSBool *vp);

extern JS_PUBLIC_API(JSBool)
JS_LookupUCProperty(JSContext *cx, JSObject *obj,
                    const jschar *name, size_t namelen,
                    jsval *vp);

extern JS_PUBLIC_API(JSBool)
JS_GetUCProperty(JSContext *cx, JSObject *obj,
                 const jschar *name, size_t namelen,
                 jsval *vp);

extern JS_PUBLIC_API(JSBool)
JS_SetUCProperty(JSContext *cx, JSObject *obj,
                 const jschar *name, size_t namelen,
                 jsval *vp);

extern JS_PUBLIC_API(JSBool)
JS_DeleteUCProperty2(JSContext *cx, JSObject *obj,
                     const jschar *name, size_t namelen,
                     jsval *rval);

extern JS_PUBLIC_API(JSObject *)
JS_NewArrayObject(JSContext *cx, jsint length, jsval *vector);

extern JS_PUBLIC_API(JSBool)
JS_IsArrayObject(JSContext *cx, JSObject *obj);

extern JS_PUBLIC_API(JSBool)
JS_GetArrayLength(JSContext *cx, JSObject *obj, jsuint *lengthp);

extern JS_PUBLIC_API(JSBool)
JS_SetArrayLength(JSContext *cx, JSObject *obj, jsuint length);

extern JS_PUBLIC_API(JSBool)
JS_HasArrayLength(JSContext *cx, JSObject *obj, jsuint *lengthp);

extern JS_PUBLIC_API(JSBool)
JS_DefineElement(JSContext *cx, JSObject *obj, jsint index, jsval value,
                 JSPropertyOp getter, JSPropertyOp setter, uintN attrs);

extern JS_PUBLIC_API(JSBool)
JS_AliasElement(JSContext *cx, JSObject *obj, const char *name, jsint alias);

extern JS_PUBLIC_API(JSBool)
JS_HasElement(JSContext *cx, JSObject *obj, jsint index, JSBool *foundp);

extern JS_PUBLIC_API(JSBool)
JS_LookupElement(JSContext *cx, JSObject *obj, jsint index, jsval *vp);

extern JS_PUBLIC_API(JSBool)
JS_GetElement(JSContext *cx, JSObject *obj, jsint index, jsval *vp);

extern JS_PUBLIC_API(JSBool)
JS_SetElement(JSContext *cx, JSObject *obj, jsint index, jsval *vp);

extern JS_PUBLIC_API(JSBool)
JS_DeleteElement(JSContext *cx, JSObject *obj, jsint index);

extern JS_PUBLIC_API(JSBool)
JS_DeleteElement2(JSContext *cx, JSObject *obj, jsint index, jsval *rval);

extern JS_PUBLIC_API(void)
JS_ClearScope(JSContext *cx, JSObject *obj);

extern JS_PUBLIC_API(JSIdArray *)
JS_Enumerate(JSContext *cx, JSObject *obj);






extern JS_PUBLIC_API(JSObject *)
JS_NewPropertyIterator(JSContext *cx, JSObject *obj);






extern JS_PUBLIC_API(JSBool)
JS_NextProperty(JSContext *cx, JSObject *iterobj, jsid *idp);

extern JS_PUBLIC_API(JSBool)
JS_CheckAccess(JSContext *cx, JSObject *obj, jsid id, JSAccessMode mode,
               jsval *vp, uintN *attrsp);

extern JS_PUBLIC_API(JSCheckAccessOp)
JS_SetCheckObjectAccessCallback(JSRuntime *rt, JSCheckAccessOp acb);

extern JS_PUBLIC_API(JSBool)
JS_GetReservedSlot(JSContext *cx, JSObject *obj, uint32 index, jsval *vp);

extern JS_PUBLIC_API(JSBool)
JS_SetReservedSlot(JSContext *cx, JSObject *obj, uint32 index, jsval v);






struct JSPrincipals {
    char *codebase;

    
    void * (* JS_DLL_CALLBACK getPrincipalArray)(JSContext *cx, JSPrincipals *);
    JSBool (* JS_DLL_CALLBACK globalPrivilegesEnabled)(JSContext *cx, JSPrincipals *);

    
    jsrefcount refcount;

    void   (* JS_DLL_CALLBACK destroy)(JSContext *cx, JSPrincipals *);
    JSBool (* JS_DLL_CALLBACK subsume)(JSPrincipals *, JSPrincipals *);
};

#ifdef JS_THREADSAFE
#define JSPRINCIPALS_HOLD(cx, principals)   JS_HoldPrincipals(cx,principals)
#define JSPRINCIPALS_DROP(cx, principals)   JS_DropPrincipals(cx,principals)

extern JS_PUBLIC_API(jsrefcount)
JS_HoldPrincipals(JSContext *cx, JSPrincipals *principals);

extern JS_PUBLIC_API(jsrefcount)
JS_DropPrincipals(JSContext *cx, JSPrincipals *principals);

#else
#define JSPRINCIPALS_HOLD(cx, principals)   (++(principals)->refcount)
#define JSPRINCIPALS_DROP(cx, principals)                                     \
    ((--(principals)->refcount == 0)                                          \
     ? ((*(principals)->destroy)((cx), (principals)), 0)                      \
     : (principals)->refcount)
#endif

extern JS_PUBLIC_API(JSPrincipalsTranscoder)
JS_SetPrincipalsTranscoder(JSRuntime *rt, JSPrincipalsTranscoder px);

extern JS_PUBLIC_API(JSObjectPrincipalsFinder)
JS_SetObjectPrincipalsFinder(JSRuntime *rt, JSObjectPrincipalsFinder fop);






extern JS_PUBLIC_API(JSFunction *)
JS_NewFunction(JSContext *cx, JSNative call, uintN nargs, uintN flags,
               JSObject *parent, const char *name);

extern JS_PUBLIC_API(JSObject *)
JS_GetFunctionObject(JSFunction *fun);





extern JS_PUBLIC_API(const char *)
JS_GetFunctionName(JSFunction *fun);











extern JS_PUBLIC_API(JSString *)
JS_GetFunctionId(JSFunction *fun);




extern JS_PUBLIC_API(uintN)
JS_GetFunctionFlags(JSFunction *fun);




extern JS_PUBLIC_API(uint16)
JS_GetFunctionArity(JSFunction *fun);







extern JS_PUBLIC_API(JSBool)
JS_ObjectIsFunction(JSContext *cx, JSObject *obj);

extern JS_PUBLIC_API(JSBool)
JS_DefineFunctions(JSContext *cx, JSObject *obj, JSFunctionSpec *fs);

extern JS_PUBLIC_API(JSFunction *)
JS_DefineFunction(JSContext *cx, JSObject *obj, const char *name, JSNative call,
                  uintN nargs, uintN attrs);

extern JS_PUBLIC_API(JSFunction *)
JS_DefineUCFunction(JSContext *cx, JSObject *obj,
                    const jschar *name, size_t namelen, JSNative call,
                    uintN nargs, uintN attrs);

extern JS_PUBLIC_API(JSObject *)
JS_CloneFunctionObject(JSContext *cx, JSObject *funobj, JSObject *parent);








extern JS_PUBLIC_API(JSBool)
JS_BufferIsCompilableUnit(JSContext *cx, JSObject *obj,
                          const char *bytes, size_t length);








extern JS_PUBLIC_API(JSScript *)
JS_CompileScript(JSContext *cx, JSObject *obj,
                 const char *bytes, size_t length,
                 const char *filename, uintN lineno);

extern JS_PUBLIC_API(JSScript *)
JS_CompileScriptForPrincipals(JSContext *cx, JSObject *obj,
                              JSPrincipals *principals,
                              const char *bytes, size_t length,
                              const char *filename, uintN lineno);

extern JS_PUBLIC_API(JSScript *)
JS_CompileUCScript(JSContext *cx, JSObject *obj,
                   const jschar *chars, size_t length,
                   const char *filename, uintN lineno);

extern JS_PUBLIC_API(JSScript *)
JS_CompileUCScriptForPrincipals(JSContext *cx, JSObject *obj,
                                JSPrincipals *principals,
                                const jschar *chars, size_t length,
                                const char *filename, uintN lineno);

extern JS_PUBLIC_API(JSScript *)
JS_CompileFile(JSContext *cx, JSObject *obj, const char *filename);

extern JS_PUBLIC_API(JSScript *)
JS_CompileFileHandle(JSContext *cx, JSObject *obj, const char *filename,
                     FILE *fh);

extern JS_PUBLIC_API(JSScript *)
JS_CompileFileHandleForPrincipals(JSContext *cx, JSObject *obj,
                                  const char *filename, FILE *fh,
                                  JSPrincipals *principals);

















extern JS_PUBLIC_API(JSObject *)
JS_NewScriptObject(JSContext *cx, JSScript *script);





extern JS_PUBLIC_API(JSObject *)
JS_GetScriptObject(JSScript *script);

extern JS_PUBLIC_API(void)
JS_DestroyScript(JSContext *cx, JSScript *script);

extern JS_PUBLIC_API(JSFunction *)
JS_CompileFunction(JSContext *cx, JSObject *obj, const char *name,
                   uintN nargs, const char **argnames,
                   const char *bytes, size_t length,
                   const char *filename, uintN lineno);

extern JS_PUBLIC_API(JSFunction *)
JS_CompileFunctionForPrincipals(JSContext *cx, JSObject *obj,
                                JSPrincipals *principals, const char *name,
                                uintN nargs, const char **argnames,
                                const char *bytes, size_t length,
                                const char *filename, uintN lineno);

extern JS_PUBLIC_API(JSFunction *)
JS_CompileUCFunction(JSContext *cx, JSObject *obj, const char *name,
                     uintN nargs, const char **argnames,
                     const jschar *chars, size_t length,
                     const char *filename, uintN lineno);

extern JS_PUBLIC_API(JSFunction *)
JS_CompileUCFunctionForPrincipals(JSContext *cx, JSObject *obj,
                                  JSPrincipals *principals, const char *name,
                                  uintN nargs, const char **argnames,
                                  const jschar *chars, size_t length,
                                  const char *filename, uintN lineno);

extern JS_PUBLIC_API(JSString *)
JS_DecompileScript(JSContext *cx, JSScript *script, const char *name,
                   uintN indent);





#define JS_DONT_PRETTY_PRINT    ((uintN)0x8000)

extern JS_PUBLIC_API(JSString *)
JS_DecompileFunction(JSContext *cx, JSFunction *fun, uintN indent);

extern JS_PUBLIC_API(JSString *)
JS_DecompileFunctionBody(JSContext *cx, JSFunction *fun, uintN indent);




































extern JS_PUBLIC_API(JSBool)
JS_ExecuteScript(JSContext *cx, JSObject *obj, JSScript *script, jsval *rval);





typedef enum JSExecPart { JSEXEC_PROLOG, JSEXEC_MAIN } JSExecPart;

extern JS_PUBLIC_API(JSBool)
JS_ExecuteScriptPart(JSContext *cx, JSObject *obj, JSScript *script,
                     JSExecPart part, jsval *rval);

extern JS_PUBLIC_API(JSBool)
JS_EvaluateScript(JSContext *cx, JSObject *obj,
                  const char *bytes, uintN length,
                  const char *filename, uintN lineno,
                  jsval *rval);

extern JS_PUBLIC_API(JSBool)
JS_EvaluateScriptForPrincipals(JSContext *cx, JSObject *obj,
                               JSPrincipals *principals,
                               const char *bytes, uintN length,
                               const char *filename, uintN lineno,
                               jsval *rval);

extern JS_PUBLIC_API(JSBool)
JS_EvaluateUCScript(JSContext *cx, JSObject *obj,
                    const jschar *chars, uintN length,
                    const char *filename, uintN lineno,
                    jsval *rval);

extern JS_PUBLIC_API(JSBool)
JS_EvaluateUCScriptForPrincipals(JSContext *cx, JSObject *obj,
                                 JSPrincipals *principals,
                                 const jschar *chars, uintN length,
                                 const char *filename, uintN lineno,
                                 jsval *rval);

extern JS_PUBLIC_API(JSBool)
JS_CallFunction(JSContext *cx, JSObject *obj, JSFunction *fun, uintN argc,
                jsval *argv, jsval *rval);

extern JS_PUBLIC_API(JSBool)
JS_CallFunctionName(JSContext *cx, JSObject *obj, const char *name, uintN argc,
                    jsval *argv, jsval *rval);

extern JS_PUBLIC_API(JSBool)
JS_CallFunctionValue(JSContext *cx, JSObject *obj, jsval fval, uintN argc,
                     jsval *argv, jsval *rval);

extern JS_PUBLIC_API(JSBranchCallback)
JS_SetBranchCallback(JSContext *cx, JSBranchCallback cb);

extern JS_PUBLIC_API(JSBool)
JS_IsRunning(JSContext *cx);

extern JS_PUBLIC_API(JSBool)
JS_IsConstructing(JSContext *cx);






extern JS_FRIEND_API(JSBool)
JS_IsAssigning(JSContext *cx);









extern JS_PUBLIC_API(void)
JS_SetCallReturnValue2(JSContext *cx, jsval v);














extern JS_PUBLIC_API(JSStackFrame *)
JS_SaveFrameChain(JSContext *cx);

extern JS_PUBLIC_API(void)
JS_RestoreFrameChain(JSContext *cx, JSStackFrame *fp);












extern JS_PUBLIC_API(JSString *)
JS_NewString(JSContext *cx, char *bytes, size_t length);

extern JS_PUBLIC_API(JSString *)
JS_NewStringCopyN(JSContext *cx, const char *s, size_t n);

extern JS_PUBLIC_API(JSString *)
JS_NewStringCopyZ(JSContext *cx, const char *s);

extern JS_PUBLIC_API(JSString *)
JS_InternString(JSContext *cx, const char *s);

extern JS_PUBLIC_API(JSString *)
JS_NewUCString(JSContext *cx, jschar *chars, size_t length);

extern JS_PUBLIC_API(JSString *)
JS_NewUCStringCopyN(JSContext *cx, const jschar *s, size_t n);

extern JS_PUBLIC_API(JSString *)
JS_NewUCStringCopyZ(JSContext *cx, const jschar *s);

extern JS_PUBLIC_API(JSString *)
JS_InternUCStringN(JSContext *cx, const jschar *s, size_t length);

extern JS_PUBLIC_API(JSString *)
JS_InternUCString(JSContext *cx, const jschar *s);

extern JS_PUBLIC_API(char *)
JS_GetStringBytes(JSString *str);

extern JS_PUBLIC_API(jschar *)
JS_GetStringChars(JSString *str);

extern JS_PUBLIC_API(size_t)
JS_GetStringLength(JSString *str);

extern JS_PUBLIC_API(intN)
JS_CompareStrings(JSString *str1, JSString *str2);




















extern JS_PUBLIC_API(JSString *)
JS_NewGrowableString(JSContext *cx, jschar *chars, size_t length);






extern JS_PUBLIC_API(JSString *)
JS_NewDependentString(JSContext *cx, JSString *str, size_t start,
                      size_t length);








extern JS_PUBLIC_API(JSString *)
JS_ConcatStrings(JSContext *cx, JSString *left, JSString *right);





extern JS_PUBLIC_API(const jschar *)
JS_UndependString(JSContext *cx, JSString *str);





extern JS_PUBLIC_API(JSBool)
JS_MakeStringImmutable(JSContext *cx, JSString *str);






JS_PUBLIC_API(JSBool)
JS_CStringsAreUTF8();























JS_PUBLIC_API(JSBool)
JS_EncodeCharacters(JSContext *cx, const jschar *src, size_t srclen, char *dst,
                    size_t *dstlenp);

JS_PUBLIC_API(JSBool)
JS_DecodeBytes(JSContext *cx, const char *src, size_t srclen, jschar *dst,
               size_t *dstlenp);






struct JSLocaleCallbacks {
    JSLocaleToUpperCase     localeToUpperCase;
    JSLocaleToLowerCase     localeToLowerCase;
    JSLocaleCompare         localeCompare;
    JSLocaleToUnicode       localeToUnicode;
    JSErrorCallback         localeGetErrorMessage;
};





extern JS_PUBLIC_API(void)
JS_SetLocaleCallbacks(JSContext *cx, JSLocaleCallbacks *callbacks);





extern JS_PUBLIC_API(JSLocaleCallbacks *)
JS_GetLocaleCallbacks(JSContext *cx);













extern JS_PUBLIC_API(void)
JS_ReportError(JSContext *cx, const char *format, ...);




extern JS_PUBLIC_API(void)
JS_ReportErrorNumber(JSContext *cx, JSErrorCallback errorCallback,
                     void *userRef, const uintN errorNumber, ...);




extern JS_PUBLIC_API(void)
JS_ReportErrorNumberUC(JSContext *cx, JSErrorCallback errorCallback,
                     void *userRef, const uintN errorNumber, ...);







extern JS_PUBLIC_API(JSBool)
JS_ReportWarning(JSContext *cx, const char *format, ...);

extern JS_PUBLIC_API(JSBool)
JS_ReportErrorFlagsAndNumber(JSContext *cx, uintN flags,
                             JSErrorCallback errorCallback, void *userRef,
                             const uintN errorNumber, ...);

extern JS_PUBLIC_API(JSBool)
JS_ReportErrorFlagsAndNumberUC(JSContext *cx, uintN flags,
                               JSErrorCallback errorCallback, void *userRef,
                               const uintN errorNumber, ...);




extern JS_PUBLIC_API(void)
JS_ReportOutOfMemory(JSContext *cx);

struct JSErrorReport {
    const char      *filename;      
    uintN           lineno;         
    const char      *linebuf;       
    const char      *tokenptr;      
    const jschar    *uclinebuf;     
    const jschar    *uctokenptr;    
    uintN           flags;          
    uintN           errorNumber;    
    const jschar    *ucmessage;     
    const jschar    **messageArgs;  
};




#define JSREPORT_ERROR      0x0     /* pseudo-flag for default case */
#define JSREPORT_WARNING    0x1     /* reported via JS_ReportWarning */
#define JSREPORT_EXCEPTION  0x2     /* exception was thrown */
#define JSREPORT_STRICT     0x4     /* error or warning due to strict option */








#define JSREPORT_IS_WARNING(flags)      (((flags) & JSREPORT_WARNING) != 0)
#define JSREPORT_IS_EXCEPTION(flags)    (((flags) & JSREPORT_EXCEPTION) != 0)
#define JSREPORT_IS_STRICT(flags)       (((flags) & JSREPORT_STRICT) != 0)

extern JS_PUBLIC_API(JSErrorReporter)
JS_SetErrorReporter(JSContext *cx, JSErrorReporter er);






#define JSREG_FOLD      0x01    /* fold uppercase to lowercase */
#define JSREG_GLOB      0x02    /* global exec, creates array of matches */
#define JSREG_MULTILINE 0x04    /* treat ^ and $ as begin and end of line */
#define JSREG_STICKY    0x08    /* only match starting at lastIndex */

extern JS_PUBLIC_API(JSObject *)
JS_NewRegExpObject(JSContext *cx, char *bytes, size_t length, uintN flags);

extern JS_PUBLIC_API(JSObject *)
JS_NewUCRegExpObject(JSContext *cx, jschar *chars, size_t length, uintN flags);

extern JS_PUBLIC_API(void)
JS_SetRegExpInput(JSContext *cx, JSString *input, JSBool multiline);

extern JS_PUBLIC_API(void)
JS_ClearRegExpStatics(JSContext *cx);

extern JS_PUBLIC_API(void)
JS_ClearRegExpRoots(JSContext *cx);





extern JS_PUBLIC_API(JSBool)
JS_IsExceptionPending(JSContext *cx);

extern JS_PUBLIC_API(JSBool)
JS_GetPendingException(JSContext *cx, jsval *vp);

extern JS_PUBLIC_API(void)
JS_SetPendingException(JSContext *cx, jsval v);

extern JS_PUBLIC_API(void)
JS_ClearPendingException(JSContext *cx);

extern JS_PUBLIC_API(JSBool)
JS_ReportPendingException(JSContext *cx);












extern JS_PUBLIC_API(JSExceptionState *)
JS_SaveExceptionState(JSContext *cx);

extern JS_PUBLIC_API(void)
JS_RestoreExceptionState(JSContext *cx, JSExceptionState *state);

extern JS_PUBLIC_API(void)
JS_DropExceptionState(JSContext *cx, JSExceptionState *state);








extern JS_PUBLIC_API(JSErrorReport *)
JS_ErrorFromException(JSContext *cx, jsval v);





extern JS_PUBLIC_API(JSBool)
JS_ThrowReportedError(JSContext *cx, const char *message,
                      JSErrorReport *reportp);

#ifdef JS_THREADSAFE










extern JS_PUBLIC_API(jsword)
JS_GetContextThread(JSContext *cx);

extern JS_PUBLIC_API(jsword)
JS_SetContextThread(JSContext *cx);

extern JS_PUBLIC_API(jsword)
JS_ClearContextThread(JSContext *cx);

#endif 



JS_END_EXTERN_C

#endif 
