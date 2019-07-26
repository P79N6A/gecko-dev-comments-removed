/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef jscntxtinlines_h
#define jscntxtinlines_h

#include "jscntxt.h"

#include "jsiter.h"

#include "builtin/Object.h"
#include "jit/IonFrames.h"
#include "vm/ForkJoin.h"
#include "vm/Interpreter.h"
#include "vm/ProxyObject.h"

#include "vm/ObjectImpl-inl.h"

namespace js {

#ifdef JS_CRASH_DIAGNOSTICS
class CompartmentChecker
{
    JSCompartment *compartment;

  public:
    explicit CompartmentChecker(ExclusiveContext *cx)
      : compartment(cx->compartment_)
    {}

    /*
     * Set a breakpoint here (break js::CompartmentChecker::fail) to debug
     * compartment mismatches.
     */
    static void fail(JSCompartment *c1, JSCompartment *c2) {
        printf("*** Compartment mismatch %p vs. %p\n", (void *) c1, (void *) c2);
        MOZ_CRASH();
    }

    static void fail(JS::Zone *z1, JS::Zone *z2) {
        printf("*** Zone mismatch %p vs. %p\n", (void *) z1, (void *) z2);
        MOZ_CRASH();
    }

    /* Note: should only be used when neither c1 nor c2 may be the default compartment. */
    static void check(JSCompartment *c1, JSCompartment *c2) {
        JS_ASSERT(c1 != c1->runtimeFromMainThread()->atomsCompartment);
        JS_ASSERT(c2 != c2->runtimeFromMainThread()->atomsCompartment);
        if (c1 != c2)
            fail(c1, c2);
    }

    void check(JSCompartment *c) {
        if (c && c != compartment->runtimeFromMainThread()->atomsCompartment) {
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
#endif /* JS_CRASH_DIAGNOSTICS */

/*
 * Don't perform these checks when called from a finalizer. The checking
 * depends on other objects not having been swept yet.
 */
#define START_ASSERT_SAME_COMPARTMENT()                                       \
    if (cx->isHeapBusy())                                                     \
        return;                                                               \
    CompartmentChecker c(cx)

template <class T1> inline void
assertSameCompartment(ExclusiveContext *cx, const T1 &t1)
{
#ifdef JS_CRASH_DIAGNOSTICS
    START_ASSERT_SAME_COMPARTMENT();
    c.check(t1);
#endif
}

template <class T1> inline void
assertSameCompartmentDebugOnly(ExclusiveContext *cx, const T1 &t1)
{
#ifdef DEBUG
    START_ASSERT_SAME_COMPARTMENT();
    c.check(t1);
#endif
}

template <class T1, class T2> inline void
assertSameCompartment(ExclusiveContext *cx, const T1 &t1, const T2 &t2)
{
#ifdef JS_CRASH_DIAGNOSTICS
    START_ASSERT_SAME_COMPARTMENT();
    c.check(t1);
    c.check(t2);
#endif
}

template <class T1, class T2, class T3> inline void
assertSameCompartment(ExclusiveContext *cx, const T1 &t1, const T2 &t2, const T3 &t3)
{
#ifdef JS_CRASH_DIAGNOSTICS
    START_ASSERT_SAME_COMPARTMENT();
    c.check(t1);
    c.check(t2);
    c.check(t3);
#endif
}

template <class T1, class T2, class T3, class T4> inline void
assertSameCompartment(ExclusiveContext *cx,
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
assertSameCompartment(ExclusiveContext *cx,
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

    /*
     * Native constructors must return non-primitive values on success.
     * Although it is legal, if a constructor returns the callee, there is a
     * 99.9999% chance it is a bug. If any valid code actually wants the
     * constructor to return the callee, the assertion can be removed or
     * (another) conjunct can be added to the antecedent.
     *
     * Exceptions:
     *
     * - Proxies are exceptions to both rules: they can return primitives and
     *   they allow content to return the callee.
     *
     * - CallOrConstructBoundFunction is an exception as well because we might
     *   have used bind on a proxy function.
     *
     * - new Iterator(x) is user-hookable; it returns x.__iterator__() which
     *   could be any object.
     *
     * - (new Object(Object)) returns the callee.
     */
    JS_ASSERT_IF(native != FunctionProxyObject::class_.construct &&
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
GetNativeStackLimit(ExclusiveContext *cx)
{
    return cx->perThreadData->nativeStackLimit;
}

inline RegExpCompartment &
ExclusiveContext::regExps()
{
    return compartment_->regExps;
}

inline PropertyTree&
ExclusiveContext::propertyTree()
{
    return compartment_->propertyTree;
}

inline BaseShapeSet &
ExclusiveContext::baseShapes()
{
    return compartment_->baseShapes;
}

inline InitialShapeSet &
ExclusiveContext::initialShapes()
{
    return compartment_->initialShapes;
}

inline DtoaCache &
ExclusiveContext::dtoaCache()
{
    return compartment_->dtoaCache;
}

inline void
ExclusiveContext::maybePause() const
{
#ifdef JS_WORKER_THREADS
    if (workerThread && runtime_->workerThreadState->shouldPause) {
        AutoLockWorkerThreadState lock(runtime_);
        workerThread->pause();
    }
#endif
}

class AutoLockForExclusiveAccess
{
#ifdef JS_THREADSAFE
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
            runtime->exclusiveAccessOwner = NULL;
#endif
            PR_Unlock(runtime->exclusiveAccessLock);
        } else {
            JS_ASSERT(runtime->mainThreadHasExclusiveAccess);
            runtime->mainThreadHasExclusiveAccess = false;
        }
    }
#else // JS_THREADSAFE
  public:
    AutoLockForExclusiveAccess(ExclusiveContext *cx MOZ_GUARD_OBJECT_NOTIFIER_PARAM) {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }
    AutoLockForExclusiveAccess(JSRuntime *rt MOZ_GUARD_OBJECT_NOTIFIER_PARAM) {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }
    ~AutoLockForExclusiveAccess() {
        // An empty destructor is needed to avoid warnings from clang about
        // unused local variables of this type.
    }
#endif // JS_THREADSAFE

    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

}  /* namespace js */

inline js::LifoAlloc &
JSContext::analysisLifoAlloc()
{
    return compartment()->analysisLifoAlloc;
}

inline js::LifoAlloc &
JSContext::typeLifoAlloc()
{
    return zone()->types.typeLifoAlloc;
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
    defaultCompartmentObject_ = obj;
}

inline void
JSContext::setDefaultCompartmentObjectIfUnset(JSObject *obj)
{
    if (!defaultCompartmentObject_)
        setDefaultCompartmentObject(obj);
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

    // Only call leave() after we've setCompartment()-ed away from the current
    // compartment.
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
    // ExclusiveContexts can only be in the atoms zone or in exclusive zones.
    JS_ASSERT_IF(!isJSContext() && comp != runtime_->atomsCompartment,
                 comp->zone()->usedByExclusiveThread);

    // Normal JSContexts cannot enter exclusive zones.
    JS_ASSERT_IF(isJSContext() && comp,
                 !comp->zone()->usedByExclusiveThread);

    // Only one thread can be in the atoms compartment at a time.
    JS_ASSERT_IF(comp == runtime_->atomsCompartment,
                 runtime_->currentThreadHasExclusiveAccess());

    // Make sure that the atoms compartment has its own zone.
    JS_ASSERT_IF(comp && comp != runtime_->atomsCompartment,
                 comp->zone() != runtime_->atomsCompartment->zone());

    // Both the current and the new compartment should be properly marked as
    // entered at this point.
    JS_ASSERT_IF(compartment_, compartment_->hasBeenEntered());
    JS_ASSERT_IF(comp, comp->hasBeenEntered());

    compartment_ = comp;
    zone_ = comp ? comp->zone() : NULL;
    allocator_ = zone_ ? &zone_->allocator : NULL;
}

inline JSScript *
JSContext::currentScript(jsbytecode **ppc,
                         MaybeAllowCrossCompartment allowCrossCompartment) const
{
    if (ppc)
        *ppc = NULL;

    js::Activation *act = mainThread().activation();
    while (act && (act->cx() != this || (act->isJit() && !act->asJit()->isActive())))
        act = act->prev();

    if (!act)
        return NULL;

    JS_ASSERT(act->cx() == this);

#ifdef JS_ION
    if (act->isJit()) {
        JSScript *script = NULL;
        js::ion::GetPcScript(const_cast<JSContext *>(this), &script, ppc);
        if (!allowCrossCompartment && script->compartment() != compartment())
            return NULL;
        return script;
    }
#endif

    JS_ASSERT(act->isInterpreter());

    js::StackFrame *fp = act->asInterpreter()->current();
    JS_ASSERT(!fp->runningInJit());

    JSScript *script = fp->script();
    if (!allowCrossCompartment && script->compartment() != compartment())
        return NULL;

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
inline js::ParallelResult
JSParallelNativeThreadSafeWrapper(js::ForkJoinSlice *slice, unsigned argc, JS::Value *vp)
{
    return threadSafeNative(slice, argc, vp) ? js::TP_SUCCESS : js::TP_FATAL;
}

#endif /* jscntxtinlines_h */
