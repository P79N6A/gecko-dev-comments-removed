





#ifndef builtin_RegExp_h
#define builtin_RegExp_h

#include "vm/RegExpObject.h"






namespace js {

JSObject*
InitRegExpClass(JSContext* cx, HandleObject obj);



enum RegExpStaticsUpdate { UpdateRegExpStatics, DontUpdateRegExpStatics };


enum RegExpStaticsUse { UseRegExpStatics, DontUseRegExpStatics };


enum RegExpCreationMode { CreateForCompile, CreateForConstruct };

RegExpRunStatus
ExecuteRegExp(JSContext* cx, HandleObject regexp, HandleString string,
              MatchPairs* matches, RegExpStaticsUpdate staticsUpdate);








bool
ExecuteRegExpLegacy(JSContext* cx, RegExpStatics* res, RegExpObject& reobj,
                    HandleLinearString input, size_t* lastIndex, bool test,
                    MutableHandleValue rval);


bool
CreateRegExpMatchResult(JSContext* cx, HandleString input, const MatchPairs& matches,
                        MutableHandleValue rval);

extern bool
regexp_exec_raw(JSContext* cx, HandleObject regexp, HandleString input, MatchPairs* maybeMatches,
                MutableHandleValue output);

extern bool
regexp_exec(JSContext* cx, unsigned argc, Value* vp);

bool
regexp_test_raw(JSContext* cx, HandleObject regexp, HandleString input, bool* result);

extern bool
regexp_test(JSContext* cx, unsigned argc, Value* vp);










extern bool
regexp_exec_no_statics(JSContext* cx, unsigned argc, Value* vp);






extern bool
regexp_test_no_statics(JSContext* cx, unsigned argc, Value* vp);








extern bool
regexp_construct_no_statics(JSContext* cx, unsigned argc, Value* vp);

extern bool
IsRegExp(JSContext* cx, HandleValue value, bool* result);


extern bool
regexp_construct(JSContext* cx, unsigned argc, Value* vp);
extern JSObject*
CreateRegExpPrototype(JSContext* cx, JSProtoKey key);
extern const JSPropertySpec regexp_static_props[];
extern const JSPropertySpec regexp_properties[];
extern const JSFunctionSpec regexp_methods[];

} 

#endif 
