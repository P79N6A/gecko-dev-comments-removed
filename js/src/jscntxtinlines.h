







































#ifndef jscntxtinlines_h___
#define jscntxtinlines_h___

#include "jscntxt.h"
#include "jscompartment.h"
#include "jsstaticcheck.h"
#include "jsxml.h"
#include "jsregexp.h"
#include "jsgc.h"

#include "frontend/ParseMaps.h"

namespace js {

static inline GlobalObject *
GetGlobalForScopeChain(JSContext *cx)
{
    







    VOUCH_DOES_NOT_REQUIRE_STACK();

    if (cx->hasfp())
        return cx->fp()->scopeChain().getGlobal();

    JSObject *scope = cx->globalObject;
    if (!scope) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_INACTIVE);
        return NULL;
    }
    OBJ_TO_INNER_OBJECT(cx, scope);
    return scope->asGlobal();
}

inline GSNCache *
GetGSNCache(JSContext *cx)
{
    return &JS_THREAD_DATA(cx)->gsnCache;
}

class AutoNamespaceArray : protected AutoGCRooter {
  public:
    AutoNamespaceArray(JSContext *cx) : AutoGCRooter(cx, NAMESPACES) {
        array.init();
    }

    ~AutoNamespaceArray() {
        array.finish(context);
    }

    uint32 length() const { return array.length; }

  public:
    friend void AutoGCRooter::trace(JSTracer *trc);

    JSXMLArray array;
};

#ifdef DEBUG
class CompartmentChecker
{
  private:
    JSContext *context;
    JSCompartment *compartment;

  public:
    explicit CompartmentChecker(JSContext *cx) : context(cx), compartment(cx->compartment) {
        check(cx->hasfp() ? JS_GetGlobalForScopeChain(cx) : cx->globalObject);
        VOUCH_DOES_NOT_REQUIRE_STACK();
    }

    



    static void fail(JSCompartment *c1, JSCompartment *c2) {
        printf("*** Compartment mismatch %p vs. %p\n", (void *) c1, (void *) c2);
        JS_NOT_REACHED("compartment mismatched");
    }

    
    static void check(JSCompartment *c1, JSCompartment *c2) {
        JS_ASSERT(c1 != c1->rt->atomsCompartment);
        JS_ASSERT(c2 != c2->rt->atomsCompartment);
        if (c1 != c2)
            fail(c1, c2);
    }

    void check(JSCompartment *c) {
        if (c && c != context->runtime->atomsCompartment) {
            if (!compartment)
                compartment = c;
            else if (c != compartment)
                fail(compartment, c);
        }
    }

    void check(JSPrincipals *) {  }

    void check(JSObject *obj) {
        if (obj)
            check(obj->compartment());
    }

    void check(JSString *str) {
        if (!str->isAtom())
            check(str->compartment());
    }

    void check(const js::Value &v) {
        if (v.isObject())
            check(&v.toObject());
        else if (v.isString())
            check(v.toString());
    }

    void check(jsval v) {
        check(Valueify(v));
    }

    void check(const ValueArray &arr) {
        for (size_t i = 0; i < arr.length; i++)
            check(arr.array[i]);
    }

    void check(const JSValueArray &arr) {
        for (size_t i = 0; i < arr.length; i++)
            check(arr.array[i]);
    }

    void check(const CallArgs &args) {
        for (Value *p = args.base(); p != args.end(); ++p)
            check(*p);
    }

    void check(jsid id) {
        if (JSID_IS_OBJECT(id))
            check(JSID_TO_OBJECT(id));
    }
    
    void check(JSIdArray *ida) {
        if (ida) {
            for (jsint i = 0; i < ida->length; i++) {
                if (JSID_IS_OBJECT(ida->vector[i]))
                    check(ida->vector[i]);
            }
        }
    }

    void check(JSScript *script) {
        if (script) {
            check(script->compartment);
            if (script->u.object)
                check(script->u.object);
        }
    }

    void check(StackFrame *fp) {
        check(&fp->scopeChain());
    }
};

#endif





#define START_ASSERT_SAME_COMPARTMENT()                                       \
    if (cx->runtime->gcRunning)                                               \
        return;                                                               \
    CompartmentChecker c(cx)

template <class T1> inline void
assertSameCompartment(JSContext *cx, T1 t1)
{
#ifdef DEBUG
    START_ASSERT_SAME_COMPARTMENT();
    c.check(t1);
#endif
}

template <class T1, class T2> inline void
assertSameCompartment(JSContext *cx, T1 t1, T2 t2)
{
#ifdef DEBUG
    START_ASSERT_SAME_COMPARTMENT();
    c.check(t1);
    c.check(t2);
#endif
}

template <class T1, class T2, class T3> inline void
assertSameCompartment(JSContext *cx, T1 t1, T2 t2, T3 t3)
{
#ifdef DEBUG
    START_ASSERT_SAME_COMPARTMENT();
    c.check(t1);
    c.check(t2);
    c.check(t3);
#endif
}

template <class T1, class T2, class T3, class T4> inline void
assertSameCompartment(JSContext *cx, T1 t1, T2 t2, T3 t3, T4 t4)
{
#ifdef DEBUG
    START_ASSERT_SAME_COMPARTMENT();
    c.check(t1);
    c.check(t2);
    c.check(t3);
    c.check(t4);
#endif
}

template <class T1, class T2, class T3, class T4, class T5> inline void
assertSameCompartment(JSContext *cx, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5)
{
#ifdef DEBUG
    START_ASSERT_SAME_COMPARTMENT();
    c.check(t1);
    c.check(t2);
    c.check(t3);
    c.check(t4);
    c.check(t5);
#endif
}

#undef START_ASSERT_SAME_COMPARTMENT

STATIC_PRECONDITION_ASSUME(ubound(args.argv_) >= argc)
JS_ALWAYS_INLINE bool
CallJSNative(JSContext *cx, js::Native native, const CallArgs &args)
{
#ifdef DEBUG
    JSBool alreadyThrowing = cx->isExceptionPending();
#endif
    assertSameCompartment(cx, args);
    JSBool ok = native(cx, args.argc(), args.base());
    if (ok) {
        assertSameCompartment(cx, args.rval());
        JS_ASSERT_IF(!alreadyThrowing, !cx->isExceptionPending());
    }
    return ok;
}

extern JSBool CallOrConstructBoundFunction(JSContext *, uintN, js::Value *);

STATIC_PRECONDITION(ubound(args.argv_) >= argc)
JS_ALWAYS_INLINE bool
CallJSNativeConstructor(JSContext *cx, js::Native native, const CallArgs &args)
{
#ifdef DEBUG
    JSObject &callee = args.callee();
#endif

    JS_ASSERT(args.thisv().isMagic());
    if (!CallJSNative(cx, native, args))
        return false;

    














    extern JSBool proxy_Construct(JSContext *, uintN, Value *);
    extern JSBool callable_Construct(JSContext *, uintN, Value *);
    JS_ASSERT_IF(native != proxy_Construct &&
                 native != callable_Construct &&
                 native != js::CallOrConstructBoundFunction &&
                 (!callee.isFunction() || callee.getFunctionPrivate()->u.n.clasp != &js_ObjectClass),
                 !args.rval().isPrimitive() && callee != args.rval().toObject());

    return true;
}

JS_ALWAYS_INLINE bool
CallJSPropertyOp(JSContext *cx, js::PropertyOp op, JSObject *receiver, jsid id, js::Value *vp)
{
    assertSameCompartment(cx, receiver, id, *vp);
    JSBool ok = op(cx, receiver, id, vp);
    if (ok)
        assertSameCompartment(cx, receiver, *vp);
    return ok;
}

JS_ALWAYS_INLINE bool
CallJSPropertyOpSetter(JSContext *cx, js::StrictPropertyOp op, JSObject *obj, jsid id,
                       JSBool strict, js::Value *vp)
{
    assertSameCompartment(cx, obj, id, *vp);
    return op(cx, obj, id, strict, vp);
}

inline bool
CallSetter(JSContext *cx, JSObject *obj, jsid id, js::StrictPropertyOp op, uintN attrs,
           uintN shortid, JSBool strict, js::Value *vp)
{
    if (attrs & JSPROP_SETTER)
        return ExternalGetOrSet(cx, obj, id, CastAsObjectJsval(op), JSACC_WRITE, 1, vp, vp);

    if (attrs & JSPROP_GETTER)
        return js_ReportGetterOnlyAssignment(cx);

    if (attrs & JSPROP_SHORTID)
        id = INT_TO_JSID(shortid);
    return CallJSPropertyOpSetter(cx, op, obj, id, strict, vp);
}

#ifdef JS_TRACER







JS_FORCES_STACK JS_FRIEND_API(void)
DeepBail(JSContext *cx);
#endif

static JS_INLINE void
LeaveTraceIfGlobalObject(JSContext *cx, JSObject *obj)
{
    if (!obj->parent)
        LeaveTrace(cx);
}

static JS_INLINE void
LeaveTraceIfArgumentsObject(JSContext *cx, JSObject *obj)
{
    if (obj->isArguments())
        LeaveTrace(cx);
}

}  

#ifdef JS_METHODJIT
inline js::mjit::JaegerCompartment *JSContext::jaegerCompartment()
{
    return compartment->jaegerCompartment();
}
#endif

inline bool
JSContext::ensureGeneratorStackSpace()
{
    bool ok = genStack.reserve(genStack.length() + 1);
    if (!ok)
        js_ReportOutOfMemory(this);
    return ok;
}

inline js::RegExpStatics *
JSContext::regExpStatics()
{
    return js::RegExpStatics::extractFrom(js::GetGlobalForScopeChain(this));
}

inline void
JSContext::setPendingException(js::Value v) {
    this->throwing = true;
    this->exception = v;
    assertSameCompartment(this, v);
}

inline bool
JSContext::ensureParseMapPool()
{
    if (parseMapPool_)
        return true;
    parseMapPool_ = js::OffTheBooks::new_<js::ParseMapPool>(this);
    return parseMapPool_;
}

#endif 
