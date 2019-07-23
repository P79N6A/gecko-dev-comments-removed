#include "tests.h"

BEGIN_TEST(testSameValue)
{
    jsvalRoot v1(cx);
    jsvalRoot v2(cx);

    






    CHECK(JS_NewDoubleValue(cx, 0.0, v1.addr()));
    CHECK(JSVAL_IS_DOUBLE(v1));
    CHECK(JS_NewNumberValue(cx, -0.0, v2.addr()));
    CHECK(!JS_SameValue(cx, v1, v2));
    return true;
}
END_TEST(testSameValue)
