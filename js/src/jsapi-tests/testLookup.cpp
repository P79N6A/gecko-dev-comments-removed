






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
    JS::RootedObject xobj(cx, JSVAL_TO_OBJECT(x));

    
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
document_resolve(JSContext *cx, JS::HandleObject obj, JS::HandleId id, unsigned flags,
                 JS::MutableHandleObject objp)
{
    
    JS::RootedValue v(cx);
    if (!JS_IdToValue(cx, id, &v))
        return false;
    if (JSVAL_IS_STRING(v)) {
        JSString *str = JSVAL_TO_STRING(v);
        JSFlatString *flatStr = JS_FlattenString(cx, str);
        if (!flatStr)
            return false;
        if (JS_FlatStringEqualsAscii(flatStr, "all")) {
            JS::Rooted<JSObject*> docAll(cx,
                                         JS_NewObject(cx, &DocumentAllClass, JS::NullPtr(), JS::NullPtr()));
            if (!docAll)
                return false;
            JS::Rooted<JS::Value> allValue(cx, ObjectValue(*docAll));
            bool ok = JS_DefinePropertyById(cx, obj, id, allValue, nullptr, nullptr, 0);
            objp.set(ok ? obj.get() : nullptr);
            return ok;
        }
    }
    objp.set(nullptr);
    return true;
}

static const JSClass document_class = {
    "document", JSCLASS_NEW_RESOLVE,
    JS_PropertyStub, JS_DeletePropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, (JSResolveOp) document_resolve, JS_ConvertStub
};

BEGIN_TEST(testLookup_bug570195)
{
    JS::RootedObject obj(cx, JS_NewObject(cx, &document_class, JS::NullPtr(), JS::NullPtr()));
    CHECK(obj);
    CHECK(JS_DefineProperty(cx, global, "document", OBJECT_TO_JSVAL(obj), nullptr, nullptr, 0));
    JS::RootedValue v(cx);
    EVAL("document.all ? true : false", &v);
    CHECK_SAME(v, JSVAL_FALSE);
    EVAL("document.hasOwnProperty('all')", &v);
    CHECK_SAME(v, JSVAL_TRUE);
    return true;
}
END_TEST(testLookup_bug570195)
