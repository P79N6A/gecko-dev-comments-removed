



#include "tests.h"
#include "jsxdrapi.h"



static const int expectedCount = 100;
static int callCount = 0;

static JSBool
addProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
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
    jsvalRoot proto(cx);
    JSObject *obj = JS_NewObject(cx, NULL, NULL, NULL);
    CHECK(obj);
    proto = OBJECT_TO_JSVAL(obj);
    JS_InitClass(cx, global, obj, &addPropertyClass, NULL, 0, NULL, NULL, NULL,
                 NULL);

    jsvalRoot arr(cx);
    obj = JS_NewArrayObject(cx, 0, NULL);
    CHECK(obj);
    arr = OBJECT_TO_JSVAL(obj);
        
    CHECK(JS_DefineProperty(cx, global, "arr", arr,
                            JS_PropertyStub, JS_StrictPropertyStub,
                            JSPROP_ENUMERATE));

    for (int i = 0; i < expectedCount; ++i) {
        jsvalRoot vobj(cx);
        obj = JS_NewObject(cx, &addPropertyClass, NULL, NULL);
        CHECK(obj);
        vobj = OBJECT_TO_JSVAL(obj);
        CHECK(JS_DefineElement(cx, JSVAL_TO_OBJECT(arr), i, vobj,
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

