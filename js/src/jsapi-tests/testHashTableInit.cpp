



#include "js/HashTable.h"
#include "jsapi-tests/tests.h"

typedef js::HashSet<uint32_t, js::DefaultHasher<uint32_t>, js::SystemAllocPolicy> IntSet;

static const uint32_t MaxAllowedHashInit = 1 << 23;

BEGIN_TEST(testHashInitAlmostTooHuge)
{
    IntSet smallEnough;
    CHECK(smallEnough.init(MaxAllowedHashInit));
    return true;
}
END_TEST(testHashInitAlmostTooHuge)

BEGIN_TEST(testHashInitTooHuge)
{
    IntSet tooBig;
    CHECK(!tooBig.init(MaxAllowedHashInit + 1));
    return true;
}
END_TEST(testHashInitTooHuge)
