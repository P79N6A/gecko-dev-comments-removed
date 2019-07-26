







#include "tests.h"



static const int expectedCount = 100;
static int callCount = 0;

static JSBool
addProperty(JSContext *cx, JS::HandleObject obj, JS::HandleId id, JS::MutableHandleValue vp)
{
  callCount++;
  return true;
}

JSClass addPropertyClass = {
    "AddPropertyTester",
    0,
    addProperty,
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub
};

BEGIN_TEST(testAddPropertyHook)
{
    js::RootedObject obj(cx, JS_NewObject(cx, NULL, NULL, NULL));
    CHECK(obj);
    js::RootedValue proto(cx, OBJECT_TO_JSVAL(obj));
    JS_InitClass(cx, global, obj, &addPropertyClass, NULL, 0, NULL, NULL, NULL,
                 NULL);

    obj = JS_NewArrayObject(cx, 0, NULL);
    CHECK(obj);
    js::RootedValue arr(cx, OBJECT_TO_JSVAL(obj));

    CHECK(JS_DefineProperty(cx, global, "arr", arr,
                            JS_PropertyStub, JS_StrictPropertyStub,
                            JSPROP_ENUMERATE));

    for (int i = 0; i < expectedCount; ++i) {
        obj = JS_NewObject(cx, &addPropertyClass, NULL, NULL);
        CHECK(obj);
        js::RootedValue vobj(cx, OBJECT_TO_JSVAL(obj));
        js::RootedObject arrObj(cx, JSVAL_TO_OBJECT(arr));
        CHECK(JS_DefineElement(cx, arrObj, i, vobj,
                               JS_PropertyStub, JS_StrictPropertyStub,
                               JSPROP_ENUMERATE));
    }

    
    
    EXEC("'use strict';                                     \n"
         "for (var i = 0; i < arr.length; ++i)              \n"
         "  arr[i].prop = 42;                               \n"
         );

    CHECK(callCount == expectedCount);

    return true;
}
END_TEST(testAddPropertyHook)

