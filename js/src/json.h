




































#ifndef json_h___
#define json_h___

#include "jsprvtd.h"
#include "jspubtd.h"
#include "jsvalue.h"
#include "jsvector.h"

#define JSON_MAX_DEPTH  2048
#define JSON_PARSER_BUFSIZE 1024

extern js::Class js_JSONClass;

extern JSObject *
js_InitJSONClass(JSContext *cx, JSObject *obj);

extern JSBool
js_Stringify(JSContext *cx, js::Value *vp, JSObject *replacer,
             const js::Value &space, js::StringBuffer &sb);

extern JSBool js_TryJSON(JSContext *cx, js::Value *vp);


enum JSONParserState {
    
    JSON_PARSE_STATE_INIT,

    
    JSON_PARSE_STATE_FINISHED,

    
    JSON_PARSE_STATE_VALUE,

    
    JSON_PARSE_STATE_OBJECT_INITIAL_PAIR,

    
    JSON_PARSE_STATE_OBJECT_PAIR,

    
    JSON_PARSE_STATE_OBJECT_IN_PAIR,

    
    JSON_PARSE_STATE_OBJECT_AFTER_PAIR,

    
    JSON_PARSE_STATE_ARRAY_INITIAL_VALUE,

    
    JSON_PARSE_STATE_ARRAY_AFTER_ELEMENT,


    

    
    JSON_PARSE_STATE_STRING,

    
    JSON_PARSE_STATE_STRING_ESCAPE,

    
    JSON_PARSE_STATE_STRING_HEX,

    
    JSON_PARSE_STATE_NUMBER,

    
    JSON_PARSE_STATE_KEYWORD
};

struct JSONParser;

extern JSONParser *
js_BeginJSONParse(JSContext *cx, js::Value *rootVal, bool suppressErrors = false);


#ifdef STRICT
#undef STRICT
#endif
#ifdef LEGACY
#undef LEGACY
#endif








enum DecodingMode { STRICT, LEGACY };

extern JS_FRIEND_API(JSBool)
js_ConsumeJSONText(JSContext *cx, JSONParser *jp, const jschar *data, uint32 len,
                   DecodingMode decodingMode = STRICT);

extern bool
js_FinishJSONParse(JSContext *cx, JSONParser *jp, const js::Value &reviver);

namespace js {

extern JS_FRIEND_API(JSBool)
ParseJSONWithReviver(JSContext *cx, const jschar *chars, size_t length, const Value &filter,
                     Value *vp, DecodingMode decodingMode = STRICT);

} 

#endif 
