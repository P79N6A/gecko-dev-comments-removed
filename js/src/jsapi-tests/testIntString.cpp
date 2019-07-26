







#include "tests.h"
#include "vm/String.h"

BEGIN_TEST(testIntString_bug515273)
{
    JS::RootedValue v(cx);

    EVAL("'1';", v.address());
    JSString *str = JSVAL_TO_STRING(v);
    CHECK(JS_StringHasBeenInterned(cx, str));
    CHECK(JS_FlatStringEqualsAscii(JS_ASSERT_STRING_IS_FLAT(str), "1"));

    EVAL("'42';", v.address());
    str = JSVAL_TO_STRING(v);
    CHECK(JS_StringHasBeenInterned(cx, str));
    CHECK(JS_FlatStringEqualsAscii(JS_ASSERT_STRING_IS_FLAT(str), "42"));

    EVAL("'111';", v.address());
    str = JSVAL_TO_STRING(v);
    CHECK(JS_StringHasBeenInterned(cx, str));
    CHECK(JS_FlatStringEqualsAscii(JS_ASSERT_STRING_IS_FLAT(str), "111"));

    
    EVAL("'a';", v.address());
    str = JSVAL_TO_STRING(v);
    CHECK(JS_StringHasBeenInterned(cx, str));
    CHECK(JS_FlatStringEqualsAscii(JS_ASSERT_STRING_IS_FLAT(str), "a"));

    EVAL("'bc';", v.address());
    str = JSVAL_TO_STRING(v);
    CHECK(JS_StringHasBeenInterned(cx, str));
    CHECK(JS_FlatStringEqualsAscii(JS_ASSERT_STRING_IS_FLAT(str), "bc"));

    return true;
}
END_TEST(testIntString_bug515273)
