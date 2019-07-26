






#include "jsapi-tests/tests.h"

BEGIN_TEST(testGCExactRooting)
{
    JS::RootedObject rootCx(cx, JS_NewObject(cx, nullptr, nullptr, nullptr));
    JS::RootedObject rootRt(cx->runtime(), JS_NewObject(cx, nullptr, nullptr, nullptr));

    JS_GC(cx->runtime());

    
    JS_DefineProperty(cx, rootCx, "foo", JS::DoubleValue(0), nullptr, nullptr, 0);
    JS_DefineProperty(cx, rootRt, "foo", JS::DoubleValue(0), nullptr, nullptr, 0);

    return true;
}
END_TEST(testGCExactRooting)
