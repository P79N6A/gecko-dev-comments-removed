









#ifndef jsexn_h___
#define jsexn_h___

#include "jsobj.h"




extern JSObject *
js_InitExceptionClasses(JSContext *cx, js::HandleObject obj);









extern JSBool
js_ErrorToException(JSContext *cx, const char *message, JSErrorReport *reportp,
                    JSErrorCallback callback, void *userRef);

















extern JSBool
js_ReportUncaughtException(JSContext *cx);

extern JSErrorReport *
js_ErrorFromException(JSContext *cx, jsval exn);

extern const JSErrorFormatString *
js_GetLocalizedErrorMessage(JSContext* cx, void *userRef, const char *locale,
                            const unsigned errorNumber);









extern JSObject *
js_CopyErrorObject(JSContext *cx, js::HandleObject errobj, js::HandleObject scope);

static JS_INLINE JSProtoKey
GetExceptionProtoKey(int exn)
{
    JS_ASSERT(JSEXN_ERR <= exn);
    JS_ASSERT(exn < JSEXN_LIMIT);
    return JSProtoKey(JSProto_Error + exn);
}

#endif 
