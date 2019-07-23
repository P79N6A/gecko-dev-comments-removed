






































#ifndef jsbool_h___
#define jsbool_h___




#include "jsapi.h"

JS_BEGIN_EXTERN_C















#define JSVAL_HOLE_FLAG jsval(4 << JSVAL_TAGBITS)
#define JSVAL_HOLE      (JSVAL_VOID | JSVAL_HOLE_FLAG)
#define JSVAL_ARETURN   SPECIAL_TO_JSVAL(8)

extern JSClass js_BooleanClass;

extern JSObject *
js_InitBooleanClass(JSContext *cx, JSObject *obj);

extern JSString *
js_BooleanToString(JSContext *cx, JSBool b);

extern JSBool
js_BooleanToCharBuffer(JSContext *cx, JSBool b, JSCharVector &buf);

extern JSBool
js_ValueToBoolean(jsval v);

JS_END_EXTERN_C

#endif 
