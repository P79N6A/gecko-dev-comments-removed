



#include "jsapi-tests/tests.h"

BEGIN_TEST(testJSEvaluateScript)
{
    JS::RootedObject obj(cx, JS_NewObject(cx, NULL, NULL, global));
    CHECK(obj);

    uint32_t options = JS_GetOptions(cx);
    CHECK(options & JSOPTION_VAROBJFIX);

    static const char src[] = "var x = 5;";

    JS::RootedValue retval(cx);
    CHECK(JS_EvaluateScript(cx, obj, src, sizeof(src) - 1, __FILE__, __LINE__,
                            retval.address()));

    bool hasProp = true;
    CHECK(JS_AlreadyHasOwnProperty(cx, obj, "x", &hasProp));
    CHECK(!hasProp);

    hasProp = false;
    CHECK(JS_HasProperty(cx, global, "x", &hasProp));
    CHECK(hasProp);

    
    JS_SetOptions(cx, options & ~JSOPTION_VAROBJFIX);

    static const char src2[] = "var y = 5;";

    CHECK(JS_EvaluateScript(cx, obj, src2, sizeof(src2) - 1, __FILE__, __LINE__,
                            retval.address()));

    hasProp = false;
    CHECK(JS_AlreadyHasOwnProperty(cx, obj, "y", &hasProp));
    CHECK(hasProp);

    hasProp = true;
    CHECK(JS_AlreadyHasOwnProperty(cx, global, "y", &hasProp));
    CHECK(!hasProp);

    return true;
}
END_TEST(testJSEvaluateScript)


