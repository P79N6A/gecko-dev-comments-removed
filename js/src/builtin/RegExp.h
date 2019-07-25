







































#ifndef RegExp_h___
#define RegExp_h___

#include "jsprvtd.h"

JSObject *
js_InitRegExpClass(JSContext *cx, JSObject *obj);






namespace js {






bool
ExecuteRegExp(JSContext *cx, RegExpStatics *res, RegExpObject *reobj, JSLinearString *input,
              const jschar *chars, size_t length,
              size_t *lastIndex, RegExpExecType type, Value *rval);

bool
ExecuteRegExp(JSContext *cx, RegExpStatics *res, RegExpMatcher &matcher, JSLinearString *input,
              const jschar *chars, size_t length,
              size_t *lastIndex, RegExpExecType type, Value *rval);

extern JSBool
regexp_exec(JSContext *cx, uintN argc, Value *vp);

extern JSBool
regexp_test(JSContext *cx, uintN argc, Value *vp);

} 

#endif
