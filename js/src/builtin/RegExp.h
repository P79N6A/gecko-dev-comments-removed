






#ifndef RegExp_h___
#define RegExp_h___

#include "jsprvtd.h"

#include "vm/MatchPairs.h"
#include "vm/RegExpObject.h"

JSObject *
js_InitRegExpClass(JSContext *cx, js::HandleObject obj);






namespace js {

RegExpRunStatus
ExecuteRegExp(JSContext *cx, HandleObject regexp, HandleString string,
              MatchConduit &matches);








bool
ExecuteRegExpLegacy(JSContext *cx, RegExpStatics *res, RegExpObject &reobj,
                    Handle<JSStableString*> input, StableCharPtr chars, size_t length,
                    size_t *lastIndex, JSBool test, jsval *rval);


bool
CreateRegExpMatchResult(JSContext *cx, HandleString string, MatchPairs &matches, Value *rval);

bool
CreateRegExpMatchResult(JSContext *cx, JSString *input_, StableCharPtr chars, size_t length,
                        MatchPairs &matches, Value *rval);

extern JSBool
regexp_exec(JSContext *cx, unsigned argc, Value *vp);

bool
regexp_test_raw(JSContext *cx, HandleObject regexp, HandleString input, JSBool *result);

extern JSBool
regexp_test(JSContext *cx, unsigned argc, Value *vp);

} 

#endif 
