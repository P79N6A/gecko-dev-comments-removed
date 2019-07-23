#include "tests.h"
#include "jsfun.h"  

BEGIN_TEST(testLookup_bug522590)
{
    
    jsvalRoot x(cx);
    EXEC("function mkobj() { return {f: function () {return 2;}} }");

    
    EVAL("mkobj().f !== mkobj().f", x.addr());
    CHECK_SAME(x, JSVAL_TRUE);

    
    EVAL("mkobj()", x.addr());
    JSObject *xobj = JSVAL_TO_OBJECT(x);

    
    jsvalRoot r(cx);
    CHECK(JS_LookupProperty(cx, xobj, "f", r.addr()));
    CHECK(JSVAL_IS_OBJECT(r));
    JSObject *funobj = JSVAL_TO_OBJECT(r);
    CHECK(HAS_FUNCTION_CLASS(funobj));
    CHECK(!js_IsInternalFunctionObject(funobj));
    CHECK(GET_FUNCTION_PRIVATE(cx, funobj) != (JSFunction *) funobj);

    return true;
}
END_TEST(testLookup_bug522590)
