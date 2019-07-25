





#ifndef StringBuffer_h___
#define StringBuffer_h___

#include "mozilla/Attributes.h"

#include "jscntxt.h"
#include "jspubtd.h"

#include "js/Vector.h"

namespace js {
















class StringBuffer
{
    
    typedef Vector<jschar, 32, ContextAllocPolicy> CharBuffer;

    CharBuffer cb;

    static inline bool checkLength(JSContext *cx, size_t length);
    inline bool checkLength(size_t length);
    JSContext *context() const { return cb.allocPolicy().context(); }
    jschar *extractWellSized();

    StringBuffer(const StringBuffer &other) MOZ_DELETE;
    void operator=(const StringBuffer &other) MOZ_DELETE;

  public:
    explicit inline StringBuffer(JSContext *cx);
    inline bool reserve(size_t len);
    inline bool resize(size_t len);
    inline bool append(const jschar c);
    inline bool append(const jschar *chars, size_t len);
    inline bool append(const jschar *begin, const jschar *end);
    inline bool append(JSString *str);
    inline bool append(JSLinearString *str);
    inline bool appendN(const jschar c, size_t n);
    inline bool appendInflated(const char *cstr, size_t len);

    
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

    JSAtom *atomize(unsigned flags = 0);
    static JSAtom *atomize(JSContext *cx, const CharBuffer &cb, unsigned flags = 0);
    static JSAtom *atomize(JSContext *cx, const jschar *begin, size_t length, unsigned flags = 0);

    void replaceRawBuffer(jschar *chars, size_t len) { cb.replaceRawBuffer(chars, len); }
    jschar *begin() { return cb.begin(); }
    jschar *end() { return cb.end(); }
    const jschar *begin() const { return cb.begin(); }
    const jschar *end() const { return cb.end(); }
    bool empty() const { return cb.empty(); }
    inline size_t length() const;

    



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
StringBuffer::append(JSString *str)
{
    JSLinearString *linear = str->ensureLinear(context());
    if (!linear)
        return false;
    return append(linear);
}

inline bool
StringBuffer::append(JSLinearString *str)
{
    JS::Anchor<JSString *> anch(str);
    return cb.append(str->chars(), str->length());
}

inline size_t
StringBuffer::length() const
{
    JS_ASSERT(cb.length() <= JSString::MAX_LENGTH);
    return cb.length();
}

inline bool
StringBuffer::appendInflated(const char *cstr, size_t cstrlen)
{
    size_t lengthBefore = length();
    if (!cb.growByUninitialized(cstrlen))
        return false;
    DebugOnly<size_t> oldcstrlen = cstrlen;
    DebugOnly<bool> ok = InflateStringToBuffer(context(), cstr, cstrlen,
                                               begin() + lengthBefore, &cstrlen);
    JS_ASSERT(ok && oldcstrlen == cstrlen);
    return true;
}

}  

#endif 
