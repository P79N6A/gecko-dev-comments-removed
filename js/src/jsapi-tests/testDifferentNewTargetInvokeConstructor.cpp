



#include "jsapi.h"

#include "jsapi-tests/tests.h"

BEGIN_TEST(testDifferentNewTargetInvokeConstructor)
{
    JS::RootedValue func(cx);
    JS::RootedValue otherFunc(cx);

    EVAL("(function() { /* This is a different new.target function */ })", &otherFunc);

    EVAL("(function(expected) { if (expected !== new.target) throw new Error('whoops'); })",
         &func);

    JS::AutoValueArray<1> args(cx);
    args[0].set(otherFunc);

    JS::RootedValue rval(cx);

    JS::RootedObject newTarget(cx, &otherFunc.toObject());

    CHECK(JS::Construct(cx, func, newTarget, args, &rval));

    
    JS::RootedValue plain(cx);
    EVAL("({})", &plain);
    args[0].set(plain);
    newTarget = &plain.toObject();
    CHECK(!JS::Construct(cx, func, newTarget, args, &rval));

    return true;
}
END_TEST(testDifferentNewTargetInvokeConstructor)
