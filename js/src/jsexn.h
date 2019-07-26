









#ifndef jsexn_h
#define jsexn_h

#include "jsapi.h"
#include "NamespaceImports.h"

namespace js {
class ErrorObject;
}



























extern bool
js_ErrorToException(JSContext *cx, const char *message, JSErrorReport *reportp,
                    JSErrorCallback callback, void *userRef);

















extern bool
js_ReportUncaughtException(JSContext *cx);

extern JSErrorReport *
js_ErrorFromException(jsval exn);

extern const JSErrorFormatString *
js_GetLocalizedErrorMessage(js::ExclusiveContext *cx, void *userRef, const char *locale,
                            const unsigned errorNumber);









extern JSObject *
js_CopyErrorObject(JSContext *cx, JS::Handle<js::ErrorObject*> errobj, js::HandleObject scope);

static inline JSProtoKey
GetExceptionProtoKey(JSExnType exn)
{
    JS_ASSERT(JSEXN_ERR <= exn);
    JS_ASSERT(exn < JSEXN_LIMIT);
    return JSProtoKey(JSProto_Error + int(exn));
}

#endif 
