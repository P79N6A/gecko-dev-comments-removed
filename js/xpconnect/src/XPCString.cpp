



















#include "nscore.h"
#include "nsString.h"
#include "nsStringBuffer.h"
#include "jsapi.h"
#include "xpcpublic.h"

using namespace JS;


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



bool
XPCStringConvert::ReadableToJSVal(JSContext *cx,
                                  const nsAString &readable,
                                  nsStringBuffer** sharedBuffer,
                                  MutableHandleValue vp)
{
    *sharedBuffer = nullptr;

    uint32_t length = readable.Length();
    if (length == 0) {
        vp.set(JS_GetEmptyStringValue(cx));
        return true;
    }

    nsStringBuffer *buf = nsStringBuffer::FromString(readable);
    if (buf) {
        bool shared;
        if (!StringBufferToJSVal(cx, buf, length, vp, &shared))
            return false;
        if (shared)
            *sharedBuffer = buf;
        return true;
    }

    
    JSString *str = JS_NewUCStringCopyN(cx, readable.BeginReading(), length);
    if (!str)
        return false;
    vp.setString(str);
    return true;
}
