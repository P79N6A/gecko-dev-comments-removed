






































#ifndef jsstrinlines_h___
#define jsstrinlines_h___

#include "jsstr.h"
#include "jscntxtinlines.h"

namespace js {

static inline bool
CheckStringLength(JSContext *cx, size_t length)
{
    if (JS_UNLIKELY(length > JSString::MAX_LENGTH)) {
        js_ReportAllocationOverflow(cx);
        return false;
    }

    return true;
}









class StringBuffer
{
    typedef Vector<jschar, 32> CharBuffer;
    CharBuffer cb;

    static inline bool checkLength(JSContext *cx, size_t length);
    inline bool checkLength(size_t length);
    JSContext *context() const { return cb.allocPolicy().context(); }

  public:
    explicit inline StringBuffer(JSContext *cx);
    bool reserve(size_t len);
    bool resize(size_t len);
    bool append(const jschar c);
    bool append(const jschar *chars, size_t len);
    bool append(const jschar *begin, const jschar *end);
    bool append(JSString *str);
    bool append(JSAtom *atom);
    bool appendN(const jschar c, size_t n);
    bool appendInflated(const char *cstr, size_t len);
    JSAtom *atomize(uintN flags = 0);
    static JSAtom *atomize(JSContext *cx, const CharBuffer &cb, uintN flags = 0);
    static JSAtom *atomize(JSContext *cx, const jschar *begin, size_t length, uintN flags = 0);

    void replaceRawBuffer(jschar *chars, size_t len) { cb.replaceRawBuffer(chars, len); }
    jschar *begin() { return cb.begin(); }
    jschar *end() { return cb.end(); }
    const jschar *begin() const { return cb.begin(); }
    const jschar *end() const { return cb.end(); }
    bool empty() const { return cb.empty(); }
    inline jsint length() const;

    




    JSFlatString *finishString();

    template <size_t ArrayLength>
    bool append(const char (&array)[ArrayLength]) {
        return cb.append(array, array + ArrayLength - 1); 
    }
};

inline
StringBuffer::StringBuffer(JSContext *cx)
  : cb(cx)
{}

inline bool
StringBuffer::reserve(size_t len)
{
    if (!checkLength(len))
        return false;
    return cb.reserve(len);
}

inline bool
StringBuffer::resize(size_t len)
{
    if (!checkLength(len))
        return false;
    return cb.resize(len);
}

inline bool
StringBuffer::append(const jschar c)
{
    if (!checkLength(cb.length() + 1))
        return false;
    return cb.append(c);
}

inline bool
StringBuffer::append(const jschar *chars, size_t len)
{
    if (!checkLength(cb.length() + len))
        return false;
    return cb.append(chars, len);
}

inline bool
StringBuffer::append(const jschar *begin, const jschar *end)
{
    if (!checkLength(cb.length() + (end - begin)))
        return false;
    return cb.append(begin, end);
}

inline bool
StringBuffer::append(JSString *str)
{
    JSLinearString *linear = str->ensureLinear(context());
    size_t strLen = linear->length();
    if (!checkLength(cb.length() + strLen))
        return false;
    return cb.append(linear->chars(), strLen);
}

inline bool
StringBuffer::append(JSAtom *atom)
{
    size_t strLen = atom->length();
    if (!checkLength(cb.length() + strLen))
        return false;
    return cb.append(atom->chars(), strLen);
}

inline bool
StringBuffer::appendN(const jschar c, size_t n)
{
    if (!checkLength(cb.length() + n))
        return false;
    return cb.appendN(c, n);
}

inline bool
StringBuffer::appendInflated(const char *cstr, size_t cstrlen)
{
    size_t lengthBefore = length();
    if (!cb.growByUninitialized(cstrlen))
        return false;
#if DEBUG
    size_t oldcstrlen = cstrlen;
    bool ok = 
#endif
    js_InflateStringToBuffer(context(), cstr, cstrlen, begin() + lengthBefore, &cstrlen);
    JS_ASSERT(ok && oldcstrlen == cstrlen);
    return true;
}

inline jsint
StringBuffer::length() const
{
    JS_STATIC_ASSERT(jsint(JSString::MAX_LENGTH) == JSString::MAX_LENGTH);
    JS_ASSERT(cb.length() <= JSString::MAX_LENGTH);
    return jsint(cb.length());
}

inline bool
StringBuffer::checkLength(size_t length)
{
    return CheckStringLength(context(), length);
}

} 

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
        



        cx->runtime->stringMemoryUsed -= length() * 2;
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
