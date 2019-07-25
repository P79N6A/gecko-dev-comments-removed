



#include "tests.h"
#include "jsdbgapi.h"

const char code[] =
    "xx = 1;       \n\
                   \n\
try {              \n\
	 debugger; \n\
                   \n\
	 xx += 1;  \n\
}                  \n\
catch (e)          \n\
{                  \n\
	 xx += 1;  \n\
}";


BEGIN_TEST(testScriptInfo)
{
    uintN startLine = 1000;

    JSObject *scriptObj = JS_CompileScript(cx, global, code, strlen(code),
                                           __FILE__, startLine);

    CHECK(scriptObj);

    JSScript *script = JS_GetScriptFromObject(scriptObj);
    jsbytecode *start = JS_LineNumberToPC(cx, script, startLine);
    CHECK_EQUAL(JS_GetScriptBaseLineNumber(cx, script), startLine);
    CHECK_EQUAL(JS_PCToLineNumber(cx, script, start), startLine);
    CHECK_EQUAL(JS_GetScriptLineExtent(cx, script), 10);
    CHECK(strcmp(JS_GetScriptFilename(cx, script), __FILE__) == 0);
    
    return true;
}
END_TEST(testScriptInfo)
