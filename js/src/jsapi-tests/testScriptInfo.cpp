






#include "js/OldDebugAPI.h"
#include "jsapi-tests/tests.h"

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
}\n\
//@ sourceMappingURL=http://example.com/path/to/source-map.json";


static bool
CharsMatch(const jschar *p, const char *q) {
    while (*q) {
        if (*p++ != *q++)
            return false;
    }
    return true;
}


BEGIN_TEST(testScriptInfo)
{
    unsigned startLine = 1000;

    JS::CompileOptions options(cx);
    options.setFileAndLine(__FILE__, startLine);
    JS::RootedScript script(cx, JS_CompileScript(cx, global, code, strlen(code),
                                                 options));

    CHECK(script);

    jsbytecode *start = JS_LineNumberToPC(cx, script, startLine);
    CHECK_EQUAL(JS_GetScriptBaseLineNumber(cx, script), startLine);
    CHECK_EQUAL(JS_PCToLineNumber(cx, script, start), startLine);
    CHECK_EQUAL(JS_GetScriptLineExtent(cx, script), 11);
    CHECK(strcmp(JS_GetScriptFilename(cx, script), __FILE__) == 0);
    const jschar *sourceMap = JS_GetScriptSourceMap(cx, script);
    CHECK(sourceMap);
    CHECK(CharsMatch(sourceMap, "http://example.com/path/to/source-map.json"));

    return true;
}
END_TEST(testScriptInfo)
