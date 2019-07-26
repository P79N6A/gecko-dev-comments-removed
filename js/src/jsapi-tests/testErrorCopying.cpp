









#include "jsapi-tests/tests.h"

static uint32_t column = 0;

static void
my_ErrorReporter(JSContext *cx, const char *message, JSErrorReport *report)
{
    column = report->column;
}

BEGIN_TEST(testErrorCopying_columnCopied)
{
        
        
    EXEC("function check() { Object; foo; }");

    JS::RootedValue rval(cx);
    JS_SetErrorReporter(cx, my_ErrorReporter);
    CHECK(!JS_CallFunctionName(cx, global, "check", 0, nullptr, rval.address()));
    CHECK(column == 27);
    return true;
}
END_TEST(testErrorCopying_columnCopied)
