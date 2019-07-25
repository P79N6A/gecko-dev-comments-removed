







































#ifndef String_inl_h__
#define String_inl_h__

#include "String.h"

#include "jsgcinlines.h"

JS_ALWAYS_INLINE void
JSRope::init(JSString *left, JSString *right, size_t length)
{
    d.lengthAndFlags = buildLengthAndFlags(length, ROPE_BIT);
    d.u1.left = left;
    d.s.u2.right = right;
}

JS_ALWAYS_INLINE JSRope *
JSRope::new_(JSContext *cx, JSString *left, JSString *right, size_t length)
{
    JSRope *str = (JSRope *)js_NewGCString(cx);
    if (!str)
        return NULL;
    str->init(left, right, length);
    return str;
}

JS_ALWAYS_INLINE void
JSDependentString::init(JSLinearString *base, const jschar *chars, size_t length)
{
    d.lengthAndFlags = buildLengthAndFlags(length, DEPENDENT_BIT);
    d.u1.chars = chars;
    d.s.u2.base = base;
}

JS_ALWAYS_INLINE JSDependentString *
JSDependentString::new_(JSContext *cx, JSLinearString *base, const jschar *chars, size_t length)
{
    
    while (base->isDependent())
        base = base->asDependent().base();

    JS_ASSERT(base->isFlat());
    JS_ASSERT(chars >= base->chars() && chars < base->chars() + base->length());
    JS_ASSERT(length <= base->length() - (chars - base->chars()));

    JSDependentString *str = (JSDependentString *)js_NewGCString(cx);
    if (!str)
        return NULL;
    str->init(base, chars, length);
#ifdef DEBUG
    JSRuntime *rt = cx->runtime;
    JS_RUNTIME_METER(rt, liveDependentStrings);
    JS_RUNTIME_METER(rt, totalDependentStrings);
    JS_RUNTIME_METER(rt, liveStrings);
    JS_RUNTIME_METER(rt, totalStrings);
    JS_LOCK_RUNTIME_VOID(rt,
        (rt->strdepLengthSum += (double)length,
         rt->strdepLengthSquaredSum += (double)length * (double)length));
    JS_LOCK_RUNTIME_VOID(rt,
        (rt->lengthSum += (double)length,
         rt->lengthSquaredSum += (double)length * (double)length));
#endif
    return str;
}

JS_ALWAYS_INLINE void
JSFixedString::init(const jschar *chars, size_t length)
{
    d.lengthAndFlags = buildLengthAndFlags(length, FIXED_FLAGS);
    d.u1.chars = chars;
}

JS_ALWAYS_INLINE JSFixedString *
JSFixedString::new_(JSContext *cx, const jschar *chars, size_t length)
{
    JS_ASSERT(length <= MAX_LENGTH);
    JS_ASSERT(chars[length] == jschar(0));

    JSFixedString *str = (JSFixedString *)js_NewGCString(cx);
    if (!str)
        return NULL;
    str->init(chars, length);

#ifdef DEBUG
    JSRuntime *rt = cx->runtime;
    JS_RUNTIME_METER(rt, liveStrings);
    JS_RUNTIME_METER(rt, totalStrings);
    JS_LOCK_RUNTIME_VOID(rt,
        (rt->lengthSum += (double)length,
         rt->lengthSquaredSum += (double)length * (double)length));
#endif
    return str;
}

JS_ALWAYS_INLINE JSAtom *
JSFixedString::morphAtomizedStringIntoAtom()
{
    JS_ASSERT((d.lengthAndFlags & FLAGS_MASK) == JS_BIT(2));
    JS_STATIC_ASSERT(NON_STATIC_ATOM == JS_BIT(3));
    d.lengthAndFlags ^= (JS_BIT(2) | JS_BIT(3));
    return &asAtom();
}

JS_ALWAYS_INLINE JSInlineString *
JSInlineString::new_(JSContext *cx)
{
    return (JSInlineString *)js_NewGCString(cx);
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

JS_ALWAYS_INLINE JSShortString *
JSShortString::new_(JSContext *cx)
{
    return js_NewGCShortString(cx);
}

JS_ALWAYS_INLINE void
JSShortString::initAtOffsetInBuffer(const jschar *chars, size_t length)
{
    JS_ASSERT(lengthFits(length + (chars - d.inlineStorage)));
    JS_ASSERT(chars >= d.inlineStorage && chars < d.inlineStorage + MAX_SHORT_LENGTH);
    d.lengthAndFlags = buildLengthAndFlags(length, FIXED_FLAGS);
    d.u1.chars = chars;
}

JS_ALWAYS_INLINE void
JSExternalString::init(const jschar *chars, size_t length, intN type, void *closure)
{
    d.lengthAndFlags = buildLengthAndFlags(length, FIXED_FLAGS);
    d.u1.chars = chars;
    d.s.u2.externalType = type;
    d.s.u3.externalClosure = closure;
}

JS_ALWAYS_INLINE JSExternalString *
JSExternalString::new_(JSContext *cx, const jschar *chars, size_t length, intN type, void *closure)
{
    JS_ASSERT(uintN(type) < JSExternalString::TYPE_LIMIT);
    JS_ASSERT(chars[length] == 0);

    JSExternalString *str = js_NewGCExternalString(cx);
    if (!str)
        return NULL;
    str->init(chars, length, type, closure);
    cx->runtime->updateMallocCounter((length + 1) * sizeof(jschar));
    return str;
}

inline bool
JSAtom::fitsInSmallChar(jschar c)
{
    return c < SMALL_CHAR_LIMIT && toSmallChar[c] != INVALID_SMALL_CHAR;
}

inline bool
JSAtom::hasUnitStatic(jschar c)
{
    return c < UNIT_STATIC_LIMIT;
}

inline JSStaticAtom &
JSAtom::unitStatic(jschar c)
{
    JS_ASSERT(hasUnitStatic(c));
    return (JSStaticAtom &)unitStaticTable[c];
}

inline bool
JSAtom::hasIntStatic(int32 i)
{
    return uint32(i) < INT_STATIC_LIMIT;
}

inline JSStaticAtom &
JSAtom::intStatic(jsint i)
{
    JS_ASSERT(hasIntStatic(i));
    return (JSStaticAtom &)*intStaticTable[i];
}

inline JSLinearString *
JSAtom::getUnitStringForElement(JSContext *cx, JSString *str, size_t index)
{
    JS_ASSERT(index < str->length());
    const jschar *chars = str->getChars(cx);
    if (!chars)
        return NULL;
    jschar c = chars[index];
    if (c < UNIT_STATIC_LIMIT)
        return &unitStatic(c);
    return js_NewDependentString(cx, str, index, 1);
}

inline JSStaticAtom &
JSAtom::length2Static(jschar c1, jschar c2)
{
    JS_ASSERT(fitsInSmallChar(c1));
    JS_ASSERT(fitsInSmallChar(c2));
    size_t index = (((size_t)toSmallChar[c1]) << 6) + toSmallChar[c2];
    return (JSStaticAtom &)length2StaticTable[index];
}

inline JSStaticAtom &
JSAtom::length2Static(uint32 i)
{
    JS_ASSERT(i < 100);
    return length2Static('0' + i / 10, '0' + i % 10);
}


inline JSStaticAtom *
JSAtom::lookupStatic(const jschar *chars, size_t length)
{
    switch (length) {
      case 1:
        if (chars[0] < UNIT_STATIC_LIMIT)
            return &unitStatic(chars[0]);
        return NULL;
      case 2:
        if (fitsInSmallChar(chars[0]) && fitsInSmallChar(chars[1]))
            return &length2Static(chars[0], chars[1]);
        return NULL;
      case 3:
        





        JS_STATIC_ASSERT(INT_STATIC_LIMIT <= 999);
        if ('1' <= chars[0] && chars[0] <= '9' &&
            '0' <= chars[1] && chars[1] <= '9' &&
            '0' <= chars[2] && chars[2] <= '9') {
            jsint i = (chars[0] - '0') * 100 +
                      (chars[1] - '0') * 10 +
                      (chars[2] - '0');

            if (jsuint(i) < INT_STATIC_LIMIT)
                return &intStatic(i);
        }
        return NULL;
    }

    return NULL;
}

JS_ALWAYS_INLINE void
JSString::finalize(JSContext *cx)
{
    JS_ASSERT(!isStaticAtom() && !isShort());

    JS_RUNTIME_UNMETER(cx->runtime, liveStrings);

    if (isDependent())
        JS_RUNTIME_UNMETER(cx->runtime, liveDependentStrings);
    else if (isFlat())
        asFlat().finalize(cx->runtime);
    else
        JS_ASSERT(isRope());
}

inline void
JSFlatString::finalize(JSRuntime *rt)
{
    JS_ASSERT(!isShort());

    



    if (chars() != d.inlineStorage)
        rt->free_(const_cast<jschar *>(chars()));
}

inline void
JSShortString::finalize(JSContext *cx)
{
    JS_ASSERT(isShort());
    JS_RUNTIME_UNMETER(cx->runtime, liveStrings);
}

inline void
JSAtom::finalize(JSRuntime *rt)
{
    JS_ASSERT(isAtom());
    if (arenaHeader()->getThingKind() == js::gc::FINALIZE_STRING)
        asFlat().finalize(rt);
    else
        JS_ASSERT(arenaHeader()->getThingKind() == js::gc::FINALIZE_SHORT_STRING);
}

inline void
JSExternalString::finalize(JSContext *cx)
{
    JS_RUNTIME_UNMETER(cx->runtime, liveStrings);
    if (JSStringFinalizeOp finalizer = str_finalizers[externalType()])
        finalizer(cx, this);
}

inline void
JSExternalString::finalize()
{
    JSStringFinalizeOp finalizer = str_finalizers[externalType()];
    if (finalizer) {
        



        finalizer(NULL, this);
    }
}

#endif
