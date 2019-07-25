






#include "tests.h"
#include "jsobj.h"
#include "vm/GlobalObject.h"

#include "jsobjinlines.h"

static JSBool
my_Equality(JSContext *cx, JSObject *obj, const jsval *, JSBool *bp)
{
    *bp = JS_TRUE;
    return JS_TRUE;
}

js::Class TestExtendedEq_JSClass = {
    "TestExtendedEq",
    0,
    JS_PropertyStub,       
    JS_PropertyStub,       
    JS_PropertyStub,       
    JS_StrictPropertyStub, 
    JS_EnumerateStub,
    JS_ResolveStub,
    NULL,                  
    NULL,                  
    NULL,                  
    NULL,                  
    NULL,                  
    NULL,                  
    NULL,                  
    NULL,                  
    NULL,                  
    {
        my_Equality,
        NULL,              
        NULL,              
        NULL,              
        NULL,              
    }
};

BEGIN_TEST(testExtendedEq_bug530489)
{
    JSClass *clasp = (JSClass *) &TestExtendedEq_JSClass;

    CHECK(JS_InitClass(cx, global, global, clasp, NULL, 0, NULL, NULL, NULL, NULL));

    CHECK(JS_DefineObject(cx, global, "obj1", clasp, NULL, 0));
    CHECK(JS_DefineObject(cx, global, "obj2", clasp, NULL, 0));

    jsval v;
    EVAL("(function() { var r; for (var i = 0; i < 10; ++i) r = obj1 == obj2; return r; })()", &v);
    CHECK_SAME(v, JSVAL_TRUE);
    return true;
}
END_TEST(testExtendedEq_bug530489)
