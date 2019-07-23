






































#ifndef jsbool_h___
#define jsbool_h___




JS_BEGIN_EXTERN_C










#define JSVAL_HOLE      BOOLEAN_TO_JSVAL(2)
#define JSVAL_ARETURN   BOOLEAN_TO_JSVAL(3)

extern JSClass js_BooleanClass;

extern JSObject *
js_InitBooleanClass(JSContext *cx, JSObject *obj);

extern JSString *
js_BooleanToString(JSContext *cx, JSBool b);

extern JSBool
js_ValueToBoolean(JSContext *cx, jsval v, JSBool *bp);

JS_END_EXTERN_C

#endif 
