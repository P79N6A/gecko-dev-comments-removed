





#ifndef jscntxtinlines_h
#define jscntxtinlines_h

#include "jscntxt.h"
#include "jscompartment.h"

#include "jsiter.h"
#include "jsworkers.h"

#include "builtin/Object.h"
#include "jit/IonFrames.h"
#include "vm/ForkJoin.h"
#include "vm/Interpreter.h"
#include "vm/ProxyObject.h"

namespace js {

#ifdef JS_CRASH_DIAGNOSTICS
class CompartmentChecker
{
    JSCompartment *compartment;

  public:
    explicit CompartmentChecker(ExclusiveContext *cx)
      : compartment(cx->compartment())
    {}

    



    static void fail(JSCompartment *c1, JSCompartment *c2) {
        printf("*** Compartment mismatch %p vs. %p\n", (void *) c1, (void *) c2);
        MOZ_CRASH();
    }

    static void fail(JS::Zone *z1, JS::Zone *z2) {
        printf("*** Zone mismatch %p vs. %p\n", (void *) z1, (void *) z2);
        MOZ_CRASH();
    }

    
    static void check(JSCompartment *c1, JSCompartment *c2) {
        JS_ASSERT(!c1->runtimeFromAnyThread()->isAtomsCompartment(c1));
        JS_ASSERT(!c2->runtimeFromAnyThread()->isAtomsCompartment(c2));
        if (c1 != c2)
            fail(c1, c2);
    }

    void check(JSCompartment *c) {
        if (c && !compartment->runtimeFromAnyThread()->isAtomsCompartment(c)) {
            if (!compartment)
                compartment = c;
            else if (c != compartment)
                fail(compartment, c);
        }
    }

    void checkZone(JS::Zone *z) {
        if (compartment && z != compartment->zone())
            fail(compartment->zone(), z);
    }

    void check(JSObject *obj) {
        if (obj)
            check(obj->compartment());
    }

    template<typename T>
    void check(Handle<T> handle) {
        check(handle.get());
    }

    void check(JSString *str) {
        if (!str->isAtom())
            checkZone(str->zone());
    }

    void check(const js::Value &v) {
        if (v.isObject())
            check(&v.toObject());
        else if (v.isString())
            check(v.toString());
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
            for (int i = 0; i < ida->length; i++) {
                if (JSID_IS_OBJECT(ida->vector[i]))
                    check(ida->vector[i]);
            }
        }
    }

    void check(JSScript *script) {
        if (script)
            check(script->compartment());
    }

    void check(StackFrame *fp);
    void check(AbstractFramePtr frame);
};
#endif 





#define START_ASSERT_SAME_COMPARTMENT()                                       \
    if (!cx->isExclusiveContext())                                            \
        return;                                                               \
    if (cx->isHeapBusy())                                                     \
        return;                                                               \
    CompartmentChecker c(cx->asExclusiveContext())

template <class T1> inline void
assertSameCompartment(ThreadSafeContext *cx, const T1 &t1)
{
#ifdef JS_CRASH_DIAGNOSTICS
    START_ASSERT_SAME_COMPARTMENT();
    c.check(t1);
#endif
}

template <class T1> inline void
assertSameCompartmentDebugOnly(ThreadSafeContext *cx, const T1 &t1)
{
#ifdef DEBUG
    START_ASSERT_SAME_COMPARTMENT();
    c.check(t1);
#endif
}

template <class T1, class T2> inline void
assertSameCompartment(ThreadSafeContext *cx, const T1 &t1, const T2 &t2)
{
#ifdef JS_CRASH_DIAGNOSTICS
    START_ASSERT_SAME_COMPARTMENT();
    c.check(t1);
    c.check(t2);
#endif
}

template <class T1, class T2, class T3> inline void
assertSameCompartment(ThreadSafeContext *cx, const T1 &t1, const T2 &t2, const T3 &t3)
{
#ifdef JS_CRASH_DIAGNOSTICS
    START_ASSERT_SAME_COMPARTMENT();
    c.check(t1);
    c.check(t2);
    c.check(t3);
#endif
}

template <class T1, class T2, class T3, class T4> inline void
assertSameCompartment(ThreadSafeContext *cx,
                      const T1 &t1, const T2 &t2, const T3 &t3, const T4 &t4)
{
#ifdef JS_CRASH_DIAGNOSTICS
    START_ASSERT_SAME_COMPARTMENT();
    c.check(t1);
    c.check(t2);
    c.check(t3);
    c.check(t4);
#endif
}

template <class T1, class T2, class T3, class T4, class T5> inline void
assertSameCompartment(ThreadSafeContext *cx,
                      const T1 &t1, const T2 &t2, const T3 &t3, const T4 &t4, const T5 &t5)
{
#ifdef JS_CRASH_DIAGNOSTICS
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
CallJSNative(JSContext *cx, Native native, const CallArgs &args)
{
    JS_CHECK_RECURSION(cx, return false);

#ifdef DEBUG
    bool alreadyThrowing = cx->isExceptionPending();
#endif
    assertSameCompartment(cx, args);
    bool ok = native(cx, args.length(), args.base());
    if (ok) {
        assertSameCompartment(cx, args.rval());
        JS_ASSERT_IF(!alreadyThrowing, !cx->isExceptionPending());
    }
    return ok;
}

STATIC_PRECONDITION_ASSUME(ubound(args.argv_) >= argc)
JS_ALWAYS_INLINE bool
CallNativeImpl(JSContext *cx, NativeImpl impl, const CallArgs &args)
{
#ifdef DEBUG
    bool alreadyThrowing = cx->isExceptionPending();
#endif
    assertSameCompartment(cx, args);
    bool ok = impl(cx, args);
    if (ok) {
        assertSameCompartment(cx, args.rval());
        JS_ASSERT_IF(!alreadyThrowing, !cx->isExceptionPending());
    }
    return ok;
}

STATIC_PRECONDITION(ubound(args.argv_) >= argc)
JS_ALWAYS_INLINE bool
CallJSNativeConstructor(JSContext *cx, Native native, const CallArgs &args)
{
#ifdef DEBUG
    RootedObject callee(cx, &args.callee());
#endif

    JS_ASSERT(args.thisv().isMagic());
    if (!CallJSNative(cx, native, args))
        return false;

    



















    JS_ASSERT_IF(native != ProxyObject::callableClass_.construct &&
                 native != js::CallOrConstructBoundFunction &&
                 native != js::IteratorConstructor &&
                 (!callee->is<JSFunction>() || callee->as<JSFunction>().native() != obj_construct),
                 !args.rval().isPrimitive() && callee != &args.rval().toObject());

    return true;
}

JS_ALWAYS_INLINE bool
CallJSPropertyOp(JSContext *cx, PropertyOp op, HandleObject receiver, HandleId id, MutableHandleValue vp)
{
    JS_CHECK_RECURSION(cx, return false);

    assertSameCompartment(cx, receiver, id, vp);
    bool ok = op(cx, receiver, id, vp);
    if (ok)
        assertSameCompartment(cx, vp);
    return ok;
}

JS_ALWAYS_INLINE bool
CallJSPropertyOpSetter(JSContext *cx, StrictPropertyOp op, HandleObject obj, HandleId id,
                       bool strict, MutableHandleValue vp)
{
    JS_CHECK_RECURSION(cx, return false);

    assertSameCompartment(cx, obj, id, vp);
    return op(cx, obj, id, strict, vp);
}

static inline bool
CallJSDeletePropertyOp(JSContext *cx, JSDeletePropertyOp op, HandleObject receiver, HandleId id,
                       bool *succeeded)
{
    JS_CHECK_RECURSION(cx, return false);

    assertSameCompartment(cx, receiver, id);
    return op(cx, receiver, id, succeeded);
}

inline bool
CallSetter(JSContext *cx, HandleObject obj, HandleId id, StrictPropertyOp op, unsigned attrs,
           unsigned shortid, bool strict, MutableHandleValue vp)
{
    if (attrs & JSPROP_SETTER) {
        RootedValue opv(cx, CastAsObjectJsval(op));
        return InvokeGetterOrSetter(cx, obj, opv, 1, vp.address(), vp);
    }

    if (attrs & JSPROP_GETTER)
        return js_ReportGetterOnlyAssignment(cx, strict);

    if (!(attrs & JSPROP_SHORTID))
        return CallJSPropertyOpSetter(cx, op, obj, id, strict, vp);

    RootedId nid(cx, INT_TO_JSID(shortid));

    return CallJSPropertyOpSetter(cx, op, obj, nid, strict, vp);
}

inline uintptr_t
GetNativeStackLimit(ThreadSafeContext *cx)
{
    StackKind kind;
    if (cx->isJSContext()) {
        kind = cx->asJSContext()->runningWithTrustedPrincipals()
                 ? StackForTrustedScript : StackForUntrustedScript;
    } else {
        
        
        kind = StackForTrustedScript;
    }
    return cx->perThreadData->nativeStackLimit[kind];
}

inline void
ExclusiveContext::maybePause() const
{
#ifdef JS_WORKER_THREADS
    if (workerThread())
        workerThread()->maybePause();
#endif
}

class AutoLockForExclusiveAccess
{
#ifdef JS_WORKER_THREADS
    JSRuntime *runtime;

    void init(JSRuntime *rt) {
        runtime = rt;
        if (runtime->numExclusiveThreads) {
            PR_Lock(runtime->exclusiveAccessLock);
#ifdef DEBUG
            runtime->exclusiveAccessOwner = PR_GetCurrentThread();
#endif
        } else {
            JS_ASSERT(!runtime->mainThreadHasExclusiveAccess);
            runtime->mainThreadHasExclusiveAccess = true;
        }
    }

  public:
    AutoLockForExclusiveAccess(ExclusiveContext *cx MOZ_GUARD_OBJECT_NOTIFIER_PARAM) {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        init(cx->runtime_);
    }
    AutoLockForExclusiveAccess(JSRuntime *rt MOZ_GUARD_OBJECT_NOTIFIER_PARAM) {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        init(rt);
    }
    ~AutoLockForExclusiveAccess() {
        if (runtime->numExclusiveThreads) {
            JS_ASSERT(runtime->exclusiveAccessOwner == PR_GetCurrentThread());
#ifdef DEBUG
            runtime->exclusiveAccessOwner = nullptr;
#endif
            PR_Unlock(runtime->exclusiveAccessLock);
        } else {
            JS_ASSERT(runtime->mainThreadHasExclusiveAccess);
            runtime->mainThreadHasExclusiveAccess = false;
        }
    }
#else 
  public:
    AutoLockForExclusiveAccess(ExclusiveContext *cx MOZ_GUARD_OBJECT_NOTIFIER_PARAM) {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }
    AutoLockForExclusiveAccess(JSRuntime *rt MOZ_GUARD_OBJECT_NOTIFIER_PARAM) {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }
    ~AutoLockForExclusiveAccess() {
        
        
    }
#endif 

    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

inline LifoAlloc &
ExclusiveContext::typeLifoAlloc()
{
    return zone()->types.typeLifoAlloc;
}

}  

inline void
JSContext::setPendingException(js::Value v) {
    JS_ASSERT(!IsPoisonedValue(v));
    this->throwing = true;
    this->exception = v;
    js::assertSameCompartment(this, v);
}

inline void
JSContext::setDefaultCompartmentObject(JSObject *obj)
{
    JS_ASSERT(!hasOption(JSOPTION_NO_DEFAULT_COMPARTMENT_OBJECT));
    defaultCompartmentObject_ = obj;
}

inline void
JSContext::setDefaultCompartmentObjectIfUnset(JSObject *obj)
{
    if (!hasOption(JSOPTION_NO_DEFAULT_COMPARTMENT_OBJECT) &&
        !defaultCompartmentObject_)
    {
        setDefaultCompartmentObject(obj);
    }
}

inline void
js::ExclusiveContext::enterCompartment(JSCompartment *c)
{
    enterCompartmentDepth_++;
    c->enter();
    setCompartment(c);

    if (JSContext *cx = maybeJSContext()) {
        if (cx->throwing)
            cx->wrapPendingException();
    }
}

inline void
js::ExclusiveContext::leaveCompartment(JSCompartment *oldCompartment)
{
    JS_ASSERT(hasEnteredCompartment());
    enterCompartmentDepth_--;

    
    
    JSCompartment *startingCompartment = compartment_;
    setCompartment(oldCompartment);
    startingCompartment->leave();

    if (JSContext *cx = maybeJSContext()) {
        if (cx->throwing && oldCompartment)
            cx->wrapPendingException();
    }
}

inline void
js::ExclusiveContext::setCompartment(JSCompartment *comp)
{
    
    JS_ASSERT_IF(!isJSContext() && !runtime_->isAtomsCompartment(comp),
                 comp->zone()->usedByExclusiveThread);

    
    JS_ASSERT_IF(isJSContext() && comp,
                 !comp->zone()->usedByExclusiveThread);

    
    JS_ASSERT_IF(runtime_->isAtomsCompartment(comp),
                 runtime_->currentThreadHasExclusiveAccess());

    
    JS_ASSERT_IF(comp && !runtime_->isAtomsCompartment(comp),
                 !runtime_->isAtomsZone(comp->zone()));

    
    
    JS_ASSERT_IF(compartment_, compartment_->hasBeenEntered());
    JS_ASSERT_IF(comp, comp->hasBeenEntered());

    compartment_ = comp;
    zone_ = comp ? comp->zone() : nullptr;
    allocator_ = zone_ ? &zone_->allocator : nullptr;
}

inline JSScript *
JSContext::currentScript(jsbytecode **ppc,
                         MaybeAllowCrossCompartment allowCrossCompartment) const
{
    if (ppc)
        *ppc = nullptr;

    js::Activation *act = mainThread().activation();
    while (act && (act->cx() != this || (act->isJit() && !act->asJit()->isActive())))
        act = act->prev();

    if (!act)
        return nullptr;

    JS_ASSERT(act->cx() == this);

#ifdef JS_ION
    if (act->isJit()) {
        JSScript *script = nullptr;
        js::jit::GetPcScript(const_cast<JSContext *>(this), &script, ppc);
        if (!allowCrossCompartment && script->compartment() != compartment())
            return nullptr;
        return script;
    }
#endif

    JS_ASSERT(act->isInterpreter());

    js::StackFrame *fp = act->asInterpreter()->current();
    JS_ASSERT(!fp->runningInJit());

    JSScript *script = fp->script();
    if (!allowCrossCompartment && script->compartment() != compartment())
        return nullptr;

    if (ppc) {
        *ppc = act->asInterpreter()->regs().pc;
        JS_ASSERT(*ppc >= script->code && *ppc < script->code + script->length);
    }
    return script;
}

template <JSThreadSafeNative threadSafeNative>
inline bool
JSNativeThreadSafeWrapper(JSContext *cx, unsigned argc, JS::Value *vp)
{
    return threadSafeNative(cx, argc, vp);
}

template <JSThreadSafeNative threadSafeNative>
inline bool
JSParallelNativeThreadSafeWrapper(js::ForkJoinSlice *slice, unsigned argc, JS::Value *vp)
{
    return threadSafeNative(slice, argc, vp);
}

 inline JSContext *
js::ExecutionModeTraits<js::SequentialExecution>::toContextType(ExclusiveContext *cx)
{
    return cx->asJSContext();
}

#endif
