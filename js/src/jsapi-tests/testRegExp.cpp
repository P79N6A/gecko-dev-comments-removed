



#include "jsapi-tests/tests.h"

BEGIN_TEST(testObjectIsRegExp)
{
    JS::RootedValue val(cx);

    EVAL("new Object", &val);
    JS::RootedObject obj(cx, val.toObjectOrNull());
    CHECK(!JS_ObjectIsRegExp(cx, obj));

    EVAL("/foopy/", &val);
    obj = val.toObjectOrNull();
    CHECK(JS_ObjectIsRegExp(cx, obj));

    return true;
}
END_TEST(testObjectIsRegExp)

BEGIN_TEST(testGetRegExpFlags)
{
    JS::RootedValue val(cx);
    JS::RootedObject obj(cx);

    EVAL("/foopy/", &val);
    obj = val.toObjectOrNull();
    CHECK_EQUAL(JS_GetRegExpFlags(cx, obj), 0u);

    EVAL("/foopy/g", &val);
    obj = val.toObjectOrNull();
    CHECK_EQUAL(JS_GetRegExpFlags(cx, obj), JSREG_GLOB);

    EVAL("/foopy/gi", &val);
    obj = val.toObjectOrNull();
    CHECK_EQUAL(JS_GetRegExpFlags(cx, obj), (JSREG_FOLD | JSREG_GLOB));

    return true;
}
END_TEST(testGetRegExpFlags)

BEGIN_TEST(testGetRegExpSource)
{
    JS::RootedValue val(cx);
    JS::RootedObject obj(cx);

    EVAL("/foopy/", &val);
    obj = val.toObjectOrNull();
    JSString *source = JS_GetRegExpSource(cx, obj);
    CHECK(JS_FlatStringEqualsAscii(JS_ASSERT_STRING_IS_FLAT(source), "foopy"));

    return true;
}
END_TEST(testGetRegExpSource)
