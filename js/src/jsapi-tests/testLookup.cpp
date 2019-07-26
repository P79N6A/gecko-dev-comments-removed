







#include "tests.h"
#include "jsfun.h"  

#include "jsobjinlines.h"

BEGIN_TEST(testLookup_bug522590)
{
    
    js::RootedValue x(cx);
    EXEC("function mkobj() { return {f: function () {return 2;}} }");

    
    EVAL("mkobj().f !== mkobj().f", x.address());
    CHECK_SAME(x, JSVAL_TRUE);

    
    EVAL("mkobj()", x.address());
    js::RootedObject xobj(cx, JSVAL_TO_OBJECT(x));

    
    js::RootedValue r(cx);
    CHECK(JS_LookupProperty(cx, xobj, "f", r.address()));
    CHECK(r.isObject());
    JSObject *funobj = &r.toObject();
    CHECK(funobj->isFunction());
    CHECK(!js::IsInternalFunctionObject(funobj));

    return true;
}
END_TEST(testLookup_bug522590)

JSBool
document_resolve(JSContext *cx, JSHandleObject obj, JSHandleId id, unsigned flags,
                 JSMutableHandleObject objp)
{
    
    js::RootedValue v(cx);
    if (!JS_IdToValue(cx, id, v.address()))
        return false;
    if (JSVAL_IS_STRING(v)) {
        JSString *str = JSVAL_TO_STRING(v);
        JSFlatString *flatStr = JS_FlattenString(cx, str);
        if (!flatStr)
            return false;
        if (JS_FlatStringEqualsAscii(flatStr, "all") && !(flags & JSRESOLVE_DETECTING)) {
            JSBool ok = JS_DefinePropertyById(cx, obj, id, JSVAL_TRUE, NULL, NULL, 0);
            objp.set(ok ? obj.get() : NULL);
            return ok;
        }
    }
    objp.set(NULL);
    return true;
}

static JSClass document_class = {
    "document", JSCLASS_NEW_RESOLVE,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, (JSResolveOp) document_resolve, JS_ConvertStub
};

BEGIN_TEST(testLookup_bug570195)
{
    js::RootedObject obj(cx, JS_NewObject(cx, &document_class, NULL, NULL));
    CHECK(obj);
    CHECK(JS_DefineProperty(cx, global, "document", OBJECT_TO_JSVAL(obj), NULL, NULL, 0));
    js::RootedValue v(cx);
    EVAL("document.all ? true : false", v.address());
    CHECK_SAME(v, JSVAL_FALSE);
    EVAL("document.hasOwnProperty('all')", v.address());
    CHECK_SAME(v, JSVAL_FALSE);
    return true;
}
END_TEST(testLookup_bug570195)
