



















#include "xpcprivate.h"
#include "nsStringBuffer.h"






nsStringBuffer* XPCStringConvert::sCachedBuffer = nullptr;
JSString* XPCStringConvert::sCachedString = nullptr;




void
XPCStringConvert::ClearCache()
{
    sCachedBuffer = nullptr;
    sCachedString = nullptr;
}

void
XPCStringConvert::FinalizeDOMString(const JSStringFinalizer *fin, jschar *chars)
{
    nsStringBuffer* buf = nsStringBuffer::FromData(chars);
    buf->Release();
}

const JSStringFinalizer XPCStringConvert::sDOMStringFinalizer =
    { XPCStringConvert::FinalizeDOMString };



jsval
XPCStringConvert::ReadableToJSVal(JSContext *cx,
                                  const nsAString &readable,
                                  nsStringBuffer** sharedBuffer)
{
    JSString *str;
    *sharedBuffer = nullptr;

    uint32_t length = readable.Length();

    if (length == 0)
        return JS_GetEmptyStringValue(cx);

    nsStringBuffer *buf = nsStringBuffer::FromString(readable);
    if (buf) {
        JS::Value val;
        bool shared;
        bool ok = StringBufferToJSVal(cx, buf, length, &val, &shared);
        if (!ok) {
            return JS::NullValue();
        }

        if (shared) {
            *sharedBuffer = buf;
        }
        return val;
    }

    

    jschar *chars = reinterpret_cast<jschar *>
                                    (JS_malloc(cx, (length + 1) *
                                               sizeof(jschar)));
    if (!chars)
        return JS::NullValue();

    if (length && !CopyUnicodeTo(readable, 0,
                                 reinterpret_cast<PRUnichar *>(chars),
                                 length)) {
        JS_free(cx, chars);
        return JS::NullValue();
    }

    chars[length] = 0;

    str = JS_NewUCString(cx, chars, length);
    if (!str) {
        JS_free(cx, chars);
    }

    return str ? STRING_TO_JSVAL(str) : JSVAL_NULL;
}
