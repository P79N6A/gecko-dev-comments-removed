



















#include "xpcprivate.h"
#include "nsStringBuffer.h"






static nsStringBuffer* sCachedBuffer = nullptr;
static JSString* sCachedString = nullptr;




void
XPCStringConvert::ClearCache()
{
    sCachedBuffer = nullptr;
    sCachedString = nullptr;
}

static void
FinalizeDOMString(const JSStringFinalizer *fin, jschar *chars)
{
    nsStringBuffer* buf = nsStringBuffer::FromData(chars);
    buf->Release();
}

static const JSStringFinalizer sDOMStringFinalizer = { FinalizeDOMString };



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
        if (buf == sCachedBuffer &&
            js::GetGCThingCompartment(sCachedString) == js::GetContextCompartment(cx)) {
            
            return JS::StringValue(sCachedString);
        }

        

        str = JS_NewExternalString(cx,
                                   reinterpret_cast<jschar *>(buf->Data()),
                                   length, &sDOMStringFinalizer);

        if (str) {
            *sharedBuffer = buf;
            sCachedString = str;
            sCachedBuffer = buf;
        }
    } else {
        

        jschar *chars = reinterpret_cast<jschar *>
                                        (JS_malloc(cx, (length + 1) *
                                                   sizeof(jschar)));
        if (!chars)
            return JSVAL_NULL;

        if (length && !CopyUnicodeTo(readable, 0,
                                     reinterpret_cast<PRUnichar *>(chars),
                                     length)) {
            JS_free(cx, chars);
            return JSVAL_NULL;
        }

        chars[length] = 0;

        str = JS_NewUCString(cx, chars, length);
        if (!str)
            JS_free(cx, chars);
    }
    return str ? STRING_TO_JSVAL(str) : JSVAL_NULL;
}
