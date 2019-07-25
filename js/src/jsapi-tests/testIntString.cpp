



#include "tests.h"
#include "jsstr.h"

BEGIN_TEST(testIntString_bug515273)
{
    jsvalRoot v(cx);

    EVAL("'1';", v.addr());
    JSString *str = JSVAL_TO_STRING(v.value());
    CHECK(JSString::isStatic(str));
    CHECK(strcmp(JS_GetStringBytes(str), "1") == 0);

    EVAL("'42';", v.addr());
    str = JSVAL_TO_STRING(v.value());
    CHECK(JSString::isStatic(str));
    CHECK(strcmp(JS_GetStringBytes(str), "42") == 0);

    EVAL("'111';", v.addr());
    str = JSVAL_TO_STRING(v.value());
    CHECK(JSString::isStatic(str));
    CHECK(strcmp(JS_GetStringBytes(str), "111") == 0);

    
    EVAL("'a';", v.addr());
    str = JSVAL_TO_STRING(v.value());
    CHECK(JSString::isStatic(str));
    CHECK(strcmp(JS_GetStringBytes(str), "a") == 0);

    EVAL("'bc';", v.addr());
    str = JSVAL_TO_STRING(v.value());
    CHECK(JSString::isStatic(str));
    CHECK(strcmp(JS_GetStringBytes(str), "bc") == 0);

    return true;
}
END_TEST(testIntString_bug515273)
