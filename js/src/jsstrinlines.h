






































#ifndef jsstrinlines_h___
#define jsstrinlines_h___

#include "jsstr.h"

inline JSFlatString *
JSString::unitString(jschar c)
{
    JS_ASSERT(c < UNIT_STRING_LIMIT);
    return const_cast<JSString *>(&unitStringTable[c])->assertIsFlat();
}

inline JSLinearString *
JSString::getUnitString(JSContext *cx, JSString *str, size_t index)
{
    JS_ASSERT(index < str->length());
    const jschar *chars = str->getChars(cx);
    if (!chars)
        return NULL;
    jschar c = chars[index];
    if (c < UNIT_STRING_LIMIT)
        return unitString(c);
    return js_NewDependentString(cx, str, index, 1);
}

inline JSFlatString *
JSString::length2String(jschar c1, jschar c2)
{
    JS_ASSERT(fitsInSmallChar(c1));
    JS_ASSERT(fitsInSmallChar(c2));
    return const_cast<JSString *> (
             &length2StringTable[(((size_t)toSmallChar[c1]) << 6) + toSmallChar[c2]]
           )->assertIsFlat();
}

inline JSFlatString *
JSString::length2String(uint32 i)
{
    JS_ASSERT(i < 100);
    return length2String('0' + i / 10, '0' + i % 10);
}

inline JSFlatString *
JSString::intString(jsint i)
{
    jsuint u = jsuint(i);
    JS_ASSERT(u < INT_STRING_LIMIT);
    return const_cast<JSString *>(JSString::intStringTable[u])->assertIsFlat();
}


inline JSFlatString *
JSString::lookupStaticString(const jschar *chars, size_t length)
{
    if (length == 1) {
        if (chars[0] < UNIT_STRING_LIMIT)
            return unitString(chars[0]);
    }

    if (length == 2) {
        if (fitsInSmallChar(chars[0]) && fitsInSmallChar(chars[1]))
            return length2String(chars[0], chars[1]);
    }

    





    JS_STATIC_ASSERT(INT_STRING_LIMIT <= 999);
    if (length == 3) {
        if ('1' <= chars[0] && chars[0] <= '9' &&
            '0' <= chars[1] && chars[1] <= '9' &&
            '0' <= chars[2] && chars[2] <= '9') {
            jsint i = (chars[0] - '0') * 100 +
                      (chars[1] - '0') * 10 +
                      (chars[2] - '0');

            if (jsuint(i) < INT_STRING_LIMIT)
                return intString(i);
        }
    }

    return NULL;
}

inline void
JSString::finalize(JSContext *cx) {
    JS_ASSERT(!JSString::isStatic(this));
    JS_RUNTIME_UNMETER(cx->runtime, liveStrings);
    if (isDependent()) {
        JS_RUNTIME_UNMETER(cx->runtime, liveDependentStrings);
    } else if (isFlat()) {
        



        cx->free(const_cast<jschar *>(flatChars()));
    }
}

inline void
JSShortString::finalize(JSContext *cx)
{
    JS_ASSERT(!JSString::isStatic(&mHeader));
    JS_ASSERT(mHeader.isFlat());
    JS_RUNTIME_UNMETER(cx->runtime, liveStrings);
}

inline void
JSExternalString::finalize(JSContext *cx)
{
    JS_ASSERT(unsigned(externalStringType) < JS_ARRAY_LENGTH(str_finalizers));
    JS_ASSERT(!isStatic(this));
    JS_ASSERT(isFlat());
    JS_RUNTIME_UNMETER(cx->runtime, liveStrings);

    
    jschar *chars = const_cast<jschar *>(flatChars());
    if (!chars)
        return;
    JSStringFinalizeOp finalizer = str_finalizers[externalStringType];
    if (finalizer)
        finalizer(cx, this);
}

inline void
JSExternalString::finalize()
{
    JS_ASSERT(unsigned(externalStringType) < JS_ARRAY_LENGTH(str_finalizers));
    JSStringFinalizeOp finalizer = str_finalizers[externalStringType];
    if (finalizer) {
        



        finalizer(NULL, this);
    }
}

namespace js {

class RopeBuilder {
    JSContext *cx;
    JSString *res;

  public:
    RopeBuilder(JSContext *cx)
      : cx(cx), res(cx->runtime->emptyString)
    {}

    inline bool append(JSString *str) {
        res = js_ConcatStrings(cx, res, str);
        return !!res;
    }

    inline JSString *result() {
        return res;
    }
};

class StringSegmentRange
{
    



    Vector<JSString *, 32> stack;
    JSString *cur;

    bool settle(JSString *str) {
        while (str->isRope()) {
            if (!stack.append(str->ropeRight()))
                return false;
            str = str->ropeLeft();
        }
        cur = str;
        return true;
    }

  public:
    StringSegmentRange(JSContext *cx)
      : stack(cx), cur(NULL)
    {}

    JS_WARN_UNUSED_RESULT bool init(JSString *str) {
        JS_ASSERT(stack.empty());
        return settle(str);
    }

    bool empty() const {
        return cur == NULL;
    }

    JSString *front() const {
        JS_ASSERT(!cur->isRope());
        return cur;
    }

    JS_WARN_UNUSED_RESULT bool popFront() {
        JS_ASSERT(!empty());
        if (stack.empty()) {
            cur = NULL;
            return true;
        }
        return settle(stack.popCopy());
    }
};

}  

#endif 
