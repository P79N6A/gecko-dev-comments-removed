



#include "tests.h"

BEGIN_TEST(testObjectIsRegExp)
{
    JS::RootedValue val(cx);

    EVAL("new Object", val.address());
    JS::RootedObject obj(cx, JSVAL_TO_OBJECT(val));
    CHECK(!JS_ObjectIsRegExp(cx, obj));

    EVAL("/foopy/", val.address());
    obj = JSVAL_TO_OBJECT(val);
    CHECK(JS_ObjectIsRegExp(cx, obj));

    return true;
}
END_TEST(testObjectIsRegExp)

BEGIN_TEST(testGetRegExpFlags)
{
    JS::RootedValue val(cx);
    JS::RootedObject obj(cx);

    EVAL("/foopy/", val.address());
    obj = JSVAL_TO_OBJECT(val);
    CHECK_EQUAL(JS_GetRegExpFlags(cx, obj), 0);

    EVAL("/foopy/g", val.address());
    obj = JSVAL_TO_OBJECT(val);
    CHECK_EQUAL(JS_GetRegExpFlags(cx, obj), JSREG_GLOB);

    EVAL("/foopy/gi", val.address());
    obj = JSVAL_TO_OBJECT(val);
    CHECK_EQUAL(JS_GetRegExpFlags(cx, obj), (JSREG_FOLD | JSREG_GLOB));

    return true;
}
END_TEST(testGetRegExpFlags)

BEGIN_TEST(testGetRegExpSource)
{
    JS::RootedValue val(cx);
    JS::RootedObject obj(cx);

    EVAL("/foopy/", val.address());
    obj = JSVAL_TO_OBJECT(val);
    JSString *source = JS_GetRegExpSource(cx, obj);
    CHECK(JS_FlatStringEqualsAscii(JS_ASSERT_STRING_IS_FLAT(source), "foopy"));

    return true;
}
END_TEST(testGetRegExpSource)
