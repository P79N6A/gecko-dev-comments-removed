





















































#include "xpcprivate.h"
#include "nsStringBuffer.h"

static int sDOMStringFinalizerIndex = -1;

static void
DOMStringFinalizer(JSContext *cx, JSString *str)
{
    jschar *chars = const_cast<jschar *>(JS_GetStringCharsZ(cx, str));
    NS_ASSERTION(chars, "How could this OOM if we allocated the memory?");
    nsStringBuffer::FromData(chars)->Release();
}

void
XPCStringConvert::ShutdownDOMStringFinalizer()
{
    if (sDOMStringFinalizerIndex == -1)
        return;

    JS_RemoveExternalStringFinalizer(DOMStringFinalizer);
    sDOMStringFinalizerIndex = -1;
}



jsval
XPCStringConvert::ReadableToJSVal(JSContext *cx,
                                  const nsAString &readable,
                                  nsStringBuffer** sharedBuffer)
{
    JSString *str;
    *sharedBuffer = nsnull;

    PRUint32 length = readable.Length();

    if (length == 0)
        return STRING_TO_JSVAL(js::GetEmptyAtom(cx));

    nsStringBuffer *buf = nsStringBuffer::FromString(readable);
    if (buf) {
        

        if (sDOMStringFinalizerIndex == -1) {
            sDOMStringFinalizerIndex =
                    JS_AddExternalStringFinalizer(DOMStringFinalizer);
            if (sDOMStringFinalizerIndex == -1)
                return JSVAL_NULL;
        }

        str = JS_NewExternalString(cx,
                                   reinterpret_cast<jschar *>(buf->Data()),
                                   length, sDOMStringFinalizerIndex);

        if (str) {
            *sharedBuffer = buf;
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
