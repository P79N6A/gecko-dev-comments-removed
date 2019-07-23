










































#ifndef jsexn_h___
#define jsexn_h___

JS_BEGIN_EXTERN_C

extern JSClass js_ErrorClass;




extern JSObject *
js_InitExceptionClasses(JSContext *cx, JSObject *obj);









extern JSBool
js_ErrorToException(JSContext *cx, const char *message, JSErrorReport *reportp,
                    JSErrorCallback callback, void *userRef);

















extern JSBool
js_ReportUncaughtException(JSContext *cx);

extern JSErrorReport *
js_ErrorFromException(JSContext *cx, jsval exn);

extern const JSErrorFormatString *
js_GetLocalizedErrorMessage(JSContext* cx, void *userRef, const char *locale,
                            const uintN errorNumber);

JS_END_EXTERN_C

#endif 
