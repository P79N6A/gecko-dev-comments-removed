






#include "jsapi-tests/tests.h"

BEGIN_TEST(testIsInsideNursery)
{
    
    CHECK(!rt->gc.nursery.isInside(rt));
    CHECK(!rt->gc.nursery.isInside((void *)nullptr));

    JS_GC(rt);

    JS::RootedObject object(cx, JS_NewPlainObject(cx));

    
    CHECK(js::gc::IsInsideNursery(object));

    JS_GC(rt);

    
    CHECK(!js::gc::IsInsideNursery(object));

    return true;
}
END_TEST(testIsInsideNursery)
