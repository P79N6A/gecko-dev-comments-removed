



#include "tests.h"

BEGIN_TEST(testSeal_bug535703)
{
    JSObject *obj = JS_NewObject(cx, NULL, NULL, NULL);
    CHECK(obj);
    JS_SealObject(cx, obj, JS_TRUE);  
    return true;
}
END_TEST(testSeal_bug535703)
