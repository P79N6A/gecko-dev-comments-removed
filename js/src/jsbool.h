






































#ifndef jsbool_h___
#define jsbool_h___




#include "jsapi.h"

JS_BEGIN_EXTERN_C












#define JSVAL_HOLE      PSEUDO_BOOLEAN_TO_JSVAL(3)
#define JSVAL_ARETURN   PSEUDO_BOOLEAN_TO_JSVAL(4)

static JS_ALWAYS_INLINE JSBool
JSVAL_TO_PUBLIC_PSEUDO_BOOLEAN(jsval v)
{
    JS_ASSERT(v == JSVAL_TRUE || v == JSVAL_FALSE || v == JSVAL_VOID);
    return JSVAL_TO_PSEUDO_BOOLEAN(v);
}

extern JSClass js_BooleanClass;

extern JSObject *
js_InitBooleanClass(JSContext *cx, JSObject *obj);

extern JSString *
js_BooleanToString(JSContext *cx, JSBool b);

extern JSBool
js_ValueToBoolean(jsval v);

JS_END_EXTERN_C

#endif 
