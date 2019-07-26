





#ifndef vm_String_inl_h
#define vm_String_inl_h

#include "vm/String.h"

#include "mozilla/PodOperations.h"

#include "jscntxt.h"

#include "gc/Marking.h"

#include "jsgcinlines.h"

namespace js {

template <AllowGC allowGC>
static MOZ_ALWAYS_INLINE JSInlineString *
NewFatInlineString(ThreadSafeContext *cx, JS::Latin1Chars chars)
{
    size_t len = chars.length();
    JS_ASSERT(JSFatInlineString::twoByteLengthFits(len));

    JSInlineString *str;
    jschar *p;
    if (JSInlineString::twoByteLengthFits(len)) {
        str = JSInlineString::new_<allowGC>(cx);
        if (!str)
            return nullptr;
        p = str->initTwoByte(len);
    } else {
        JSFatInlineString *fatstr = JSFatInlineString::new_<allowGC>(cx);
        if (!fatstr)
            return nullptr;
        p = fatstr->initTwoByte(len);
        str = fatstr;
    }

    for (size_t i = 0; i < len; ++i)
        p[i] = static_cast<jschar>(chars[i]);
    p[len] = '\0';
    return str;
}

template <AllowGC allowGC>
static MOZ_ALWAYS_INLINE JSInlineString *
NewFatInlineString(ExclusiveContext *cx, JS::TwoByteChars chars)
{
    size_t len = chars.length();

    



    JS_ASSERT(JSFatInlineString::twoByteLengthFits(len));

    JSInlineString *str;
    jschar *storage;
    if (JSInlineString::twoByteLengthFits(len)) {
        str = JSInlineString::new_<allowGC>(cx);
        if (!str)
            return nullptr;
        storage = str->initTwoByte(len);
    } else {
        JSFatInlineString *fatstr = JSFatInlineString::new_<allowGC>(cx);
        if (!fatstr)
            return nullptr;
        storage = fatstr->initTwoByte(len);
        str = fatstr;
    }

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

MOZ_ALWAYS_INLINE bool
JSString::validateLength(js::ThreadSafeContext *maybecx, size_t length)
{
    if (MOZ_UNLIKELY(length > JSString::MAX_LENGTH)) {
        js_ReportAllocationOverflow(maybecx);
        return false;
    }

    return true;
}

MOZ_ALWAYS_INLINE void
JSRope::init(js::ThreadSafeContext *cx, JSString *left, JSString *right, size_t length)
{
    d.u1.length = length;
    d.u1.flags = ROPE_FLAGS;
    if (left->hasLatin1Chars() && right->hasLatin1Chars())
        d.u1.flags |= LATIN1_CHARS_BIT;
    d.s.u2.left = left;
    d.s.u3.right = right;
    js::StringWriteBarrierPost(cx, &d.s.u2.left);
    js::StringWriteBarrierPost(cx, &d.s.u3.right);
}

template <js::AllowGC allowGC>
MOZ_ALWAYS_INLINE JSRope *
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
    js::gc::MarkStringUnbarriered(trc, &d.s.u2.left, "left child");
    js::gc::MarkStringUnbarriered(trc, &d.s.u3.right, "right child");
}

MOZ_ALWAYS_INLINE void
JSDependentString::init(js::ThreadSafeContext *cx, JSLinearString *base, const jschar *chars,
                        size_t length)
{
    JS_ASSERT(!js::IsPoisonedPtr(base));
    d.u1.length = length;
    d.u1.flags = DEPENDENT_FLAGS;
    d.s.u2.nonInlineCharsTwoByte = chars;
    d.s.u3.base = base;
    js::StringWriteBarrierPost(cx, reinterpret_cast<JSString **>(&d.s.u3.base));
}

MOZ_ALWAYS_INLINE JSLinearString *
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

    




    if (JSFatInlineString::twoByteLengthFits(length))
        return js::NewFatInlineString<js::CanGC>(cx, JS::TwoByteChars(chars, length));

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
    js::gc::MarkStringUnbarriered(trc, &d.s.u3.base, "base");
}

MOZ_ALWAYS_INLINE void
JSFlatString::init(const jschar *chars, size_t length)
{
    d.u1.length = length;
    d.u1.flags = FLAT_BIT;
    d.s.u2.nonInlineCharsTwoByte = chars;
}

template <js::AllowGC allowGC>
MOZ_ALWAYS_INLINE JSFlatString *
JSFlatString::new_(js::ThreadSafeContext *cx, const jschar *chars, size_t length)
{
    JS_ASSERT(chars[length] == jschar(0));

    if (!validateLength(cx, length))
        return nullptr;
    JSFlatString *str = (JSFlatString *)js_NewGCString<allowGC>(cx);
    if (!str)
        return nullptr;
    str->init(chars, length);
    return str;
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
    JSAtom *atom = js::AtomizeString(cx, this);
    if (!atom)
        return nullptr;
    return atom->asPropertyName();
}

template <js::AllowGC allowGC>
MOZ_ALWAYS_INLINE JSInlineString *
JSInlineString::new_(js::ThreadSafeContext *cx)
{
    return (JSInlineString *)js_NewGCString<allowGC>(cx);
}

MOZ_ALWAYS_INLINE jschar *
JSInlineString::initTwoByte(size_t length)
{
    JS_ASSERT(twoByteLengthFits(length));
    d.u1.length = length;
    d.u1.flags = INIT_INLINE_FLAGS;
    return d.inlineStorageTwoByte;
}

MOZ_ALWAYS_INLINE JS::Latin1Char *
JSInlineString::initLatin1(size_t length)
{
    JS_ASSERT(latin1LengthFits(length));
    d.u1.length = length;
    d.u1.flags = INIT_INLINE_FLAGS | LATIN1_CHARS_BIT;
    return d.inlineStorageLatin1;
}

MOZ_ALWAYS_INLINE jschar *
JSFatInlineString::initTwoByte(size_t length)
{
    JS_ASSERT(twoByteLengthFits(length));
    d.u1.length = length;
    d.u1.flags = INIT_FAT_INLINE_FLAGS;
    return d.inlineStorageTwoByte;
}

MOZ_ALWAYS_INLINE JS::Latin1Char *
JSFatInlineString::initLatin1(size_t length)
{
    JS_ASSERT(latin1LengthFits(length));
    d.u1.length = length;
    d.u1.flags = INIT_FAT_INLINE_FLAGS | LATIN1_CHARS_BIT;
    return d.inlineStorageLatin1;
}

template <js::AllowGC allowGC>
MOZ_ALWAYS_INLINE JSFatInlineString *
JSFatInlineString::new_(js::ThreadSafeContext *cx)
{
    return js_NewGCFatInlineString<allowGC>(cx);
}

MOZ_ALWAYS_INLINE void
JSExternalString::init(const jschar *chars, size_t length, const JSStringFinalizer *fin)
{
    JS_ASSERT(fin);
    JS_ASSERT(fin->finalize);
    d.u1.length = length;
    d.u1.flags = EXTERNAL_FLAGS;
    d.s.u2.nonInlineCharsTwoByte = chars;
    d.s.u3.externalFinalizer = fin;
}

MOZ_ALWAYS_INLINE JSExternalString *
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

MOZ_ALWAYS_INLINE void
JSString::finalize(js::FreeOp *fop)
{
    
    JS_ASSERT(getAllocKind() != js::gc::FINALIZE_FAT_INLINE_STRING);

    if (isFlat())
        asFlat().finalize(fop);
    else
        JS_ASSERT(isDependent() || isRope());
}

inline void
JSFlatString::finalize(js::FreeOp *fop)
{
    JS_ASSERT(getAllocKind() != js::gc::FINALIZE_FAT_INLINE_STRING);

    if (!isInline())
        fop->free_(nonInlineCharsRaw());
}

inline void
JSFatInlineString::finalize(js::FreeOp *fop)
{
    JS_ASSERT(getAllocKind() == js::gc::FINALIZE_FAT_INLINE_STRING);

    if (!isInline())
        fop->free_(nonInlineCharsRaw());
}

inline void
JSAtom::finalize(js::FreeOp *fop)
{
    JS_ASSERT(JSString::isAtom());
    JS_ASSERT(JSString::isFlat());

    if (!isInline())
        fop->free_(nonInlineCharsRaw());
}

inline void
JSExternalString::finalize(js::FreeOp *fop)
{
    const JSStringFinalizer *fin = externalFinalizer();
    fin->finalize(fin, const_cast<jschar *>(nonInlineChars()));
}

#endif 
