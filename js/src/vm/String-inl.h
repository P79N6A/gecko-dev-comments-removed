





#ifndef vm_String_inl_h
#define vm_String_inl_h

#include "vm/String.h"

#include "mozilla/PodOperations.h"

#include "jscntxt.h"

#include "gc/Marking.h"

#include "jsgcinlines.h"

namespace js {

template <AllowGC allowGC>
static JS_ALWAYS_INLINE JSInlineString *
NewShortString(ThreadSafeContext *cx, JS::Latin1Chars chars)
{
    size_t len = chars.length();
    JS_ASSERT(JSShortString::lengthFits(len));
    JSInlineString *str = JSInlineString::lengthFits(len)
                          ? JSInlineString::new_<allowGC>(cx)
                          : JSShortString::new_<allowGC>(cx);
    if (!str)
        return nullptr;

    jschar *p = str->init(len);
    for (size_t i = 0; i < len; ++i)
        p[i] = static_cast<jschar>(chars[i]);
    p[len] = '\0';
    return str;
}

template <AllowGC allowGC>
static JS_ALWAYS_INLINE JSInlineString *
NewShortString(ExclusiveContext *cx, JS::StableTwoByteChars chars)
{
    size_t len = chars.length();

    



    JS_ASSERT(JSShortString::lengthFits(len));
    JSInlineString *str = JSInlineString::lengthFits(len)
                          ? JSInlineString::new_<allowGC>(cx)
                          : JSShortString::new_<allowGC>(cx);
    if (!str)
        return nullptr;

    jschar *storage = str->init(len);
    mozilla::PodCopy(storage, chars.start().get(), len);
    storage[len] = 0;
    return str;
}

template <AllowGC allowGC>
static JS_ALWAYS_INLINE JSInlineString *
NewShortString(ExclusiveContext *cx, JS::TwoByteChars chars)
{
    size_t len = chars.length();

    



    JS_ASSERT(JSShortString::lengthFits(len));
    JSInlineString *str = JSInlineString::lengthFits(len)
                          ? JSInlineString::new_<NoGC>(cx)
                          : JSShortString::new_<NoGC>(cx);
    if (!str) {
        if (!allowGC)
            return nullptr;
        jschar tmp[JSShortString::MAX_SHORT_LENGTH];
        mozilla::PodCopy(tmp, chars.start().get(), len);
        return NewShortString<CanGC>(cx, JS::StableTwoByteChars(tmp, len));
    }

    jschar *storage = str->init(len);
    mozilla::PodCopy(storage, chars.start().get(), len);
    storage[len] = 0;
    return str;
}

static inline void
StringWriteBarrierPost(js::ThreadSafeContext *maybecx, JSString **strp)
{
}

static inline void
StringWriteBarrierPostRemove(js::ThreadSafeContext *maybecx, JSString **strp)
{
}

} 

JS_ALWAYS_INLINE bool
JSString::validateLength(js::ThreadSafeContext *maybecx, size_t length)
{
    if (JS_UNLIKELY(length > JSString::MAX_LENGTH)) {
        js_ReportAllocationOverflow(maybecx);
        return false;
    }

    return true;
}

JS_ALWAYS_INLINE void
JSRope::init(js::ThreadSafeContext *cx, JSString *left, JSString *right, size_t length)
{
    d.lengthAndFlags = buildLengthAndFlags(length, ROPE_FLAGS);
    d.u1.left = left;
    d.s.u2.right = right;
    js::StringWriteBarrierPost(cx, &d.u1.left);
    js::StringWriteBarrierPost(cx, &d.s.u2.right);
}

template <js::AllowGC allowGC>
JS_ALWAYS_INLINE JSRope *
JSRope::new_(js::ThreadSafeContext *cx,
             typename js::MaybeRooted<JSString*, allowGC>::HandleType left,
             typename js::MaybeRooted<JSString*, allowGC>::HandleType right,
             size_t length)
{
    if (!validateLength(cx, length))
        return nullptr;
    JSRope *str = (JSRope *) js_NewGCString<allowGC>(cx);
    if (!str)
        return nullptr;
    str->init(cx, left, right, length);
    return str;
}

inline void
JSRope::markChildren(JSTracer *trc)
{
    js::gc::MarkStringUnbarriered(trc, &d.u1.left, "left child");
    js::gc::MarkStringUnbarriered(trc, &d.s.u2.right, "right child");
}

JS_ALWAYS_INLINE void
JSDependentString::init(js::ThreadSafeContext *cx, JSLinearString *base, const jschar *chars,
                        size_t length)
{
    JS_ASSERT(!js::IsPoisonedPtr(base));
    d.lengthAndFlags = buildLengthAndFlags(length, DEPENDENT_FLAGS);
    d.u1.chars = chars;
    d.s.u2.base = base;
    js::StringWriteBarrierPost(cx, reinterpret_cast<JSString **>(&d.s.u2.base));
}

JS_ALWAYS_INLINE JSLinearString *
JSDependentString::new_(js::ExclusiveContext *cx,
                        JSLinearString *baseArg, const jschar *chars, size_t length)
{
    
    while (baseArg->isDependent())
        baseArg = baseArg->asDependent().base();

    JS_ASSERT(baseArg->isFlat());

    



#ifdef DEBUG
    for (JSLinearString *b = baseArg; ; b = b->base()) {
        if (chars >= b->chars() && chars < b->chars() + b->length() &&
            length <= b->length() - (chars - b->chars()))
        {
            break;
        }
    }
#endif

    




    if (JSShortString::lengthFits(length))
        return js::NewShortString<js::CanGC>(cx, JS::TwoByteChars(chars, length));

    JSDependentString *str = (JSDependentString *)js_NewGCString<js::NoGC>(cx);
    if (str) {
        str->init(cx, baseArg, chars, length);
        return str;
    }

    JS::Rooted<JSLinearString*> base(cx, baseArg);

    str = (JSDependentString *)js_NewGCString<js::CanGC>(cx);
    if (!str)
        return nullptr;
    str->init(cx, base, chars, length);
    return str;
}

inline void
JSString::markBase(JSTracer *trc)
{
    JS_ASSERT(hasBase());
    js::gc::MarkStringUnbarriered(trc, &d.s.u2.base, "base");
}

inline js::PropertyName *
JSFlatString::toPropertyName(JSContext *cx)
{
#ifdef DEBUG
    uint32_t dummy;
    JS_ASSERT(!isIndex(&dummy));
#endif
    if (isAtom())
        return asAtom().asPropertyName();
    JSAtom *atom = js::AtomizeString<js::CanGC>(cx, this);
    if (!atom)
        return nullptr;
    return atom->asPropertyName();
}

JS_ALWAYS_INLINE void
JSStableString::init(const jschar *chars, size_t length)
{
    d.lengthAndFlags = buildLengthAndFlags(length, FIXED_FLAGS);
    d.u1.chars = chars;
}

template <js::AllowGC allowGC>
JS_ALWAYS_INLINE JSStableString *
JSStableString::new_(js::ThreadSafeContext *cx, const jschar *chars, size_t length)
{
    JS_ASSERT(chars[length] == jschar(0));

    if (!validateLength(cx, length))
        return nullptr;
    JSStableString *str = (JSStableString *)js_NewGCString<allowGC>(cx);
    if (!str)
        return nullptr;
    str->init(chars, length);
    return str;
}

template <js::AllowGC allowGC>
JS_ALWAYS_INLINE JSInlineString *
JSInlineString::new_(js::ThreadSafeContext *cx)
{
    return (JSInlineString *)js_NewGCString<allowGC>(cx);
}

JS_ALWAYS_INLINE jschar *
JSInlineString::init(size_t length)
{
    d.lengthAndFlags = buildLengthAndFlags(length, FIXED_FLAGS);
    d.u1.chars = d.inlineStorage;
    JS_ASSERT(lengthFits(length) || (isShort() && JSShortString::lengthFits(length)));
    return d.inlineStorage;
}

JS_ALWAYS_INLINE void
JSInlineString::resetLength(size_t length)
{
    d.lengthAndFlags = buildLengthAndFlags(length, FIXED_FLAGS);
    JS_ASSERT(lengthFits(length) || (isShort() && JSShortString::lengthFits(length)));
}

template <js::AllowGC allowGC>
JS_ALWAYS_INLINE JSShortString *
JSShortString::new_(js::ThreadSafeContext *cx)
{
    return js_NewGCShortString<allowGC>(cx);
}

JS_ALWAYS_INLINE void
JSExternalString::init(const jschar *chars, size_t length, const JSStringFinalizer *fin)
{
    JS_ASSERT(fin);
    JS_ASSERT(fin->finalize);
    d.lengthAndFlags = buildLengthAndFlags(length, FIXED_FLAGS);
    d.u1.chars = chars;
    d.s.u2.externalFinalizer = fin;
}

JS_ALWAYS_INLINE JSExternalString *
JSExternalString::new_(JSContext *cx, const jschar *chars, size_t length,
                       const JSStringFinalizer *fin)
{
    JS_ASSERT(chars[length] == 0);

    if (!validateLength(cx, length))
        return nullptr;
    JSExternalString *str = js_NewGCExternalString(cx);
    if (!str)
        return nullptr;
    str->init(chars, length, fin);
    cx->runtime()->updateMallocCounter(cx->zone(), (length + 1) * sizeof(jschar));
    return str;
}

inline JSLinearString *
js::StaticStrings::getUnitStringForElement(JSContext *cx, JSString *str, size_t index)
{
    JS_ASSERT(index < str->length());

    jschar c;
    if (!str->getChar(cx, index, &c))
        return nullptr;
    if (c < UNIT_STATIC_LIMIT)
        return getUnit(c);
    return js_NewDependentString(cx, str, index, 1);
}

inline JSAtom *
js::StaticStrings::getLength2(jschar c1, jschar c2)
{
    JS_ASSERT(fitsInSmallChar(c1));
    JS_ASSERT(fitsInSmallChar(c2));
    size_t index = (((size_t)toSmallChar[c1]) << 6) + toSmallChar[c2];
    return length2StaticTable[index];
}

JS_ALWAYS_INLINE void
JSString::finalize(js::FreeOp *fop)
{
    
    JS_ASSERT(getAllocKind() != js::gc::FINALIZE_SHORT_STRING);

    if (isFlat())
        asFlat().finalize(fop);
    else
        JS_ASSERT(isDependent() || isRope());
}

inline void
JSFlatString::finalize(js::FreeOp *fop)
{
    JS_ASSERT(getAllocKind() != js::gc::FINALIZE_SHORT_STRING);

    if (chars() != d.inlineStorage)
        fop->free_(const_cast<jschar *>(chars()));
}

inline void
JSShortString::finalize(js::FreeOp *fop)
{
    JS_ASSERT(getAllocKind() == js::gc::FINALIZE_SHORT_STRING);

    if (chars() != d.inlineStorage)
        fop->free_(const_cast<jschar *>(chars()));
}

inline void
JSAtom::finalize(js::FreeOp *fop)
{
    JS_ASSERT(JSString::isAtom());
    JS_ASSERT(JSString::isFlat());

    if (chars() != d.inlineStorage)
        fop->free_(const_cast<jschar *>(chars()));
}

inline void
JSExternalString::finalize(js::FreeOp *fop)
{
    const JSStringFinalizer *fin = externalFinalizer();
    fin->finalize(fin, const_cast<jschar *>(chars()));
}

#endif 
