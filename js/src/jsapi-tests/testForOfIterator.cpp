






#include "jsapi-tests/tests.h"

#ifdef JS_HAS_SYMBOLS
#define STD_ITERATOR "Symbol.iterator"
#else
#define STD_ITERATOR "'@@iterator'"
#endif

BEGIN_TEST(testForOfIterator_basicNonIterable)
{
    JS::RootedValue v(cx);
    
    
    EVAL("({[" STD_ITERATOR "]: 5})", &v);
    JS::ForOfIterator iter(cx);
    bool ok = iter.init(v);
    CHECK(!ok);
    JS_ClearPendingException(cx);
    return true;
}
END_TEST(testForOfIterator_basicNonIterable)

BEGIN_TEST(testForOfIterator_bug515273_part1)
{
    JS::RootedValue v(cx);

    
    
    EVAL("({[" STD_ITERATOR "]: 5})", &v);

    JS::ForOfIterator iter(cx);
    bool ok = iter.init(v, JS::ForOfIterator::AllowNonIterable);
    CHECK(!ok);
    JS_ClearPendingException(cx);
    return true;
}
END_TEST(testForOfIterator_bug515273_part1)

BEGIN_TEST(testForOfIterator_bug515273_part2)
{
    JS::RootedObject obj(cx,
			 JS_NewObject(cx, nullptr, JS::NullPtr(), JS::NullPtr()));
    CHECK(obj);
    JS::RootedValue v(cx, JS::ObjectValue(*obj));

    JS::ForOfIterator iter(cx);
    bool ok = iter.init(v, JS::ForOfIterator::AllowNonIterable);
    CHECK(ok);
    CHECK(!iter.valueIsIterable());
    return true;
}
END_TEST(testForOfIterator_bug515273_part2)
