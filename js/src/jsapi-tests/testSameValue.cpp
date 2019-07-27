






#include "jsapi-tests/tests.h"

BEGIN_TEST(testSameValue)
{

    






    JS::RootedValue v1(cx, JS::DoubleValue(0.0));
    JS::RootedValue v2(cx, JS::DoubleValue(-0.0));
    bool same;
    CHECK(JS_SameValue(cx, v1, v2, &same));
    CHECK(!same);
    return true;
}
END_TEST(testSameValue)
