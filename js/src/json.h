




































#ifndef json_h___
#define json_h___

#include "jsprvtd.h"
#include "jspubtd.h"
#include "jsvector.h"

#define JSON_MAX_DEPTH  2048
#define JSON_PARSER_BUFSIZE 1024

extern JSObject *
js_InitJSONClass(JSContext *cx, JSObject *obj);

extern JSBool
js_Stringify(JSContext *cx, js::Value *vp, JSObject *replacer, js::Value space,
             js::StringBuffer &sb);








enum DecodingMode { STRICT, LEGACY };

namespace js {

extern JS_FRIEND_API(JSBool)
ParseJSONWithReviver(JSContext *cx, const jschar *chars, size_t length, const Value &filter,
                     Value *vp, DecodingMode decodingMode = STRICT);

} 

#endif 
