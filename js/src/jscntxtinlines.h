





#ifndef jscntxtinlines_h
#define jscntxtinlines_h

#include "jscntxt.h"
#include "jscompartment.h"

#include "jsiter.h"

#include "builtin/Object.h"
#include "jit/JitFrames.h"
#include "vm/HelperThreads.h"
#include "vm/Interpreter.h"
#include "vm/ProxyObject.h"
#include "vm/Symbol.h"

namespace js {

#ifdef JS_CRASH_DIAGNOSTICS
class CompartmentChecker
{
    JSCompartment* compartment;

  public:
    explicit CompartmentChecker(ExclusiveContext* cx)
      : compartment(cx->compartment())
    {
#ifdef DEBUG
        
        
        JSContext* activeContext = nullptr;
        if (cx->isJSContext())
            activeContext = cx->asJSContext()->runtime()->activeContext;
        MOZ_ASSERT_IF(activeContext, cx == activeContext);
#endif
    }

    



    static void fail(JSCompartment* c1, JSCompartment* c2) {
        printf("*** Compartment mismatch %p vs. %p\n", (void*) c1, (void*) c2);
        MOZ_CRASH();
    }

    static void fail(JS::Zone* z1, JS::Zone* z2) {
        printf("*** Zone mismatch %p vs. %p\n", (void*) z1, (void*) z2);
        MOZ_CRASH();
    }

    
    static void check(JSCompartment* c1, JSCompartment* c2) {
        MOZ_ASSERT(!c1->runtimeFromAnyThread()->isAtomsCompartment(c1));
        MOZ_ASSERT(!c2->runtimeFromAnyThread()->isAtomsCompartment(c2));
        if (c1 != c2)
            fail(c1, c2);
    }

    void check(JSCompartment* c) {
        if (c && !compartment->runtimeFromAnyThread()->isAtomsCompartment(c)) {
            if (!compartment)
                compartment = c;
            else if (c != compartment)
                fail(compartment, c);
        }
    }

    void checkZone(JS::Zone* z) {
        if (compartment && z != compartment->zone())
            fail(compartment->zone(), z);
    }

    void check(JSObject* obj) {
        if (obj)
            check(obj->compartment());
    }

    template<typename T>
    void check(const Rooted<T>& rooted) {
        check(rooted.get());
    }

    template<typename T>
    void check(Handle<T> handle) {
        check(handle.get());
    }

    void check(JSString* str) {
        if (!str->isAtom())
            checkZone(str->zone());
    }

    void check(const js::Value& v) {
        if (v.isObject())
            check(&v.toObject());
        else if (v.isString())
            check(v.toString());
    }

    void check(const ValueArray& arr) {
        for (size_t i = 0; i < arr.length; i++)
            check(arr.array[i]);
    }

    void check(const JSValueArray& arr) {
        for (size_t i = 0; i < arr.length; i++)
            check(arr.array[i]);
    }

    void check(const JS::HandleValueArray& arr) {
        for (size_t i = 0; i < arr.length(); i++)
            check(arr[i]);
    }

    void check(const CallArgs& args) {
        for (Value* p = args.base(); p != args.end(); ++p)
            check(*p);
    }

    void check(jsid id) {}

    void check(JSScript* script) {
        if (script)
            check(script->compartment());
    }

    void check(InterpreterFrame* fp);
    void check(AbstractFramePtr frame);
    void check(SavedStacks* stacks);

    void check(Handle<JSPropertyDescriptor> desc) {
        check(desc.object());
        if (desc.hasGetterObject())
            check(desc.getterObject());
        if (desc.hasSetterObject())
            check(desc.setterObject());
        check(desc.value());
    }
};
#endif 





#define START_ASSERT_SAME_COMPARTMENT()                                       \
    if (cx->isJSContext() && cx->asJSContext()->runtime()->isHeapBusy())      \
        return;                                                               \
    CompartmentChecker c(cx)

template <class T1> inline void
assertSameCompartment(ExclusiveContext* cx, const T1& t1)
{
#ifdef JS_CRASH_DIAGNOSTICS
    START_ASSERT_SAME_COMPARTMENT();
    c.check(t1);
#endif
}

template <class T1> inline void
assertSameCompartmentDebugOnly(ExclusiveContext* cx, const T1& t1)
{
#if defined(DEBUG) && defined(JS_CRASH_DIAGNOSTICS)
    START_ASSERT_SAME_COMPARTMENT();
    c.check(t1);
#endif
}

template <class T1, class T2> inline void
assertSameCompartment(ExclusiveContext* cx, const T1& t1, const T2& t2)
{
#ifdef JS_CRASH_DIAGNOSTICS
    START_ASSERT_SAME_COMPARTMENT();
    c.check(t1);
    c.check(t2);
#endif
}

template <class T1, class T2, class T3> inline void
assertSameCompartment(ExclusiveContext* cx, const T1& t1, const T2& t2, const T3& t3)
{
#ifdef JS_CRASH_DIAGNOSTICS
    START_ASSERT_SAME_COMPARTMENT();
    c.check(t1);
    c.check(t2);
    c.check(t3);
#endif
}

template <class T1, class T2, class T3, class T4> inline void
assertSameCompartment(ExclusiveContext* cx,
                      const T1& t1, const T2& t2, const T3& t3, const T4& t4)
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
assertSameCompartment(ExclusiveContext* cx,
                      const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5)
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
MOZ_ALWAYS_INLINE bool
CallJSNative(JSContext* cx, Native native, const CallArgs& args)
{
    JS_CHECK_RECURSION(cx, return false);

#ifdef DEBUG
    bool alreadyThrowing = cx->isExceptionPending();
#endif
    assertSameCompartment(cx, args);
    bool ok = native(cx, args.length(), args.base());
    if (ok) {
        assertSameCompartment(cx, args.rval());
        MOZ_ASSERT_IF(!alreadyThrowing, !cx->isExceptionPending());
    }
    return ok;
}

STATIC_PRECONDITION_ASSUME(ubound(args.argv_) >= argc)
MOZ_ALWAYS_INLINE bool
CallNativeImpl(JSContext* cx, NativeImpl impl, const CallArgs& args)
{
#ifdef DEBUG
    bool alreadyThrowing = cx->isExceptionPending();
#endif
    assertSameCompartment(cx, args);
    bool ok = impl(cx, args);
    if (ok) {
        assertSameCompartment(cx, args.rval());
        MOZ_ASSERT_IF(!alreadyThrowing, !cx->isExceptionPending());
    }
    return ok;
}

STATIC_PRECONDITION(ubound(args.argv_) >= argc)
MOZ_ALWAYS_INLINE bool
CallJSNativeConstructor(JSContext* cx, Native native, const CallArgs& args)
{
#ifdef DEBUG
    RootedObject callee(cx, &args.callee());
#endif

    MOZ_ASSERT(args.thisv().isMagic());
    if (!CallJSNative(cx, native, args))
        return false;

    



















    MOZ_ASSERT_IF(native != js::proxy_Construct &&
                  native != js::CallOrConstructBoundFunction &&
                  native != js::IteratorConstructor &&
                  (!callee->is<JSFunction>() || callee->as<JSFunction>().native() != obj_construct),
                  args.rval().isObject() && callee != &args.rval().toObject());

    return true;
}

MOZ_ALWAYS_INLINE bool
CallJSGetterOp(JSContext* cx, GetterOp op, HandleObject obj, HandleId id,
               MutableHandleValue vp)
{
    JS_CHECK_RECURSION(cx, return false);

    assertSameCompartment(cx, obj, id, vp);
    bool ok = op(cx, obj, id, vp);
    if (ok)
        assertSameCompartment(cx, vp);
    return ok;
}

MOZ_ALWAYS_INLINE bool
CallJSSetterOp(JSContext* cx, SetterOp op, HandleObject obj, HandleId id, MutableHandleValue vp,
               ObjectOpResult& result)
{
    JS_CHECK_RECURSION(cx, return false);

    assertSameCompartment(cx, obj, id, vp);
    return op(cx, obj, id, vp, result);
}

inline bool
CallJSAddPropertyOp(JSContext* cx, JSAddPropertyOp op, HandleObject obj, HandleId id,
                    HandleValue v)
{
    JS_CHECK_RECURSION(cx, return false);

    assertSameCompartment(cx, obj, id, v);
    return op(cx, obj, id, v);
}

inline bool
CallJSDeletePropertyOp(JSContext* cx, JSDeletePropertyOp op, HandleObject receiver, HandleId id,
                       ObjectOpResult& result)
{
    JS_CHECK_RECURSION(cx, return false);

    assertSameCompartment(cx, receiver, id);
    if (op)
        return op(cx, receiver, id, result);
    return result.succeed();
}

inline uintptr_t
GetNativeStackLimit(ExclusiveContext* cx)
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

inline LifoAlloc&
ExclusiveContext::typeLifoAlloc()
{
    return zone()->types.typeLifoAlloc;
}

}  

inline void
JSContext::setPendingException(js::Value v)
{
    
    this->overRecursed_ = false;
    this->throwing = true;
    this->unwrappedException_ = v;
    
    
    MOZ_ASSERT_IF(v.isObject(), v.toObject().compartment() == compartment());
}

inline bool
JSContext::runningWithTrustedPrincipals() const
{
    return !compartment() || compartment()->principals() == runtime()->trustedPrincipals();
}

inline void
js::ExclusiveContext::enterCompartment(JSCompartment* c)
{
    enterCompartmentDepth_++;
    c->enter();
    setCompartment(c);
}

inline void
js::ExclusiveContext::enterNullCompartment()
{
    enterCompartmentDepth_++;
    setCompartment(nullptr);
}

inline void
js::ExclusiveContext::leaveCompartment(JSCompartment* oldCompartment)
{
    MOZ_ASSERT(hasEnteredCompartment());
    enterCompartmentDepth_--;

    
    
    JSCompartment* startingCompartment = compartment_;
    setCompartment(oldCompartment);
    if (startingCompartment)
        startingCompartment->leave();
}

inline void
js::ExclusiveContext::setCompartment(JSCompartment* comp)
{
    
    MOZ_ASSERT_IF(!isJSContext() && !runtime_->isAtomsCompartment(comp),
                  comp->zone()->usedByExclusiveThread);

    
    MOZ_ASSERT_IF(isJSContext() && comp,
                  !comp->zone()->usedByExclusiveThread);

    
    MOZ_ASSERT_IF(runtime_->isAtomsCompartment(comp),
                  runtime_->currentThreadHasExclusiveAccess());

    
    MOZ_ASSERT_IF(comp && !runtime_->isAtomsCompartment(comp),
                  !runtime_->isAtomsZone(comp->zone()));

    
    
    MOZ_ASSERT_IF(compartment_, compartment_->hasBeenEntered());
    MOZ_ASSERT_IF(comp, comp->hasBeenEntered());

    compartment_ = comp;
    zone_ = comp ? comp->zone() : nullptr;
    arenas_ = zone_ ? &zone_->arenas : nullptr;
}

inline JSScript*
JSContext::currentScript(jsbytecode** ppc,
                         MaybeAllowCrossCompartment allowCrossCompartment) const
{
    if (ppc)
        *ppc = nullptr;

    js::Activation* act = runtime()->activation();
    while (act && (act->cx() != this || (act->isJit() && !act->asJit()->isActive())))
        act = act->prev();

    if (!act)
        return nullptr;

    MOZ_ASSERT(act->cx() == this);

    if (act->isJit()) {
        JSScript* script = nullptr;
        js::jit::GetPcScript(const_cast<JSContext*>(this), &script, ppc);
        if (!allowCrossCompartment && script->compartment() != compartment()) {
            if (ppc)
                *ppc = nullptr;
            return nullptr;
        }
        return script;
    }

    if (act->isAsmJS())
        return nullptr;

    MOZ_ASSERT(act->isInterpreter());

    js::InterpreterFrame* fp = act->asInterpreter()->current();
    MOZ_ASSERT(!fp->runningInJit());

    JSScript* script = fp->script();
    if (!allowCrossCompartment && script->compartment() != compartment())
        return nullptr;

    if (ppc) {
        *ppc = act->asInterpreter()->regs().pc;
        MOZ_ASSERT(script->containsPC(*ppc));
    }
    return script;
}

#endif 
