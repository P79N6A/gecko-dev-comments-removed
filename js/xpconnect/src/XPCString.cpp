



















#include "nscore.h"
#include "nsString.h"
#include "nsStringBuffer.h"
#include "jsapi.h"
#include "xpcpublic.h"



void
XPCStringConvert::FreeZoneCache(JS::Zone *zone)
{
    
    
    nsAutoPtr<ZoneStringCache> cache(static_cast<ZoneStringCache*>(JS_GetZoneUserData(zone)));
    JS_SetZoneUserData(zone, nullptr);
}


void
XPCStringConvert::ClearZoneCache(JS::Zone *zone)
{
    ZoneStringCache *cache = static_cast<ZoneStringCache*>(JS_GetZoneUserData(zone));
    if (cache) {
        cache->mBuffer = nullptr;
        cache->mString = nullptr;
    }
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
        JS::RootedValue val(cx);
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

    
    str = JS_NewUCStringCopyN(cx, readable.BeginReading(), length);
    return str ? JS::StringValue(str) : JS::NullValue();
}
