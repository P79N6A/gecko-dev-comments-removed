






#include <math.h>

#include "js/Conversions.h"

#include "jsapi-tests/tests.h"

using JS::detail::ToIntWidth;
using JS::detail::ToUintWidth;

BEGIN_TEST(testToUint8TwiceUint8Range)
{
    double d = -256;
    uint8_t expected = 0;
    do {
        CHECK(ToUintWidth<uint8_t>(d) == expected);

        d++;
        expected++;
    } while (d <= 256);
    return true;
}
END_TEST(testToUint8TwiceUint8Range)

BEGIN_TEST(testToInt8)
{
    double d = -128;
    int8_t expected = -128;
    do {
        CHECK(ToIntWidth<int8_t>(d) == expected);

        d++;
        expected++;
    } while (expected < 127);
    return true;
}
END_TEST(testToInt8)

BEGIN_TEST(testToUint32Large)
{
    CHECK(ToUintWidth<uint32_t>(pow(2.0, 83)) == 0);
    CHECK(ToUintWidth<uint32_t>(pow(2.0, 83) + pow(2.0, 31)) == (1U << 31));
    CHECK(ToUintWidth<uint32_t>(pow(2.0, 83) + 2 * pow(2.0, 31)) == 0);
    CHECK(ToUintWidth<uint32_t>(pow(2.0, 83) + 3 * pow(2.0, 31)) == (1U << 31));
    CHECK(ToUintWidth<uint32_t>(pow(2.0, 84)) == 0);
    CHECK(ToUintWidth<uint32_t>(pow(2.0, 84) + pow(2.0, 31)) == 0);
    CHECK(ToUintWidth<uint32_t>(pow(2.0, 84) + pow(2.0, 32)) == 0);
    return true;
}
END_TEST(testToUint32Large)

BEGIN_TEST(testToUint64Large)
{
    CHECK(ToUintWidth<uint64_t>(pow(2.0, 115)) == 0);
    CHECK(ToUintWidth<uint64_t>(pow(2.0, 115) + pow(2.0, 63)) == (1ULL << 63));
    CHECK(ToUintWidth<uint64_t>(pow(2.0, 115) + 2 * pow(2.0, 63)) == 0);
    CHECK(ToUintWidth<uint64_t>(pow(2.0, 115) + 3 * pow(2.0, 63)) == (1ULL << 63));
    CHECK(ToUintWidth<uint64_t>(pow(2.0, 116)) == 0);
    CHECK(ToUintWidth<uint64_t>(pow(2.0, 116) + pow(2.0, 63)) == 0);
    CHECK(ToUintWidth<uint64_t>(pow(2.0, 116) + pow(2.0, 64)) == 0);
    return true;
}
END_TEST(testToUint64Large)
