




































#ifndef json_h___
#define json_h___



#include "jsscan.h"

#define JSON_MAX_DEPTH  2048
#define JSON_PARSER_BUFSIZE 1024

JS_BEGIN_EXTERN_C

extern JSClass js_JSONClass;

extern JSObject *
js_InitJSONClass(JSContext *cx, JSObject *obj);

extern JSBool
js_Stringify(JSContext *cx, jsval *vp, JSObject *replacer, jsval space,
             JSCharVector &cb);

extern JSBool js_TryJSON(JSContext *cx, jsval *vp);

enum JSONParserState {
    JSON_PARSE_STATE_INIT,
    JSON_PARSE_STATE_OBJECT_VALUE,
    JSON_PARSE_STATE_VALUE,
    JSON_PARSE_STATE_OBJECT,
    JSON_PARSE_STATE_OBJECT_PAIR,
    JSON_PARSE_STATE_OBJECT_IN_PAIR,
    JSON_PARSE_STATE_ARRAY,
    JSON_PARSE_STATE_STRING,
    JSON_PARSE_STATE_STRING_ESCAPE,
    JSON_PARSE_STATE_STRING_HEX,
    JSON_PARSE_STATE_NUMBER,
    JSON_PARSE_STATE_KEYWORD,
    JSON_PARSE_STATE_FINISHED
};

enum JSONDataType {
    JSON_DATA_STRING,
    JSON_DATA_KEYSTRING,
    JSON_DATA_NUMBER,
    JSON_DATA_KEYWORD
};

struct JSONParser;

extern JSONParser *
js_BeginJSONParse(JSContext *cx, jsval *rootVal);

extern JSBool
js_ConsumeJSONText(JSContext *cx, JSONParser *jp, const jschar *data, uint32 len);

extern JSBool
js_FinishJSONParse(JSContext *cx, JSONParser *jp, jsval reviver);

JS_END_EXTERN_C

#endif 
