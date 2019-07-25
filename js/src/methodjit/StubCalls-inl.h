







































#ifndef jslogic_h_inl__
#define jslogic_h_inl__

namespace js {
namespace mjit {

static inline void
ThrowException(VMFrame &f)
{
    void *ptr = JS_FUNC_TO_DATA_PTR(void *, JaegerThrowpoline);
    *f.returnAddressLocation() = ptr;
}

#define THROW()   do { ThrowException(f); return; } while (0)
#define THROWV(v) do { ThrowException(f); return v; } while (0)

static inline JSObject *
ValueToObject(JSContext *cx, Value *vp)
{
    if (vp->isObject())
        return &vp->toObject();
    return js_ValueToNonNullObject(cx, *vp);
}

static inline void
ReportAtomNotDefined(JSContext *cx, JSAtom *atom)
{
    JSAutoByteString printable;
    if (js_AtomToPrintableString(cx, atom, &printable))
        js_ReportIsNotDefined(cx, printable.ptr());
}

#define NATIVE_SET(cx,obj,shape,entry,strict,vp)                              \
    JS_BEGIN_MACRO                                                            \
        if (shape->hasDefaultSetter() &&                                      \
            (shape)->slot != SHAPE_INVALID_SLOT &&                            \
            !(obj)->brandedOrHasMethodBarrier()) {                            \
            /* Fast path for, e.g., plain Object instance properties. */      \
            (obj)->nativeSetSlot((shape)->slot, *vp);                         \
        } else {                                                              \
            if (!js_NativeSet(cx, obj, shape, false, strict, vp))             \
                THROW();                                                      \
        }                                                                     \
    JS_END_MACRO

#define NATIVE_GET(cx,obj,pobj,shape,getHow,vp,onerr)                         \
    JS_BEGIN_MACRO                                                            \
        if (shape->isDataDescriptor() && shape->hasDefaultGetter()) {         \
            /* Fast path for Object instance properties. */                   \
            JS_ASSERT((shape)->slot != SHAPE_INVALID_SLOT ||                  \
                      !shape->hasDefaultSetter());                            \
            if (((shape)->slot != SHAPE_INVALID_SLOT))                        \
                *(vp) = (pobj)->nativeGetSlot((shape)->slot);                 \
            else                                                              \
                (vp)->setUndefined();                                         \
        } else {                                                              \
            if (!js_NativeGet(cx, obj, pobj, shape, getHow, vp))              \
                onerr;                                                        \
        }                                                                     \
    JS_END_MACRO

}}

#endif 

