





#ifndef StringBuffer_h___
#define StringBuffer_h___

#include "mozilla/Attributes.h"
#include "mozilla/DebugOnly.h"

#include "jscntxt.h"

#include "js/Vector.h"

ForwardDeclareJS(Atom);
ForwardDeclareJS(FlatString);

namespace js {












class StringBuffer
{
    
    typedef Vector<jschar, 32, ContextAllocPolicy> CharBuffer;

    CharBuffer cb;

    JSContext *context() const { return cb.allocPolicy().context(); }

    StringBuffer(const StringBuffer &other) MOZ_DELETE;
    void operator=(const StringBuffer &other) MOZ_DELETE;

  public:
    explicit StringBuffer(JSContext *cx) : cb(cx) { }

    inline bool reserve(size_t len) { return cb.reserve(len); }
    inline bool resize(size_t len) { return cb.resize(len); }
    inline bool append(const jschar c) { return cb.append(c); }
    inline bool append(const jschar *chars, size_t len) { return cb.append(chars, len); }
    inline bool append(const CharPtr chars, size_t len) { return cb.append(chars.get(), len); }
    inline bool append(const jschar *begin, const jschar *end) { return cb.append(begin, end); }
    inline bool append(JSString *str);
    inline bool append(JSLinearString *str);
    inline bool appendN(const jschar c, size_t n) { return cb.appendN(c, n); }
    inline bool appendInflated(const char *cstr, size_t len);

    template <size_t ArrayLength>
    bool append(const char (&array)[ArrayLength]) {
        return cb.append(array, array + ArrayLength - 1); 
    }

    
    void infallibleAppend(const jschar c) {
        cb.infallibleAppend(c);
    }
    void infallibleAppend(const jschar *chars, size_t len) {
        cb.infallibleAppend(chars, len);
    }
    void infallibleAppend(const CharPtr chars, size_t len) {
        cb.infallibleAppend(chars.get(), len);
    }
    void infallibleAppend(const jschar *begin, const jschar *end) {
        cb.infallibleAppend(begin, end);
    }
    void infallibleAppendN(const jschar c, size_t n) {
        cb.infallibleAppendN(c, n);
    }

    jschar *begin() { return cb.begin(); }
    jschar *end() { return cb.end(); }
    const jschar *begin() const { return cb.begin(); }
    const jschar *end() const { return cb.end(); }
    bool empty() const { return cb.empty(); }
    size_t length() const { return cb.length(); }

    



    js::RawFlatString finishString();

    
    js::RawAtom finishAtom();

    




    jschar *extractWellSized();
};

inline bool
StringBuffer::append(JSLinearString *str)
{
    JS::Anchor<JSString *> anch(str);
    return cb.append(str->chars(), str->length());
}

inline bool
StringBuffer::append(JSString *str)
{
    JSLinearString *linear = str->ensureLinear(context());
    if (!linear)
        return false;
    return append(linear);
}

inline bool
StringBuffer::appendInflated(const char *cstr, size_t cstrlen)
{
    size_t lengthBefore = length();
    if (!cb.growByUninitialized(cstrlen))
        return false;
    mozilla::DebugOnly<size_t> oldcstrlen = cstrlen;
    mozilla::DebugOnly<bool> ok = InflateStringToBuffer(context(), cstr, cstrlen,
                                                        begin() + lengthBefore, &cstrlen);
    JS_ASSERT(ok && oldcstrlen == cstrlen);
    return true;
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


inline bool
BooleanToStringBuffer(JSContext *cx, bool b, StringBuffer &sb)
{
    return b ? sb.append("true") : sb.append("false");
}

}  

#endif 
