






#ifndef RegExp_h___
#define RegExp_h___

#include "jsprvtd.h"

JSObject *
js_InitRegExpClass(JSContext *cx, js::HandleObject obj);






namespace js {






bool
ExecuteRegExp(JSContext *cx, RegExpStatics *res, RegExpObject &reobj,
              Handle<JSStableString*> input, StableCharPtr chars, size_t length,
              size_t *lastIndex, RegExpExecType type, Value *rval);

bool
ExecuteRegExp(JSContext *cx, RegExpStatics *res, RegExpShared &shared,
              Handle<JSStableString*> input, StableCharPtr chars, size_t length,
              size_t *lastIndex, RegExpExecType type, Value *rval);

bool
ExecuteRegExp(JSContext *cx, RegExpExecType execType, HandleObject regexp,
              HandleString string, MutableHandleValue rval);

extern JSBool
regexp_exec(JSContext *cx, unsigned argc, Value *vp);

extern JSBool
regexp_test(JSContext *cx, unsigned argc, Value *vp);

} 

#endif
