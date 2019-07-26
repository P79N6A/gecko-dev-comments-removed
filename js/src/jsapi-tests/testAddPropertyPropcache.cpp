






#include "jsapi-tests/tests.h"

static int callCount = 0;

static bool
AddProperty(JSContext *cx, JS::HandleObject obj, JS::HandleId id, JS::MutableHandleValue vp)
{
    callCount++;
    return true;
}

static const JSClass AddPropertyClass = {
    "AddPropertyTester",
    0,
    AddProperty,
    JS_DeletePropertyStub,   
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub
};

BEGIN_TEST(testAddPropertyHook)
{
    



    static const int ExpectedCount = 100;

    JS::RootedObject obj(cx, JS_NewObject(cx, nullptr, JS::NullPtr(), JS::NullPtr()));
    CHECK(obj);
    JS::RootedValue proto(cx, OBJECT_TO_JSVAL(obj));
    JS_InitClass(cx, global, obj, &AddPropertyClass, nullptr, 0, nullptr, nullptr, nullptr,
                 nullptr);

    obj = JS_NewArrayObject(cx, 0);
    CHECK(obj);
    JS::RootedValue arr(cx, OBJECT_TO_JSVAL(obj));

    CHECK(JS_DefineProperty(cx, global, "arr", arr, JSPROP_ENUMERATE,
                            JS_PropertyStub, JS_StrictPropertyStub));

    for (int i = 0; i < ExpectedCount; ++i) {
        obj = JS_NewObject(cx, &AddPropertyClass, JS::NullPtr(), JS::NullPtr());
        CHECK(obj);
        JS::RootedValue vobj(cx, OBJECT_TO_JSVAL(obj));
        JS::RootedObject arrObj(cx, arr.toObjectOrNull());
        CHECK(JS_DefineElement(cx, arrObj, i, vobj,
                               JS_PropertyStub, JS_StrictPropertyStub,
                               JSPROP_ENUMERATE));
    }

    
    
    EXEC("'use strict';                                     \n"
         "for (var i = 0; i < arr.length; ++i)              \n"
         "  arr[i].prop = 42;                               \n"
         );

    CHECK(callCount == ExpectedCount);

    return true;
}
END_TEST(testAddPropertyHook)

