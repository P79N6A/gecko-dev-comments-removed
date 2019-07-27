









#ifndef jsexn_h
#define jsexn_h

#include "jsapi.h"
#include "NamespaceImports.h"

namespace js {
class ErrorObject;

JSErrorReport *
CopyErrorReport(JSContext *cx, JSErrorReport *report);

JSString *
ComputeStackString(JSContext *cx);
}



























extern bool
js_ErrorToException(JSContext *cx, const char *message, JSErrorReport *reportp,
                    JSErrorCallback callback, void *userRef);

















extern bool
js_ReportUncaughtException(JSContext *cx);

extern JSErrorReport *
js_ErrorFromException(JSContext *cx, js::HandleObject obj);

extern const JSErrorFormatString *
js_GetLocalizedErrorMessage(js::ExclusiveContext *cx, void *userRef, const char *locale,
                            const unsigned errorNumber);








extern JSObject *
js_CopyErrorObject(JSContext *cx, JS::Handle<js::ErrorObject*> errobj);

static inline JSProtoKey
GetExceptionProtoKey(JSExnType exn)
{
    JS_ASSERT(JSEXN_ERR <= exn);
    JS_ASSERT(exn < JSEXN_LIMIT);
    return JSProtoKey(JSProto_Error + int(exn));
}

static inline JSExnType
ExnTypeFromProtoKey(JSProtoKey key)
{
    JSExnType type = static_cast<JSExnType>(key - JSProto_Error);
    JS_ASSERT(type >= JSEXN_ERR);
    JS_ASSERT(type < JSEXN_LIMIT);
    return type;
}

#endif 
