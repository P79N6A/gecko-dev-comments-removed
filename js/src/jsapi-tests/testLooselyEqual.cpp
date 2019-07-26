



#include "tests.h"
#include <limits>
#include <math.h>

using namespace std;

struct LooseEqualityFixture : public JSAPITest
{
    virtual ~LooseEqualityFixture() {}

    bool leq(JS::HandleValue x, JS::HandleValue y) {
        JSBool equal;
        CHECK(JS_LooselyEqual(cx, x, y, &equal) && equal);
        CHECK(JS_LooselyEqual(cx, y, x, &equal) && equal);
        return true;
    }

    bool nleq(JS::HandleValue x, JS::HandleValue y) {
        JSBool equal;
        CHECK(JS_LooselyEqual(cx, x, y, &equal) && !equal);
        CHECK(JS_LooselyEqual(cx, y, x, &equal) && !equal);
        return true;
    }
};

struct LooseEqualityData
{
    JS::RootedValue qNaN;
    JS::RootedValue sNaN;
    JS::RootedValue d42;
    JS::RootedValue i42;
    JS::RootedValue undef;
    JS::RootedValue null;
    JS::RootedValue obj;
    JS::RootedValue poszero;
    JS::RootedValue negzero;

    LooseEqualityData(JSContext *cx)
      : qNaN(cx),
        sNaN(cx),
        d42(cx),
        i42(cx),
        undef(cx),
        null(cx),
        obj(cx),
        poszero(cx),
        negzero(cx)
    {
        qNaN = DOUBLE_TO_JSVAL(numeric_limits<double>::quiet_NaN());
        sNaN = DOUBLE_TO_JSVAL(numeric_limits<double>::signaling_NaN());
        d42 = DOUBLE_TO_JSVAL(42.0);
        i42 = INT_TO_JSVAL(42);
        undef = JSVAL_VOID;
        null = JSVAL_NULL;
        obj = OBJECT_TO_JSVAL(JS_GetGlobalObject(cx));
        poszero = DOUBLE_TO_JSVAL(0.0);
        negzero = DOUBLE_TO_JSVAL(-0.0);
#ifdef XP_WIN
# define copysign _copysign
#endif
        JS_ASSERT(copysign(1.0, JSVAL_TO_DOUBLE(poszero)) == 1.0);
        JS_ASSERT(copysign(1.0, JSVAL_TO_DOUBLE(negzero)) == -1.0);
#ifdef XP_WIN
# undef copysign
#endif
    }
};


BEGIN_FIXTURE_TEST(LooseEqualityFixture, test_undef_leq_undef)
{
    LooseEqualityData d(cx);
    CHECK(leq(d.undef, d.undef));
    return true;
}
END_FIXTURE_TEST(LooseEqualityFixture, test_undef_leq_undef)


BEGIN_FIXTURE_TEST(LooseEqualityFixture, test_null_leq_null)
{
    LooseEqualityData d(cx);
    CHECK(leq(d.null, d.null));
    return true;
}
END_FIXTURE_TEST(LooseEqualityFixture, test_null_leq_null)


BEGIN_FIXTURE_TEST(LooseEqualityFixture, test_nan_nleq_all)
{
    LooseEqualityData d(cx);

    CHECK(nleq(d.qNaN, d.qNaN));
    CHECK(nleq(d.qNaN, d.sNaN));

    CHECK(nleq(d.sNaN, d.sNaN));
    CHECK(nleq(d.sNaN, d.qNaN));

    CHECK(nleq(d.qNaN, d.d42));
    CHECK(nleq(d.qNaN, d.i42));
    CHECK(nleq(d.qNaN, d.undef));
    CHECK(nleq(d.qNaN, d.null));
    CHECK(nleq(d.qNaN, d.obj));

    CHECK(nleq(d.sNaN, d.d42));
    CHECK(nleq(d.sNaN, d.i42));
    CHECK(nleq(d.sNaN, d.undef));
    CHECK(nleq(d.sNaN, d.null));
    CHECK(nleq(d.sNaN, d.obj));
    return true;
}
END_FIXTURE_TEST(LooseEqualityFixture, test_nan_nleq_all)


BEGIN_FIXTURE_TEST(LooseEqualityFixture, test_all_nleq_nan)
{
    LooseEqualityData d(cx);

    CHECK(nleq(d.qNaN, d.qNaN));
    CHECK(nleq(d.qNaN, d.sNaN));

    CHECK(nleq(d.sNaN, d.sNaN));
    CHECK(nleq(d.sNaN, d.qNaN));

    CHECK(nleq(d.d42,   d.qNaN));
    CHECK(nleq(d.i42,   d.qNaN));
    CHECK(nleq(d.undef, d.qNaN));
    CHECK(nleq(d.null,  d.qNaN));
    CHECK(nleq(d.obj,   d.qNaN));

    CHECK(nleq(d.d42,   d.sNaN));
    CHECK(nleq(d.i42,   d.sNaN));
    CHECK(nleq(d.undef, d.sNaN));
    CHECK(nleq(d.null,  d.sNaN));
    CHECK(nleq(d.obj,   d.sNaN));
    return true;
}
END_FIXTURE_TEST(LooseEqualityFixture, test_all_nleq_nan)


BEGIN_FIXTURE_TEST(LooseEqualityFixture, test_leq_same_nums)
{
    LooseEqualityData d(cx);

    CHECK(leq(d.d42, d.d42));
    CHECK(leq(d.i42, d.i42));
    CHECK(leq(d.d42, d.i42));
    CHECK(leq(d.i42, d.d42));
    return true;
}
END_FIXTURE_TEST(LooseEqualityFixture, test_leq_same_nums)


BEGIN_FIXTURE_TEST(LooseEqualityFixture, test_pz_leq_nz)
{
    LooseEqualityData d(cx);
    CHECK(leq(d.poszero, d.negzero));
    return true;
}
END_FIXTURE_TEST(LooseEqualityFixture, test_pz_leq_nz)


BEGIN_FIXTURE_TEST(LooseEqualityFixture, test_nz_leq_pz)
{
    LooseEqualityData d(cx);
    CHECK(leq(d.negzero, d.poszero));
    return true;
}
END_FIXTURE_TEST(LooseEqualityFixture, test_nz_leq_pz)




BEGIN_FIXTURE_TEST(LooseEqualityFixture, test_null_leq_undef)
{
    LooseEqualityData d(cx);
    CHECK(leq(d.null, d.undef));
    return true;
}
END_FIXTURE_TEST(LooseEqualityFixture, test_null_leq_undef)


BEGIN_FIXTURE_TEST(LooseEqualityFixture, test_undef_leq_null)
{
    LooseEqualityData d(cx);
    CHECK(leq(d.undef, d.null));
    return true;
}
END_FIXTURE_TEST(LooseEqualityFixture, test_undef_leq_null)


