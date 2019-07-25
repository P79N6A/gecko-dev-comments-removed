







#include "tests.h"

BEGIN_TEST(selfTest_NaNsAreSame)
{
    JS::RootedValue v1(cx), v2(cx);
    EVAL("0/0", v1.address());  
    CHECK_SAME(v1, v1);

    EVAL("Math.sin('no')", v2.address());  
    CHECK_SAME(v1, v2);
    return true;
}
END_TEST(selfTest_NaNsAreSame)

BEGIN_TEST(selfTest_globalHasNoParent)
{
    CHECK(JS_GetParent(global) == NULL);
    return true;
}
END_TEST(selfTest_globalHasNoParent)
