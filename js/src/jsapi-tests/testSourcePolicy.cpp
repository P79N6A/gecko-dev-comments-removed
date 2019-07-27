



#include "jsscript.h"

#include "jsapi-tests/tests.h"

BEGIN_TEST(testBug795104)
{
    JS::CompileOptions opts(cx);
    JS::CompartmentOptionsRef(cx->compartment()).setDiscardSource(true);
    const size_t strLen = 60002;
    char* s = static_cast<char*>(JS_malloc(cx, strLen));
    CHECK(s);
    s[0] = '"';
    memset(s + 1, 'x', strLen - 2);
    s[strLen - 1] = '"';
    
    opts.setNoScriptRval(true);
    JS::RootedValue unused(cx);
    CHECK(JS::Evaluate(cx, opts, s, strLen, &unused));
    JS::RootedFunction fun(cx);
    JS::AutoObjectVector emptyScopeChain(cx);
    
    
    opts.setNoScriptRval(false);
    CHECK(JS::CompileFunction(cx, emptyScopeChain, opts, "f", 0, nullptr, s, strLen, &fun));
    CHECK(fun);
    JS_free(cx, s);

    return true;
}
END_TEST(testBug795104)
