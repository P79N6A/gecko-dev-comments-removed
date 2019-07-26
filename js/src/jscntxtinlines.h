





#ifndef jscntxtinlines_h
#define jscntxtinlines_h

#include "jscntxt.h"

#include "jscompartment.h"
#include "jsfriendapi.h"
#include "jsgc.h"
#include "jsiter.h"

#include "builtin/Object.h" 
#include "frontend/ParseMaps.h"
#include "vm/Interpreter.h"
#include "vm/Probes.h"
#include "vm/RegExpObject.h"

#include "jsgcinlines.h"

#include "vm/ObjectImpl-inl.h"

namespace js {

inline void
NewObjectCache::staticAsserts()
{
    JS_STATIC_ASSERT(NewObjectCache::MAX_OBJ_SIZE == sizeof(JSObject_Slots16));
    JS_STATIC_ASSERT(gc::FINALIZE_OBJECT_LAST == gc::FINALIZE_OBJECT16_BACKGROUND);
}

inline bool
NewObjectCache::lookup(Class *clasp, gc::Cell *key, gc::AllocKind kind, EntryIndex *pentry)
{
    uintptr_t hash = (uintptr_t(clasp) ^ uintptr_t(key)) + kind;
    *pentry = hash % mozilla::ArrayLength(entries);

    Entry *entry = &entries[*pentry];

    
    return (entry->clasp == clasp && entry->key == key);
}

inline bool
NewObjectCache::lookupProto(Class *clasp, JSObject *proto, gc::AllocKind kind, EntryIndex *pentry)
{
    JS_ASSERT(!proto->isGlobal());
    return lookup(clasp, proto, kind, pentry);
}

inline bool
NewObjectCache::lookupGlobal(Class *clasp, js::GlobalObject *global, gc::AllocKind kind, EntryIndex *pentry)
{
    return lookup(clasp, global, kind, pentry);
}

inline bool
NewObjectCache::lookupType(Class *clasp, js::types::TypeObject *type, gc::AllocKind kind, EntryIndex *pentry)
{
    return lookup(clasp, type, kind, pentry);
}

inline void
NewObjectCache::fill(EntryIndex entry_, Class *clasp, gc::Cell *key, gc::AllocKind kind, JSObject *obj)
{
    JS_ASSERT(unsigned(entry_) < mozilla::ArrayLength(entries));
    Entry *entry = &entries[entry_];

    JS_ASSERT(!obj->hasDynamicSlots() && !obj->hasDynamicElements());

    entry->clasp = clasp;
    entry->key = key;
    entry->kind = kind;

    entry->nbytes = gc::Arena::thingSize(kind);
    js_memcpy(&entry->templateObject, obj, entry->nbytes);
}

inline void
NewObjectCache::fillGlobal(EntryIndex entry, Class *clasp, js::GlobalObject *global, gc::AllocKind kind, JSObject *obj)
{
    
    return fill(entry, clasp, global, kind, obj);
}

inline void
NewObjectCache::fillType(EntryIndex entry, Class *clasp, js::types::TypeObject *type, gc::AllocKind kind, JSObject *obj)
{
    JS_ASSERT(obj->type() == type);
    return fill(entry, clasp, type, kind, obj);
}

inline void
NewObjectCache::copyCachedToObject(JSObject *dst, JSObject *src, gc::AllocKind kind)
{
    js_memcpy(dst, src, gc::Arena::thingSize(kind));
#ifdef JSGC_GENERATIONAL
    Shape::writeBarrierPost(dst->shape_, &dst->shape_);
    types::TypeObject::writeBarrierPost(dst->type_, &dst->type_);
#endif
}

inline JSObject *
NewObjectCache::newObjectFromHit(JSContext *cx, EntryIndex entry_, js::gc::InitialHeap heap)
{
    
    JS_ASSERT(!cx->compartment()->objectMetadataCallback);

    JS_ASSERT(unsigned(entry_) < mozilla::ArrayLength(entries));
    Entry *entry = &entries[entry_];

    JSObject *obj = js_NewGCObject<NoGC>(cx, entry->kind, heap);
    if (obj) {
        copyCachedToObject(obj, reinterpret_cast<JSObject *>(&entry->templateObject), entry->kind);
        Probes::createObject(cx, obj);
        return obj;
    }

    return NULL;
}

struct PreserveRegsGuard
{
    PreserveRegsGuard(JSContext *cx, FrameRegs &regs)
      : prevContextRegs(cx->stack.maybeRegs()), cx(cx), regs_(regs) {
        cx->stack.repointRegs(&regs_);
    }
    ~PreserveRegsGuard() {
        JS_ASSERT(cx->stack.maybeRegs() == &regs_);
        *prevContextRegs = regs_;
        cx->stack.repointRegs(prevContextRegs);
    }

    FrameRegs *prevContextRegs;

  private:
    JSContext *cx;
    FrameRegs &regs_;
};

#ifdef JS_CRASH_DIAGNOSTICS
class CompartmentChecker
{
    JSContext *context;
    JSCompartment *compartment;

  public:
    explicit CompartmentChecker(JSContext *cx)
      : context(cx), compartment(cx->compartment())
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
        JS_ASSERT(c1 != c1->rt->atomsCompartment);
        JS_ASSERT(c2 != c2->rt->atomsCompartment);
        if (c1 != c2)
            fail(c1, c2);
    }

    void check(JSCompartment *c) {
        if (c && c != context->runtime()->atomsCompartment) {
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
    JS_ASSERT(cx->compartment()->zone() == cx->zone());                       \
    if (cx->runtime()->isHeapBusy())                                          \
        return;                                                               \
    CompartmentChecker c(cx)

template <class T1> inline void
assertSameCompartment(JSContext *cx, const T1 &t1)
{
#ifdef JS_CRASH_DIAGNOSTICS
    START_ASSERT_SAME_COMPARTMENT();
    c.check(t1);
#endif
}

template <class T1> inline void
assertSameCompartmentDebugOnly(JSContext *cx, const T1 &t1)
{
#ifdef DEBUG
    START_ASSERT_SAME_COMPARTMENT();
    c.check(t1);
#endif
}

template <class T1, class T2> inline void
assertSameCompartment(JSContext *cx, const T1 &t1, const T2 &t2)
{
#ifdef JS_CRASH_DIAGNOSTICS
    START_ASSERT_SAME_COMPARTMENT();
    c.check(t1);
    c.check(t2);
#endif
}

template <class T1, class T2, class T3> inline void
assertSameCompartment(JSContext *cx, const T1 &t1, const T2 &t2, const T3 &t3)
{
#ifdef JS_CRASH_DIAGNOSTICS
    START_ASSERT_SAME_COMPARTMENT();
    c.check(t1);
    c.check(t2);
    c.check(t3);
#endif
}

template <class T1, class T2, class T3, class T4> inline void
assertSameCompartment(JSContext *cx, const T1 &t1, const T2 &t2, const T3 &t3, const T4 &t4)
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
assertSameCompartment(JSContext *cx, const T1 &t1, const T2 &t2, const T3 &t3, const T4 &t4, const T5 &t5)
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

    



















    JS_ASSERT_IF(native != FunctionProxyClass.construct &&
                 native != js::CallOrConstructBoundFunction &&
                 native != js::IteratorConstructor &&
                 (!callee->isFunction() || callee->toFunction()->native() != obj_construct),
                 !args.rval().isPrimitive() && callee != &args.rval().toObject());

    return true;
}

JS_ALWAYS_INLINE bool
CallJSPropertyOp(JSContext *cx, PropertyOp op, HandleObject receiver, HandleId id, MutableHandleValue vp)
{
    JS_CHECK_RECURSION(cx, return false);

    assertSameCompartment(cx, receiver, id, vp);
    JSBool ok = op(cx, receiver, id, vp);
    if (ok)
        assertSameCompartment(cx, vp);
    return ok;
}

JS_ALWAYS_INLINE bool
CallJSPropertyOpSetter(JSContext *cx, StrictPropertyOp op, HandleObject obj, HandleId id,
                       JSBool strict, MutableHandleValue vp)
{
    JS_CHECK_RECURSION(cx, return false);

    assertSameCompartment(cx, obj, id, vp);
    return op(cx, obj, id, strict, vp);
}

static inline bool
CallJSDeletePropertyOp(JSContext *cx, JSDeletePropertyOp op, HandleObject receiver, HandleId id,
                       JSBool *succeeded)
{
    JS_CHECK_RECURSION(cx, return false);

    assertSameCompartment(cx, receiver, id);
    return op(cx, receiver, id, succeeded);
}

inline bool
CallSetter(JSContext *cx, HandleObject obj, HandleId id, StrictPropertyOp op, unsigned attrs,
           unsigned shortid, JSBool strict, MutableHandleValue vp)
{
    if (attrs & JSPROP_SETTER) {
        RootedValue opv(cx, CastAsObjectJsval(op));
        return InvokeGetterOrSetter(cx, obj, opv, 1, vp.address(), vp.address());
    }

    if (attrs & JSPROP_GETTER)
        return js_ReportGetterOnlyAssignment(cx);

    if (!(attrs & JSPROP_SHORTID))
        return CallJSPropertyOpSetter(cx, op, obj, id, strict, vp);

    RootedId nid(cx, INT_TO_JSID(shortid));

    return CallJSPropertyOpSetter(cx, op, obj, nid, strict, vp);
}

}  

inline bool
JSContext::canSetDefaultVersion() const
{
    return !currentlyRunning() && !hasVersionOverride;
}

inline void
JSContext::overrideVersion(JSVersion newVersion)
{
    JS_ASSERT(!canSetDefaultVersion());
    versionOverride = newVersion;
    hasVersionOverride = true;
}

inline bool
JSContext::maybeOverrideVersion(JSVersion newVersion)
{
    if (canSetDefaultVersion()) {
        setDefaultVersion(newVersion);
        return false;
    }
    overrideVersion(newVersion);
    return true;
}

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

inline js::PropertyTree&
JSContext::propertyTree()
{
    return compartment()->propertyTree;
}

inline void
JSContext::setDefaultCompartmentObject(JSObject *obj)
{
    defaultCompartmentObject_ = obj;

    if (!hasEnteredCompartment()) {
        





        JS_ASSERT(!currentlyRunning());
        setCompartment(obj ? obj->compartment() : NULL);
        if (throwing)
            wrapPendingException();
    }
}

inline void
JSContext::setDefaultCompartmentObjectIfUnset(JSObject *obj)
{
    if (!defaultCompartmentObject_)
        setDefaultCompartmentObject(obj);
}

inline void
JSContext::enterCompartment(JSCompartment *c)
{
    enterCompartmentDepth_++;
    setCompartment(c);
    c->enter();
    if (throwing)
        wrapPendingException();
}

inline void
JSContext::leaveCompartment(JSCompartment *oldCompartment)
{
    JS_ASSERT(hasEnteredCompartment());
    enterCompartmentDepth_--;

    compartment()->leave();

    







    if (hasEnteredCompartment() || !defaultCompartmentObject_)
        setCompartment(oldCompartment);
    else
        setCompartment(defaultCompartmentObject_->compartment());

    if (throwing)
        wrapPendingException();
}

inline void
JSContext::setCompartment(JSCompartment *comp)
{
    compartment_ = comp;
    zone_ = comp ? comp->zone() : NULL;
    allocator_ = zone_ ? &zone_->allocator : NULL;
}

template <typename T>
inline bool
js::ThreadSafeContext::isInsideCurrentZone(T thing) const
{
    return thing->isInsideZone(zone_);
}

inline js::AllowGC
js::ThreadSafeContext::allowGC() const
{
    switch (contextKind_) {
      case Context_JS:
        return CanGC;
      case Context_ForkJoin:
        return NoGC;
      default:
        
        JS_NOT_REACHED("Bad context kind");
        return NoGC;
    }
}

#endif 
