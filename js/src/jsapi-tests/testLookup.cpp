






#include "jsfun.h"  

#include "jsapi-tests/tests.h"

#include "jsobjinlines.h"

BEGIN_TEST(testLookup_bug522590)
{
    
    JS::RootedValue x(cx);
    EXEC("function mkobj() { return {f: function () {return 2;}} }");

    
    EVAL("mkobj().f !== mkobj().f", &x);
    CHECK_SAME(x, JSVAL_TRUE);

    
    EVAL("mkobj()", &x);
    JS::RootedObject xobj(cx, x.toObjectOrNull());

    
    JS::RootedValue r(cx);
    CHECK(JS_LookupProperty(cx, xobj, "f", &r));
    CHECK(r.isObject());
    JSObject *funobj = &r.toObject();
    CHECK(funobj->is<JSFunction>());
    CHECK(!js::IsInternalFunctionObject(funobj));

    return true;
}
END_TEST(testLookup_bug522590)

static const JSClass DocumentAllClass = {
    "DocumentAll",
    JSCLASS_EMULATES_UNDEFINED,
    JS_PropertyStub,
    JS_DeletePropertyStub,
    JS_PropertyStub,
    JS_StrictPropertyStub,
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub
};

bool
document_resolve(JSContext *cx, JS::HandleObject obj, JS::HandleId id, bool *resolvedp)
{
    
    JS::RootedValue v(cx);
    if (!JS_IdToValue(cx, id, &v))
        return false;

    if (v.isString()) {
        JSString *str = v.toString();
        JSFlatString *flatStr = JS_FlattenString(cx, str);
        if (!flatStr)
            return false;
        if (JS_FlatStringEqualsAscii(flatStr, "all")) {
            JS::Rooted<JSObject*> docAll(cx,
                                         JS_NewObject(cx, &DocumentAllClass, JS::NullPtr(), JS::NullPtr()));
            if (!docAll)
                return false;

            JS::Rooted<JS::Value> allValue(cx, JS::ObjectValue(*docAll));
            if (!JS_DefinePropertyById(cx, obj, id, allValue, 0))
                return false;

            *resolvedp = true;
            return true;
        }
    }

    *resolvedp = false;
    return true;
}

static const JSClass document_class = {
    "document", 0,
    JS_PropertyStub, JS_DeletePropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, document_resolve, JS_ConvertStub
};

BEGIN_TEST(testLookup_bug570195)
{
    JS::RootedObject obj(cx, JS_NewObject(cx, &document_class, JS::NullPtr(), JS::NullPtr()));
    CHECK(obj);
    CHECK(JS_DefineProperty(cx, global, "document", obj, 0));
    JS::RootedValue v(cx);
    EVAL("document.all ? true : false", &v);
    CHECK_SAME(v, JSVAL_FALSE);
    EVAL("document.hasOwnProperty('all')", &v);
    CHECK_SAME(v, JSVAL_TRUE);
    return true;
}
END_TEST(testLookup_bug570195)
