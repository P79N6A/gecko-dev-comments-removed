



#include "tests.h"

BEGIN_TEST(testDeepFreeze_bug535703)
{
    JSObject *obj = JS_NewObject(cx, NULL, NULL, NULL);
    CHECK(obj);
    JS_DeepFreezeObject(cx, obj);  
    return true;
}
END_TEST(testDeepFreeze_bug535703)
