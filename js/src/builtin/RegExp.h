







































#ifndef RegExp_h___
#define RegExp_h___

#include "jsprvtd.h"

JSObject *
js_InitRegExpClass(JSContext *cx, JSObject *obj);






namespace js {

extern JSBool
regexp_exec(JSContext *cx, uintN argc, Value *vp);

extern JSBool
regexp_test(JSContext *cx, uintN argc, Value *vp);

} 

#endif
