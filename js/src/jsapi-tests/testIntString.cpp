






#include "jsapi-tests/tests.h"

BEGIN_TEST(testIntString_bug515273)
{
    JS::RootedValue v(cx);

    EVAL("'1';", &v);
    JSString* str = v.toString();
    CHECK(JS_StringHasBeenPinned(cx, str));
    CHECK(JS_FlatStringEqualsAscii(JS_ASSERT_STRING_IS_FLAT(str), "1"));

    EVAL("'42';", &v);
    str = v.toString();
    CHECK(JS_StringHasBeenPinned(cx, str));
    CHECK(JS_FlatStringEqualsAscii(JS_ASSERT_STRING_IS_FLAT(str), "42"));

    EVAL("'111';", &v);
    str = v.toString();
    CHECK(JS_StringHasBeenPinned(cx, str));
    CHECK(JS_FlatStringEqualsAscii(JS_ASSERT_STRING_IS_FLAT(str), "111"));

    
    EVAL("'a';", &v);
    str = v.toString();
    CHECK(JS_StringHasBeenPinned(cx, str));
    CHECK(JS_FlatStringEqualsAscii(JS_ASSERT_STRING_IS_FLAT(str), "a"));

    EVAL("'bc';", &v);
    str = v.toString();
    CHECK(JS_StringHasBeenPinned(cx, str));
    CHECK(JS_FlatStringEqualsAscii(JS_ASSERT_STRING_IS_FLAT(str), "bc"));

    return true;
}
END_TEST(testIntString_bug515273)
