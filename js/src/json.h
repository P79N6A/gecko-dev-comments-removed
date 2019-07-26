



#ifndef json_h___
#define json_h___

#include "jsprvtd.h"
#include "jspubtd.h"
#include "jsapi.h"

#include "js/Vector.h"

#define JSON_MAX_DEPTH  2048
#define JSON_PARSER_BUFSIZE 1024

extern JSObject *
js_InitJSONClass(JSContext *cx, js::HandleObject obj);

extern JSBool
js_Stringify(JSContext *cx, js::MutableHandleValue vp,
             JSObject *replacer, js::Value space,
             js::StringBuffer &sb);


#undef STRICT
#undef LEGACY








enum DecodingMode { STRICT, LEGACY };

namespace js {

extern JS_FRIEND_API(JSBool)
ParseJSONWithReviver(JSContext *cx, JS::StableCharPtr chars, size_t length, HandleValue filter,
                     MutableHandleValue vp, DecodingMode decodingMode = STRICT);

} 

#endif 
