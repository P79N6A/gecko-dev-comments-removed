







































#ifndef jslogic_h_inl__
#define jslogic_h_inl__

namespace js {
namespace mjit {

#define THROW()  \
    do {         \
        void *ptr = JS_FUNC_TO_DATA_PTR(void *, JaegerThrowpoline); \
        *f.returnAddressLocation() = ptr; \
        return;  \
    } while (0)

#define THROWV(v)       \
    do {                \
        void *ptr = JS_FUNC_TO_DATA_PTR(void *, JaegerThrowpoline); \
        *f.returnAddressLocation() = ptr; \
        return v;       \
    } while (0)

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
    const char *printable = js_AtomToPrintableString(cx, atom);
    if (printable)
        js_ReportIsNotDefined(cx, printable);
}

#define NATIVE_SET(cx,obj,sprop,entry,vp)                                     \
    JS_BEGIN_MACRO                                                            \
        if (sprop->hasDefaultSetter() &&                                      \
            (sprop)->slot != SPROP_INVALID_SLOT &&                            \
            !obj->scope()->brandedOrHasMethodBarrier()) {                     \
            /* Fast path for, e.g., plain Object instance properties. */      \
            obj->setSlot(sprop->slot, *vp);                                   \
        } else {                                                              \
            if (!js_NativeSet(cx, obj, sprop, false, vp))                     \
                THROW();                                                      \
        }                                                                     \
    JS_END_MACRO

#define NATIVE_GET(cx,obj,pobj,sprop,getHow,vp,onerr)                         \
    JS_BEGIN_MACRO                                                            \
        if (sprop->hasDefaultGetter()) {                                      \
            /* Fast path for Object instance properties. */                   \
            JS_ASSERT((sprop)->slot != SPROP_INVALID_SLOT ||                  \
                      !sprop->hasDefaultSetter());                            \
            if (((sprop)->slot != SPROP_INVALID_SLOT))                        \
                *(vp) = (pobj)->lockedGetSlot((sprop)->slot);                 \
            else                                                              \
                (vp)->setUndefined();                                         \
        } else {                                                              \
            if (!js_NativeGet(cx, obj, pobj, sprop, getHow, vp))              \
                onerr;                                                        \
        }                                                                     \
    JS_END_MACRO


}}

#endif 

