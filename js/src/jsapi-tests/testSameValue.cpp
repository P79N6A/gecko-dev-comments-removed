#include "tests.h"

BEGIN_TEST(testSameValue)
{

    






    jsval v1 = DOUBLE_TO_JSVAL(0.0);
    jsval v2 = DOUBLE_TO_JSVAL(-0.0);
    CHECK(!JS_SameValue(cx, v1, v2));
    return true;
}
END_TEST(testSameValue)
