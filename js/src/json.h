




































#ifndef json_h___
#define json_h___



#include "jsscan.h"

#define JSON_MAX_DEPTH  2048
#define JSON_PARSER_BUFSIZE 1024

extern js::Class js_JSONClass;

extern JSObject *
js_InitJSONClass(JSContext *cx, JSObject *obj);

extern JSBool
js_Stringify(JSContext *cx, js::Value *vp, JSObject *replacer,
             const js::Value &space, JSCharBuffer &cb);

extern JSBool js_TryJSON(JSContext *cx, js::Value *vp);


enum JSONParserState {
    
    JSON_PARSE_STATE_INIT,

    
    JSON_PARSE_STATE_FINISHED,

    
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

    
    JSON_PARSE_STATE_KEYWORD
};

enum JSONDataType {
    JSON_DATA_STRING,
    JSON_DATA_KEYSTRING,
    JSON_DATA_NUMBER,
    JSON_DATA_KEYWORD
};

struct JSONParser;

extern JSONParser *
js_BeginJSONParse(JSContext *cx, js::Value *rootVal);

extern JSBool
js_ConsumeJSONText(JSContext *cx, JSONParser *jp, const jschar *data, uint32 len);

extern bool
js_FinishJSONParse(JSContext *cx, JSONParser *jp, const js::Value &reviver);

#endif 
