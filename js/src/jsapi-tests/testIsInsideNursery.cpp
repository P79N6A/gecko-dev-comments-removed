






#include "jsapi-tests/tests.h"

BEGIN_TEST(testIsInsideNursery)
{
    
    CHECK(!js::gc::IsInsideNursery(rt, rt));
    CHECK(!js::gc::IsInsideNursery(rt, nullptr));
    CHECK(!js::gc::IsInsideNursery(nullptr));

    JS_GC(rt);

    JS::RootedObject object(cx, JS_NewObject(cx, nullptr, JS::NullPtr(), JS::NullPtr()));

#ifdef JSGC_GENERATIONAL
    
    CHECK(js::gc::IsInsideNursery(rt, object));
    CHECK(js::gc::IsInsideNursery(object));
#else
    CHECK(!js::gc::IsInsideNursery(rt, object));
    CHECK(!js::gc::IsInsideNursery(object));
#endif

    JS_GC(rt);

    CHECK(!js::gc::IsInsideNursery(rt, object));
    CHECK(!js::gc::IsInsideNursery(object));

    return true;
}
END_TEST(testIsInsideNursery)
