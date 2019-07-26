






#include "jsapi-tests/tests.h"

BEGIN_TEST(testGCExactRooting)
{
    JS::RootedObject rootCx(cx, JS_NewObject(cx, NULL, NULL, NULL));
    JS::RootedObject rootRt(cx->runtime(), JS_NewObject(cx, NULL, NULL, NULL));

    JS_GC(cx->runtime());

    
    JS_DefineProperty(cx, rootCx, "foo", JS::DoubleValue(0), NULL, NULL, 0);
    JS_DefineProperty(cx, rootRt, "foo", JS::DoubleValue(0), NULL, NULL, 0);

    return true;
}
END_TEST(testGCExactRooting)
