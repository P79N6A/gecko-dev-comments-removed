






































#ifndef jsstrinlines_h___
#define jsstrinlines_h___

#include "jsatom.h"
#include "jsstr.h"

#include "jscntxtinlines.h"
#include "jsgcinlines.h"

namespace js {

static inline bool
CheckStringLength(JSContext *cx, size_t length)
{
    if (JS_UNLIKELY(length > JSString::MAX_LENGTH)) {
        if (JS_ON_TRACE(cx)) {
            



            if (!CanLeaveTrace(cx))
                return NULL;

            LeaveTrace(cx);
        }
        js_ReportAllocationOverflow(cx);
        return false;
    }

    return true;
}









class StringBuffer
{
    
    typedef Vector<jschar, 32, ContextAllocPolicy> CharBuffer;

    CharBuffer cb;

    static inline bool checkLength(JSContext *cx, size_t length);
    inline bool checkLength(size_t length);
    JSContext *context() const { return cb.allocPolicy().context(); }
    jschar *extractWellSized();

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

    
    void infallibleAppend(const jschar c) {
        cb.infallibleAppend(c);
    }
    void infallibleAppend(const jschar *chars, size_t len) {
        cb.infallibleAppend(chars, len);
    }
    void infallibleAppend(const jschar *begin, const jschar *end) {
        cb.infallibleAppend(begin, end);
    }
    void infallibleAppendN(const jschar c, size_t n) {
        cb.infallibleAppendN(c, n);
    }

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

    



    JSFixedString *finishString();

    
    JSAtom *finishAtom();

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
    if (!linear)
        return false;
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
    InflateStringToBuffer(context(), cstr, cstrlen, begin() + lengthBefore, &cstrlen);
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

extern bool
ValueToStringBufferSlow(JSContext *cx, const Value &v, StringBuffer &sb);

inline bool
ValueToStringBuffer(JSContext *cx, const Value &v, StringBuffer &sb)
{
    if (v.isString())
        return sb.append(v.toString());

    return ValueToStringBufferSlow(cx, v, sb);
}

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
    JSLinearString *cur;

    bool settle(JSString *str) {
        while (str->isRope()) {
            JSRope &rope = str->asRope();
            if (!stack.append(rope.rightChild()))
                return false;
            str = rope.leftChild();
        }
        cur = &str->asLinear();
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

    JSLinearString *front() const {
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
