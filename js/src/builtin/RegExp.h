





#ifndef builtin_RegExp_h
#define builtin_RegExp_h

#include "jsprvtd.h"

#include "vm/MatchPairs.h"
#include "vm/RegExpObject.h"

JSObject *
js_InitRegExpClass(JSContext *cx, js::HandleObject obj);






namespace js {



enum RegExpStaticsUpdate { UpdateRegExpStatics, DontUpdateRegExpStatics };

RegExpRunStatus
ExecuteRegExp(JSContext *cx, HandleObject regexp, HandleString string,
              MatchConduit &matches, RegExpStaticsUpdate staticsUpdate);








bool
ExecuteRegExpLegacy(JSContext *cx, RegExpStatics *res, RegExpObject &reobj,
                    Handle<JSLinearString*> input, const jschar *chars, size_t length,
                    size_t *lastIndex, bool test, MutableHandleValue rval);


bool
CreateRegExpMatchResult(JSContext *cx, HandleString string, MatchPairs &matches,
                        MutableHandleValue rval);

bool
CreateRegExpMatchResult(JSContext *cx, HandleString input, const jschar *chars, size_t length,
                        MatchPairs &matches, MutableHandleValue rval);

extern bool
regexp_exec(JSContext *cx, unsigned argc, Value *vp);

bool
regexp_test_raw(JSContext *cx, HandleObject regexp, HandleString input, bool *result);

extern bool
regexp_test(JSContext *cx, unsigned argc, Value *vp);










extern bool
regexp_exec_no_statics(JSContext *cx, unsigned argc, Value *vp);






extern bool
regexp_test_no_statics(JSContext *cx, unsigned argc, Value *vp);

} 

#endif 
