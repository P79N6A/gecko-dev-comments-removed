



#include "js/Class.h"
#include "jsapi-tests/tests.h"

int count = 0;

static bool
IterNext(JSContext *cx, unsigned argc, jsval *vp)
{
    if (count++ == 100)
        return JS_ThrowStopIteration(cx);
    JS_SET_RVAL(cx, vp, INT_TO_JSVAL(count));
    return true;
}

static JSObject *
IterHook(JSContext *cx, JS::HandleObject obj, bool keysonly)
{
    JS::RootedObject iterObj(cx, JS_NewObject(cx, nullptr, nullptr, nullptr));
    if (!iterObj)
        return nullptr;
    if (!JS_DefineFunction(cx, iterObj, "next", IterNext, 0, 0))
        return nullptr;
    return iterObj;
}

const js::Class HasCustomIterClass = {
    "HasCustomIter",
    0,
    JS_PropertyStub,
    JS_DeletePropertyStub,
    JS_PropertyStub,
    JS_StrictPropertyStub,
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    nullptr,
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    {
        nullptr,     
        nullptr,     
        IterHook,
        false        
    }
};

bool
IterClassConstructor(JSContext *cx, unsigned argc, jsval *vp)
{
    JSObject *obj = JS_NewObjectForConstructor(cx, Jsvalify(&HasCustomIterClass), vp);
    if (!obj)
        return false;
    JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(obj));
    return true;
}

BEGIN_TEST(testCustomIterator_bug612523)
{
    CHECK(JS_InitClass(cx, global, nullptr, Jsvalify(&HasCustomIterClass),
                       IterClassConstructor, 0, nullptr, nullptr, nullptr, nullptr));

    JS::RootedValue result(cx);
    EVAL("var o = new HasCustomIter(); \n"
         "var j = 0; \n"
         "for (var i in o) { ++j; }; \n"
         "j;", result.address());

    CHECK(JSVAL_IS_INT(result));
    CHECK_EQUAL(JSVAL_TO_INT(result), 100);
    CHECK_EQUAL(count, 101);

    return true;
}
END_TEST(testCustomIterator_bug612523)
