



#include "jsapi.h"

#include "jsapi-tests/tests.h"

BEGIN_TEST(testNewTargetInvokeConstructor)
{
    JS::RootedValue func(cx);

    EVAL("(function(expected) { if (expected !== new.target) throw new Error('whoops'); })",
         &func);

    JS::AutoValueArray<1> args(cx);
    args[0].set(func);

    JS::RootedValue rval(cx);

    CHECK(JS::Construct(cx, func, args, &rval));

    return true;
}
END_TEST(testNewTargetInvokeConstructor)
