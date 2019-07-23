





















































#include "xpcprivate.h"
#include "nsStringBuffer.h"

static int sDOMStringFinalizerIndex = -1;

static void
DOMStringFinalizer(JSContext *cx, JSString *str)
{
    nsStringBuffer::FromData(JS_GetStringChars(str))->Release();
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
                                  const nsAString &readable)
{
    JSString *str;

    PRUint32 length = readable.Length();

    JSAtom *atom;
    if (length == 0 && (atom = cx->runtime->atomState.emptyAtom))
    {
        NS_ASSERTION(ATOM_IS_STRING(atom), "What kind of atom is this?");
        return ATOM_KEY(atom);
    }

    nsStringBuffer *buf = nsStringBuffer::FromString(readable);
    if (buf)
    {
        

        if (sDOMStringFinalizerIndex == -1)
        {
            sDOMStringFinalizerIndex =
                    JS_AddExternalStringFinalizer(DOMStringFinalizer);
            if (sDOMStringFinalizerIndex == -1)
                return JSVAL_NULL;
        }

        str = JS_NewExternalString(cx, 
                                   reinterpret_cast<jschar *>(buf->Data()),
                                   length, sDOMStringFinalizerIndex);

        if (str)
            buf->AddRef();
    }
    else
    {
        

        jschar *chars = reinterpret_cast<jschar *>
                                        (JS_malloc(cx, (length + 1) *
                                                      sizeof(jschar)));
        if (!chars)
            return JSVAL_NULL;

        if (length && !CopyUnicodeTo(readable, 0,
                                     reinterpret_cast<PRUnichar *>(chars),
                                     length))
        {
            JS_free(cx, chars);
            return JSVAL_NULL;
        }

        chars[length] = 0;

        str = JS_NewUCString(cx, chars, length);
        if (!str)
            JS_free(cx, chars);
    }
    return STRING_TO_JSVAL(str);
}


XPCReadableJSStringWrapper *
XPCStringConvert::JSStringToReadable(XPCCallContext& ccx, JSString *str)
{
    return ccx.NewStringWrapper(reinterpret_cast<PRUnichar *>(JS_GetStringChars(str)),
                                JS_GetStringLength(str));
}
