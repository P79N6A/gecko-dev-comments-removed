






#include "jsapi-tests/tests.h"

#ifdef JSGC_GENERATIONAL

BEGIN_TEST(testIsInsideNursery)
{
    
    CHECK(!rt->gc.nursery.isInside(rt));
    CHECK(!rt->gc.nursery.isInside((void *)nullptr));

    JS_GC(rt);

    JS::RootedObject object(cx, JS_NewObject(cx, nullptr, JS::NullPtr(), JS::NullPtr()));

    
    CHECK(js::gc::IsInsideNursery(object));

    JS_GC(rt);

    
    CHECK(!js::gc::IsInsideNursery(object));

    return true;
}
END_TEST(testIsInsideNursery)

#endif
