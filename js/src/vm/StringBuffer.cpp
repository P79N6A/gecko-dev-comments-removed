





#include "vm/StringBuffer.h"

#include "jsobjinlines.h"

#include "vm/String-inl.h"

using namespace js;

jschar *
StringBuffer::extractWellSized()
{
    size_t capacity = cb.capacity();
    size_t length = cb.length();

    jschar *buf = cb.extractRawBuffer();
    if (!buf)
        return NULL;

    
    JS_ASSERT(capacity >= length);
    if (length > CharBuffer::sMaxInlineStorage && capacity - length > length / 4) {
        size_t bytes = sizeof(jschar) * (length + 1);
        JSContext *cx = context();
        jschar *tmp = (jschar *)cx->realloc_(buf, bytes);
        if (!tmp) {
            js_free(buf);
            return NULL;
        }
        buf = tmp;
    }

    return buf;
}

UnrootedFlatString
StringBuffer::finishString()
{
    JSContext *cx = context();
    if (cb.empty())
        return UnrootedFlatString(cx->names().empty);

    size_t length = cb.length();
    if (!JSString::validateLength(cx, length))
        return UnrootedFlatString();

    JS_STATIC_ASSERT(JSShortString::MAX_SHORT_LENGTH < CharBuffer::InlineLength);
    if (JSShortString::lengthFits(length))
        return NewShortString(cx, cb.begin(), length);

    if (!cb.append('\0'))
        return UnrootedFlatString();

    jschar *buf = extractWellSized();
    if (!buf)
        return UnrootedFlatString();

    JSFlatString *str = js_NewString(cx, buf, length);
    if (!str)
        js_free(buf);
    return str;
}

UnrootedAtom
StringBuffer::finishAtom()
{
    AssertCanGC();
    JSContext *cx = context();

    size_t length = cb.length();
    if (length == 0)
        return UnrootedAtom(cx->names().empty);

    UnrootedAtom atom = AtomizeChars(cx, cb.begin(), length);
    cb.clear();
    return atom;
}

bool
js::ValueToStringBufferSlow(JSContext *cx, const Value &arg, StringBuffer &sb)
{
    Value v = arg;
    if (!ToPrimitive(cx, JSTYPE_STRING, &v))
        return false;

    if (v.isString())
        return sb.append(v.toString());
    if (v.isNumber())
        return NumberValueToStringBuffer(cx, v, sb);
    if (v.isBoolean())
        return BooleanToStringBuffer(cx, v.toBoolean(), sb);
    if (v.isNull())
        return sb.append(cx->names().null);
    JS_ASSERT(v.isUndefined());
    return sb.append(cx->names().undefined);
}
