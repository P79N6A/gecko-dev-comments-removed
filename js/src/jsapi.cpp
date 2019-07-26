









#include "jsapi.h"

#include "mozilla/FloatingPoint.h"
#include "mozilla/PodOperations.h"

#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>

#include "jsarray.h"
#include "jsatom.h"
#include "jsbool.h"
#include "jscntxt.h"
#include "jsdate.h"
#include "jsexn.h"
#include "jsfun.h"
#include "jsgc.h"
#include "jsiter.h"
#include "jslock.h"
#include "jsmath.h"
#include "jsnum.h"
#include "jsobj.h"
#include "json.h"
#include "jsprf.h"
#include "jsproxy.h"
#include "jsscript.h"
#include "jsstr.h"
#include "jstypes.h"
#include "jsutil.h"
#include "jswatchpoint.h"
#include "jsweakmap.h"
#ifdef JS_THREADSAFE
#include "jsworkers.h"
#endif
#include "jswrapper.h"
#include "prmjtime.h"

#if ENABLE_YARR_JIT
#include "assembler/jit/ExecutableAllocator.h"
#endif
#include "builtin/Eval.h"
#include "builtin/Intl.h"
#include "builtin/MapObject.h"
#include "builtin/ParallelArray.h"
#include "builtin/RegExp.h"
#include "builtin/TypedObject.h"
#include "frontend/BytecodeCompiler.h"
#include "frontend/FullParseHandler.h"  
#include "frontend/Parser.h" 
#include "gc/Marking.h"
#include "jit/AsmJSLink.h"
#include "js/CharacterEncoding.h"
#include "js/StructuredClone.h"
#if ENABLE_INTL_API
#include "unicode/uclean.h"
#include "unicode/utypes.h"
#endif 
#include "vm/DateObject.h"
#include "vm/Debugger.h"
#include "vm/ErrorObject.h"
#include "vm/Interpreter.h"
#include "vm/NumericConversions.h"
#include "vm/RegExpStatics.h"
#include "vm/Runtime.h"
#include "vm/Shape.h"
#include "vm/StopIterationObject.h"
#include "vm/StringBuffer.h"
#include "vm/TypedArrayObject.h"
#include "vm/WeakMapObject.h"
#include "vm/WrapperObject.h"
#include "vm/Xdr.h"
#include "yarr/BumpPointerAllocator.h"

#include "jsatominlines.h"
#include "jsfuninlines.h"
#include "jsinferinlines.h"
#include "jsscriptinlines.h"

#include "vm/Interpreter-inl.h"
#include "vm/ObjectImpl-inl.h"
#include "vm/String-inl.h"

using namespace js;
using namespace js::gc;
using namespace js::types;

using mozilla::Maybe;
using mozilla::PodCopy;
using mozilla::PodZero;

using js::frontend::Parser;

#ifdef HAVE_VA_LIST_AS_ARRAY
#define JS_ADDRESSOF_VA_LIST(ap) ((va_list *)(ap))
#else
#define JS_ADDRESSOF_VA_LIST(ap) (&(ap))
#endif


JS_STATIC_ASSERT((jschar)-1 > 0);
JS_STATIC_ASSERT(sizeof(jschar) == 2);

JS_PUBLIC_API(int64_t)
JS_Now()
{
    return PRMJ_Now();
}

JS_PUBLIC_API(jsval)
JS_GetNaNValue(JSContext *cx)
{
    return cx->runtime()->NaNValue;
}

JS_PUBLIC_API(jsval)
JS_GetNegativeInfinityValue(JSContext *cx)
{
    return cx->runtime()->negativeInfinityValue;
}

JS_PUBLIC_API(jsval)
JS_GetPositiveInfinityValue(JSContext *cx)
{
    return cx->runtime()->positiveInfinityValue;
}

JS_PUBLIC_API(jsval)
JS_GetEmptyStringValue(JSContext *cx)
{
    return STRING_TO_JSVAL(cx->runtime()->emptyString);
}

JS_PUBLIC_API(JSString *)
JS_GetEmptyString(JSRuntime *rt)
{
    JS_ASSERT(rt->hasContexts());
    return rt->emptyString;
}

namespace js {

void
AssertHeapIsIdle(JSRuntime *rt)
{
    JS_ASSERT(rt->heapState == js::Idle);
}

void
AssertHeapIsIdle(JSContext *cx)
{
    AssertHeapIsIdle(cx->runtime());
}

}

static void
AssertHeapIsIdleOrIterating(JSRuntime *rt)
{
    JS_ASSERT(!rt->isHeapCollecting());
}

static void
AssertHeapIsIdleOrIterating(JSContext *cx)
{
    AssertHeapIsIdleOrIterating(cx->runtime());
}

static void
AssertHeapIsIdleOrStringIsFlat(JSContext *cx, JSString *str)
{
    



    JS_ASSERT_IF(cx->runtime()->isHeapBusy(), str->isFlat());
}

JS_PUBLIC_API(bool)
JS_ConvertArguments(JSContext *cx, unsigned argc, jsval *argv, const char *format, ...)
{
    va_list ap;
    bool ok;

    AssertHeapIsIdle(cx);

    va_start(ap, format);
    ok = JS_ConvertArgumentsVA(cx, argc, argv, format, ap);
    va_end(ap);
    return ok;
}

JS_PUBLIC_API(bool)
JS_ConvertArgumentsVA(JSContext *cx, unsigned argc, jsval *argv, const char *format, va_list ap)
{
    jsval *sp;
    bool required;
    char c;
    double d;
    JSString *str;
    RootedObject obj(cx);
    RootedValue val(cx);

    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, JSValueArray(argv - 2, argc + 2));
    sp = argv;
    required = true;
    while ((c = *format++) != '\0') {
        if (isspace(c))
            continue;
        if (c == '/') {
            required = false;
            continue;
        }
        if (sp == argv + argc) {
            if (required) {
                if (JSFunction *fun = ReportIfNotFunction(cx, argv[-2])) {
                    char numBuf[12];
                    JS_snprintf(numBuf, sizeof numBuf, "%u", argc);
                    JSAutoByteString funNameBytes;
                    if (const char *name = GetFunctionNameBytes(cx, fun, &funNameBytes)) {
                        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                                             JSMSG_MORE_ARGS_NEEDED,
                                             name, numBuf, (argc == 1) ? "" : "s");
                    }
                }
                return false;
            }
            break;
        }
        RootedValue arg(cx, *sp);
        switch (c) {
          case 'b':
            *va_arg(ap, bool *) = ToBoolean(*sp);
            break;
          case 'c':
            if (!JS_ValueToUint16(cx, *sp, va_arg(ap, uint16_t *)))
                return false;
            break;
          case 'i':
            if (!ToInt32(cx, arg, va_arg(ap, int32_t *)))
                return false;
            break;
          case 'u':
            if (!JS_ValueToECMAUint32(cx, *sp, va_arg(ap, uint32_t *)))
                return false;
            break;
          case 'j':
            if (!JS_ValueToInt32(cx, *sp, va_arg(ap, int32_t *)))
                return false;
            break;
          case 'd':
            if (!JS_ValueToNumber(cx, *sp, va_arg(ap, double *)))
                return false;
            break;
          case 'I':
            if (!JS_ValueToNumber(cx, *sp, &d))
                return false;
            *va_arg(ap, double *) = ToInteger(d);
            break;
          case 'S':
          case 'W':
            val = *sp;
            str = ToString<CanGC>(cx, val);
            if (!str)
                return false;
            *sp = STRING_TO_JSVAL(str);
            if (c == 'W') {
                JSStableString *stable = str->ensureStable(cx);
                if (!stable)
                    return false;
                *va_arg(ap, const jschar **) = stable->chars().get();
            } else {
                *va_arg(ap, JSString **) = str;
            }
            break;
          case 'o':
            if (sp->isNullOrUndefined()) {
                obj = nullptr;
            } else {
                RootedValue v(cx, *sp);
                obj = ToObject(cx, v);
                if (!obj)
                    return false;
            }
            *sp = ObjectOrNullValue(obj);
            *va_arg(ap, JSObject **) = obj;
            break;
          case 'f':
            obj = ReportIfNotFunction(cx, *sp);
            if (!obj)
                return false;
            *sp = OBJECT_TO_JSVAL(obj);
            *va_arg(ap, JSFunction **) = &obj->as<JSFunction>();
            break;
          case 'v':
            *va_arg(ap, jsval *) = *sp;
            break;
          case '*':
            break;
          default:
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_BAD_CHAR, format);
            return false;
        }
        sp++;
    }
    return true;
}

JS_PUBLIC_API(bool)
JS_ConvertValue(JSContext *cx, HandleValue value, JSType type, MutableHandleValue vp)
{
    bool ok;
    RootedObject obj(cx);
    JSString *str;
    double d;

    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, value);
    switch (type) {
      case JSTYPE_VOID:
        vp.setUndefined();
        ok = true;
        break;
      case JSTYPE_OBJECT:
        if (value.isNullOrUndefined()) {
            obj.set(nullptr);
        } else {
            obj = ToObject(cx, value);
            if (!obj)
                return false;
        }
        ok = true;
        break;
      case JSTYPE_FUNCTION:
        vp.set(value);
        obj = ReportIfNotFunction(cx, vp);
        ok = (obj != nullptr);
        break;
      case JSTYPE_STRING:
        str = ToString<CanGC>(cx, value);
        ok = (str != nullptr);
        if (ok)
            vp.setString(str);
        break;
      case JSTYPE_NUMBER:
        ok = JS_ValueToNumber(cx, value, &d);
        if (ok)
            vp.setDouble(d);
        break;
      case JSTYPE_BOOLEAN:
        vp.setBoolean(ToBoolean(value));
        return true;
      default: {
        char numBuf[12];
        JS_snprintf(numBuf, sizeof numBuf, "%d", (int)type);
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_BAD_TYPE, numBuf);
        ok = false;
        break;
      }
    }
    return ok;
}

JS_PUBLIC_API(bool)
JS_ValueToObject(JSContext *cx, HandleValue value, MutableHandleObject objp)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, value);
    if (value.isNullOrUndefined()) {
        objp.set(nullptr);
        return true;
    }
    JSObject *obj = ToObject(cx, value);
    if (!obj)
        return false;
    objp.set(obj);
    return true;
}

JS_PUBLIC_API(JSFunction *)
JS_ValueToFunction(JSContext *cx, HandleValue value)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, value);
    return ReportIfNotFunction(cx, value);
}

JS_PUBLIC_API(JSFunction *)
JS_ValueToConstructor(JSContext *cx, HandleValue value)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, value);
    return ReportIfNotFunction(cx, value);
}

JS_PUBLIC_API(JSString *)
JS_ValueToString(JSContext *cx, jsval valueArg)
{
    RootedValue value(cx, valueArg);
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, value);
    return ToString<CanGC>(cx, value);
}

JS_PUBLIC_API(JSString *)
JS_ValueToSource(JSContext *cx, jsval valueArg)
{
    RootedValue value(cx, valueArg);
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, value);
    return ValueToSource(cx, value);
}

JS_PUBLIC_API(bool)
JS_ValueToNumber(JSContext *cx, jsval valueArg, double *dp)
{
    RootedValue value(cx, valueArg);
    return JS::ToNumber(cx, value, dp);
}

JS_PUBLIC_API(bool)
JS_DoubleIsInt32(double d, int32_t *ip)
{
    return mozilla::DoubleIsInt32(d, ip);
}

JS_PUBLIC_API(int32_t)
JS_DoubleToInt32(double d)
{
    return ToInt32(d);
}

JS_PUBLIC_API(uint32_t)
JS_DoubleToUint32(double d)
{
    return ToUint32(d);
}


JS_PUBLIC_API(bool)
JS_ValueToECMAUint32(JSContext *cx, jsval valueArg, uint32_t *ip)
{
    RootedValue value(cx, valueArg);
    return JS::ToUint32(cx, value, ip);
}

JS_PUBLIC_API(bool)
JS_ValueToInt64(JSContext *cx, jsval valueArg, int64_t *ip)
{
    RootedValue value(cx, valueArg);
    return JS::ToInt64(cx, value, ip);
}

JS_PUBLIC_API(bool)
JS_ValueToUint64(JSContext *cx, jsval valueArg, uint64_t *ip)
{
    RootedValue value(cx, valueArg);
    return JS::ToUint64(cx, value, ip);
}

JS_PUBLIC_API(bool)
JS_ValueToInt32(JSContext *cx, jsval vArg, int32_t *ip)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);

    RootedValue v(cx, vArg);
    assertSameCompartment(cx, v);

    if (v.isInt32()) {
        *ip = v.toInt32();
        return true;
    }

    double d;
    if (v.isDouble()) {
        d = v.toDouble();
    } else if (!ToNumberSlow(cx, v, &d)) {
        return false;
    }

    if (mozilla::IsNaN(d) || d <= -2147483649.0 || 2147483648.0 <= d) {
        js_ReportValueError(cx, JSMSG_CANT_CONVERT,
                            JSDVG_SEARCH_STACK, v, NullPtr());
        return false;
    }

    *ip = (int32_t) floor(d + 0.5);  
    return true;
}

JS_PUBLIC_API(bool)
JS_ValueToUint16(JSContext *cx, jsval valueArg, uint16_t *ip)
{
    RootedValue value(cx, valueArg);
    return ToUint16(cx, value, ip);
}

JS_PUBLIC_API(bool)
JS_ValueToBoolean(JSContext *cx, jsval value, bool *bp)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, value);
    *bp = ToBoolean(value);
    return true;
}

JS_PUBLIC_API(JSType)
JS_TypeOfValue(JSContext *cx, jsval valueArg)
{
    RootedValue value(cx, valueArg);
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, value);
    return TypeOfValue(value);
}

JS_PUBLIC_API(const char *)
JS_GetTypeName(JSContext *cx, JSType type)
{
    if ((unsigned)type >= (unsigned)JSTYPE_LIMIT)
        return nullptr;
    return TypeStrings[type];
}

JS_PUBLIC_API(bool)
JS_StrictlyEqual(JSContext *cx, jsval value1, jsval value2, bool *equal)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, value1, value2);
    bool eq;
    if (!StrictlyEqual(cx, value1, value2, &eq))
        return false;
    *equal = eq;
    return true;
}

JS_PUBLIC_API(bool)
JS_LooselyEqual(JSContext *cx, jsval value1Arg, jsval value2Arg, bool *equal)
{
    RootedValue value1(cx, value1Arg);
    RootedValue value2(cx, value2Arg);
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, value1, value2);
    bool eq;
    if (!LooselyEqual(cx, value1, value2, &eq))
        return false;
    *equal = eq;
    return true;
}

JS_PUBLIC_API(bool)
JS_SameValue(JSContext *cx, jsval value1, jsval value2, bool *same)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, value1, value2);
    bool s;
    if (!SameValue(cx, value1, value2, &s))
        return false;
    *same = s;
    return true;
}

JS_PUBLIC_API(bool)
JS_IsBuiltinEvalFunction(JSFunction *fun)
{
    return IsAnyBuiltinEval(fun);
}

JS_PUBLIC_API(bool)
JS_IsBuiltinFunctionConstructor(JSFunction *fun)
{
    return fun->isBuiltinFunctionConstructor();
}



















enum InitState { Uninitialized, Running, ShutDown };
static InitState jsInitState = Uninitialized;

#ifdef DEBUG
static void
CheckMessageNumbering()
{
    
    
    
    int errorNumber = 0;
# define MSG_DEF(name, number, count, exception, format)                      \
    JS_ASSERT(name == errorNumber++);
# include "js.msg"
# undef MSG_DEF
}

static unsigned
MessageParameterCount(const char *format)
{
    unsigned numfmtspecs = 0;
    for (const char *fmt = format; *fmt != '\0'; fmt++) {
        if (*fmt == '{' && isdigit(fmt[1]))
            ++numfmtspecs;
    }
    return numfmtspecs;
}

static void
CheckMessageParameterCounts()
{
    
    
# define MSG_DEF(name, number, count, exception, format)                      \
    JS_BEGIN_MACRO                                                            \
        JS_ASSERT(MessageParameterCount(format) == count);                    \
    JS_END_MACRO;
# include "js.msg"
# undef MSG_DEF
}
#endif 

JS_PUBLIC_API(bool)
JS_Init(void)
{
    MOZ_ASSERT(jsInitState == Uninitialized,
               "must call JS_Init once before any JSAPI operation except "
               "JS_SetICUMemoryFunctions");
    MOZ_ASSERT(!JSRuntime::hasLiveRuntimes(),
               "how do we have live runtimes before JS_Init?");

#ifdef DEBUG
    CheckMessageNumbering();
    CheckMessageParameterCounts();
#endif

    using js::TlsPerThreadData;
    if (!TlsPerThreadData.initialized() && !TlsPerThreadData.init())
        return false;

#if defined(JS_ION)
    if (!jit::InitializeIon())
        return false;
#endif

    if (!ForkJoinSlice::InitializeTLS())
        return false;

#if EXPOSE_INTL_API
    UErrorCode err = U_ZERO_ERROR;
    u_init(&err);
    if (U_FAILURE(err))
        return false;
#endif 

    jsInitState = Running;
    return true;
}

JS_PUBLIC_API(void)
JS_ShutDown(void)
{
    MOZ_ASSERT(jsInitState == Running,
               "JS_ShutDown must only be called after JS_Init and can't race with it");
#ifdef DEBUG
    if (JSRuntime::hasLiveRuntimes()) {
        
        fprintf(stderr,
                "WARNING: YOU ARE LEAKING THE WORLD (at least one JSRuntime "
                "and everything alive inside it, that is) AT JS_ShutDown "
                "TIME.  FIX THIS!\n");
    }
#endif

    PRMJ_NowShutdown();

#if EXPOSE_INTL_API
    u_cleanup();
#endif 

    jsInitState = ShutDown;
}

#ifdef DEBUG
JS_FRIEND_API(bool)
JS::isGCEnabled()
{
    return !TlsPerThreadData.get()->suppressGC;
}
#else
JS_FRIEND_API(bool) JS::isGCEnabled() { return true; }
#endif

JS_PUBLIC_API(JSRuntime *)
JS_NewRuntime(uint32_t maxbytes, JSUseHelperThreads useHelperThreads)
{
    MOZ_ASSERT(jsInitState == Running,
               "must call JS_Init prior to creating any JSRuntimes");

    JSRuntime *rt = js_new<JSRuntime>(useHelperThreads);
    if (!rt)
        return nullptr;

    if (!rt->init(maxbytes)) {
        JS_DestroyRuntime(rt);
        return nullptr;
    }

    return rt;
}

JS_PUBLIC_API(void)
JS_DestroyRuntime(JSRuntime *rt)
{
    js_delete(rt);
}

JS_PUBLIC_API(bool)
JS_SetICUMemoryFunctions(JS_ICUAllocFn allocFn, JS_ICUReallocFn reallocFn, JS_ICUFreeFn freeFn)
{
    MOZ_ASSERT(jsInitState == Uninitialized,
               "must call JS_SetICUMemoryFunctions before any other JSAPI "
               "operation (including JS_Init)");

#if EXPOSE_INTL_API
    UErrorCode status = U_ZERO_ERROR;
    u_setMemoryFunctions( nullptr, allocFn, reallocFn, freeFn, &status);
    return U_SUCCESS(status);
#else
    return true;
#endif
}

JS_PUBLIC_API(void *)
JS_GetRuntimePrivate(JSRuntime *rt)
{
    return rt->data;
}

JS_PUBLIC_API(void)
JS_SetRuntimePrivate(JSRuntime *rt, void *data)
{
    rt->data = data;
}

#ifdef JS_THREADSAFE
static void
StartRequest(JSContext *cx)
{
    JSRuntime *rt = cx->runtime();
    JS_ASSERT(CurrentThreadCanAccessRuntime(rt));

    if (rt->requestDepth) {
        rt->requestDepth++;
    } else {
        
        rt->requestDepth = 1;

        if (rt->activityCallback)
            rt->activityCallback(rt->activityCallbackArg, true);
    }
}

static void
StopRequest(JSContext *cx)
{
    JSRuntime *rt = cx->runtime();
    JS_ASSERT(CurrentThreadCanAccessRuntime(rt));

    JS_ASSERT(rt->requestDepth != 0);
    if (rt->requestDepth != 1) {
        rt->requestDepth--;
    } else {
        rt->conservativeGC.updateForRequestEnd();
        rt->requestDepth = 0;

        if (rt->activityCallback)
            rt->activityCallback(rt->activityCallbackArg, false);
    }
}
#endif 

JS_PUBLIC_API(void)
JS_BeginRequest(JSContext *cx)
{
#ifdef JS_THREADSAFE
    cx->outstandingRequests++;
    StartRequest(cx);
#endif
}

JS_PUBLIC_API(void)
JS_EndRequest(JSContext *cx)
{
#ifdef JS_THREADSAFE
    JS_ASSERT(cx->outstandingRequests != 0);
    cx->outstandingRequests--;
    StopRequest(cx);
#endif
}

JS_PUBLIC_API(bool)
JS_IsInRequest(JSRuntime *rt)
{
#ifdef JS_THREADSAFE
    JS_ASSERT(CurrentThreadCanAccessRuntime(rt));
    return rt->requestDepth != 0;
#else
    return false;
#endif
}

JS_PUBLIC_API(void)
JS_SetContextCallback(JSRuntime *rt, JSContextCallback cxCallback, void *data)
{
    rt->cxCallback = cxCallback;
    rt->cxCallbackData = data;
}

JS_PUBLIC_API(JSContext *)
JS_NewContext(JSRuntime *rt, size_t stackChunkSize)
{
    return NewContext(rt, stackChunkSize);
}

JS_PUBLIC_API(void)
JS_DestroyContext(JSContext *cx)
{
    JS_ASSERT(!cx->compartment());
    DestroyContext(cx, DCM_FORCE_GC);
}

JS_PUBLIC_API(void)
JS_DestroyContextNoGC(JSContext *cx)
{
    JS_ASSERT(!cx->compartment());
    DestroyContext(cx, DCM_NO_GC);
}

JS_PUBLIC_API(void *)
JS_GetContextPrivate(JSContext *cx)
{
    return cx->data;
}

JS_PUBLIC_API(void)
JS_SetContextPrivate(JSContext *cx, void *data)
{
    cx->data = data;
}

JS_PUBLIC_API(void *)
JS_GetSecondContextPrivate(JSContext *cx)
{
    return cx->data2;
}

JS_PUBLIC_API(void)
JS_SetSecondContextPrivate(JSContext *cx, void *data)
{
    cx->data2 = data;
}

JS_PUBLIC_API(JSRuntime *)
JS_GetRuntime(JSContext *cx)
{
    return cx->runtime();
}

JS_PUBLIC_API(JSContext *)
JS_ContextIterator(JSRuntime *rt, JSContext **iterp)
{
    JSContext *cx = *iterp;
    cx = cx ? cx->getNext() : rt->contextList.getFirst();
    *iterp = cx;
    return cx;
}

JS_PUBLIC_API(JSVersion)
JS_GetVersion(JSContext *cx)
{
    return VersionNumber(cx->findVersion());
}

JS_PUBLIC_API(void)
JS_SetVersionForCompartment(JSCompartment *compartment, JSVersion version)
{
    compartment->options().setVersion(version);
}

static const struct v2smap {
    JSVersion   version;
    const char  *string;
} v2smap[] = {
    {JSVERSION_ECMA_3,  "ECMAv3"},
    {JSVERSION_1_6,     "1.6"},
    {JSVERSION_1_7,     "1.7"},
    {JSVERSION_1_8,     "1.8"},
    {JSVERSION_ECMA_5,  "ECMAv5"},
    {JSVERSION_DEFAULT, js_default_str},
    {JSVERSION_DEFAULT, "1.0"},
    {JSVERSION_DEFAULT, "1.1"},
    {JSVERSION_DEFAULT, "1.2"},
    {JSVERSION_DEFAULT, "1.3"},
    {JSVERSION_DEFAULT, "1.4"},
    {JSVERSION_DEFAULT, "1.5"},
    {JSVERSION_UNKNOWN, nullptr},          
};

JS_PUBLIC_API(const char *)
JS_VersionToString(JSVersion version)
{
    int i;

    for (i = 0; v2smap[i].string; i++)
        if (v2smap[i].version == version)
            return v2smap[i].string;
    return "unknown";
}

JS_PUBLIC_API(JSVersion)
JS_StringToVersion(const char *string)
{
    int i;

    for (i = 0; v2smap[i].string; i++)
        if (strcmp(v2smap[i].string, string) == 0)
            return v2smap[i].version;
    return JSVERSION_UNKNOWN;
}

static unsigned
GetOptionsCommon(JSContext *cx)
{
    return (cx->options().extraWarnings() ? JSOPTION_EXTRA_WARNINGS : 0)
         | (cx->options().werror() ? JSOPTION_WERROR : 0)
         | (cx->options().varObjFix() ? JSOPTION_VAROBJFIX : 0)
         | (cx->options().privateIsNSISupports() ? JSOPTION_PRIVATE_IS_NSISUPPORTS : 0)
         | (cx->options().compileAndGo() ? JSOPTION_COMPILE_N_GO : 0)
         | (cx->options().dontReportUncaught() ? JSOPTION_DONT_REPORT_UNCAUGHT : 0)
         | (cx->options().noDefaultCompartmentObject() ? JSOPTION_NO_DEFAULT_COMPARTMENT_OBJECT : 0)
         | (cx->options().noScriptRval() ? JSOPTION_NO_SCRIPT_RVAL : 0)
         | (cx->options().baseline() ? JSOPTION_BASELINE : 0)
         | (cx->options().typeInference() ? JSOPTION_TYPE_INFERENCE : 0)
         | (cx->options().strictMode() ? JSOPTION_STRICT_MODE : 0)
         | (cx->options().ion() ? JSOPTION_ION : 0)
         | (cx->options().asmJS() ? JSOPTION_ASMJS : 0);
}

static unsigned
SetOptionsCommon(JSContext *cx, unsigned newopts)
{
    JS_ASSERT((newopts & JSOPTION_MASK) == newopts);
    unsigned oldopts = GetOptionsCommon(cx);

    cx->options().setExtraWarnings(newopts & JSOPTION_EXTRA_WARNINGS);
    cx->options().setWerror(newopts & JSOPTION_WERROR);
    cx->options().setVarObjFix(newopts & JSOPTION_VAROBJFIX);
    cx->options().setPrivateIsNSISupports(newopts & JSOPTION_PRIVATE_IS_NSISUPPORTS);
    cx->options().setCompileAndGo(newopts & JSOPTION_COMPILE_N_GO);
    cx->options().setDontReportUncaught(newopts & JSOPTION_DONT_REPORT_UNCAUGHT);
    cx->options().setNoDefaultCompartmentObject(newopts & JSOPTION_NO_DEFAULT_COMPARTMENT_OBJECT);
    cx->options().setNoScriptRval(newopts & JSOPTION_NO_SCRIPT_RVAL);
    cx->options().setBaseline(newopts & JSOPTION_BASELINE);
    cx->options().setTypeInference(newopts & JSOPTION_TYPE_INFERENCE);
    cx->options().setStrictMode(newopts & JSOPTION_STRICT_MODE);
    cx->options().setIon(newopts & JSOPTION_ION);
    cx->options().setAsmJS(newopts & JSOPTION_ASMJS);

    cx->updateJITEnabled();
    return oldopts;
}

JS_PUBLIC_API(uint32_t)
JS_GetOptions(JSContext *cx)
{
    




    return GetOptionsCommon(cx);
}

JS_PUBLIC_API(uint32_t)
JS_SetOptions(JSContext *cx, uint32_t options)
{
    return SetOptionsCommon(cx, options);
}

JS_PUBLIC_API(uint32_t)
JS_ToggleOptions(JSContext *cx, uint32_t options)
{
    unsigned oldopts = GetOptionsCommon(cx);
    unsigned newopts = oldopts ^ options;
    return SetOptionsCommon(cx, newopts);
}

JS_PUBLIC_API(JS::ContextOptions &)
JS::ContextOptionsRef(JSContext *cx)
{
    return cx->options();
}

JS_PUBLIC_API(void)
JS_SetJitHardening(JSRuntime *rt, bool enabled)
{
    rt->setJitHardening(!!enabled);
}

JS_PUBLIC_API(const char *)
JS_GetImplementationVersion(void)
{
    return "JavaScript-C" MOZILLA_VERSION;
}

JS_PUBLIC_API(void)
JS_SetDestroyCompartmentCallback(JSRuntime *rt, JSDestroyCompartmentCallback callback)
{
    rt->destroyCompartmentCallback = callback;
}

JS_PUBLIC_API(void)
JS_SetCompartmentNameCallback(JSRuntime *rt, JSCompartmentNameCallback callback)
{
    rt->compartmentNameCallback = callback;
}

JS_PUBLIC_API(JSWrapObjectCallback)
JS_SetWrapObjectCallbacks(JSRuntime *rt,
                          JSWrapObjectCallback callback,
                          JSSameCompartmentWrapObjectCallback sccallback,
                          JSPreWrapCallback precallback)
{
    JSWrapObjectCallback old = rt->wrapObjectCallback;
    rt->wrapObjectCallback = callback;
    rt->sameCompartmentWrapObjectCallback = sccallback;
    rt->preWrapObjectCallback = precallback;
    return old;
}

JS_PUBLIC_API(JSCompartment *)
JS_EnterCompartment(JSContext *cx, JSObject *target)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);

    JSCompartment *oldCompartment = cx->compartment();
    cx->enterCompartment(target->compartment());
    return oldCompartment;
}

JS_PUBLIC_API(JSCompartment *)
JS_EnterCompartmentOfScript(JSContext *cx, JSScript *target)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    GlobalObject &global = target->global();
    return JS_EnterCompartment(cx, &global);
}

JS_PUBLIC_API(void)
JS_LeaveCompartment(JSContext *cx, JSCompartment *oldCompartment)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    cx->leaveCompartment(oldCompartment);
}

JSAutoCompartment::JSAutoCompartment(JSContext *cx, JSObject *target)
  : cx_(cx),
    oldCompartment_(cx->compartment())
{
    AssertHeapIsIdleOrIterating(cx_);
    cx_->enterCompartment(target->compartment());
}

JSAutoCompartment::JSAutoCompartment(JSContext *cx, JSScript *target)
  : cx_(cx),
    oldCompartment_(cx->compartment())
{
    AssertHeapIsIdleOrIterating(cx_);
    cx_->enterCompartment(target->compartment());
}

JSAutoCompartment::~JSAutoCompartment()
{
    cx_->leaveCompartment(oldCompartment_);
}

JS_PUBLIC_API(void)
JS_SetCompartmentPrivate(JSCompartment *compartment, void *data)
{
    compartment->data = data;
}

JS_PUBLIC_API(void *)
JS_GetCompartmentPrivate(JSCompartment *compartment)
{
    return compartment->data;
}

JS_PUBLIC_API(bool)
JS_WrapObject(JSContext *cx, MutableHandleObject objp)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    if (objp)
        JS::ExposeGCThingToActiveJS(objp, JSTRACE_OBJECT);
    return cx->compartment()->wrap(cx, objp);
}

JS_PUBLIC_API(bool)
JS_WrapValue(JSContext *cx, jsval *vp)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    if (vp)
        JS::ExposeValueToActiveJS(*vp);
    RootedValue value(cx, *vp);
    bool ok = cx->compartment()->wrap(cx, &value);
    *vp = value.get();
    return ok;
}

JS_PUBLIC_API(bool)
JS_WrapId(JSContext *cx, jsid *idp)
{
  AssertHeapIsIdle(cx);
  CHECK_REQUEST(cx);
  if (idp) {
      jsid id = *idp;
      if (JSID_IS_STRING(id))
          JS::ExposeGCThingToActiveJS(JSID_TO_STRING(id), JSTRACE_STRING);
      else if (JSID_IS_OBJECT(id))
          JS::ExposeGCThingToActiveJS(JSID_TO_OBJECT(id), JSTRACE_OBJECT);
  }
  return cx->compartment()->wrapId(cx, idp);
}
































JS_PUBLIC_API(JSObject *)
JS_TransplantObject(JSContext *cx, HandleObject origobj, HandleObject target)
{
    AssertHeapIsIdle(cx);
    JS_ASSERT(origobj != target);
    JS_ASSERT(!origobj->is<CrossCompartmentWrapperObject>());
    JS_ASSERT(!target->is<CrossCompartmentWrapperObject>());

    AutoMaybeTouchDeadZones agc(cx);
    AutoDisableProxyCheck adpc(cx->runtime());

    JSCompartment *destination = target->compartment();
    RootedValue origv(cx, ObjectValue(*origobj));
    RootedObject newIdentity(cx);

    if (origobj->compartment() == destination) {
        
        
        
        
        if (!JSObject::swap(cx, origobj, target))
            MOZ_CRASH();
        newIdentity = origobj;
    } else if (WrapperMap::Ptr p = destination->lookupWrapper(origv)) {
        
        
        
        newIdentity = &p->value.toObject();

        
        
        destination->removeWrapper(p);
        NukeCrossCompartmentWrapper(cx, newIdentity);

        if (!JSObject::swap(cx, newIdentity, target))
            MOZ_CRASH();
    } else {
        
        newIdentity = target;
    }

    
    
    if (!RemapAllWrappersForObject(cx, origobj, newIdentity))
        MOZ_CRASH();

    
    if (origobj->compartment() != destination) {
        RootedObject newIdentityWrapper(cx, newIdentity);
        AutoCompartment ac(cx, origobj);
        if (!JS_WrapObject(cx, &newIdentityWrapper))
            MOZ_CRASH();
        JS_ASSERT(Wrapper::wrappedObject(newIdentityWrapper) == newIdentity);
        if (!JSObject::swap(cx, origobj, newIdentityWrapper))
            MOZ_CRASH();
        origobj->compartment()->putWrapper(ObjectValue(*newIdentity), origv);
    }

    
    
    return newIdentity;
}






JS_PUBLIC_API(bool)
JS_RefreshCrossCompartmentWrappers(JSContext *cx, JSObject *objArg)
{
    RootedObject obj(cx, objArg);
    return RemapAllWrappersForObject(cx, obj, obj);
}

JS_PUBLIC_API(bool)
JS_InitStandardClasses(JSContext *cx, JSObject *objArg)
{
    RootedObject obj(cx, objArg);
    JS_ASSERT(!cx->runtime()->isAtomsCompartment(cx->compartment()));
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);

    cx->setDefaultCompartmentObjectIfUnset(obj);
    assertSameCompartment(cx, obj);

    Rooted<GlobalObject*> global(cx, &obj->global());
    return GlobalObject::initStandardClasses(cx, global);
}

#define CLASP(name)                 (&name##Class)
#define OCLASP(name)                (&name##Object::class_)
#define TYPED_ARRAY_CLASP(type)     (&TypedArrayObject::classes[ScalarTypeRepresentation::type])
#define EAGER_ATOM(name)            NAME_OFFSET(name)
#define EAGER_CLASS_ATOM(name)      NAME_OFFSET(name)
#define EAGER_ATOM_AND_CLASP(name)  EAGER_CLASS_ATOM(name), CLASP(name)
#define EAGER_ATOM_AND_OCLASP(name) EAGER_CLASS_ATOM(name), OCLASP(name)

typedef struct JSStdName {
    ClassInitializerOp init;
    size_t      atomOffset;     
    const Class *clasp;
} JSStdName;

static Handle<PropertyName*>
StdNameToPropertyName(JSContext *cx, const JSStdName *stdn)
{
    return OFFSET_TO_NAME(cx->runtime(), stdn->atomOffset);
}





static const JSStdName standard_class_atoms[] = {
    {js_InitFunctionClass,              EAGER_CLASS_ATOM(Function), &JSFunction::class_},
    {js_InitObjectClass,                EAGER_CLASS_ATOM(Object), &JSObject::class_},
    {js_InitArrayClass,                 EAGER_ATOM_AND_OCLASP(Array)},
    {js_InitBooleanClass,               EAGER_ATOM_AND_OCLASP(Boolean)},
    {js_InitDateClass,                  EAGER_ATOM_AND_OCLASP(Date)},
    {js_InitMathClass,                  EAGER_ATOM_AND_CLASP(Math)},
    {js_InitNumberClass,                EAGER_ATOM_AND_OCLASP(Number)},
    {js_InitStringClass,                EAGER_ATOM_AND_OCLASP(String)},
    {js_InitExceptionClasses,           EAGER_ATOM_AND_OCLASP(Error)},
    {js_InitRegExpClass,                EAGER_ATOM_AND_OCLASP(RegExp)},
    {js_InitIteratorClasses,            EAGER_ATOM_AND_OCLASP(StopIteration)},
    {js_InitJSONClass,                  EAGER_ATOM_AND_CLASP(JSON)},
    {js_InitTypedArrayClasses,          EAGER_CLASS_ATOM(ArrayBuffer), &js::ArrayBufferObject::protoClass},
    {js_InitWeakMapClass,               EAGER_ATOM_AND_OCLASP(WeakMap)},
    {js_InitMapClass,                   EAGER_ATOM_AND_OCLASP(Map)},
    {js_InitSetClass,                   EAGER_ATOM_AND_OCLASP(Set)},
#ifdef ENABLE_PARALLEL_JS
    {js_InitParallelArrayClass,         EAGER_ATOM_AND_OCLASP(ParallelArray)},
#endif
    {js_InitProxyClass,                 EAGER_CLASS_ATOM(Proxy), &ProxyObject::uncallableClass_},
#if EXPOSE_INTL_API
    {js_InitIntlClass,                  EAGER_ATOM_AND_CLASP(Intl)},
#endif
#ifdef ENABLE_BINARYDATA
    {js_InitTypedObjectClass,           EAGER_ATOM_AND_CLASP(TypedObject)},
#endif
    {nullptr,                           0, nullptr}
};






static const JSStdName standard_class_names[] = {
    {js_InitObjectClass,        EAGER_ATOM(eval), &JSObject::class_},

    
    {js_InitNumberClass,        EAGER_ATOM(NaN), OCLASP(Number)},
    {js_InitNumberClass,        EAGER_ATOM(Infinity), OCLASP(Number)},
    {js_InitNumberClass,        EAGER_ATOM(isNaN), OCLASP(Number)},
    {js_InitNumberClass,        EAGER_ATOM(isFinite), OCLASP(Number)},
    {js_InitNumberClass,        EAGER_ATOM(parseFloat), OCLASP(Number)},
    {js_InitNumberClass,        EAGER_ATOM(parseInt), OCLASP(Number)},

    
    {js_InitStringClass,        EAGER_ATOM(escape), OCLASP(String)},
    {js_InitStringClass,        EAGER_ATOM(unescape), OCLASP(String)},
    {js_InitStringClass,        EAGER_ATOM(decodeURI), OCLASP(String)},
    {js_InitStringClass,        EAGER_ATOM(encodeURI), OCLASP(String)},
    {js_InitStringClass,        EAGER_ATOM(decodeURIComponent), OCLASP(String)},
    {js_InitStringClass,        EAGER_ATOM(encodeURIComponent), OCLASP(String)},
#if JS_HAS_UNEVAL
    {js_InitStringClass,        EAGER_ATOM(uneval), OCLASP(String)},
#endif

    
    {js_InitExceptionClasses,   EAGER_CLASS_ATOM(Error), OCLASP(Error)},
    {js_InitExceptionClasses,   EAGER_CLASS_ATOM(InternalError), OCLASP(Error)},
    {js_InitExceptionClasses,   EAGER_CLASS_ATOM(EvalError), OCLASP(Error)},
    {js_InitExceptionClasses,   EAGER_CLASS_ATOM(RangeError), OCLASP(Error)},
    {js_InitExceptionClasses,   EAGER_CLASS_ATOM(ReferenceError), OCLASP(Error)},
    {js_InitExceptionClasses,   EAGER_CLASS_ATOM(SyntaxError), OCLASP(Error)},
    {js_InitExceptionClasses,   EAGER_CLASS_ATOM(TypeError), OCLASP(Error)},
    {js_InitExceptionClasses,   EAGER_CLASS_ATOM(URIError), OCLASP(Error)},

    {js_InitIteratorClasses,    EAGER_CLASS_ATOM(Iterator), &PropertyIteratorObject::class_},

    
    {js_InitTypedArrayClasses,  EAGER_CLASS_ATOM(ArrayBuffer),  &ArrayBufferObject::class_},
    {js_InitTypedArrayClasses,  EAGER_CLASS_ATOM(Int8Array),    TYPED_ARRAY_CLASP(TYPE_INT8)},
    {js_InitTypedArrayClasses,  EAGER_CLASS_ATOM(Uint8Array),   TYPED_ARRAY_CLASP(TYPE_UINT8)},
    {js_InitTypedArrayClasses,  EAGER_CLASS_ATOM(Int16Array),   TYPED_ARRAY_CLASP(TYPE_INT16)},
    {js_InitTypedArrayClasses,  EAGER_CLASS_ATOM(Uint16Array),  TYPED_ARRAY_CLASP(TYPE_UINT16)},
    {js_InitTypedArrayClasses,  EAGER_CLASS_ATOM(Int32Array),   TYPED_ARRAY_CLASP(TYPE_INT32)},
    {js_InitTypedArrayClasses,  EAGER_CLASS_ATOM(Uint32Array),  TYPED_ARRAY_CLASP(TYPE_UINT32)},
    {js_InitTypedArrayClasses,  EAGER_CLASS_ATOM(Float32Array), TYPED_ARRAY_CLASP(TYPE_FLOAT32)},
    {js_InitTypedArrayClasses,  EAGER_CLASS_ATOM(Float64Array), TYPED_ARRAY_CLASP(TYPE_FLOAT64)},
    {js_InitTypedArrayClasses,  EAGER_CLASS_ATOM(Uint8ClampedArray),
                                TYPED_ARRAY_CLASP(TYPE_UINT8_CLAMPED)},
    {js_InitTypedArrayClasses,  EAGER_CLASS_ATOM(DataView),     &DataViewObject::class_},

    {nullptr,                     0, nullptr}
};

static const JSStdName object_prototype_names[] = {
    
    {js_InitObjectClass,        EAGER_ATOM(proto), &JSObject::class_},
#if JS_HAS_TOSOURCE
    {js_InitObjectClass,        EAGER_ATOM(toSource), &JSObject::class_},
#endif
    {js_InitObjectClass,        EAGER_ATOM(toString), &JSObject::class_},
    {js_InitObjectClass,        EAGER_ATOM(toLocaleString), &JSObject::class_},
    {js_InitObjectClass,        EAGER_ATOM(valueOf), &JSObject::class_},
#if JS_HAS_OBJ_WATCHPOINT
    {js_InitObjectClass,        EAGER_ATOM(watch), &JSObject::class_},
    {js_InitObjectClass,        EAGER_ATOM(unwatch), &JSObject::class_},
#endif
    {js_InitObjectClass,        EAGER_ATOM(hasOwnProperty), &JSObject::class_},
    {js_InitObjectClass,        EAGER_ATOM(isPrototypeOf), &JSObject::class_},
    {js_InitObjectClass,        EAGER_ATOM(propertyIsEnumerable), &JSObject::class_},
#if JS_OLD_GETTER_SETTER_METHODS
    {js_InitObjectClass,        EAGER_ATOM(defineGetter), &JSObject::class_},
    {js_InitObjectClass,        EAGER_ATOM(defineSetter), &JSObject::class_},
    {js_InitObjectClass,        EAGER_ATOM(lookupGetter), &JSObject::class_},
    {js_InitObjectClass,        EAGER_ATOM(lookupSetter), &JSObject::class_},
#endif

    {nullptr,                   0, nullptr}
};

#undef CLASP
#undef TYPED_ARRAY_CLASP
#undef EAGER_ATOM
#undef EAGER_CLASS_ATOM
#undef EAGER_ATOM_CLASP
#undef EAGER_ATOM_AND_CLASP

JS_PUBLIC_API(bool)
JS_ResolveStandardClass(JSContext *cx, HandleObject obj, HandleId id, bool *resolved)
{
    JSRuntime *rt;
    JSAtom *atom;
    const JSStdName *stdnm;
    unsigned i;

    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, obj, id);
    JS_ASSERT(obj->is<GlobalObject>());
    *resolved = false;

    rt = cx->runtime();
    if (!rt->hasContexts() || !JSID_IS_ATOM(id))
        return true;

    RootedString idstr(cx, JSID_TO_STRING(id));

    
    atom = rt->atomState.undefined;
    if (idstr == atom) {
        *resolved = true;
        RootedValue undefinedValue(cx, UndefinedValue());
        return JSObject::defineProperty(cx, obj, atom->asPropertyName(), undefinedValue,
                                        JS_PropertyStub, JS_StrictPropertyStub,
                                        JSPROP_PERMANENT | JSPROP_READONLY);
    }

    
    stdnm = nullptr;
    for (i = 0; standard_class_atoms[i].init; i++) {
        JS_ASSERT(standard_class_atoms[i].clasp);
        atom = OFFSET_TO_NAME(rt, standard_class_atoms[i].atomOffset);
        if (idstr == atom) {
            stdnm = &standard_class_atoms[i];
            break;
        }
    }

    if (!stdnm) {
        
        for (i = 0; standard_class_names[i].init; i++) {
            JS_ASSERT(standard_class_names[i].clasp);
            atom = StdNameToPropertyName(cx, &standard_class_names[i]);
            if (!atom)
                return false;
            if (idstr == atom) {
                stdnm = &standard_class_names[i];
                break;
            }
        }

        RootedObject proto(cx);
        if (!JSObject::getProto(cx, obj, &proto))
            return false;
        if (!stdnm && !proto) {
            




            for (i = 0; object_prototype_names[i].init; i++) {
                JS_ASSERT(object_prototype_names[i].clasp);
                atom = StdNameToPropertyName(cx, &object_prototype_names[i]);
                if (!atom)
                    return false;
                if (idstr == atom) {
                    stdnm = &object_prototype_names[i];
                    break;
                }
            }
        }
    }

    if (stdnm) {
        



        if (stdnm->clasp->flags & JSCLASS_IS_ANONYMOUS)
            return true;

        if (obj->as<GlobalObject>().isStandardClassResolved(stdnm->clasp))
            return true;

        if (!stdnm->init(cx, obj))
            return false;
        *resolved = true;
    }
    return true;
}

JS_PUBLIC_API(bool)
JS_EnumerateStandardClasses(JSContext *cx, HandleObject obj)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, obj);
    MOZ_ASSERT(obj->is<GlobalObject>());

    



    HandlePropertyName undefinedName = cx->names().undefined;
    RootedValue undefinedValue(cx, UndefinedValue());
    if (!obj->nativeContains(cx, undefinedName) &&
        !JSObject::defineProperty(cx, obj, undefinedName, undefinedValue,
                                  JS_PropertyStub, JS_StrictPropertyStub,
                                  JSPROP_PERMANENT | JSPROP_READONLY)) {
        return false;
    }

    
    for (unsigned i = 0; standard_class_atoms[i].init; i++) {
        const JSStdName &stdnm = standard_class_atoms[i];
        if (!obj->as<GlobalObject>().isStandardClassResolved(stdnm.clasp)) {
            if (!stdnm.init(cx, obj))
                return false;
        }
    }

    return true;
}

JS_PUBLIC_API(bool)
JS_GetClassObject(JSContext *cx, JSObject *obj, JSProtoKey key, JSObject **objpArg)
{
    RootedObject objp(cx, *objpArg);
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);

    assertSameCompartment(cx, obj);
    if (!js_GetClassObject(cx, obj, key, &objp))
        return false;
    *objpArg = objp;
    return true;
}

JS_PUBLIC_API(bool)
JS_GetClassPrototype(JSContext *cx, JSProtoKey key, JSObject **objp_)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);

    RootedObject objp(cx);
    bool result = js_GetClassPrototype(cx, key, &objp);
    *objp_ = objp;
    return result;
}

JS_PUBLIC_API(JSProtoKey)
JS_IdentifyClassPrototype(JSContext *cx, JSObject *obj)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, obj);
    JS_ASSERT(!obj->is<CrossCompartmentWrapperObject>());
    return js_IdentifyClassPrototype(obj);
}

JS_PUBLIC_API(JSObject *)
JS_GetObjectPrototype(JSContext *cx, JSObject *forObj)
{
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, forObj);
    return forObj->global().getOrCreateObjectPrototype(cx);
}

JS_PUBLIC_API(JSObject *)
JS_GetFunctionPrototype(JSContext *cx, JSObject *forObj)
{
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, forObj);
    return forObj->global().getOrCreateFunctionPrototype(cx);
}

JS_PUBLIC_API(JSObject *)
JS_GetArrayPrototype(JSContext *cx, JSObject *forObj)
{
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, forObj);
    return forObj->global().getOrCreateArrayPrototype(cx);
}

JS_PUBLIC_API(JSObject *)
JS_GetGlobalForObject(JSContext *cx, JSObject *obj)
{
    AssertHeapIsIdle(cx);
    assertSameCompartment(cx, obj);
    return &obj->global();
}

extern JS_PUBLIC_API(bool)
JS_IsGlobalObject(JSObject *obj)
{
    return obj->is<GlobalObject>();
}

JS_PUBLIC_API(JSObject *)
JS_GetGlobalForCompartmentOrNull(JSContext *cx, JSCompartment *c)
{
    AssertHeapIsIdleOrIterating(cx);
    assertSameCompartment(cx, c);
    return c->maybeGlobal();
}

JS_PUBLIC_API(JSObject *)
JS::CurrentGlobalOrNull(JSContext *cx)
{
    AssertHeapIsIdleOrIterating(cx);
    CHECK_REQUEST(cx);
    if (!cx->compartment())
        return nullptr;
    return cx->global();
}

JS_PUBLIC_API(jsval)
JS_ComputeThis(JSContext *cx, jsval *vp)
{
    AssertHeapIsIdle(cx);
    assertSameCompartment(cx, JSValueArray(vp, 2));
    CallReceiver call = CallReceiverFromVp(vp);
    if (!BoxNonStrictThis(cx, call))
        return JSVAL_NULL;
    return call.thisv();
}

JS_PUBLIC_API(void *)
JS_malloc(JSContext *cx, size_t nbytes)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    return cx->malloc_(nbytes);
}

JS_PUBLIC_API(void *)
JS_realloc(JSContext *cx, void *p, size_t nbytes)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    return cx->realloc_(p, nbytes);
}

JS_PUBLIC_API(void)
JS_free(JSContext *cx, void *p)
{
    return js_free(p);
}

JS_PUBLIC_API(void)
JS_freeop(JSFreeOp *fop, void *p)
{
    return FreeOp::get(fop)->free_(p);
}

JS_PUBLIC_API(JSFreeOp *)
JS_GetDefaultFreeOp(JSRuntime *rt)
{
    return rt->defaultFreeOp();
}

JS_PUBLIC_API(void)
JS_updateMallocCounter(JSContext *cx, size_t nbytes)
{
    return cx->runtime()->updateMallocCounter(cx->zone(), nbytes);
}

JS_PUBLIC_API(char *)
JS_strdup(JSContext *cx, const char *s)
{
    AssertHeapIsIdle(cx);
    return js_strdup(cx, s);
}

JS_PUBLIC_API(char *)
JS_strdup(JSRuntime *rt, const char *s)
{
    AssertHeapIsIdle(rt);
    size_t n = strlen(s) + 1;
    void *p = rt->malloc_(n);
    if (!p)
        return nullptr;
    return static_cast<char*>(js_memcpy(p, s, n));
}

#undef JS_AddRoot

JS_PUBLIC_API(bool)
JS_AddValueRoot(JSContext *cx, jsval *vp)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    return AddValueRoot(cx, vp, nullptr);
}

JS_PUBLIC_API(bool)
JS_AddStringRoot(JSContext *cx, JSString **rp)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    return AddStringRoot(cx, rp, nullptr);
}

JS_PUBLIC_API(bool)
JS_AddObjectRoot(JSContext *cx, JSObject **rp)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    return AddObjectRoot(cx, rp, nullptr);
}

JS_PUBLIC_API(bool)
JS_AddNamedValueRoot(JSContext *cx, jsval *vp, const char *name)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    return AddValueRoot(cx, vp, name);
}

JS_PUBLIC_API(bool)
JS_AddNamedValueRootRT(JSRuntime *rt, jsval *vp, const char *name)
{
    return AddValueRootRT(rt, vp, name);
}

JS_PUBLIC_API(bool)
JS_AddNamedStringRoot(JSContext *cx, JSString **rp, const char *name)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    return AddStringRoot(cx, rp, name);
}

JS_PUBLIC_API(bool)
JS_AddNamedObjectRoot(JSContext *cx, JSObject **rp, const char *name)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    return AddObjectRoot(cx, rp, name);
}

JS_PUBLIC_API(bool)
JS_AddNamedScriptRoot(JSContext *cx, JSScript **rp, const char *name)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    return AddScriptRoot(cx, rp, name);
}



JS_PUBLIC_API(void)
JS_RemoveValueRoot(JSContext *cx, jsval *vp)
{
    CHECK_REQUEST(cx);
    js_RemoveRoot(cx->runtime(), (void *)vp);
}

JS_PUBLIC_API(void)
JS_RemoveStringRoot(JSContext *cx, JSString **rp)
{
    CHECK_REQUEST(cx);
    js_RemoveRoot(cx->runtime(), (void *)rp);
}

JS_PUBLIC_API(void)
JS_RemoveObjectRoot(JSContext *cx, JSObject **rp)
{
    CHECK_REQUEST(cx);
    js_RemoveRoot(cx->runtime(), (void *)rp);
}

JS_PUBLIC_API(void)
JS_RemoveScriptRoot(JSContext *cx, JSScript **rp)
{
    CHECK_REQUEST(cx);
    js_RemoveRoot(cx->runtime(), (void *)rp);
}

JS_PUBLIC_API(void)
JS_RemoveValueRootRT(JSRuntime *rt, jsval *vp)
{
    js_RemoveRoot(rt, (void *)vp);
}

JS_PUBLIC_API(void)
JS_RemoveStringRootRT(JSRuntime *rt, JSString **rp)
{
    js_RemoveRoot(rt, (void *)rp);
}

JS_PUBLIC_API(void)
JS_RemoveObjectRootRT(JSRuntime *rt, JSObject **rp)
{
    js_RemoveRoot(rt, (void *)rp);
}

JS_PUBLIC_API(void)
JS_RemoveScriptRootRT(JSRuntime *rt, JSScript **rp)
{
    js_RemoveRoot(rt, (void *)rp);
}

JS_NEVER_INLINE JS_PUBLIC_API(void)
JS_AnchorPtr(void *p)
{
}

JS_PUBLIC_API(bool)
JS_AddExtraGCRootsTracer(JSRuntime *rt, JSTraceDataOp traceOp, void *data)
{
    AssertHeapIsIdle(rt);
    return !!rt->gcBlackRootTracers.append(JSRuntime::ExtraTracer(traceOp, data));
}

JS_PUBLIC_API(void)
JS_RemoveExtraGCRootsTracer(JSRuntime *rt, JSTraceDataOp traceOp, void *data)
{
    AssertHeapIsIdle(rt);
    for (size_t i = 0; i < rt->gcBlackRootTracers.length(); i++) {
        JSRuntime::ExtraTracer *e = &rt->gcBlackRootTracers[i];
        if (e->op == traceOp && e->data == data) {
            rt->gcBlackRootTracers.erase(e);
            break;
        }
    }
}

#ifdef DEBUG

typedef struct JSHeapDumpNode JSHeapDumpNode;

struct JSHeapDumpNode {
    void            *thing;
    JSGCTraceKind   kind;
    JSHeapDumpNode  *next;          
    JSHeapDumpNode  *parent;        

    char            edgeName[1];    

};

typedef HashSet<void *, PointerHasher<void *, 3>, SystemAllocPolicy> VisitedSet;

typedef struct JSDumpingTracer {
    JSTracer            base;
    VisitedSet          visited;
    bool                ok;
    void                *startThing;
    void                *thingToFind;
    void                *thingToIgnore;
    JSHeapDumpNode      *parentNode;
    JSHeapDumpNode      **lastNodep;
    char                buffer[200];
} JSDumpingTracer;

static void
DumpNotify(JSTracer *trc, void **thingp, JSGCTraceKind kind)
{
    JS_ASSERT(trc->callback == DumpNotify);

    JSDumpingTracer *dtrc = (JSDumpingTracer *)trc;
    void *thing = *thingp;

    if (!dtrc->ok || thing == dtrc->thingToIgnore)
        return;

    









    if (dtrc->thingToFind != thing) {
        



        if (thing == dtrc->startThing)
            return;
        VisitedSet::AddPtr p = dtrc->visited.lookupForAdd(thing);
        if (p)
            return;
        if (!dtrc->visited.add(p, thing)) {
            dtrc->ok = false;
            return;
        }
    }

    const char *edgeName = JS_GetTraceEdgeName(&dtrc->base, dtrc->buffer, sizeof(dtrc->buffer));
    size_t edgeNameSize = strlen(edgeName) + 1;
    size_t bytes = offsetof(JSHeapDumpNode, edgeName) + edgeNameSize;
    JSHeapDumpNode *node = (JSHeapDumpNode *) js_malloc(bytes);
    if (!node) {
        dtrc->ok = false;
        return;
    }

    node->thing = thing;
    node->kind = kind;
    node->next = nullptr;
    node->parent = dtrc->parentNode;
    js_memcpy(node->edgeName, edgeName, edgeNameSize);

    JS_ASSERT(!*dtrc->lastNodep);
    *dtrc->lastNodep = node;
    dtrc->lastNodep = &node->next;
}


static bool
DumpNode(JSDumpingTracer *dtrc, FILE* fp, JSHeapDumpNode *node)
{
    JSHeapDumpNode *prev, *following;
    size_t chainLimit;
    enum { MAX_PARENTS_TO_PRINT = 10 };

    JS_GetTraceThingInfo(dtrc->buffer, sizeof dtrc->buffer,
                         &dtrc->base, node->thing, node->kind, true);
    if (fprintf(fp, "%p %-22s via ", node->thing, dtrc->buffer) < 0)
        return false;

    





    chainLimit = MAX_PARENTS_TO_PRINT;
    prev = nullptr;
    for (;;) {
        following = node->parent;
        node->parent = prev;
        prev = node;
        node = following;
        if (!node)
            break;
        if (chainLimit == 0) {
            if (fputs("...", fp) < 0)
                return false;
            break;
        }
        --chainLimit;
    }

    node = prev;
    prev = following;
    bool ok = true;
    do {
        
        if (ok) {
            if (!prev) {
                
                if (fputs(node->edgeName, fp) < 0)
                    ok = false;
            } else {
                JS_GetTraceThingInfo(dtrc->buffer, sizeof dtrc->buffer,
                                     &dtrc->base, prev->thing, prev->kind,
                                     false);
                if (fprintf(fp, "(%p %s).%s",
                           prev->thing, dtrc->buffer, node->edgeName) < 0) {
                    ok = false;
                }
            }
        }
        following = node->parent;
        node->parent = prev;
        prev = node;
        node = following;
    } while (node);

    return ok && putc('\n', fp) >= 0;
}

JS_PUBLIC_API(bool)
JS_DumpHeap(JSRuntime *rt, FILE *fp, void* startThing, JSGCTraceKind startKind,
            void *thingToFind, size_t maxDepth, void *thingToIgnore)
{
    if (maxDepth == 0)
        return true;

    JSDumpingTracer dtrc;
    if (!dtrc.visited.init())
        return false;
    JS_TracerInit(&dtrc.base, rt, DumpNotify);
    dtrc.ok = true;
    dtrc.startThing = startThing;
    dtrc.thingToFind = thingToFind;
    dtrc.thingToIgnore = thingToIgnore;
    dtrc.parentNode = nullptr;
    JSHeapDumpNode *node = nullptr;
    dtrc.lastNodep = &node;
    if (!startThing) {
        JS_ASSERT(startKind == JSTRACE_OBJECT);
        TraceRuntime(&dtrc.base);
    } else {
        JS_TraceChildren(&dtrc.base, startThing, startKind);
    }

    if (!node)
        return dtrc.ok;

    size_t depth = 1;
    JSHeapDumpNode *children, *next, *parent;
    bool thingToFindWasTraced = thingToFind && thingToFind == startThing;
    for (;;) {
        



        if (dtrc.ok) {
            if (thingToFind == nullptr || thingToFind == node->thing)
                dtrc.ok = DumpNode(&dtrc, fp, node);

            
            if (dtrc.ok &&
                depth < maxDepth &&
                (thingToFind != node->thing || !thingToFindWasTraced)) {
                dtrc.parentNode = node;
                children = nullptr;
                dtrc.lastNodep = &children;
                JS_TraceChildren(&dtrc.base, node->thing, node->kind);
                if (thingToFind == node->thing)
                    thingToFindWasTraced = true;
                if (children != nullptr) {
                    ++depth;
                    node = children;
                    continue;
                }
            }
        }

        
        for (;;) {
            next = node->next;
            parent = node->parent;
            js_free(node);
            node = next;
            if (node)
                break;
            if (!parent)
                return dtrc.ok;
            JS_ASSERT(depth > 1);
            --depth;
            node = parent;
        }
    }

    JS_ASSERT(depth == 1);
    return dtrc.ok;
}

#endif 

extern JS_PUBLIC_API(bool)
JS_IsGCMarkingTracer(JSTracer *trc)
{
    return IS_GC_MARKING_TRACER(trc);
}

#ifdef DEBUG
extern JS_PUBLIC_API(bool)
JS_IsMarkingGray(JSTracer *trc)
{
    JS_ASSERT(JS_IsGCMarkingTracer(trc));
    return trc->callback == GCMarker::GrayCallback;
}
#endif

JS_PUBLIC_API(void)
JS_GC(JSRuntime *rt)
{
    AssertHeapIsIdle(rt);
    JS::PrepareForFullGC(rt);
    GC(rt, GC_NORMAL, JS::gcreason::API);
}

JS_PUBLIC_API(void)
JS_MaybeGC(JSContext *cx)
{
    MaybeGC(cx);
}

JS_PUBLIC_API(void)
JS_SetGCCallback(JSRuntime *rt, JSGCCallback cb, void *data)
{
    AssertHeapIsIdle(rt);
    rt->gcCallback = cb;
    rt->gcCallbackData = data;
}

JS_PUBLIC_API(void)
JS_SetFinalizeCallback(JSRuntime *rt, JSFinalizeCallback cb)
{
    AssertHeapIsIdle(rt);
    rt->gcFinalizeCallback = cb;
}

JS_PUBLIC_API(bool)
JS_IsAboutToBeFinalized(JS::Heap<JSObject *> *objp)
{
    return IsObjectAboutToBeFinalized(objp->unsafeGet());
}

JS_PUBLIC_API(bool)
JS_IsAboutToBeFinalizedUnbarriered(JSObject **objp)
{
    return IsObjectAboutToBeFinalized(objp);
}

JS_PUBLIC_API(void)
JS_SetGCParameter(JSRuntime *rt, JSGCParamKey key, uint32_t value)
{
    switch (key) {
      case JSGC_MAX_BYTES: {
        JS_ASSERT(value >= rt->gcBytes);
        rt->gcMaxBytes = value;
        break;
      }
      case JSGC_MAX_MALLOC_BYTES:
        rt->setGCMaxMallocBytes(value);
        break;
      case JSGC_SLICE_TIME_BUDGET:
        rt->gcSliceBudget = SliceBudget::TimeBudget(value);
        break;
      case JSGC_MARK_STACK_LIMIT:
        js::SetMarkStackLimit(rt, value);
        break;
      case JSGC_HIGH_FREQUENCY_TIME_LIMIT:
        rt->gcHighFrequencyTimeThreshold = value;
        break;
      case JSGC_HIGH_FREQUENCY_LOW_LIMIT:
        rt->gcHighFrequencyLowLimitBytes = value * 1024 * 1024;
        break;
      case JSGC_HIGH_FREQUENCY_HIGH_LIMIT:
        rt->gcHighFrequencyHighLimitBytes = value * 1024 * 1024;
        break;
      case JSGC_HIGH_FREQUENCY_HEAP_GROWTH_MAX:
        rt->gcHighFrequencyHeapGrowthMax = value / 100.0;
        break;
      case JSGC_HIGH_FREQUENCY_HEAP_GROWTH_MIN:
        rt->gcHighFrequencyHeapGrowthMin = value / 100.0;
        break;
      case JSGC_LOW_FREQUENCY_HEAP_GROWTH:
        rt->gcLowFrequencyHeapGrowth = value / 100.0;
        break;
      case JSGC_DYNAMIC_HEAP_GROWTH:
        rt->gcDynamicHeapGrowth = value;
        break;
      case JSGC_DYNAMIC_MARK_SLICE:
        rt->gcDynamicMarkSlice = value;
        break;
      case JSGC_ALLOCATION_THRESHOLD:
        rt->gcAllocationThreshold = value * 1024 * 1024;
        break;
      case JSGC_DECOMMIT_THRESHOLD:
        rt->gcDecommitThreshold = value * 1024 * 1024;
        break;
      default:
        JS_ASSERT(key == JSGC_MODE);
        rt->gcMode = JSGCMode(value);
        JS_ASSERT(rt->gcMode == JSGC_MODE_GLOBAL ||
                  rt->gcMode == JSGC_MODE_COMPARTMENT ||
                  rt->gcMode == JSGC_MODE_INCREMENTAL);
        return;
    }
}

JS_PUBLIC_API(uint32_t)
JS_GetGCParameter(JSRuntime *rt, JSGCParamKey key)
{
    switch (key) {
      case JSGC_MAX_BYTES:
        return uint32_t(rt->gcMaxBytes);
      case JSGC_MAX_MALLOC_BYTES:
        return rt->gcMaxMallocBytes;
      case JSGC_BYTES:
        return uint32_t(rt->gcBytes);
      case JSGC_MODE:
        return uint32_t(rt->gcMode);
      case JSGC_UNUSED_CHUNKS:
        return uint32_t(rt->gcChunkPool.getEmptyCount());
      case JSGC_TOTAL_CHUNKS:
        return uint32_t(rt->gcChunkSet.count() + rt->gcChunkPool.getEmptyCount());
      case JSGC_SLICE_TIME_BUDGET:
        return uint32_t(rt->gcSliceBudget > 0 ? rt->gcSliceBudget / PRMJ_USEC_PER_MSEC : 0);
      case JSGC_MARK_STACK_LIMIT:
        return rt->gcMarker.sizeLimit();
      case JSGC_HIGH_FREQUENCY_TIME_LIMIT:
        return rt->gcHighFrequencyTimeThreshold;
      case JSGC_HIGH_FREQUENCY_LOW_LIMIT:
        return rt->gcHighFrequencyLowLimitBytes / 1024 / 1024;
      case JSGC_HIGH_FREQUENCY_HIGH_LIMIT:
        return rt->gcHighFrequencyHighLimitBytes / 1024 / 1024;
      case JSGC_HIGH_FREQUENCY_HEAP_GROWTH_MAX:
        return uint32_t(rt->gcHighFrequencyHeapGrowthMax * 100);
      case JSGC_HIGH_FREQUENCY_HEAP_GROWTH_MIN:
        return uint32_t(rt->gcHighFrequencyHeapGrowthMin * 100);
      case JSGC_LOW_FREQUENCY_HEAP_GROWTH:
        return uint32_t(rt->gcLowFrequencyHeapGrowth * 100);
      case JSGC_DYNAMIC_HEAP_GROWTH:
        return rt->gcDynamicHeapGrowth;
      case JSGC_DYNAMIC_MARK_SLICE:
        return rt->gcDynamicMarkSlice;
      case JSGC_ALLOCATION_THRESHOLD:
        return rt->gcAllocationThreshold / 1024 / 1024;
      default:
        JS_ASSERT(key == JSGC_NUMBER);
        return uint32_t(rt->gcNumber);
    }
}

JS_PUBLIC_API(void)
JS_SetGCParameterForThread(JSContext *cx, JSGCParamKey key, uint32_t value)
{
    JS_ASSERT(key == JSGC_MAX_CODE_CACHE_BYTES);
}

JS_PUBLIC_API(uint32_t)
JS_GetGCParameterForThread(JSContext *cx, JSGCParamKey key)
{
    JS_ASSERT(key == JSGC_MAX_CODE_CACHE_BYTES);
    return 0;
}

JS_PUBLIC_API(JSString *)
JS_NewExternalString(JSContext *cx, const jschar *chars, size_t length,
                     const JSStringFinalizer *fin)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    JSString *s = JSExternalString::new_(cx, chars, length, fin);
    return s;
}

extern JS_PUBLIC_API(bool)
JS_IsExternalString(JSString *str)
{
    return str->isExternal();
}

extern JS_PUBLIC_API(const JSStringFinalizer *)
JS_GetExternalStringFinalizer(JSString *str)
{
    return str->asExternal().externalFinalizer();
}

static void
SetNativeStackQuota(JSRuntime *rt, StackKind kind, size_t stackSize)
{
    rt->nativeStackQuota[kind] = stackSize;
    if (rt->nativeStackBase)
        RecomputeStackLimit(rt, kind);
}

void
js::RecomputeStackLimit(JSRuntime *rt, StackKind kind)
{
    size_t stackSize = rt->nativeStackQuota[kind];
#if JS_STACK_GROWTH_DIRECTION > 0
    if (stackSize == 0) {
        rt->mainThread.nativeStackLimit[kind] = UINTPTR_MAX;
    } else {
        JS_ASSERT(rt->nativeStackBase <= size_t(-1) - stackSize);
        rt->mainThread.nativeStackLimit[kind] =
          rt->nativeStackBase + stackSize - 1;
    }
#else
    if (stackSize == 0) {
        rt->mainThread.nativeStackLimit[kind] = 0;
    } else {
        JS_ASSERT(rt->nativeStackBase >= stackSize);
        rt->mainThread.nativeStackLimit[kind] =
          rt->nativeStackBase - (stackSize - 1);
    }
#endif

    
    
    
    
    
    
#ifdef JS_ION
    if (kind == StackForUntrustedScript) {
        JSRuntime::AutoLockForOperationCallback lock(rt);
        if (rt->mainThread.ionStackLimit != uintptr_t(-1))
            rt->mainThread.ionStackLimit = rt->mainThread.nativeStackLimit[kind];
    }
#endif
}

JS_PUBLIC_API(void)
JS_SetNativeStackQuota(JSRuntime *rt, size_t systemCodeStackSize,
                       size_t trustedScriptStackSize,
                       size_t untrustedScriptStackSize)
{
    JS_ASSERT_IF(trustedScriptStackSize,
                 trustedScriptStackSize < systemCodeStackSize);
    if (!trustedScriptStackSize)
        trustedScriptStackSize = systemCodeStackSize;
    JS_ASSERT_IF(untrustedScriptStackSize,
                 untrustedScriptStackSize < trustedScriptStackSize);
    if (!untrustedScriptStackSize)
        untrustedScriptStackSize = trustedScriptStackSize;
    SetNativeStackQuota(rt, StackForSystemCode, systemCodeStackSize);
    SetNativeStackQuota(rt, StackForTrustedScript, trustedScriptStackSize);
    SetNativeStackQuota(rt, StackForUntrustedScript, untrustedScriptStackSize);
}



JS_PUBLIC_API(int)
JS_IdArrayLength(JSContext *cx, JSIdArray *ida)
{
    return ida->length;
}

JS_PUBLIC_API(jsid)
JS_IdArrayGet(JSContext *cx, JSIdArray *ida, int index)
{
    JS_ASSERT(index >= 0 && index < ida->length);
    return ida->vector[index];
}

JS_PUBLIC_API(void)
JS_DestroyIdArray(JSContext *cx, JSIdArray *ida)
{
    cx->runtime()->defaultFreeOp()->free_(ida);
}

JS_PUBLIC_API(bool)
JS_ValueToId(JSContext *cx, jsval valueArg, jsid *idp)
{
    RootedValue value(cx, valueArg);
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, value);

    RootedId id(cx);
    if (!ValueToId<CanGC>(cx, value, &id))
        return false;

    *idp = id;
    return true;
}

JS_PUBLIC_API(bool)
JS_IdToValue(JSContext *cx, jsid id, jsval *vp)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    *vp = IdToJsval(id);
    assertSameCompartment(cx, *vp);
    return true;
}

JS_PUBLIC_API(bool)
JS_DefaultValue(JSContext *cx, JSObject *objArg, JSType hint, jsval *vp)
{
    RootedObject obj(cx, objArg);
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    JS_ASSERT(obj != nullptr);
    JS_ASSERT(hint == JSTYPE_VOID || hint == JSTYPE_STRING || hint == JSTYPE_NUMBER);

    RootedValue value(cx);
    if (!JSObject::defaultValue(cx, obj, hint, &value))
        return false;

    *vp = value;
    return true;
}

JS_PUBLIC_API(bool)
JS_PropertyStub(JSContext *cx, HandleObject obj, HandleId id, MutableHandleValue vp)
{
    return true;
}

JS_PUBLIC_API(bool)
JS_StrictPropertyStub(JSContext *cx, HandleObject obj, HandleId id, bool strict, MutableHandleValue vp)
{
    return true;
}

JS_PUBLIC_API(bool)
JS_DeletePropertyStub(JSContext *cx, HandleObject obj, HandleId id, bool *succeeded)
{
    *succeeded = true;
    return true;
}

JS_PUBLIC_API(bool)
JS_EnumerateStub(JSContext *cx, HandleObject obj)
{
    return true;
}

JS_PUBLIC_API(bool)
JS_ResolveStub(JSContext *cx, HandleObject obj, HandleId id)
{
    return true;
}

JS_PUBLIC_API(bool)
JS_ConvertStub(JSContext *cx, HandleObject obj, JSType type, MutableHandleValue vp)
{
    JS_ASSERT(type != JSTYPE_OBJECT && type != JSTYPE_FUNCTION);
    JS_ASSERT(obj);
    return DefaultValue(cx, obj, type, vp);
}

JS_PUBLIC_API(JSObject *)
JS_InitClass(JSContext *cx, JSObject *objArg, JSObject *parent_protoArg,
             const JSClass *clasp, JSNative constructor, unsigned nargs,
             const JSPropertySpec *ps, const JSFunctionSpec *fs,
             const JSPropertySpec *static_ps, const JSFunctionSpec *static_fs)
{
    RootedObject obj(cx, objArg);
    RootedObject parent_proto(cx, parent_protoArg);
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, obj, parent_proto);
    return js_InitClass(cx, obj, parent_proto, Valueify(clasp), constructor,
                        nargs, ps, fs, static_ps, static_fs);
}

JS_PUBLIC_API(bool)
JS_LinkConstructorAndPrototype(JSContext *cx, JSObject *ctorArg, JSObject *protoArg)
{
    RootedObject ctor(cx, ctorArg);
    RootedObject proto(cx, protoArg);
    return LinkConstructorAndPrototype(cx, ctor, proto);
}

JS_PUBLIC_API(const JSClass *)
JS_GetClass(JSObject *obj)
{
    return obj->getJSClass();
}

JS_PUBLIC_API(bool)
JS_InstanceOf(JSContext *cx, JSObject *objArg, const JSClass *clasp, jsval *argv)
{
    RootedObject obj(cx, objArg);
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
#ifdef DEBUG
    if (argv) {
        assertSameCompartment(cx, obj);
        assertSameCompartment(cx, JSValueArray(argv - 2, 2));
    }
#endif
    if (!obj || obj->getJSClass() != clasp) {
        if (argv)
            ReportIncompatibleMethod(cx, CallReceiverFromArgv(argv), Valueify(clasp));
        return false;
    }
    return true;
}

JS_PUBLIC_API(bool)
JS_HasInstance(JSContext *cx, JSObject *objArg, jsval valueArg, bool *bp)
{
    RootedObject obj(cx, objArg);
    RootedValue value(cx, valueArg);
    AssertHeapIsIdle(cx);
    assertSameCompartment(cx, obj, value);

    return HasInstance(cx, obj, value, bp);
}

JS_PUBLIC_API(void *)
JS_GetPrivate(JSObject *obj)
{
    
    return obj->getPrivate();
}

JS_PUBLIC_API(void)
JS_SetPrivate(JSObject *obj, void *data)
{
    
    obj->setPrivate(data);
}

JS_PUBLIC_API(void *)
JS_GetInstancePrivate(JSContext *cx, JSObject *objArg, const JSClass *clasp, jsval *argv)
{
    RootedObject obj(cx, objArg);
    if (!JS_InstanceOf(cx, obj, clasp, argv))
        return nullptr;
    return obj->getPrivate();
}

JS_PUBLIC_API(bool)
JS_GetPrototype(JSContext *cx, JS::Handle<JSObject*> obj, JS::MutableHandle<JSObject*> protop)
{
    return JSObject::getProto(cx, obj, protop);
}

JS_PUBLIC_API(bool)
JS_SetPrototype(JSContext *cx, JS::Handle<JSObject*> obj, JS::Handle<JSObject*> proto)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, obj, proto);

    return SetClassAndProto(cx, obj, obj->getClass(), proto, false);
}

JS_PUBLIC_API(JSObject *)
JS_GetParent(JSObject *obj)
{
    JS_ASSERT(!obj->is<ScopeObject>());
    return obj->getParent();
}

JS_PUBLIC_API(bool)
JS_SetParent(JSContext *cx, JSObject *objArg, JSObject *parentArg)
{
    RootedObject obj(cx, objArg);
    RootedObject parent(cx, parentArg);
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    JS_ASSERT(!obj->is<ScopeObject>());
    JS_ASSERT(parent || !obj->getParent());
    assertSameCompartment(cx, obj, parent);

    return JSObject::setParent(cx, obj, parent);
}

JS_PUBLIC_API(JSObject *)
JS_GetConstructor(JSContext *cx, JSObject *protoArg)
{
    RootedObject proto(cx, protoArg);
    RootedValue cval(cx);

    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, proto);
    {
        JSAutoResolveFlags rf(cx, 0);

        if (!JSObject::getProperty(cx, proto, proto, cx->names().constructor, &cval))
            return nullptr;
    }
    if (!IsFunctionObject(cval)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_NO_CONSTRUCTOR,
                             proto->getClass()->name);
        return nullptr;
    }
    return &cval.toObject();
}

JS_PUBLIC_API(bool)
JS_GetObjectId(JSContext *cx, JSObject *obj, jsid *idp)
{
    AssertHeapIsIdle(cx);
    assertSameCompartment(cx, obj);
    *idp = OBJECT_TO_JSID(obj);
    return true;
}

namespace {

class AutoHoldZone
{
  public:
    explicit AutoHoldZone(Zone *zone
                          MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : holdp(&zone->hold)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        *holdp = true;
    }

    ~AutoHoldZone() {
        *holdp = false;
    }

  private:
    bool *holdp;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

} 

JS::CompartmentOptions &
JS::CompartmentOptions::setZone(ZoneSpecifier spec)
{
    zone_.spec = spec;
    return *this;
}

JS::CompartmentOptions &
JS::CompartmentOptions::setSameZoneAs(JSObject *obj)
{
    zone_.pointer = static_cast<void *>(obj->zone());
    return *this;
}

JS_PUBLIC_API(JSObject *)
JS_NewGlobalObject(JSContext *cx, const JSClass *clasp, JSPrincipals *principals,
                   JS::OnNewGlobalHookOption hookOption,
                   const JS::CompartmentOptions &options )
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    JS_ASSERT(!cx->runtime()->isAtomsCompartment(cx->compartment()));
    JS_ASSERT(!cx->isExceptionPending());

    JSRuntime *rt = cx->runtime();

    Zone *zone;
    if (options.zoneSpecifier() == JS::SystemZone)
        zone = rt->systemZone;
    else if (options.zoneSpecifier() == JS::FreshZone)
        zone = nullptr;
    else
        zone = static_cast<Zone *>(options.zonePointer());

    JSCompartment *compartment = NewCompartment(cx, zone, principals, options);
    if (!compartment)
        return nullptr;

    
    if (!rt->systemZone && options.zoneSpecifier() == JS::SystemZone) {
        rt->systemZone = compartment->zone();
        rt->systemZone->isSystem = true;
    }

    AutoHoldZone hold(compartment->zone());

    Rooted<GlobalObject *> global(cx);
    {
        AutoCompartment ac(cx, compartment);
        global = GlobalObject::create(cx, Valueify(clasp));
    }

    if (!global)
        return nullptr;

    if (hookOption == JS::FireOnNewGlobalHook)
        JS_FireOnNewGlobalObject(cx, global);

    return global;
}

JS_PUBLIC_API(void)
JS_FireOnNewGlobalObject(JSContext *cx, JS::HandleObject global)
{
    
    
    
    
    Rooted<js::GlobalObject*> globalObject(cx, &global->as<GlobalObject>());
    Debugger::onNewGlobalObject(cx, globalObject);
}

JS_PUBLIC_API(JSObject *)
JS_NewObject(JSContext *cx, const JSClass *jsclasp, JSObject *protoArg, JSObject *parentArg)
{
    RootedObject proto(cx, protoArg);
    RootedObject parent(cx, parentArg);
    JS_ASSERT(!cx->runtime()->isAtomsCompartment(cx->compartment()));
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, proto, parent);

    const Class *clasp = Valueify(jsclasp);
    if (!clasp)
        clasp = &JSObject::class_;    

    JS_ASSERT(clasp != &JSFunction::class_);
    JS_ASSERT(!(clasp->flags & JSCLASS_IS_GLOBAL));

    JSObject *obj = NewObjectWithClassProto(cx, clasp, proto, parent);
    JS_ASSERT_IF(obj, obj->getParent());
    return obj;
}

JS_PUBLIC_API(JSObject *)
JS_NewObjectWithGivenProto(JSContext *cx, const JSClass *jsclasp, JSObject *protoArg, JSObject *parentArg)
{
    RootedObject proto(cx, protoArg);
    RootedObject parent(cx, parentArg);
    JS_ASSERT(!cx->runtime()->isAtomsCompartment(cx->compartment()));
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, proto, parent);

    const Class *clasp = Valueify(jsclasp);
    if (!clasp)
        clasp = &JSObject::class_;    

    JS_ASSERT(clasp != &JSFunction::class_);
    JS_ASSERT(!(clasp->flags & JSCLASS_IS_GLOBAL));

    JSObject *obj = NewObjectWithGivenProto(cx, clasp, proto, parent);
    if (obj)
        MarkTypeObjectUnknownProperties(cx, obj->type());
    return obj;
}

JS_PUBLIC_API(JSObject *)
JS_NewObjectForConstructor(JSContext *cx, const JSClass *clasp, const jsval *vp)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, *vp);

    RootedObject obj(cx, JSVAL_TO_OBJECT(*vp));
    return CreateThis(cx, Valueify(clasp), obj);
}

JS_PUBLIC_API(bool)
JS_IsExtensible(JSContext *cx, HandleObject obj, bool *extensible)
{
    bool isExtensible;
    if (!JSObject::isExtensible(cx, obj, &isExtensible))
        return false;
    *extensible = isExtensible;
    return true;
}

JS_PUBLIC_API(bool)
JS_IsNative(JSObject *obj)
{
    return obj->isNative();
}

JS_PUBLIC_API(JSRuntime *)
JS_GetObjectRuntime(JSObject *obj)
{
    return obj->compartment()->runtimeFromMainThread();
}

JS_PUBLIC_API(bool)
JS_FreezeObject(JSContext *cx, JSObject *objArg)
{
    RootedObject obj(cx, objArg);
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, obj);

    return JSObject::freeze(cx, obj);
}

JS_PUBLIC_API(bool)
JS_DeepFreezeObject(JSContext *cx, JSObject *objArg)
{
    RootedObject obj(cx, objArg);
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, obj);

    
    bool extensible;
    if (!JSObject::isExtensible(cx, obj, &extensible))
        return false;
    if (!extensible)
        return true;

    if (!JSObject::freeze(cx, obj))
        return false;

    
    for (uint32_t i = 0, n = obj->slotSpan(); i < n; ++i) {
        const Value &v = obj->getSlot(i);
        if (v.isPrimitive())
            continue;
        RootedObject obj(cx, &v.toObject());
        if (!JS_DeepFreezeObject(cx, obj))
            return false;
    }

    return true;
}

static bool
LookupPropertyById(JSContext *cx, HandleObject obj, HandleId id, unsigned flags,
                   MutableHandleObject objp, MutableHandleShape propp)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, obj, id);

    JSAutoResolveFlags rf(cx, flags);
    return JSObject::lookupGeneric(cx, obj, id, objp, propp);
}

#define AUTO_NAMELEN(s,n)   (((n) == (size_t)-1) ? js_strlen(s) : (n))

static bool
LookupResult(JSContext *cx, HandleObject obj, HandleObject obj2, HandleId id,
             HandleShape shape, MutableHandleValue vp)
{
    if (!shape) {
        
        vp.setUndefined();
        return true;
    }

    if (!obj2->isNative()) {
        if (obj2->is<ProxyObject>()) {
            Rooted<PropertyDescriptor> desc(cx);
            if (!Proxy::getPropertyDescriptor(cx, obj2, id, &desc, 0))
                return false;
            if (!desc.isShared()) {
                vp.set(desc.value());
                return true;
            }
        }
    } else if (IsImplicitDenseElement(shape)) {
        vp.set(obj2->getDenseElement(JSID_TO_INT(id)));
        return true;
    } else {
        
        if (shape->hasSlot()) {
            vp.set(obj2->nativeGetSlot(shape->slot()));
            return true;
        }
    }

    
    vp.setBoolean(true);
    return true;
}

JS_PUBLIC_API(bool)
JS_LookupPropertyById(JSContext *cx, JSObject *objArg, jsid idArg, MutableHandleValue vp)
{
    RootedId id(cx, idArg);
    RootedObject obj(cx, objArg);
    RootedObject obj2(cx);
    RootedShape prop(cx);

    return LookupPropertyById(cx, obj, id, 0, &obj2, &prop) &&
           LookupResult(cx, obj, obj2, id, prop, vp);
}

JS_PUBLIC_API(bool)
JS_LookupElement(JSContext *cx, JSObject *objArg, uint32_t index, MutableHandleValue vp)
{
    RootedObject obj(cx, objArg);
    CHECK_REQUEST(cx);
    RootedId id(cx);
    if (!IndexToId(cx, index, &id))
        return false;
    return JS_LookupPropertyById(cx, obj, id, vp);
}

JS_PUBLIC_API(bool)
JS_LookupProperty(JSContext *cx, JSObject *objArg, const char *name, MutableHandleValue vp)
{
    RootedObject obj(cx, objArg);
    JSAtom *atom = Atomize(cx, name, strlen(name));
    return atom && JS_LookupPropertyById(cx, obj, AtomToId(atom), vp);
}

JS_PUBLIC_API(bool)
JS_LookupUCProperty(JSContext *cx, JSObject *objArg, const jschar *name, size_t namelen,
                    MutableHandleValue vp)
{
    RootedObject obj(cx, objArg);
    JSAtom *atom = AtomizeChars<CanGC>(cx, name, AUTO_NAMELEN(name, namelen));
    return atom && JS_LookupPropertyById(cx, obj, AtomToId(atom), vp);
}

JS_PUBLIC_API(bool)
JS_LookupPropertyWithFlagsById(JSContext *cx, JSObject *objArg, jsid id_, unsigned flags,
                               JSObject **objpArg, MutableHandleValue vp)
{
    RootedObject obj(cx, objArg);
    RootedObject objp(cx, *objpArg);
    RootedId id(cx, id_);
    RootedShape prop(cx);

    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, obj, id);
    if (!(obj->isNative()
          ? LookupPropertyWithFlags(cx, obj, id, flags, &objp, &prop)
          : JSObject::lookupGeneric(cx, obj, id, &objp, &prop)))
        return false;

    if (!LookupResult(cx, obj, objp, id, prop, vp))
        return false;

    *objpArg = objp;
    return true;
}

JS_PUBLIC_API(bool)
JS_LookupPropertyWithFlags(JSContext *cx, JSObject *objArg, const char *name, unsigned flags,
                           MutableHandleValue vp)
{
    RootedObject obj(cx, objArg);
    JSObject *obj2;
    JSAtom *atom = Atomize(cx, name, strlen(name));
    return atom && JS_LookupPropertyWithFlagsById(cx, obj, AtomToId(atom), flags, &obj2, vp);
}

JS_PUBLIC_API(bool)
JS_HasPropertyById(JSContext *cx, JSObject *objArg, jsid idArg, bool *foundp)
{
    RootedObject obj(cx, objArg);
    RootedId id(cx, idArg);
    RootedObject obj2(cx);
    RootedShape prop(cx);
    bool ok = LookupPropertyById(cx, obj, id, 0, &obj2, &prop);
    *foundp = (prop != nullptr);
    return ok;
}

JS_PUBLIC_API(bool)
JS_HasElement(JSContext *cx, JSObject *objArg, uint32_t index, bool *foundp)
{
    RootedObject obj(cx, objArg);
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    RootedId id(cx);
    if (!IndexToId(cx, index, &id))
        return false;
    return JS_HasPropertyById(cx, obj, id, foundp);
}

JS_PUBLIC_API(bool)
JS_HasProperty(JSContext *cx, JSObject *objArg, const char *name, bool *foundp)
{
    RootedObject obj(cx, objArg);
    JSAtom *atom = Atomize(cx, name, strlen(name));
    return atom && JS_HasPropertyById(cx, obj, AtomToId(atom), foundp);
}

JS_PUBLIC_API(bool)
JS_HasUCProperty(JSContext *cx, JSObject *objArg, const jschar *name, size_t namelen, bool *foundp)
{
    RootedObject obj(cx, objArg);
    JSAtom *atom = AtomizeChars<CanGC>(cx, name, AUTO_NAMELEN(name, namelen));
    return atom && JS_HasPropertyById(cx, obj, AtomToId(atom), foundp);
}

JS_PUBLIC_API(bool)
JS_AlreadyHasOwnPropertyById(JSContext *cx, JSObject *objArg, jsid id_, bool *foundp)
{
    RootedObject obj(cx, objArg);
    RootedId id(cx, id_);
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, obj, id);

    if (!obj->isNative()) {
        RootedObject obj2(cx);
        RootedShape prop(cx);

        if (!LookupPropertyById(cx, obj, id, 0, &obj2, &prop))
            return false;
        *foundp = (obj == obj2);
        return true;
    }

    if (JSID_IS_INT(id) && obj->containsDenseElement(JSID_TO_INT(id))) {
        *foundp = true;
        return true;
    }

    *foundp = obj->nativeContains(cx, id);
    return true;
}

JS_PUBLIC_API(bool)
JS_AlreadyHasOwnElement(JSContext *cx, JSObject *objArg, uint32_t index, bool *foundp)
{
    RootedObject obj(cx, objArg);
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    RootedId id(cx);
    if (!IndexToId(cx, index, &id))
        return false;
    return JS_AlreadyHasOwnPropertyById(cx, obj, id, foundp);
}

JS_PUBLIC_API(bool)
JS_AlreadyHasOwnProperty(JSContext *cx, JSObject *objArg, const char *name, bool *foundp)
{
    RootedObject obj(cx, objArg);
    JSAtom *atom = Atomize(cx, name, strlen(name));
    return atom && JS_AlreadyHasOwnPropertyById(cx, obj, AtomToId(atom), foundp);
}

JS_PUBLIC_API(bool)
JS_AlreadyHasOwnUCProperty(JSContext *cx, JSObject *objArg, const jschar *name, size_t namelen,
                           bool *foundp)
{
    RootedObject obj(cx, objArg);
    JSAtom *atom = AtomizeChars<CanGC>(cx, name, AUTO_NAMELEN(name, namelen));
    return atom && JS_AlreadyHasOwnPropertyById(cx, obj, AtomToId(atom), foundp);
}




static JSPropertyOpWrapper
GetterWrapper(JSPropertyOp getter)
{
    JSPropertyOpWrapper ret;
    ret.op = getter;
    ret.info = nullptr;
    return ret;
}

static JSStrictPropertyOpWrapper
SetterWrapper(JSStrictPropertyOp setter)
{
    JSStrictPropertyOpWrapper ret;
    ret.op = setter;
    ret.info = nullptr;
    return ret;
}

static bool
DefinePropertyById(JSContext *cx, HandleObject obj, HandleId id, HandleValue value,
                   const JSPropertyOpWrapper &get, const JSStrictPropertyOpWrapper &set,
                   unsigned attrs, unsigned flags, int tinyid)
{
    PropertyOp getter = get.op;
    StrictPropertyOp setter = set.op;
    





    if (attrs & (JSPROP_GETTER | JSPROP_SETTER))
        attrs &= ~JSPROP_READONLY;

    





    if (attrs & JSPROP_NATIVE_ACCESSORS) {
        JS_ASSERT(!(attrs & (JSPROP_GETTER | JSPROP_SETTER)));
        JSFunction::Flags zeroFlags = JSAPIToJSFunctionFlags(0);
        
        
        RootedAtom atom(cx, JSID_IS_ATOM(id) ? JSID_TO_ATOM(id) : nullptr);
        attrs &= ~JSPROP_NATIVE_ACCESSORS;
        if (getter) {
            RootedObject global(cx, (JSObject*) &obj->global());
            JSFunction *getobj = NewFunction(cx, NullPtr(), (Native) getter, 0,
                                             zeroFlags, global, atom);
            if (!getobj)
                return false;

            if (get.info)
                getobj->setJitInfo(get.info);

            getter = JS_DATA_TO_FUNC_PTR(PropertyOp, getobj);
            attrs |= JSPROP_GETTER;
        }
        if (setter) {
            
            AutoRooterGetterSetter getRoot(cx, JSPROP_GETTER, &getter, nullptr);
            RootedObject global(cx, (JSObject*) &obj->global());
            JSFunction *setobj = NewFunction(cx, NullPtr(), (Native) setter, 1,
                                             zeroFlags, global, atom);
            if (!setobj)
                return false;

            if (set.info)
                setobj->setJitInfo(set.info);

            setter = JS_DATA_TO_FUNC_PTR(StrictPropertyOp, setobj);
            attrs |= JSPROP_SETTER;
        }
    }


    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, obj, id, value,
                            (attrs & JSPROP_GETTER)
                            ? JS_FUNC_TO_DATA_PTR(JSObject *, getter)
                            : nullptr,
                            (attrs & JSPROP_SETTER)
                            ? JS_FUNC_TO_DATA_PTR(JSObject *, setter)
                            : nullptr);

    JSAutoResolveFlags rf(cx, 0);
    if (flags != 0 && obj->isNative()) {
        return DefineNativeProperty(cx, obj, id, value, getter, setter,
                                    attrs, flags, tinyid);
    }
    return JSObject::defineGeneric(cx, obj, id, value, getter, setter, attrs);
}

JS_PUBLIC_API(bool)
JS_DefinePropertyById(JSContext *cx, JSObject *objArg, jsid idArg, jsval valueArg,
                      JSPropertyOp getter, JSStrictPropertyOp setter, unsigned attrs)
{
    RootedObject obj(cx, objArg);
    RootedId id(cx, idArg);
    RootedValue value(cx, valueArg);
    return DefinePropertyById(cx, obj, id, value, GetterWrapper(getter),
                              SetterWrapper(setter), attrs, 0, 0);
}

JS_PUBLIC_API(bool)
JS_DefineElement(JSContext *cx, JSObject *objArg, uint32_t index, jsval valueArg,
                 JSPropertyOp getter, JSStrictPropertyOp setter, unsigned attrs)
{
    RootedObject obj(cx, objArg);
    RootedValue value(cx, valueArg);
    AutoRooterGetterSetter gsRoot(cx, attrs, &getter, &setter);
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    RootedId id(cx);
    if (!IndexToId(cx, index, &id))
        return false;
    return DefinePropertyById(cx, obj, id, value, GetterWrapper(getter),
                              SetterWrapper(setter), attrs, 0, 0);
}

static bool
DefineProperty(JSContext *cx, HandleObject obj, const char *name, const Value &value_,
               const JSPropertyOpWrapper &getter, const JSStrictPropertyOpWrapper &setter,
               unsigned attrs, unsigned flags, int tinyid)
{
    RootedValue value(cx, value_);
    AutoRooterGetterSetter gsRoot(cx, attrs, const_cast<JSPropertyOp *>(&getter.op),
                                  const_cast<JSStrictPropertyOp *>(&setter.op));
    RootedId id(cx);

    if (attrs & JSPROP_INDEX) {
        id = INT_TO_JSID(intptr_t(name));
        attrs &= ~JSPROP_INDEX;
    } else {
        JSAtom *atom = Atomize(cx, name, strlen(name));
        if (!atom)
            return false;
        id = AtomToId(atom);
    }

    return DefinePropertyById(cx, obj, id, value, getter, setter, attrs, flags, tinyid);
}

JS_PUBLIC_API(bool)
JS_DefineProperty(JSContext *cx, JSObject *objArg, const char *name, jsval valueArg,
                  PropertyOp getter, JSStrictPropertyOp setter, unsigned attrs)
{
    RootedObject obj(cx, objArg);
    RootedValue value(cx, valueArg);
    return DefineProperty(cx, obj, name, value, GetterWrapper(getter),
                          SetterWrapper(setter), attrs, 0, 0);
}

JS_PUBLIC_API(bool)
JS_DefinePropertyWithTinyId(JSContext *cx, JSObject *objArg, const char *name, int8_t tinyid,
                            jsval valueArg, PropertyOp getter, JSStrictPropertyOp setter, unsigned attrs)
{
    RootedObject obj(cx, objArg);
    RootedValue value(cx, valueArg);
    return DefineProperty(cx, obj, name, value, GetterWrapper(getter),
                          SetterWrapper(setter), attrs, Shape::HAS_SHORTID, tinyid);
}

static bool
DefineUCProperty(JSContext *cx, HandleObject obj, const jschar *name, size_t namelen,
                 const Value &value_, PropertyOp getter, StrictPropertyOp setter, unsigned attrs,
                 unsigned flags, int tinyid)
{
    RootedValue value(cx, value_);
    AutoRooterGetterSetter gsRoot(cx, attrs, &getter, &setter);
    JSAtom *atom = AtomizeChars<CanGC>(cx, name, AUTO_NAMELEN(name, namelen));
    if (!atom)
        return false;
    RootedId id(cx, AtomToId(atom));
    return DefinePropertyById(cx, obj, id, value, GetterWrapper(getter),
                              SetterWrapper(setter), attrs, flags, tinyid);
}

JS_PUBLIC_API(bool)
JS_DefineUCProperty(JSContext *cx, JSObject *objArg, const jschar *name, size_t namelen,
                    jsval valueArg, JSPropertyOp getter, JSStrictPropertyOp setter, unsigned attrs)
{
    RootedObject obj(cx, objArg);
    RootedValue value(cx, valueArg);
    return DefineUCProperty(cx, obj, name, namelen, value, getter, setter, attrs, 0, 0);
}

JS_PUBLIC_API(bool)
JS_DefineUCPropertyWithTinyId(JSContext *cx, JSObject *objArg, const jschar *name, size_t namelen,
                              int8_t tinyid, jsval valueArg,
                              JSPropertyOp getter, JSStrictPropertyOp setter, unsigned attrs)
{
    RootedObject obj(cx, objArg);
    RootedValue value(cx, valueArg);
    return DefineUCProperty(cx, obj, name, namelen, value, getter, setter, attrs,
                            Shape::HAS_SHORTID, tinyid);
}

JS_PUBLIC_API(bool)
JS_DefineOwnProperty(JSContext *cx, JSObject *objArg, jsid idArg, jsval descriptorArg, bool *bp)
{
    RootedObject obj(cx, objArg);
    RootedId id(cx, idArg);
    RootedValue descriptor(cx, descriptorArg);
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, obj, id, descriptor);

    return DefineOwnProperty(cx, obj, id, descriptor, bp);
}

JS_PUBLIC_API(JSObject *)
JS_DefineObject(JSContext *cx, JSObject *objArg, const char *name, const JSClass *jsclasp,
                JSObject *protoArg, unsigned attrs)
{
    RootedObject obj(cx, objArg);
    RootedObject proto(cx, protoArg);
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, obj, proto);

    const Class *clasp = Valueify(jsclasp);
    if (!clasp)
        clasp = &JSObject::class_;    

    RootedObject nobj(cx, NewObjectWithClassProto(cx, clasp, proto, obj));
    if (!nobj)
        return nullptr;

    if (!DefineProperty(cx, obj, name, ObjectValue(*nobj), GetterWrapper(nullptr),
                        SetterWrapper(nullptr), attrs, 0, 0))
    {
        return nullptr;
    }

    return nobj;
}

JS_PUBLIC_API(bool)
JS_DefineConstDoubles(JSContext *cx, JSObject *objArg, const JSConstDoubleSpec *cds)
{
    RootedObject obj(cx, objArg);
    bool ok;
    unsigned attrs;

    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    JSPropertyOpWrapper noget = GetterWrapper(nullptr);
    JSStrictPropertyOpWrapper noset = SetterWrapper(nullptr);
    for (ok = true; cds->name; cds++) {
        Value value = DoubleValue(cds->dval);
        attrs = cds->flags;
        if (!attrs)
            attrs = JSPROP_READONLY | JSPROP_PERMANENT;
        ok = DefineProperty(cx, obj, cds->name, value, noget, noset, attrs, 0, 0);
        if (!ok)
            break;
    }
    return ok;
}

JS_PUBLIC_API(bool)
JS_DefineProperties(JSContext *cx, JSObject *objArg, const JSPropertySpec *ps)
{
    RootedObject obj(cx, objArg);
    bool ok;
    for (ok = true; ps->name; ps++) {
        ok = DefineProperty(cx, obj, ps->name, UndefinedValue(), ps->getter, ps->setter,
                            ps->flags, Shape::HAS_SHORTID, ps->tinyid);
        if (!ok)
            break;
    }
    return ok;
}

static bool
GetPropertyDescriptorById(JSContext *cx, HandleObject obj, HandleId id, unsigned flags,
                          bool own, MutableHandle<PropertyDescriptor> desc)
{
    RootedObject obj2(cx);
    RootedShape shape(cx);

    if (!LookupPropertyById(cx, obj, id, flags, &obj2, &shape))
        return false;

    desc.clear();
    if (!shape || (own && obj != obj2))
        return true;

    desc.object().set(obj2);
    if (obj2->isNative()) {
        if (IsImplicitDenseElement(shape)) {
            desc.setEnumerable();
            desc.value().set(obj2->getDenseElement(JSID_TO_INT(id)));
        } else {
            desc.setAttributes(shape->attributes());
            desc.setGetter(shape->getter());
            desc.setSetter(shape->setter());
            JS_ASSERT(desc.value().isUndefined());
            if (shape->hasSlot())
                desc.value().set(obj2->nativeGetSlot(shape->slot()));
        }
    } else {
        if (obj2->is<ProxyObject>()) {
            JSAutoResolveFlags rf(cx, flags);
            return own
                   ? Proxy::getOwnPropertyDescriptor(cx, obj2, id, desc, 0)
                   : Proxy::getPropertyDescriptor(cx, obj2, id, desc, 0);
        }
        if (!JSObject::getGenericAttributes(cx, obj2, id, &desc.attributesRef()))
            return false;
        JS_ASSERT(desc.getter() == nullptr);
        JS_ASSERT(desc.setter() == nullptr);
        JS_ASSERT(desc.value().isUndefined());
    }
    return true;
}

JS_PUBLIC_API(bool)
JS_GetOwnPropertyDescriptorById(JSContext *cx, JSObject *objArg, jsid idArg, unsigned flags,
                                MutableHandle<JSPropertyDescriptor> desc)
{
    RootedObject obj(cx, objArg);
    RootedId id(cx, idArg);
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);

    return GetPropertyDescriptorById(cx, obj, id, flags, true, desc);
}

JS_PUBLIC_API(bool)
JS_GetOwnPropertyDescriptor(JSContext *cx, JSObject *objArg, const char *name, unsigned flags,
                            MutableHandle<JSPropertyDescriptor> desc)
{
    RootedObject obj(cx, objArg);
    JSAtom *atom = Atomize(cx, name, strlen(name));
    return atom && JS_GetOwnPropertyDescriptorById(cx, obj, AtomToId(atom), flags, desc);
}

JS_PUBLIC_API(bool)
JS_GetPropertyDescriptorById(JSContext *cx, JSObject *objArg, jsid idArg, unsigned flags,
                             MutableHandle<JSPropertyDescriptor> desc)
{
    RootedObject obj(cx, objArg);
    RootedId id(cx, idArg);
    return GetPropertyDescriptorById(cx, obj, id, flags, false, desc);
}

JS_PUBLIC_API(bool)
JS_GetPropertyDescriptor(JSContext *cx, JSObject *objArg, const char *name, unsigned flags,
                         MutableHandle<JSPropertyDescriptor> desc)
{
    RootedObject obj(cx, objArg);
    JSAtom *atom = Atomize(cx, name, strlen(name));
    return atom && JS_GetPropertyDescriptorById(cx, obj, AtomToId(atom), flags, desc);
}

JS_PUBLIC_API(bool)
JS_GetPropertyById(JSContext *cx, JSObject *objArg, jsid idArg, MutableHandleValue vp)
{
    return JS_ForwardGetPropertyTo(cx, objArg, idArg, objArg, vp);
}

JS_PUBLIC_API(bool)
JS_ForwardGetPropertyTo(JSContext *cx, JSObject *objArg, jsid idArg, JSObject *onBehalfOfArg,
                        MutableHandleValue vp)
{
    RootedObject obj(cx, objArg);
    RootedObject onBehalfOf(cx, onBehalfOfArg);
    RootedId id(cx, idArg);

    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, obj, id);
    assertSameCompartment(cx, onBehalfOf);
    JSAutoResolveFlags rf(cx, 0);

    return JSObject::getGeneric(cx, obj, onBehalfOf, id, vp);
}

JS_PUBLIC_API(bool)
JS_GetElement(JSContext *cx, JSObject *objArg, uint32_t index, MutableHandleValue vp)
{
    return JS_ForwardGetElementTo(cx, objArg, index, objArg, vp);
}

JS_PUBLIC_API(bool)
JS_ForwardGetElementTo(JSContext *cx, JSObject *objArg, uint32_t index, JSObject *onBehalfOfArg,
                       MutableHandleValue vp)
{
    RootedObject obj(cx, objArg);
    RootedObject onBehalfOf(cx, onBehalfOfArg);
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, obj);
    JSAutoResolveFlags rf(cx, 0);

    return JSObject::getElement(cx, obj, onBehalfOf, index, vp);
}

JS_PUBLIC_API(bool)
JS_GetElementIfPresent(JSContext *cx, JSObject *objArg, uint32_t index, JSObject *onBehalfOfArg,
                       MutableHandleValue vp, bool* present)
{
    RootedObject obj(cx, objArg);
    RootedObject onBehalfOf(cx, onBehalfOfArg);
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, obj);
    JSAutoResolveFlags rf(cx, 0);

    bool isPresent;
    if (!JSObject::getElementIfPresent(cx, obj, onBehalfOf, index, vp, &isPresent))
        return false;

    *present = isPresent;
    return true;
}

JS_PUBLIC_API(bool)
JS_GetProperty(JSContext *cx, JSObject *objArg, const char *name, MutableHandleValue vp)
{
    RootedObject obj(cx, objArg);
    JSAtom *atom = Atomize(cx, name, strlen(name));
    return atom && JS_GetPropertyById(cx, obj, AtomToId(atom), vp);
}

JS_PUBLIC_API(bool)
JS_GetUCProperty(JSContext *cx, JSObject *objArg, const jschar *name, size_t namelen,
                 MutableHandleValue vp)
{
    RootedObject obj(cx, objArg);
    JSAtom *atom = AtomizeChars<CanGC>(cx, name, AUTO_NAMELEN(name, namelen));
    return atom && JS_GetPropertyById(cx, obj, AtomToId(atom), vp);
}

JS_PUBLIC_API(bool)
JS_SetPropertyById(JSContext *cx, JSObject *objArg, jsid idArg, HandleValue v)
{
    RootedObject obj(cx, objArg);
    RootedId id(cx, idArg);
    RootedValue value(cx, v);
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, obj, id);
    JSAutoResolveFlags rf(cx, JSRESOLVE_ASSIGNING);

    return JSObject::setGeneric(cx, obj, obj, id, &value, false);
}

JS_PUBLIC_API(bool)
JS_SetElement(JSContext *cx, JSObject *objArg, uint32_t index, MutableHandleValue vp)
{
    RootedObject obj(cx, objArg);
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, obj, vp);
    JSAutoResolveFlags rf(cx, JSRESOLVE_ASSIGNING);

    return JSObject::setElement(cx, obj, obj, index, vp, false);
}

JS_PUBLIC_API(bool)
JS_SetProperty(JSContext *cx, JSObject *objArg, const char *name, HandleValue v)
{
    RootedObject obj(cx, objArg);
    JSAtom *atom = Atomize(cx, name, strlen(name));
    return atom && JS_SetPropertyById(cx, obj, AtomToId(atom), v);
}

JS_PUBLIC_API(bool)
JS_SetUCProperty(JSContext *cx, JSObject *objArg, const jschar *name, size_t namelen,
                 HandleValue v)
{
    RootedObject obj(cx, objArg);
    JSAtom *atom = AtomizeChars<CanGC>(cx, name, AUTO_NAMELEN(name, namelen));
    return atom && JS_SetPropertyById(cx, obj, AtomToId(atom), v);
}

JS_PUBLIC_API(bool)
JS_DeletePropertyById2(JSContext *cx, JSObject *objArg, jsid id, bool *result)
{
    RootedObject obj(cx, objArg);
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, obj, id);
    JSAutoResolveFlags rf(cx, 0);

    RootedValue value(cx);

    bool succeeded;
    if (JSID_IS_SPECIAL(id)) {
        Rooted<SpecialId> sid(cx, JSID_TO_SPECIALID(id));
        if (!JSObject::deleteSpecial(cx, obj, sid, &succeeded))
            return false;
    } else {
        if (!JSObject::deleteByValue(cx, obj, IdToValue(id), &succeeded))
            return false;
    }

    *result = !!succeeded;
    return true;
}

JS_PUBLIC_API(bool)
JS_DeleteElement2(JSContext *cx, JSObject *objArg, uint32_t index, bool *result)
{
    RootedObject obj(cx, objArg);
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, obj);
    JSAutoResolveFlags rf(cx, 0);

    bool succeeded;
    if (!JSObject::deleteElement(cx, obj, index, &succeeded))
        return false;

    *result = !!succeeded;
    return true;
}

JS_PUBLIC_API(bool)
JS_DeleteProperty2(JSContext *cx, JSObject *objArg, const char *name, bool *result)
{
    RootedObject obj(cx, objArg);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, obj);
    JSAutoResolveFlags rf(cx, 0);

    JSAtom *atom = Atomize(cx, name, strlen(name));
    if (!atom)
        return false;

    bool succeeded;
    if (!JSObject::deleteByValue(cx, obj, StringValue(atom), &succeeded))
        return false;

    *result = !!succeeded;
    return true;
}

JS_PUBLIC_API(bool)
JS_DeleteUCProperty2(JSContext *cx, JSObject *objArg, const jschar *name, size_t namelen,
                     bool *result)
{
    RootedObject obj(cx, objArg);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, obj);
    JSAutoResolveFlags rf(cx, 0);

    JSAtom *atom = AtomizeChars<CanGC>(cx, name, AUTO_NAMELEN(name, namelen));
    if (!atom)
        return false;

    bool succeeded;
    if (!JSObject::deleteByValue(cx, obj, StringValue(atom), &succeeded))
        return false;

    *result = !!succeeded;
    return true;
}

JS_PUBLIC_API(bool)
JS_DeletePropertyById(JSContext *cx, JSObject *objArg, jsid idArg)
{
    bool junk;
    return JS_DeletePropertyById2(cx, objArg, idArg, &junk);
}

JS_PUBLIC_API(bool)
JS_DeleteElement(JSContext *cx, JSObject *objArg, uint32_t index)
{
    bool junk;
    return JS_DeleteElement2(cx, objArg, index, &junk);
}

JS_PUBLIC_API(bool)
JS_DeleteProperty(JSContext *cx, JSObject *objArg, const char *name)
{
    bool junk;
    return JS_DeleteProperty2(cx, objArg, name, &junk);
}

static Shape *
LastConfigurableShape(JSObject *obj)
{
    for (Shape::Range<NoGC> r(obj->lastProperty()); !r.empty(); r.popFront()) {
        Shape *shape = &r.front();
        if (shape->configurable())
            return shape;
    }
    return nullptr;
}

JS_PUBLIC_API(void)
JS_ClearNonGlobalObject(JSContext *cx, JSObject *objArg)
{
    RootedObject obj(cx, objArg);
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, obj);

    JS_ASSERT(!obj->is<GlobalObject>());

    if (!obj->isNative())
        return;

    
    RootedShape shape(cx);
    while ((shape = LastConfigurableShape(obj))) {
        if (!obj->removeProperty(cx, shape->propid()))
            return;
    }

    
    for (Shape::Range<NoGC> r(obj->lastProperty()); !r.empty(); r.popFront()) {
        Shape *shape = &r.front();
        if (shape->isDataDescriptor() &&
            shape->writable() &&
            shape->hasDefaultSetter() &&
            shape->hasSlot())
        {
            obj->nativeSetSlot(shape->slot(), UndefinedValue());
        }
    }
}

JS_PUBLIC_API(void)
JS_SetAllNonReservedSlotsToUndefined(JSContext *cx, JSObject *objArg)
{
    RootedObject obj(cx, objArg);
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, obj);

    if (!obj->isNative())
        return;

    const Class *clasp = obj->getClass();
    unsigned numReserved = JSCLASS_RESERVED_SLOTS(clasp);
    unsigned numSlots = obj->slotSpan();
    for (unsigned i = numReserved; i < numSlots; i++)
        obj->setSlot(i, UndefinedValue());
}

JS_PUBLIC_API(JSIdArray *)
JS_Enumerate(JSContext *cx, JSObject *objArg)
{
    RootedObject obj(cx, objArg);
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, obj);

    AutoIdVector props(cx);
    JSIdArray *ida;
    if (!GetPropertyNames(cx, obj, JSITER_OWNONLY, &props) || !VectorToIdArray(cx, props, &ida))
        return nullptr;
    return ida;
}








static const uint32_t JSSLOT_ITER_INDEX = 0;

static void
prop_iter_finalize(FreeOp *fop, JSObject *obj)
{
    void *pdata = obj->getPrivate();
    if (!pdata)
        return;

    if (obj->getSlot(JSSLOT_ITER_INDEX).toInt32() >= 0) {
        
        JSIdArray *ida = (JSIdArray *) pdata;
        fop->free_(ida);
    }
}

static void
prop_iter_trace(JSTracer *trc, JSObject *obj)
{
    void *pdata = obj->getPrivate();
    if (!pdata)
        return;

    if (obj->getSlot(JSSLOT_ITER_INDEX).toInt32() < 0) {
        




        Shape *tmp = static_cast<Shape *>(pdata);
        MarkShapeUnbarriered(trc, &tmp, "prop iter shape");
        obj->setPrivateUnbarriered(tmp);
    } else {
        
        JSIdArray *ida = (JSIdArray *) pdata;
        MarkIdRange(trc, ida->length, ida->vector, "prop iter");
    }
}

static const Class prop_iter_class = {
    "PropertyIterator",
    JSCLASS_HAS_PRIVATE | JSCLASS_IMPLEMENTS_BARRIERS | JSCLASS_HAS_RESERVED_SLOTS(1),
    JS_PropertyStub,         
    JS_DeletePropertyStub,   
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    prop_iter_finalize,
    nullptr,        
    nullptr,        
    nullptr,        
    nullptr,        
    prop_iter_trace
};

JS_PUBLIC_API(JSObject *)
JS_NewPropertyIterator(JSContext *cx, JSObject *objArg)
{
    RootedObject obj(cx, objArg);

    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, obj);

    RootedObject iterobj(cx, NewObjectWithClassProto(cx, &prop_iter_class, nullptr, obj));
    if (!iterobj)
        return nullptr;

    int index;
    if (obj->isNative()) {
        
        iterobj->setPrivateGCThing(obj->lastProperty());
        index = -1;
    } else {
        
        JSIdArray *ida = JS_Enumerate(cx, obj);
        if (!ida)
            return nullptr;
        iterobj->setPrivate((void *)ida);
        index = ida->length;
    }

    
    iterobj->setSlot(JSSLOT_ITER_INDEX, Int32Value(index));
    return iterobj;
}

JS_PUBLIC_API(bool)
JS_NextProperty(JSContext *cx, JSObject *iterobjArg, jsid *idp)
{
    RootedObject iterobj(cx, iterobjArg);

    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, iterobj);
    int32_t i = iterobj->getSlot(JSSLOT_ITER_INDEX).toInt32();
    if (i < 0) {
        
        JS_ASSERT(iterobj->getParent()->isNative());
        Shape *shape = static_cast<Shape *>(iterobj->getPrivate());

        while (shape->previous() && !shape->enumerable())
            shape = shape->previous();

        if (!shape->previous()) {
            JS_ASSERT(shape->isEmptyShape());
            *idp = JSID_VOID;
        } else {
            iterobj->setPrivateGCThing(const_cast<Shape *>(shape->previous().get()));
            *idp = shape->propid();
        }
    } else {
        
        JSIdArray *ida = (JSIdArray *) iterobj->getPrivate();
        JS_ASSERT(i <= ida->length);
        STATIC_ASSUME(i <= ida->length);
        if (i == 0) {
            *idp = JSID_VOID;
        } else {
            *idp = ida->vector[--i];
            iterobj->setSlot(JSSLOT_ITER_INDEX, Int32Value(i));
        }
    }
    return true;
}

JS_PUBLIC_API(bool)
JS_ArrayIterator(JSContext *cx, unsigned argc, jsval *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    Rooted<Value> target(cx, args.thisv());
    AssertHeapIsIdle(cx);
    assertSameCompartment(cx, target);
    CHECK_REQUEST(cx);

    JSObject *iterobj = ElementIteratorObject::create(cx, target);
    if (!iterobj)
        return false;
    vp->setObject(*iterobj);
    return true;
}

JS_PUBLIC_API(jsval)
JS_GetReservedSlot(JSObject *obj, uint32_t index)
{
    return obj->getReservedSlot(index);
}

JS_PUBLIC_API(void)
JS_SetReservedSlot(JSObject *obj, uint32_t index, Value value)
{
    obj->setReservedSlot(index, value);
}

JS_PUBLIC_API(JSObject *)
JS_NewArrayObject(JSContext *cx, int length, jsval *vector)
{
    AutoArrayRooter tvr(cx, length, vector);

    JS_ASSERT(!cx->runtime()->isAtomsCompartment(cx->compartment()));
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);

    assertSameCompartment(cx, JSValueArray(vector, vector ? (uint32_t)length : 0));
    return NewDenseCopiedArray(cx, (uint32_t)length, vector);
}

JS_PUBLIC_API(bool)
JS_IsArrayObject(JSContext *cx, JSObject *objArg)
{
    RootedObject obj(cx, objArg);
    assertSameCompartment(cx, obj);
    return ObjectClassIs(obj, ESClass_Array, cx);
}

JS_PUBLIC_API(bool)
JS_GetArrayLength(JSContext *cx, JSObject *objArg, uint32_t *lengthp)
{
    RootedObject obj(cx, objArg);
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, obj);
    return GetLengthProperty(cx, obj, lengthp);
}

JS_PUBLIC_API(bool)
JS_SetArrayLength(JSContext *cx, JSObject *objArg, uint32_t length)
{
    RootedObject obj(cx, objArg);
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, obj);
    return SetLengthProperty(cx, obj, length);
}

JS_PUBLIC_API(bool)
JS_CheckAccess(JSContext *cx, JSObject *objArg, jsid idArg, JSAccessMode mode,
               jsval *vp, unsigned *attrsp)
{
    RootedObject obj(cx, objArg);
    RootedId id(cx, idArg);
    RootedValue value(cx, *vp);

    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, obj, id);
    bool status = CheckAccess(cx, obj, id, mode, &value, attrsp);
    *vp = value;
    return status;
}

JS_PUBLIC_API(void)
JS_HoldPrincipals(JSPrincipals *principals)
{
    JS_ATOMIC_INCREMENT(&principals->refcount);
}

JS_PUBLIC_API(void)
JS_DropPrincipals(JSRuntime *rt, JSPrincipals *principals)
{
    int rc = JS_ATOMIC_DECREMENT(&principals->refcount);
    if (rc == 0)
        rt->destroyPrincipals(principals);
}

JS_PUBLIC_API(void)
JS_SetSecurityCallbacks(JSRuntime *rt, const JSSecurityCallbacks *scb)
{
    JS_ASSERT(scb != &NullSecurityCallbacks);
    rt->securityCallbacks = scb ? scb : &NullSecurityCallbacks;
}

JS_PUBLIC_API(const JSSecurityCallbacks *)
JS_GetSecurityCallbacks(JSRuntime *rt)
{
    return (rt->securityCallbacks != &NullSecurityCallbacks) ? rt->securityCallbacks : nullptr;
}

JS_PUBLIC_API(void)
JS_SetTrustedPrincipals(JSRuntime *rt, const JSPrincipals *prin)
{
    rt->setTrustedPrincipals(prin);
}

extern JS_PUBLIC_API(void)
JS_InitDestroyPrincipalsCallback(JSRuntime *rt, JSDestroyPrincipalsOp destroyPrincipals)
{
    JS_ASSERT(destroyPrincipals);
    JS_ASSERT(!rt->destroyPrincipals);
    rt->destroyPrincipals = destroyPrincipals;
}

JS_PUBLIC_API(JSFunction *)
JS_NewFunction(JSContext *cx, JSNative native, unsigned nargs, unsigned flags,
               JSObject *parentArg, const char *name)
{
    RootedObject parent(cx, parentArg);
    JS_ASSERT(!cx->runtime()->isAtomsCompartment(cx->compartment()));

    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, parent);

    RootedAtom atom(cx);
    if (name) {
        atom = Atomize(cx, name, strlen(name));
        if (!atom)
            return nullptr;
    }

    JSFunction::Flags funFlags = JSAPIToJSFunctionFlags(flags);
    return NewFunction(cx, NullPtr(), native, nargs, funFlags, parent, atom);
}

JS_PUBLIC_API(JSFunction *)
JS_NewFunctionById(JSContext *cx, JSNative native, unsigned nargs, unsigned flags, JSObject *parentArg,
                   jsid id)
{
    RootedObject parent(cx, parentArg);
    JS_ASSERT(JSID_IS_STRING(id));
    JS_ASSERT(!cx->runtime()->isAtomsCompartment(cx->compartment()));
    JS_ASSERT(native);
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, parent);

    RootedAtom name(cx, JSID_TO_ATOM(id));
    JSFunction::Flags funFlags = JSAPIToJSFunctionFlags(flags);
    return NewFunction(cx, NullPtr(), native, nargs, funFlags, parent, name);
}

JS_PUBLIC_API(JSFunction *)
JS::GetSelfHostedFunction(JSContext *cx, const char *selfHostedName, jsid id, unsigned nargs)
{
    JS_ASSERT(JSID_IS_STRING(id));
    JS_ASSERT(!cx->runtime()->isAtomsCompartment(cx->compartment()));
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);

    RootedAtom name(cx, JSID_TO_ATOM(id));
    RootedAtom shName(cx, Atomize(cx, selfHostedName, strlen(selfHostedName)));
    if (!shName)
        return nullptr;
    RootedValue funVal(cx);
    if (!cx->global()->getSelfHostedFunction(cx, shName, name, nargs, &funVal))
        return nullptr;
    return &funVal.toObject().as<JSFunction>();
}

JS_PUBLIC_API(JSObject *)
JS_CloneFunctionObject(JSContext *cx, JSObject *funobjArg, JSObject *parentArg)
{
    RootedObject funobj(cx, funobjArg);
    RootedObject parent(cx, parentArg);
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, parent);
    

    if (!parent)
        parent = cx->global();

    if (!funobj->is<JSFunction>()) {
        AutoCompartment ac(cx, funobj);
        ReportIsNotFunction(cx, ObjectValue(*funobj));
        return nullptr;
    }

    



    RootedFunction fun(cx, &funobj->as<JSFunction>());
    if (fun->isInterpretedLazy()) {
        AutoCompartment ac(cx, funobj);
        if (!fun->getOrCreateScript(cx))
            return nullptr;
    }
    if (fun->isInterpreted() && (fun->nonLazyScript()->enclosingStaticScope() ||
        (fun->nonLazyScript()->compileAndGo && !parent->is<GlobalObject>())))
    {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_BAD_CLONE_FUNOBJ_SCOPE);
        return nullptr;
    }

    if (fun->isBoundFunction()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_CANT_CLONE_OBJECT);
        return nullptr;
    }

    if (fun->isNative() && IsAsmJSModuleNative(fun->native())) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_CANT_CLONE_OBJECT);
        return nullptr;
    }

    return CloneFunctionObject(cx, fun, parent, fun->getAllocKind());
}

JS_PUBLIC_API(JSObject *)
JS_GetFunctionObject(JSFunction *fun)
{
    return fun;
}

JS_PUBLIC_API(JSString *)
JS_GetFunctionId(JSFunction *fun)
{
    return fun->atom();
}

JS_PUBLIC_API(JSString *)
JS_GetFunctionDisplayId(JSFunction *fun)
{
    return fun->displayAtom();
}

JS_PUBLIC_API(uint16_t)
JS_GetFunctionArity(JSFunction *fun)
{
    return fun->nargs;
}

JS_PUBLIC_API(bool)
JS_ObjectIsFunction(JSContext *cx, JSObject *obj)
{
    return obj->is<JSFunction>();
}

JS_PUBLIC_API(bool)
JS_ObjectIsCallable(JSContext *cx, JSObject *obj)
{
    return obj->isCallable();
}

JS_PUBLIC_API(bool)
JS_IsNativeFunction(JSObject *funobj, JSNative call)
{
    if (!funobj->is<JSFunction>())
        return false;
    JSFunction *fun = &funobj->as<JSFunction>();
    return fun->isNative() && fun->native() == call;
}

extern JS_PUBLIC_API(bool)
JS_IsConstructor(JSFunction *fun)
{
    return fun->isNativeConstructor() || fun->isInterpretedConstructor();
}

JS_PUBLIC_API(JSObject*)
JS_BindCallable(JSContext *cx, JSObject *targetArg, JSObject *newThis)
{
    RootedObject target(cx, targetArg);
    RootedValue thisArg(cx, ObjectValue(*newThis));
    return js_fun_bind(cx, target, thisArg, nullptr, 0);
}

static bool
js_generic_native_method_dispatcher(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    const JSFunctionSpec *fs = (JSFunctionSpec *)
        vp->toObject().as<JSFunction>().getExtendedSlot(0).toPrivate();
    JS_ASSERT((fs->flags & JSFUN_GENERIC_NATIVE) != 0);

    if (argc < 1) {
        js_ReportMissingArg(cx, args.calleev(), 0);
        return false;
    }

    





    memmove(vp + 1, vp + 2, argc * sizeof(jsval));

    
    vp[2 + --argc].setUndefined();

    return fs->call.op(cx, argc, vp);
}

JS_PUBLIC_API(bool)
JS_DefineFunctions(JSContext *cx, JSObject *objArg, const JSFunctionSpec *fs)
{
    JS_ASSERT(!cx->runtime()->isAtomsCompartment(cx->compartment()));
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, objArg);

    RootedObject obj(cx, objArg);
    RootedObject ctor(cx);

    for (; fs->name; fs++) {
        RootedAtom atom(cx);
        
        if (fs->name[0] != '@' || fs->name[1] != '@')
            atom = Atomize(cx, fs->name, strlen(fs->name));
        else if (strcmp(fs->name, "@@iterator") == 0)
            
            atom = cx->names().std_iterator;
        else
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_BAD_SYMBOL, fs->name);
        if (!atom)
            return false;

        Rooted<jsid> id(cx, AtomToId(atom));

        



        unsigned flags = fs->flags;
        if (flags & JSFUN_GENERIC_NATIVE) {
            if (!ctor) {
                ctor = JS_GetConstructor(cx, obj);
                if (!ctor)
                    return false;
            }

            flags &= ~JSFUN_GENERIC_NATIVE;
            JSFunction *fun = DefineFunction(cx, ctor, id,
                                             js_generic_native_method_dispatcher,
                                             fs->nargs + 1, flags,
                                             JSFunction::ExtendedFinalizeKind);
            if (!fun)
                return false;

            



            fun->setExtendedSlot(0, PrivateValue(const_cast<JSFunctionSpec*>(fs)));
        }

        





        if (fs->selfHostedName) {
            






            JS_ASSERT(!fs->call.op);
            JS_ASSERT(!fs->call.info);
            if (cx->runtime()->isSelfHostingGlobal(cx->global()))
                continue;

            RootedAtom shName(cx, Atomize(cx, fs->selfHostedName, strlen(fs->selfHostedName)));
            if (!shName)
                return false;
            RootedValue funVal(cx);
            if (!cx->global()->getSelfHostedFunction(cx, shName, atom, fs->nargs, &funVal))
                return false;
            if (!JSObject::defineGeneric(cx, obj, id, funVal, nullptr, nullptr, flags))
                return false;
        } else {
            JSFunction *fun = DefineFunction(cx, obj, id, fs->call.op, fs->nargs, flags);
            if (!fun)
                return false;
            if (fs->call.info)
                fun->setJitInfo(fs->call.info);
        }
    }
    return true;
}

JS_PUBLIC_API(JSFunction *)
JS_DefineFunction(JSContext *cx, JSObject *objArg, const char *name, JSNative call,
                  unsigned nargs, unsigned attrs)
{
    RootedObject obj(cx, objArg);
    JS_ASSERT(!cx->runtime()->isAtomsCompartment(cx->compartment()));
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, obj);
    JSAtom *atom = Atomize(cx, name, strlen(name));
    if (!atom)
        return nullptr;
    Rooted<jsid> id(cx, AtomToId(atom));
    return DefineFunction(cx, obj, id, call, nargs, attrs);
}

JS_PUBLIC_API(JSFunction *)
JS_DefineUCFunction(JSContext *cx, JSObject *objArg,
                    const jschar *name, size_t namelen, JSNative call,
                    unsigned nargs, unsigned attrs)
{
    RootedObject obj(cx, objArg);
    JS_ASSERT(!cx->runtime()->isAtomsCompartment(cx->compartment()));
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, obj);
    JSAtom *atom = AtomizeChars<CanGC>(cx, name, AUTO_NAMELEN(name, namelen));
    if (!atom)
        return nullptr;
    Rooted<jsid> id(cx, AtomToId(atom));
    return DefineFunction(cx, obj, id, call, nargs, attrs);
}

extern JS_PUBLIC_API(JSFunction *)
JS_DefineFunctionById(JSContext *cx, JSObject *objArg, jsid id_, JSNative call,
                      unsigned nargs, unsigned attrs)
{
    RootedObject obj(cx, objArg);
    RootedId id(cx, id_);
    JS_ASSERT(!cx->runtime()->isAtomsCompartment(cx->compartment()));
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, obj);
    return DefineFunction(cx, obj, id, call, nargs, attrs);
}

struct AutoLastFrameCheck
{
    AutoLastFrameCheck(JSContext *cx
                       MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : cx(cx)
    {
        JS_ASSERT(cx);
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }

    ~AutoLastFrameCheck() {
        if (cx->isExceptionPending() &&
            !JS_IsRunning(cx) &&
            !cx->options().dontReportUncaught()) {
            js_ReportUncaughtException(cx);
        }
    }

  private:
    JSContext *cx;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};


#if defined(HAVE_GETC_UNLOCKED)
# define fast_getc getc_unlocked
#elif defined(HAVE__GETC_NOLOCK)
# define fast_getc _getc_nolock
#else
# define fast_getc getc
#endif

typedef Vector<char, 8, TempAllocPolicy> FileContents;

static bool
ReadCompleteFile(JSContext *cx, FILE *fp, FileContents &buffer)
{
    
    struct stat st;
    int ok = fstat(fileno(fp), &st);
    if (ok != 0)
        return false;
    if (st.st_size > 0) {
        if (!buffer.reserve(st.st_size))
            return false;
    }

    
    
    
    
    for (;;) {
        int c = fast_getc(fp);
        if (c == EOF)
            break;
        if (!buffer.append(c))
            return false;
    }

    return true;
}

namespace {

class AutoFile
{
    FILE *fp_;
  public:
    AutoFile()
      : fp_(nullptr)
    {}
    ~AutoFile()
    {
        if (fp_ && fp_ != stdin)
            fclose(fp_);
    }
    FILE *fp() const { return fp_; }
    bool open(JSContext *cx, const char *filename);
    bool readAll(JSContext *cx, FileContents &buffer)
    {
        JS_ASSERT(fp_);
        return ReadCompleteFile(cx, fp_, buffer);
    }
};

} 





bool
AutoFile::open(JSContext *cx, const char *filename)
{
    if (!filename || strcmp(filename, "-") == 0) {
        fp_ = stdin;
    } else {
        fp_ = fopen(filename, "r");
        if (!fp_) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_CANT_OPEN,
                                 filename, "No such file or directory");
            return false;
        }
    }
    return true;
}


JS::CompileOptions::CompileOptions(JSContext *cx, JSVersion version)
    : principals_(nullptr),
      originPrincipals_(nullptr),
      version(version != JSVERSION_UNKNOWN ? version : cx->findVersion()),
      versionSet(false),
      utf8(false),
      filename(nullptr),
      sourceMapURL(nullptr),
      lineno(1),
      column(0),
      element(NullPtr()),
      compileAndGo(cx->options().compileAndGo()),
      forEval(false),
      noScriptRval(cx->options().noScriptRval()),
      selfHostingMode(false),
      canLazilyParse(true),
      strictOption(cx->options().strictMode()),
      extraWarningsOption(cx->options().extraWarnings()),
      werrorOption(cx->options().werror()),
      asmJSOption(cx->options().asmJS()),
      sourcePolicy(SAVE_SOURCE)
{
}

JSPrincipals *
CompileOptions::originPrincipals() const
{
    return NormalizeOriginPrincipals(principals_, originPrincipals_);
}

JSScript *
JS::Compile(JSContext *cx, HandleObject obj, CompileOptions options,
            const jschar *chars, size_t length)
{
    JS_ASSERT(!cx->runtime()->isAtomsCompartment(cx->compartment()));
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, obj);
    JS_ASSERT_IF(options.principals(), cx->compartment()->principals == options.principals());
    AutoLastFrameCheck lfc(cx);

    return frontend::CompileScript(cx, &cx->tempLifoAlloc(), obj, NullPtr(), options, chars, length);
}

JSScript *
JS::Compile(JSContext *cx, HandleObject obj, CompileOptions options,
            const char *bytes, size_t length)
{
    jschar *chars;
    if (options.utf8)
        chars = UTF8CharsToNewTwoByteCharsZ(cx, UTF8Chars(bytes, length), &length).get();
    else
        chars = InflateString(cx, bytes, &length);
    if (!chars)
        return nullptr;

    JSScript *script = Compile(cx, obj, options, chars, length);
    js_free(chars);
    return script;
}

JSScript *
JS::Compile(JSContext *cx, HandleObject obj, CompileOptions options, FILE *fp)
{
    FileContents buffer(cx);
    if (!ReadCompleteFile(cx, fp, buffer))
        return nullptr;

    JSScript *script = Compile(cx, obj, options, buffer.begin(), buffer.length());
    return script;
}

JSScript *
JS::Compile(JSContext *cx, HandleObject obj, CompileOptions options, const char *filename)
{
    AutoFile file;
    if (!file.open(cx, filename))
        return nullptr;
    options = options.setFileAndLine(filename, 1);
    JSScript *script = Compile(cx, obj, options, file.fp());
    return script;
}

JS_PUBLIC_API(bool)
JS::CanCompileOffThread(JSContext *cx, const CompileOptions &options)
{
#ifdef JS_WORKER_THREADS
    if (!cx->runtime()->useHelperThreads() || !cx->runtime()->helperThreadCount())
        return false;

    if (!cx->runtime()->useHelperThreadsForParsing())
        return false;

    
    
    
    
    if (cx->runtime()->activeGCInAtomsZone())
        return false;

    
    
    
    static const char *blacklist[] = {
#ifdef XP_MACOSX
        "chrome://browser/content/places/editBookmarkOverlay.js",
        "chrome://browser/content/nsContextMenu.js",
        "chrome://browser/content/newtab/newTab.js",
        "chrome://browser/content/places/browserPlacesViews.js",
#endif
        nullptr
    };

    const char *filename = options.filename;
    for (const char **ptest = blacklist; *ptest; ptest++) {
        if (!strcmp(*ptest, filename))
            return false;
    }

    return true;
#else
    return false;
#endif
}

JS_PUBLIC_API(bool)
JS::CompileOffThread(JSContext *cx, Handle<JSObject*> obj, CompileOptions options,
                     const jschar *chars, size_t length,
                     OffThreadCompileCallback callback, void *callbackData)
{
#ifdef JS_WORKER_THREADS
    JS_ASSERT(CanCompileOffThread(cx, options));
    return StartOffThreadParseScript(cx, options, chars, length, obj, callback, callbackData);
#else
    MOZ_ASSUME_UNREACHABLE("Off thread compilation is not available.");
#endif
}

JS_PUBLIC_API(JSScript *)
JS::FinishOffThreadScript(JSContext *maybecx, JSRuntime *rt, void *token)
{
#ifdef JS_WORKER_THREADS
    JS_ASSERT(CurrentThreadCanAccessRuntime(rt));
    return rt->workerThreadState->finishParseTask(maybecx, rt, token);
#else
    MOZ_ASSUME_UNREACHABLE("Off thread compilation is not available.");
#endif
}

JS_PUBLIC_API(JSScript *)
JS_CompileUCScriptForPrincipals(JSContext *cx, JSObject *objArg, JSPrincipals *principals,
                                const jschar *chars, size_t length,
                                const char *filename, unsigned lineno)
{
    RootedObject obj(cx, objArg);
    CompileOptions options(cx);
    options.setPrincipals(principals)
           .setFileAndLine(filename, lineno);

    return Compile(cx, obj, options, chars, length);
}

JS_PUBLIC_API(JSScript *)
JS_CompileUCScript(JSContext *cx, JSObject *objArg, const jschar *chars, size_t length,
                   const char *filename, unsigned lineno)
{
    RootedObject obj(cx, objArg);
    CompileOptions options(cx);
    options.setFileAndLine(filename, lineno);

    return Compile(cx, obj, options, chars, length);
}

JS_PUBLIC_API(JSScript *)
JS_CompileScriptForPrincipals(JSContext *cx, JSObject *objArg,
                              JSPrincipals *principals,
                              const char *ascii, size_t length,
                              const char *filename, unsigned lineno)
{
    RootedObject obj(cx, objArg);
    CompileOptions options(cx);
    options.setPrincipals(principals)
           .setFileAndLine(filename, lineno);

    return Compile(cx, obj, options, ascii, length);
}

JS_PUBLIC_API(JSScript *)
JS_CompileScript(JSContext *cx, JSObject *objArg, const char *ascii, size_t length,
                 const char *filename, unsigned lineno)
{
    RootedObject obj(cx, objArg);
    CompileOptions options(cx);
    options.setFileAndLine(filename, lineno);

    return Compile(cx, obj, options, ascii, length);
}

JS_PUBLIC_API(bool)
JS_BufferIsCompilableUnit(JSContext *cx, JSObject *objArg, const char *utf8, size_t length)
{
    RootedObject obj(cx, objArg);
    bool result;
    JSExceptionState *exnState;
    JSErrorReporter older;

    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, obj);
    jschar *chars = JS::UTF8CharsToNewTwoByteCharsZ(cx, JS::UTF8Chars(utf8, length), &length).get();
    if (!chars)
        return true;

    



    result = true;
    exnState = JS_SaveExceptionState(cx);
    {
        CompileOptions options(cx);
        options.setCompileAndGo(false);
        Parser<frontend::FullParseHandler> parser(cx, &cx->tempLifoAlloc(),
                                                  options, chars, length,
                                                   true, nullptr, nullptr);
        older = JS_SetErrorReporter(cx, nullptr);
        if (!parser.parse(obj) && parser.isUnexpectedEOF()) {
            




            result = false;
        }
        JS_SetErrorReporter(cx, older);
    }
    js_free(chars);
    JS_RestoreExceptionState(cx, exnState);
    return result;
}

JS_PUBLIC_API(JSObject *)
JS_GetGlobalFromScript(JSScript *script)
{
    JS_ASSERT(!script->isCachedEval);
    return &script->global();
}

JS_PUBLIC_API(JSFunction *)
JS::CompileFunction(JSContext *cx, HandleObject obj, CompileOptions options,
                    const char *name, unsigned nargs, const char *const *argnames,
                    const jschar *chars, size_t length)
{
    JS_ASSERT(!cx->runtime()->isAtomsCompartment(cx->compartment()));
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, obj);
    JS_ASSERT_IF(options.principals(), cx->compartment()->principals == options.principals());
    AutoLastFrameCheck lfc(cx);

    RootedAtom funAtom(cx);
    if (name) {
        funAtom = Atomize(cx, name, strlen(name));
        if (!funAtom)
            return nullptr;
    }

    AutoNameVector formals(cx);
    for (unsigned i = 0; i < nargs; i++) {
        RootedAtom argAtom(cx, Atomize(cx, argnames[i], strlen(argnames[i])));
        if (!argAtom || !formals.append(argAtom->asPropertyName()))
            return nullptr;
    }

    RootedFunction fun(cx, NewFunction(cx, NullPtr(), nullptr, 0, JSFunction::INTERPRETED, obj,
                                       funAtom, JSFunction::FinalizeKind, TenuredObject));
    if (!fun)
        return nullptr;

    if (!frontend::CompileFunctionBody(cx, &fun, options, formals, chars, length))
        return nullptr;

    if (obj && funAtom) {
        Rooted<jsid> id(cx, AtomToId(funAtom));
        RootedValue value(cx, ObjectValue(*fun));
        if (!JSObject::defineGeneric(cx, obj, id, value, nullptr, nullptr, JSPROP_ENUMERATE))
            return nullptr;
    }

    return fun;
}

JS_PUBLIC_API(JSFunction *)
JS::CompileFunction(JSContext *cx, HandleObject obj, CompileOptions options,
                    const char *name, unsigned nargs, const char *const *argnames,
                    const char *bytes, size_t length)
{
    jschar *chars;
    if (options.utf8)
        chars = UTF8CharsToNewTwoByteCharsZ(cx, UTF8Chars(bytes, length), &length).get();
    else
        chars = InflateString(cx, bytes, &length);
    if (!chars)
        return nullptr;

    JSFunction *fun = CompileFunction(cx, obj, options, name, nargs, argnames, chars, length);
    js_free(chars);
    return fun;
}

JS_PUBLIC_API(JSFunction *)
JS_CompileUCFunction(JSContext *cx, JSObject *objArg, const char *name,
                     unsigned nargs, const char *const *argnames,
                     const jschar *chars, size_t length,
                     const char *filename, unsigned lineno)
{
    RootedObject obj(cx, objArg);
    CompileOptions options(cx);
    options.setFileAndLine(filename, lineno);

    return CompileFunction(cx, obj, options, name, nargs, argnames, chars, length);
}

JS_PUBLIC_API(JSFunction *)
JS_CompileFunctionForPrincipals(JSContext *cx, JSObject *objArg,
                                JSPrincipals *principals, const char *name,
                                unsigned nargs, const char *const *argnames,
                                const char *ascii, size_t length,
                                const char *filename, unsigned lineno)
{
    RootedObject obj(cx, objArg);
    CompileOptions options(cx);
    options.setPrincipals(principals)
           .setFileAndLine(filename, lineno);

    return CompileFunction(cx, obj, options, name, nargs, argnames, ascii, length);
}

JS_PUBLIC_API(JSFunction *)
JS_CompileFunction(JSContext *cx, JSObject *objArg, const char *name,
                   unsigned nargs, const char *const *argnames,
                   const char *ascii, size_t length,
                   const char *filename, unsigned lineno)
{
    RootedObject obj(cx, objArg);
    CompileOptions options(cx);
    options.setFileAndLine(filename, lineno);

    return CompileFunction(cx, obj, options, name, nargs, argnames, ascii, length);
}

JS_PUBLIC_API(JSString *)
JS_DecompileScript(JSContext *cx, JSScript *scriptArg, const char *name, unsigned indent)
{
    JS_ASSERT(!cx->runtime()->isAtomsCompartment(cx->compartment()));

    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    RootedScript script(cx, scriptArg);
    RootedFunction fun(cx, script->function());
    if (fun)
        return JS_DecompileFunction(cx, fun, indent);
    bool haveSource = script->scriptSource()->hasSourceData();
    if (!haveSource && !JSScript::loadSource(cx, script->scriptSource(), &haveSource))
        return nullptr;
    return haveSource ? script->sourceData(cx) : js_NewStringCopyZ<CanGC>(cx, "[no source]");
}

JS_PUBLIC_API(JSString *)
JS_DecompileFunction(JSContext *cx, JSFunction *funArg, unsigned indent)
{
    JS_ASSERT(!cx->runtime()->isAtomsCompartment(cx->compartment()));
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, funArg);
    RootedFunction fun(cx, funArg);
    return FunctionToString(cx, fun, false, !(indent & JS_DONT_PRETTY_PRINT));
}

JS_PUBLIC_API(JSString *)
JS_DecompileFunctionBody(JSContext *cx, JSFunction *funArg, unsigned indent)
{
    JS_ASSERT(!cx->runtime()->isAtomsCompartment(cx->compartment()));
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, funArg);
    RootedFunction fun(cx, funArg);
    return FunctionToString(cx, fun, true, !(indent & JS_DONT_PRETTY_PRINT));
}

JS_NEVER_INLINE JS_PUBLIC_API(bool)
JS_ExecuteScript(JSContext *cx, JSObject *objArg, JSScript *scriptArg, jsval *rval)
{
    RootedObject obj(cx, objArg);
    RootedScript script(cx, scriptArg);

    JS_ASSERT(!cx->runtime()->isAtomsCompartment(cx->compartment()));
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, obj);
    if (cx->compartment() != obj->compartment())
        *(volatile int *) 0 = 0xf0;
    AutoLastFrameCheck lfc(cx);

    







    if (script->compartment() != obj->compartment()) {
        script = CloneScript(cx, NullPtr(), NullPtr(), script);
        if (!script.get())
            return false;
    } else {
        script = scriptArg;
    }

    return Execute(cx, script, *obj, rval);
}

JS_PUBLIC_API(bool)
JS_ExecuteScriptVersion(JSContext *cx, JSObject *objArg, JSScript *script, jsval *rval,
                        JSVersion version)
{
    RootedObject obj(cx, objArg);
    return JS_ExecuteScript(cx, obj, script, rval);
}

static const unsigned LARGE_SCRIPT_LENGTH = 500*1024;

extern JS_PUBLIC_API(bool)
JS::Evaluate(JSContext *cx, HandleObject obj, CompileOptions options,
             const jschar *chars, size_t length, jsval *rval)
{
    JS_ASSERT(!cx->runtime()->isAtomsCompartment(cx->compartment()));
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, obj);
    JS_ASSERT_IF(options.principals(), cx->compartment()->principals == options.principals());

    AutoLastFrameCheck lfc(cx);

    options.setCompileAndGo(obj->is<GlobalObject>());
    options.setNoScriptRval(!rval);
    SourceCompressionTask sct(cx);
    RootedScript script(cx, frontend::CompileScript(cx, &cx->tempLifoAlloc(),
                                                    obj, NullPtr(), options,
                                                    chars, length, nullptr, 0, &sct));
    if (!script)
        return false;

    JS_ASSERT(script->getVersion() == options.version);

    bool result = Execute(cx, script, *obj, rval);
    if (!sct.complete())
        result = false;

    
    
    
    
    
    if (script->length > LARGE_SCRIPT_LENGTH) {
        script = nullptr;
        PrepareZoneForGC(cx->zone());
        GC(cx->runtime(), GC_NORMAL, gcreason::FINISH_LARGE_EVALUTE);
    }

    return result;
}

extern JS_PUBLIC_API(bool)
JS::Evaluate(JSContext *cx, HandleObject obj, CompileOptions options,
             const char *bytes, size_t length, jsval *rval)
{
    jschar *chars;
    if (options.utf8)
        chars = UTF8CharsToNewTwoByteCharsZ(cx, UTF8Chars(bytes, length), &length).get();
    else
        chars = InflateString(cx, bytes, &length);
    if (!chars)
        return false;

    bool ok = Evaluate(cx, obj, options, chars, length, rval);
    js_free(chars);
    return ok;
}

extern JS_PUBLIC_API(bool)
JS::Evaluate(JSContext *cx, HandleObject obj, CompileOptions options,
             const char *filename, jsval *rval)
{
    FileContents buffer(cx);
    {
        AutoFile file;
        if (!file.open(cx, filename) || !file.readAll(cx, buffer))
            return false;
    }

    options.setFileAndLine(filename, 1);
    return Evaluate(cx, obj, options, buffer.begin(), buffer.length(), rval);
}

JS_PUBLIC_API(bool)
JS_EvaluateUCScriptForPrincipals(JSContext *cx, JSObject *objArg,
                                 JSPrincipals *principals,
                                 const jschar *chars, unsigned length,
                                 const char *filename, unsigned lineno,
                                 jsval *rval)
{
    RootedObject obj(cx, objArg);
    CompileOptions options(cx);
    options.setPrincipals(principals)
           .setFileAndLine(filename, lineno);

    return Evaluate(cx, obj, options, chars, length, rval);
}

JS_PUBLIC_API(bool)
JS_EvaluateUCScriptForPrincipalsVersion(JSContext *cx, JSObject *objArg,
                                        JSPrincipals *principals,
                                        const jschar *chars, unsigned length,
                                        const char *filename, unsigned lineno,
                                        jsval *rval, JSVersion version)
{
    RootedObject obj(cx, objArg);
    CompileOptions options(cx);
    options.setPrincipals(principals)
           .setFileAndLine(filename, lineno)
           .setVersion(version);

    return Evaluate(cx, obj, options, chars, length, rval);
}

extern JS_PUBLIC_API(bool)
JS_EvaluateUCScriptForPrincipalsVersionOrigin(JSContext *cx, JSObject *objArg,
                                              JSPrincipals *principals,
                                              JSPrincipals *originPrincipals,
                                              const jschar *chars, unsigned length,
                                              const char *filename, unsigned lineno,
                                              jsval *rval, JSVersion version)
{
    RootedObject obj(cx, objArg);
    CompileOptions options(cx);
    options.setPrincipals(principals)
           .setOriginPrincipals(originPrincipals)
           .setFileAndLine(filename, lineno)
           .setVersion(version);

    return Evaluate(cx, obj, options, chars, length, rval);
}

JS_PUBLIC_API(bool)
JS_EvaluateUCScript(JSContext *cx, JSObject *objArg, const jschar *chars, unsigned length,
                    const char *filename, unsigned lineno, jsval *rval)
{
    RootedObject obj(cx, objArg);
    CompileOptions options(cx);
    options.setFileAndLine(filename, lineno);

    return Evaluate(cx, obj, options, chars, length, rval);
}


JS_PUBLIC_API(bool)
JS_EvaluateScriptForPrincipals(JSContext *cx, JSObject *objArg, JSPrincipals *principals,
                               const char *bytes, unsigned nbytes,
                               const char *filename, unsigned lineno, jsval *rval)
{
    RootedObject obj(cx, objArg);
    CompileOptions options(cx);
    options.setPrincipals(principals)
           .setFileAndLine(filename, lineno);

    return Evaluate(cx, obj, options, bytes, nbytes, rval);
}

JS_PUBLIC_API(bool)
JS_EvaluateScriptForPrincipalsVersion(JSContext *cx, JSObject *objArg, JSPrincipals *principals,
                                      const char *bytes, unsigned nbytes,
                                      const char *filename, unsigned lineno, jsval *rval,
                                      JSVersion version)
{
    RootedObject obj(cx, objArg);
    CompileOptions options(cx);
    options.setPrincipals(principals)
           .setVersion(version)
           .setFileAndLine(filename, lineno);

    return Evaluate(cx, obj, options, bytes, nbytes, rval);
}

JS_PUBLIC_API(bool)
JS_EvaluateScript(JSContext *cx, JSObject *objArg, const char *bytes, unsigned nbytes,
                  const char *filename, unsigned lineno, jsval *rval)
{
    RootedObject obj(cx, objArg);
    CompileOptions options(cx);
    options.setFileAndLine(filename, lineno);

    return Evaluate(cx, obj, options, bytes, nbytes, rval);
}

JS_PUBLIC_API(bool)
JS_CallFunction(JSContext *cx, JSObject *objArg, JSFunction *fun, unsigned argc, jsval *argv,
                jsval *rval)
{
    RootedObject obj(cx, objArg);
    JS_ASSERT(!cx->runtime()->isAtomsCompartment(cx->compartment()));
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, obj, fun, JSValueArray(argv, argc));
    AutoLastFrameCheck lfc(cx);

    RootedValue rv(cx);
    if (!Invoke(cx, ObjectOrNullValue(obj), ObjectValue(*fun), argc, argv, &rv))
        return false;
    *rval = rv;
    return true;
}

JS_PUBLIC_API(bool)
JS_CallFunctionName(JSContext *cx, JSObject *objArg, const char *name, unsigned argc, jsval *argv,
                    jsval *rval)
{
    RootedObject obj(cx, objArg);
    JS_ASSERT(!cx->runtime()->isAtomsCompartment(cx->compartment()));
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, obj, JSValueArray(argv, argc));
    AutoLastFrameCheck lfc(cx);

    JSAtom *atom = Atomize(cx, name, strlen(name));
    if (!atom)
        return false;

    RootedValue v(cx);
    RootedId id(cx, AtomToId(atom));
    if (!JSObject::getGeneric(cx, obj, obj, id, &v))
        return false;

    RootedValue rv(cx);
    if (!Invoke(cx, ObjectOrNullValue(obj), v, argc, argv, &rv))
        return false;
    *rval = rv;
    return true;
}

JS_PUBLIC_API(bool)
JS_CallFunctionValue(JSContext *cx, JSObject *objArg, jsval fval, unsigned argc, jsval *argv,
                     jsval *rval)
{
    RootedObject obj(cx, objArg);
    JS_ASSERT(!cx->runtime()->isAtomsCompartment(cx->compartment()));
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, obj, fval, JSValueArray(argv, argc));
    AutoLastFrameCheck lfc(cx);

    RootedValue rv(cx);
    if (!Invoke(cx, ObjectOrNullValue(obj), fval, argc, argv, &rv))
        return false;
    *rval = rv;
    return true;
}

JS_PUBLIC_API(bool)
JS::Call(JSContext *cx, jsval thisv, jsval fval, unsigned argc, jsval *argv,
         MutableHandleValue rval)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, thisv, fval, JSValueArray(argv, argc));
    AutoLastFrameCheck lfc(cx);

    return Invoke(cx, thisv, fval, argc, argv, rval);
}

JS_PUBLIC_API(JSObject *)
JS_New(JSContext *cx, JSObject *ctorArg, unsigned argc, jsval *argv)
{
    RootedObject ctor(cx, ctorArg);
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, ctor, JSValueArray(argv, argc));
    AutoLastFrameCheck lfc(cx);

    
    
    
    
    InvokeArgs args(cx);
    if (!args.init(argc))
        return nullptr;

    args.setCallee(ObjectValue(*ctor));
    args.setThis(NullValue());
    PodCopy(args.array(), argv, argc);

    if (!InvokeConstructor(cx, args))
        return nullptr;

    if (!args.rval().isObject()) {
        



        JSAutoByteString bytes;
        if (js_ValueToPrintable(cx, args.rval(), &bytes)) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_BAD_NEW_RESULT,
                                 bytes.ptr());
        }
        return nullptr;
    }

    return &args.rval().toObject();
}

JS_PUBLIC_API(JSOperationCallback)
JS_SetOperationCallback(JSRuntime *rt, JSOperationCallback callback)
{
    JSOperationCallback old = rt->operationCallback;
    rt->operationCallback = callback;
    return old;
}

JS_PUBLIC_API(JSOperationCallback)
JS_GetOperationCallback(JSRuntime *rt)
{
    return rt->operationCallback;
}

JS_PUBLIC_API(void)
JS_TriggerOperationCallback(JSRuntime *rt)
{
    rt->triggerOperationCallback(JSRuntime::TriggerCallbackAnyThread);
}

JS_PUBLIC_API(bool)
JS_IsRunning(JSContext *cx)
{
    return cx->currentlyRunning();
}

JS_PUBLIC_API(bool)
JS_SaveFrameChain(JSContext *cx)
{
    AssertHeapIsIdleOrIterating(cx);
    CHECK_REQUEST(cx);
    return cx->saveFrameChain();
}

JS_PUBLIC_API(void)
JS_RestoreFrameChain(JSContext *cx)
{
    AssertHeapIsIdleOrIterating(cx);
    CHECK_REQUEST(cx);
    cx->restoreFrameChain();
}

#ifdef MOZ_TRACE_JSCALLS
JS_PUBLIC_API(void)
JS_SetFunctionCallback(JSContext *cx, JSFunctionCallback fcb)
{
    cx->functionCallback = fcb;
}

JS_PUBLIC_API(JSFunctionCallback)
JS_GetFunctionCallback(JSContext *cx)
{
    return cx->functionCallback;
}
#endif


JS_PUBLIC_API(JSString *)
JS_NewStringCopyN(JSContext *cx, const char *s, size_t n)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    return js_NewStringCopyN<CanGC>(cx, s, n);
}

JS_PUBLIC_API(JSString *)
JS_NewStringCopyZ(JSContext *cx, const char *s)
{
    size_t n;
    jschar *js;
    JSString *str;

    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    if (!s || !*s)
        return cx->runtime()->emptyString;
    n = strlen(s);
    js = InflateString(cx, s, &n);
    if (!js)
        return nullptr;
    str = js_NewString<CanGC>(cx, js, n);
    if (!str)
        js_free(js);
    return str;
}

JS_PUBLIC_API(bool)
JS_StringHasBeenInterned(JSContext *cx, JSString *str)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);

    if (!str->isAtom())
        return false;

    return AtomIsInterned(cx, &str->asAtom());
}

JS_PUBLIC_API(jsid)
INTERNED_STRING_TO_JSID(JSContext *cx, JSString *str)
{
    JS_ASSERT(str);
    JS_ASSERT(((size_t)str & JSID_TYPE_MASK) == 0);
    JS_ASSERT_IF(cx, JS_StringHasBeenInterned(cx, str));
    return AtomToId(&str->asAtom());
}

JS_PUBLIC_API(JSString *)
JS_InternJSString(JSContext *cx, HandleString str)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    JSAtom *atom = AtomizeString<CanGC>(cx, str, InternAtom);
    JS_ASSERT_IF(atom, JS_StringHasBeenInterned(cx, atom));
    return atom;
}

JS_PUBLIC_API(JSString *)
JS_InternString(JSContext *cx, const char *s)
{
    return JS_InternStringN(cx, s, strlen(s));
}

JS_PUBLIC_API(JSString *)
JS_InternStringN(JSContext *cx, const char *s, size_t length)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    JSAtom *atom = Atomize(cx, s, length, InternAtom);
    JS_ASSERT_IF(atom, JS_StringHasBeenInterned(cx, atom));
    return atom;
}

JS_PUBLIC_API(JSString *)
JS_NewUCString(JSContext *cx, jschar *chars, size_t length)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    return js_NewString<CanGC>(cx, chars, length);
}

JS_PUBLIC_API(JSString *)
JS_NewUCStringCopyN(JSContext *cx, const jschar *s, size_t n)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    return js_NewStringCopyN<CanGC>(cx, s, n);
}

JS_PUBLIC_API(JSString *)
JS_NewUCStringCopyZ(JSContext *cx, const jschar *s)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    if (!s)
        return cx->runtime()->emptyString;
    return js_NewStringCopyZ<CanGC>(cx, s);
}

JS_PUBLIC_API(JSString *)
JS_InternUCStringN(JSContext *cx, const jschar *s, size_t length)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    JSAtom *atom = AtomizeChars<CanGC>(cx, s, length, InternAtom);
    JS_ASSERT_IF(atom, JS_StringHasBeenInterned(cx, atom));
    return atom;
}

JS_PUBLIC_API(JSString *)
JS_InternUCString(JSContext *cx, const jschar *s)
{
    return JS_InternUCStringN(cx, s, js_strlen(s));
}

JS_PUBLIC_API(size_t)
JS_GetStringLength(JSString *str)
{
    return str->length();
}

JS_PUBLIC_API(const jschar *)
JS_GetStringCharsZ(JSContext *cx, JSString *str)
{
    size_t dummy;
    return JS_GetStringCharsZAndLength(cx, str, &dummy);
}

JS_PUBLIC_API(const jschar *)
JS_GetStringCharsZAndLength(JSContext *cx, JSString *str, size_t *plength)
{
    



    JS_ASSERT(plength);
    AssertHeapIsIdleOrStringIsFlat(cx, str);
    CHECK_REQUEST(cx);
    JSFlatString *flat = str->ensureFlat(cx);
    if (!flat)
        return nullptr;
    *plength = flat->length();
    return flat->chars();
}

JS_PUBLIC_API(const jschar *)
JS_GetStringCharsAndLength(JSContext *cx, JSString *str, size_t *plength)
{
    JS_ASSERT(plength);
    AssertHeapIsIdleOrStringIsFlat(cx, str);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, str);
    JSLinearString *linear = str->ensureLinear(cx);
    if (!linear)
        return nullptr;
    *plength = linear->length();
    return linear->chars();
}

JS_PUBLIC_API(const jschar *)
JS_GetInternedStringChars(JSString *str)
{
    JS_ASSERT(str->isAtom());
    JSFlatString *flat = str->ensureFlat(nullptr);
    if (!flat)
        return nullptr;
    return flat->chars();
}

JS_PUBLIC_API(const jschar *)
JS_GetInternedStringCharsAndLength(JSString *str, size_t *plength)
{
    JS_ASSERT(str->isAtom());
    JS_ASSERT(plength);
    JSFlatString *flat = str->ensureFlat(nullptr);
    if (!flat)
        return nullptr;
    *plength = flat->length();
    return flat->chars();
}

extern JS_PUBLIC_API(JSFlatString *)
JS_FlattenString(JSContext *cx, JSString *str)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, str);
    JSFlatString *flat = str->ensureFlat(cx);
    if (!flat)
        return nullptr;
    return flat;
}

extern JS_PUBLIC_API(const jschar *)
JS_GetFlatStringChars(JSFlatString *str)
{
    return str->chars();
}

JS_PUBLIC_API(bool)
JS_CompareStrings(JSContext *cx, JSString *str1, JSString *str2, int32_t *result)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);

    return CompareStrings(cx, str1, str2, result);
}

JS_PUBLIC_API(bool)
JS_StringEqualsAscii(JSContext *cx, JSString *str, const char *asciiBytes, bool *match)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);

    JSLinearString *linearStr = str->ensureLinear(cx);
    if (!linearStr)
        return false;
    *match = StringEqualsAscii(linearStr, asciiBytes);
    return true;
}

JS_PUBLIC_API(bool)
JS_FlatStringEqualsAscii(JSFlatString *str, const char *asciiBytes)
{
    return StringEqualsAscii(str, asciiBytes);
}

JS_PUBLIC_API(size_t)
JS_PutEscapedFlatString(char *buffer, size_t size, JSFlatString *str, char quote)
{
    return PutEscapedString(buffer, size, str, quote);
}

JS_PUBLIC_API(size_t)
JS_PutEscapedString(JSContext *cx, char *buffer, size_t size, JSString *str, char quote)
{
    AssertHeapIsIdle(cx);
    JSLinearString *linearStr = str->ensureLinear(cx);
    if (!linearStr)
        return size_t(-1);
    return PutEscapedString(buffer, size, linearStr, quote);
}

JS_PUBLIC_API(bool)
JS_FileEscapedString(FILE *fp, JSString *str, char quote)
{
    JSLinearString *linearStr = str->ensureLinear(nullptr);
    return linearStr && FileEscapedString(fp, linearStr, quote);
}

JS_PUBLIC_API(JSString *)
JS_NewDependentString(JSContext *cx, HandleString str, size_t start, size_t length)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    return js_NewDependentString(cx, str, start, length);
}

JS_PUBLIC_API(JSString *)
JS_ConcatStrings(JSContext *cx, HandleString left, HandleString right)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    return ConcatStrings<CanGC>(cx, left, right);
}

JS_PUBLIC_API(bool)
JS_DecodeBytes(JSContext *cx, const char *src, size_t srclen, jschar *dst, size_t *dstlenp)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);

    if (!dst) {
        *dstlenp = srclen;
        return true;
    }

    size_t dstlen = *dstlenp;

    if (srclen > dstlen) {
        InflateStringToBuffer(src, dstlen, dst);

        AutoSuppressGC suppress(cx);
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_BUFFER_TOO_SMALL);
        return false;
    }

    InflateStringToBuffer(src, srclen, dst);
    *dstlenp = srclen;
    return true;
}

JS_PUBLIC_API(char *)
JS_EncodeString(JSContext *cx, JSString *str)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);

    JSLinearString *linear = str->ensureLinear(cx);
    if (!linear)
        return nullptr;

    return LossyTwoByteCharsToNewLatin1CharsZ(cx, linear->range()).c_str();
}

JS_PUBLIC_API(char *)
JS_EncodeStringToUTF8(JSContext *cx, JSString *str)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);

    JSLinearString *linear = str->ensureLinear(cx);
    if (!linear)
        return nullptr;

    return TwoByteCharsToNewUTF8CharsZ(cx, linear->range()).c_str();
}

JS_PUBLIC_API(size_t)
JS_GetStringEncodingLength(JSContext *cx, JSString *str)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);

    const jschar *chars = str->getChars(cx);
    if (!chars)
        return size_t(-1);
    return str->length();
}

JS_PUBLIC_API(size_t)
JS_EncodeStringToBuffer(JSContext *cx, JSString *str, char *buffer, size_t length)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);

    




    size_t writtenLength = length;
    const jschar *chars = str->getChars(nullptr);
    if (!chars)
        return size_t(-1);
    if (DeflateStringToBuffer(nullptr, chars, str->length(), buffer, &writtenLength)) {
        JS_ASSERT(writtenLength <= length);
        return writtenLength;
    }
    JS_ASSERT(writtenLength <= length);
    size_t necessaryLength = str->length();
    if (necessaryLength == size_t(-1))
        return size_t(-1);
    JS_ASSERT(writtenLength == length); 
    return necessaryLength;
}

JS_PUBLIC_API(bool)
JS_Stringify(JSContext *cx, MutableHandleValue vp, HandleObject replacer,
             HandleValue space, JSONWriteCallback callback, void *data)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, replacer, space);
    StringBuffer sb(cx);
    if (!js_Stringify(cx, vp, replacer, space, sb))
        return false;
    if (sb.empty()) {
        HandlePropertyName null = cx->names().null;
        return callback(null->chars(), null->length(), data);
    }
    return callback(sb.begin(), sb.length(), data);
}

JS_PUBLIC_API(bool)
JS_ParseJSON(JSContext *cx, const jschar *chars, uint32_t len, JS::MutableHandleValue vp)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);

    RootedValue reviver(cx, NullValue());
    return ParseJSONWithReviver(cx, JS::StableCharPtr(chars, len), len, reviver, vp);
}

JS_PUBLIC_API(bool)
JS_ParseJSONWithReviver(JSContext *cx, const jschar *chars, uint32_t len, HandleValue reviver, MutableHandleValue vp)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    return ParseJSONWithReviver(cx, StableCharPtr(chars, len), len, reviver, vp);
}



JS_PUBLIC_API(void)
JS_ReportError(JSContext *cx, const char *format, ...)
{
    va_list ap;

    AssertHeapIsIdle(cx);
    va_start(ap, format);
    js_ReportErrorVA(cx, JSREPORT_ERROR, format, ap);
    va_end(ap);
}

JS_PUBLIC_API(void)
JS_ReportErrorNumber(JSContext *cx, JSErrorCallback errorCallback,
                     void *userRef, const unsigned errorNumber, ...)
{
    va_list ap;
    va_start(ap, errorNumber);
    JS_ReportErrorNumberVA(cx, errorCallback, userRef, errorNumber, ap);
    va_end(ap);
}

JS_PUBLIC_API(void)
JS_ReportErrorNumberVA(JSContext *cx, JSErrorCallback errorCallback,
                       void *userRef, const unsigned errorNumber,
                       va_list ap)
{
    AssertHeapIsIdle(cx);
    js_ReportErrorNumberVA(cx, JSREPORT_ERROR, errorCallback, userRef,
                           errorNumber, ArgumentsAreASCII, ap);
}

JS_PUBLIC_API(void)
JS_ReportErrorNumberUC(JSContext *cx, JSErrorCallback errorCallback,
                       void *userRef, const unsigned errorNumber, ...)
{
    va_list ap;

    AssertHeapIsIdle(cx);
    va_start(ap, errorNumber);
    js_ReportErrorNumberVA(cx, JSREPORT_ERROR, errorCallback, userRef,
                           errorNumber, ArgumentsAreUnicode, ap);
    va_end(ap);
}

JS_PUBLIC_API(void)
JS_ReportErrorNumberUCArray(JSContext *cx, JSErrorCallback errorCallback,
                            void *userRef, const unsigned errorNumber,
                            const jschar **args)
{
    AssertHeapIsIdle(cx);
    js_ReportErrorNumberUCArray(cx, JSREPORT_ERROR, errorCallback, userRef,
                                errorNumber, args);
}

JS_PUBLIC_API(bool)
JS_ReportWarning(JSContext *cx, const char *format, ...)
{
    va_list ap;
    bool ok;

    AssertHeapIsIdle(cx);
    va_start(ap, format);
    ok = js_ReportErrorVA(cx, JSREPORT_WARNING, format, ap);
    va_end(ap);
    return ok;
}

JS_PUBLIC_API(bool)
JS_ReportErrorFlagsAndNumber(JSContext *cx, unsigned flags,
                             JSErrorCallback errorCallback, void *userRef,
                             const unsigned errorNumber, ...)
{
    va_list ap;
    bool ok;

    AssertHeapIsIdle(cx);
    va_start(ap, errorNumber);
    ok = js_ReportErrorNumberVA(cx, flags, errorCallback, userRef,
                                errorNumber, ArgumentsAreASCII, ap);
    va_end(ap);
    return ok;
}

JS_PUBLIC_API(bool)
JS_ReportErrorFlagsAndNumberUC(JSContext *cx, unsigned flags,
                               JSErrorCallback errorCallback, void *userRef,
                               const unsigned errorNumber, ...)
{
    va_list ap;
    bool ok;

    AssertHeapIsIdle(cx);
    va_start(ap, errorNumber);
    ok = js_ReportErrorNumberVA(cx, flags, errorCallback, userRef,
                                errorNumber, ArgumentsAreUnicode, ap);
    va_end(ap);
    return ok;
}

JS_PUBLIC_API(void)
JS_ReportOutOfMemory(JSContext *cx)
{
    js_ReportOutOfMemory(cx);
}

JS_PUBLIC_API(void)
JS_ReportAllocationOverflow(JSContext *cx)
{
    js_ReportAllocationOverflow(cx);
}

JS_PUBLIC_API(JSErrorReporter)
JS_GetErrorReporter(JSContext *cx)
{
    return cx->errorReporter;
}

JS_PUBLIC_API(JSErrorReporter)
JS_SetErrorReporter(JSContext *cx, JSErrorReporter er)
{
    JSErrorReporter older;

    older = cx->errorReporter;
    cx->errorReporter = er;
    return older;
}






JS_PUBLIC_API(JSObject *)
JS_NewDateObject(JSContext *cx, int year, int mon, int mday, int hour, int min, int sec)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    return js_NewDateObject(cx, year, mon, mday, hour, min, sec);
}

JS_PUBLIC_API(JSObject *)
JS_NewDateObjectMsec(JSContext *cx, double msec)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    return js_NewDateObjectMsec(cx, msec);
}

JS_PUBLIC_API(bool)
JS_ObjectIsDate(JSContext *cx, HandleObject obj)
{
    assertSameCompartment(cx, obj);
    return ObjectClassIs(obj, ESClass_Date, cx);
}

JS_PUBLIC_API(void)
JS_ClearDateCaches(JSContext *cx)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    cx->runtime()->dateTimeInfo.updateTimeZoneAdjustment();
}






JS_PUBLIC_API(JSObject *)
JS_NewRegExpObject(JSContext *cx, HandleObject obj, char *bytes, size_t length, unsigned flags)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    jschar *chars = InflateString(cx, bytes, &length);
    if (!chars)
        return nullptr;

    RegExpStatics *res = obj->as<GlobalObject>().getRegExpStatics();
    RegExpObject *reobj = RegExpObject::create(cx, res, chars, length,
                                               RegExpFlag(flags), nullptr);
    js_free(chars);
    return reobj;
}

JS_PUBLIC_API(JSObject *)
JS_NewUCRegExpObject(JSContext *cx, HandleObject obj, jschar *chars, size_t length,
                     unsigned flags)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    RegExpStatics *res = obj->as<GlobalObject>().getRegExpStatics();
    return RegExpObject::create(cx, res, chars, length,
                                RegExpFlag(flags), nullptr);
}

JS_PUBLIC_API(void)
JS_SetRegExpInput(JSContext *cx, HandleObject obj, HandleString input, bool multiline)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, input);

    obj->as<GlobalObject>().getRegExpStatics()->reset(cx, input, !!multiline);
}

JS_PUBLIC_API(void)
JS_ClearRegExpStatics(JSContext *cx, HandleObject obj)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    JS_ASSERT(obj);

    obj->as<GlobalObject>().getRegExpStatics()->clear();
}

JS_PUBLIC_API(bool)
JS_ExecuteRegExp(JSContext *cx, HandleObject obj, HandleObject reobj, jschar *chars,
                 size_t length, size_t *indexp, bool test, MutableHandleValue rval)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);

    RegExpStatics *res = obj->as<GlobalObject>().getRegExpStatics();

    return ExecuteRegExpLegacy(cx, res, reobj->as<RegExpObject>(), NullPtr(), chars, length, indexp,
                               test, rval);
}

JS_PUBLIC_API(JSObject *)
JS_NewRegExpObjectNoStatics(JSContext *cx, char *bytes, size_t length, unsigned flags)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    jschar *chars = InflateString(cx, bytes, &length);
    if (!chars)
        return nullptr;
    RegExpObject *reobj = RegExpObject::createNoStatics(cx, chars, length,
                                                        RegExpFlag(flags), nullptr);
    js_free(chars);
    return reobj;
}

JS_PUBLIC_API(JSObject *)
JS_NewUCRegExpObjectNoStatics(JSContext *cx, jschar *chars, size_t length, unsigned flags)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    return RegExpObject::createNoStatics(cx, chars, length,
                                         RegExpFlag(flags), nullptr);
}

JS_PUBLIC_API(bool)
JS_ExecuteRegExpNoStatics(JSContext *cx, HandleObject obj, jschar *chars, size_t length,
                          size_t *indexp, bool test, MutableHandleValue rval)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);

    return ExecuteRegExpLegacy(cx, nullptr, obj->as<RegExpObject>(), NullPtr(), chars, length,
                               indexp, test, rval);
}

JS_PUBLIC_API(bool)
JS_ObjectIsRegExp(JSContext *cx, HandleObject obj)
{
    assertSameCompartment(cx, obj);
    return ObjectClassIs(obj, ESClass_RegExp, cx);
}

JS_PUBLIC_API(unsigned)
JS_GetRegExpFlags(JSContext *cx, HandleObject obj)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);

    return obj->as<RegExpObject>().getFlags();
}

JS_PUBLIC_API(JSString *)
JS_GetRegExpSource(JSContext *cx, HandleObject obj)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);

    return obj->as<RegExpObject>().getSource();
}



JS_PUBLIC_API(bool)
JS_SetDefaultLocale(JSRuntime *rt, const char *locale)
{
    AssertHeapIsIdle(rt);
    return rt->setDefaultLocale(locale);
}

JS_PUBLIC_API(void)
JS_ResetDefaultLocale(JSRuntime *rt)
{
    AssertHeapIsIdle(rt);
    rt->resetDefaultLocale();
}

JS_PUBLIC_API(void)
JS_SetLocaleCallbacks(JSRuntime *rt, JSLocaleCallbacks *callbacks)
{
    AssertHeapIsIdle(rt);
    rt->localeCallbacks = callbacks;
}

JS_PUBLIC_API(JSLocaleCallbacks *)
JS_GetLocaleCallbacks(JSRuntime *rt)
{
    
    return rt->localeCallbacks;
}



JS_PUBLIC_API(bool)
JS_IsExceptionPending(JSContext *cx)
{
    
    return (bool) cx->isExceptionPending();
}

JS_PUBLIC_API(bool)
JS_GetPendingException(JSContext *cx, MutableHandleValue vp)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    if (!cx->isExceptionPending())
        return false;
    vp.set(cx->getPendingException());
    assertSameCompartment(cx, vp);
    return true;
}

JS_PUBLIC_API(void)
JS_SetPendingException(JSContext *cx, HandleValue value)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, value);
    cx->setPendingException(value);
}

JS_PUBLIC_API(void)
JS_ClearPendingException(JSContext *cx)
{
    AssertHeapIsIdle(cx);
    cx->clearPendingException();
}

JS_PUBLIC_API(bool)
JS_ReportPendingException(JSContext *cx)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);

    
    bool ok = js_ReportUncaughtException(cx);
    JS_ASSERT(!cx->isExceptionPending());
    return ok;
}

struct JSExceptionState {
    bool throwing;
    jsval exception;
};

JS_PUBLIC_API(JSExceptionState *)
JS_SaveExceptionState(JSContext *cx)
{
    JSExceptionState *state;

    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    state = cx->pod_malloc<JSExceptionState>();
    if (state) {
        state->throwing =
            JS_GetPendingException(cx, MutableHandleValue::fromMarkedLocation(&state->exception));
        if (state->throwing && JSVAL_IS_GCTHING(state->exception))
            AddValueRoot(cx, &state->exception, "JSExceptionState.exception");
    }
    return state;
}

JS_PUBLIC_API(void)
JS_RestoreExceptionState(JSContext *cx, JSExceptionState *state)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    if (state) {
        if (state->throwing)
            JS_SetPendingException(cx, HandleValue::fromMarkedLocation(&state->exception));
        else
            JS_ClearPendingException(cx);
        JS_DropExceptionState(cx, state);
    }
}

JS_PUBLIC_API(void)
JS_DropExceptionState(JSContext *cx, JSExceptionState *state)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    if (state) {
        if (state->throwing && JSVAL_IS_GCTHING(state->exception)) {
            assertSameCompartment(cx, state->exception);
            JS_RemoveValueRoot(cx, &state->exception);
        }
        js_free(state);
    }
}

JS_PUBLIC_API(JSErrorReport *)
JS_ErrorFromException(JSContext *cx, HandleValue value)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, value);
    return js_ErrorFromException(value);
}

JS_PUBLIC_API(bool)
JS_ThrowReportedError(JSContext *cx, const char *message,
                      JSErrorReport *reportp)
{
    AssertHeapIsIdle(cx);
    return JS_IsRunning(cx) &&
           js_ErrorToException(cx, message, reportp, nullptr, nullptr);
}

JS_PUBLIC_API(bool)
JS_ThrowStopIteration(JSContext *cx)
{
    AssertHeapIsIdle(cx);
    return js_ThrowStopIteration(cx);
}

JS_PUBLIC_API(bool)
JS_IsStopIteration(jsval v)
{
    return v.isObject() && v.toObject().is<StopIterationObject>();
}

JS_PUBLIC_API(intptr_t)
JS_GetCurrentThread()
{
#ifdef JS_THREADSAFE
    return reinterpret_cast<intptr_t>(PR_GetCurrentThread());
#else
    return 0;
#endif
}

extern JS_NEVER_INLINE JS_PUBLIC_API(void)
JS_AbortIfWrongThread(JSRuntime *rt)
{
    if (!CurrentThreadCanAccessRuntime(rt))
        MOZ_CRASH();
    if (!js::TlsPerThreadData.get()->associatedWith(rt))
        MOZ_CRASH();
}

#ifdef JS_GC_ZEAL
JS_PUBLIC_API(void)
JS_SetGCZeal(JSContext *cx, uint8_t zeal, uint32_t frequency)
{
    SetGCZeal(cx->runtime(), zeal, frequency);
}

JS_PUBLIC_API(void)
JS_ScheduleGC(JSContext *cx, uint32_t count)
{
    cx->runtime()->gcNextScheduled = count;
}
#endif

JS_PUBLIC_API(void)
JS_SetParallelParsingEnabled(JSContext *cx, bool enabled)
{
#ifdef JS_ION
    cx->runtime()->setCanUseHelperThreadsForParsing(enabled);
#endif
}

JS_PUBLIC_API(void)
JS_SetParallelIonCompilationEnabled(JSContext *cx, bool enabled)
{
#ifdef JS_ION
    cx->runtime()->setCanUseHelperThreadsForIonCompilation(enabled);
#endif
}

JS_PUBLIC_API(void)
JS_SetGlobalJitCompilerOption(JSContext *cx, JSJitCompilerOption opt, uint32_t value)
{
#ifdef JS_ION
    jit::IonOptions defaultValues;

    switch (opt) {
      case JSJITCOMPILER_BASELINE_USECOUNT_TRIGGER:
        if (value == uint32_t(-1))
            value = defaultValues.baselineUsesBeforeCompile;
        jit::js_IonOptions.baselineUsesBeforeCompile = value;
        break;
      case JSJITCOMPILER_ION_USECOUNT_TRIGGER:
        if (value == uint32_t(-1))
            value = defaultValues.usesBeforeCompile;
        jit::js_IonOptions.usesBeforeCompile = value;
        if (value == 0)
            jit::js_IonOptions.setEagerCompilation();
        break;

      default:
        break;
    }
#endif
}



#if !defined(STATIC_EXPORTABLE_JS_API) && !defined(STATIC_JS_API) && defined(XP_WIN)

#include "jswin.h"




BOOL WINAPI DllMain (HINSTANCE hDLL, DWORD dwReason, LPVOID lpReserved)
{
    return TRUE;
}

#endif

JS_PUBLIC_API(bool)
JS_IndexToId(JSContext *cx, uint32_t index, MutableHandleId id)
{
    return IndexToId(cx, index, id);
}

JS_PUBLIC_API(bool)
JS_CharsToId(JSContext* cx, JS::TwoByteChars chars, MutableHandleId idp)
{
    RootedAtom atom(cx, AtomizeChars<CanGC>(cx, chars.start().get(), chars.length()));
    if (!atom)
        return false;
#ifdef DEBUG
    uint32_t dummy;
    MOZ_ASSERT(!atom->isIndex(&dummy), "API misuse: |chars| must not encode an index");
#endif
    idp.set(AtomToId(atom));
    return true;
}

JS_PUBLIC_API(bool)
JS_IsIdentifier(JSContext *cx, HandleString str, bool *isIdentifier)
{
    assertSameCompartment(cx, str);

    JSLinearString* linearStr = str->ensureLinear(cx);
    if (!linearStr)
        return false;

    *isIdentifier = js::frontend::IsIdentifier(linearStr);
    return true;
}

JS_PUBLIC_API(bool)
JS_DescribeScriptedCaller(JSContext *cx, MutableHandleScript script, unsigned *lineno)
{
    script.set(nullptr);
    if (lineno)
        *lineno = 0;

    NonBuiltinScriptFrameIter i(cx);
    if (i.done())
        return false;

    script.set(i.script());
    if (lineno)
        *lineno = js::PCToLineNumber(i.script(), i.pc());
    return true;
}

#ifdef JS_THREADSAFE
static PRStatus
CallOnce(void *func)
{
    JSInitCallback init = JS_DATA_TO_FUNC_PTR(JSInitCallback, func);
    return init() ? PR_SUCCESS : PR_FAILURE;
}
#endif

JS_PUBLIC_API(bool)
JS_CallOnce(JSCallOnceType *once, JSInitCallback func)
{
#ifdef JS_THREADSAFE
    return PR_CallOnceWithArg(once, CallOnce, JS_FUNC_TO_DATA_PTR(void *, func)) == PR_SUCCESS;
#else
    if (!*once) {
        *once = true;
        return func();
    } else {
        return true;
    }
#endif
}

AutoGCRooter::AutoGCRooter(JSContext *cx, ptrdiff_t tag)
  : down(ContextFriendFields::get(cx)->autoGCRooters),
    tag_(tag),
    stackTop(&ContextFriendFields::get(cx)->autoGCRooters)
{
    JS_ASSERT(this != *stackTop);
    *stackTop = this;
}

AutoGCRooter::AutoGCRooter(ContextFriendFields *cx, ptrdiff_t tag)
  : down(cx->autoGCRooters),
    tag_(tag),
    stackTop(&cx->autoGCRooters)
{
    JS_ASSERT(this != *stackTop);
    *stackTop = this;
}

#ifdef DEBUG
JS_PUBLIC_API(void)
JS::AssertArgumentsAreSane(JSContext *cx, HandleValue value)
{
    AssertHeapIsIdle(cx);
    CHECK_REQUEST(cx);
    assertSameCompartment(cx, value);
}
#endif 

JS_PUBLIC_API(void *)
JS_EncodeScript(JSContext *cx, HandleScript scriptArg, uint32_t *lengthp)
{
    XDREncoder encoder(cx);
    RootedScript script(cx, scriptArg);
    if (!encoder.codeScript(&script))
        return nullptr;
    return encoder.forgetData(lengthp);
}

JS_PUBLIC_API(void *)
JS_EncodeInterpretedFunction(JSContext *cx, HandleObject funobjArg, uint32_t *lengthp)
{
    XDREncoder encoder(cx);
    RootedObject funobj(cx, funobjArg);
    if (!encoder.codeFunction(&funobj))
        return nullptr;
    return encoder.forgetData(lengthp);
}

JS_PUBLIC_API(JSScript *)
JS_DecodeScript(JSContext *cx, const void *data, uint32_t length,
                JSPrincipals *principals, JSPrincipals *originPrincipals)
{
    XDRDecoder decoder(cx, data, length, principals, originPrincipals);
    RootedScript script(cx);
    if (!decoder.codeScript(&script))
        return nullptr;
    return script;
}

JS_PUBLIC_API(JSObject *)
JS_DecodeInterpretedFunction(JSContext *cx, const void *data, uint32_t length,
                             JSPrincipals *principals, JSPrincipals *originPrincipals)
{
    XDRDecoder decoder(cx, data, length, principals, originPrincipals);
    RootedObject funobj(cx);
    if (!decoder.codeFunction(&funobj))
        return nullptr;
    return funobj;
}

JS_PUBLIC_API(JSObject *)
JS_GetScriptedGlobal(JSContext *cx)
{
    ScriptFrameIter i(cx);
    if (i.done())
        return cx->global();
    return &i.scopeChain()->global();
}

JS_PUBLIC_API(bool)
JS_PreventExtensions(JSContext *cx, JS::HandleObject obj)
{
    bool extensible;
    if (!JSObject::isExtensible(cx, obj, &extensible))
        return false;
    if (!extensible)
        return true;
    return JSObject::preventExtensions(cx, obj);
}

char *
JSAutoByteString::encodeLatin1(ExclusiveContext *cx, JSString *str)
{
    JSLinearString *linear = str->ensureLinear(cx);
    if (!linear)
        return nullptr;

    mBytes = LossyTwoByteCharsToNewLatin1CharsZ(cx, linear->range()).c_str();
    return mBytes;
}
