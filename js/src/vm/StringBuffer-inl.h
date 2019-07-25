





#ifndef StringBuffer_inl_h___
#define StringBuffer_inl_h___

#include "vm/StringBuffer.h"

#include "vm/String-inl.h"

namespace js {

inline bool
StringBuffer::reserve(size_t len)
{
    return cb.reserve(len);
}

inline bool
StringBuffer::resize(size_t len)
{
    return cb.resize(len);
}

inline bool
StringBuffer::append(const jschar c)
{
    return cb.append(c);
}

inline bool
StringBuffer::append(const jschar *chars, size_t len)
{
    return cb.append(chars, len);
}

inline bool
StringBuffer::append(const jschar *begin, const jschar *end)
{
    return cb.append(begin, end);
}

inline bool
StringBuffer::appendN(const jschar c, size_t n)
{
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
