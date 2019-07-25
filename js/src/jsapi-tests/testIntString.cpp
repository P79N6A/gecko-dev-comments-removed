



#include "tests.h"
#include "vm/String.h"

BEGIN_TEST(testIntString_bug515273)
{
    jsvalRoot v(cx);

    EVAL("'1';", v.addr());
    JSString *str = JSVAL_TO_STRING(v.value());
    CHECK(str->isStaticAtom());
    CHECK(JS_FlatStringEqualsAscii(JS_ASSERT_STRING_IS_FLAT(str), "1"));

    EVAL("'42';", v.addr());
    str = JSVAL_TO_STRING(v.value());
    CHECK(str->isStaticAtom());
    CHECK(JS_FlatStringEqualsAscii(JS_ASSERT_STRING_IS_FLAT(str), "42"));

    EVAL("'111';", v.addr());
    str = JSVAL_TO_STRING(v.value());
    CHECK(str->isStaticAtom());
    CHECK(JS_FlatStringEqualsAscii(JS_ASSERT_STRING_IS_FLAT(str), "111"));

    
    EVAL("'a';", v.addr());
    str = JSVAL_TO_STRING(v.value());
    CHECK(str->isStaticAtom());
    CHECK(JS_FlatStringEqualsAscii(JS_ASSERT_STRING_IS_FLAT(str), "a"));

    EVAL("'bc';", v.addr());
    str = JSVAL_TO_STRING(v.value());
    CHECK(str->isStaticAtom());
    CHECK(JS_FlatStringEqualsAscii(JS_ASSERT_STRING_IS_FLAT(str), "bc"));

    return true;
}
END_TEST(testIntString_bug515273)
