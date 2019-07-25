





#ifndef StringBuffer_inl_h___
#define StringBuffer_inl_h___

#include "vm/StringBuffer.h"

#include "vm/String-inl.h"

namespace js {

inline bool
StringBuffer::checkLength(size_t length)
{
    return JSString::validateLength(context(), length);
}

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
StringBuffer::appendN(const jschar c, size_t n)
{
    if (!checkLength(cb.length() + n))
        return false;
    return cb.appendN(c, n);
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
