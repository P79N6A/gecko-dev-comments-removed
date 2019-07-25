



#include "tests.h"

using namespace JS;

static JSClass CustomClass = {
  "CustomClass",
  JSCLASS_HAS_RESERVED_SLOTS(1),
  JS_PropertyStub,
  JS_PropertyStub,
  JS_PropertyStub,
  JS_StrictPropertyStub,
  JS_EnumerateStub,
  JS_ResolveStub,
  JS_ConvertStub
};

static const uint32_t CUSTOM_SLOT = 0;

static JSBool
CustomMethod(JSContext *cx, unsigned argc, Value *vp)
{
  const Value &thisv = JS_THIS_VALUE(cx, vp);
  JSObject *thisObj;
  if (!thisv.isObject() || JS_GetClass((thisObj = &thisv.toObject())) != &CustomClass)
    return JS_CallNonGenericMethodOnProxy(cx, argc, vp, CustomMethod, &CustomClass);

  JS_SET_RVAL(cx, vp, JS_GetReservedSlot(thisObj, CUSTOM_SLOT));
  return true;
}

BEGIN_TEST(test_CallNonGenericMethodOnProxy)
{
  
  JSObject *globalA = JS_NewGlobalObject(cx, getGlobalClass(), NULL);
  CHECK(globalA);

  JSObject *customA = JS_NewObject(cx, &CustomClass, NULL, NULL);
  CHECK(customA);
  JS_SetReservedSlot(customA, CUSTOM_SLOT, Int32Value(17));

  JSFunction *customMethodA = JS_NewFunction(cx, CustomMethod, 0, 0, customA, "customMethodA");
  CHECK(customMethodA);

  jsval rval;
  CHECK(JS_CallFunction(cx, customA, customMethodA, 0, NULL, &rval));
  CHECK_SAME(rval, Int32Value(17));

  
  {
    JSObject *globalB = JS_NewGlobalObject(cx, getGlobalClass(), NULL);
    CHECK(globalB);

    
    JSAutoEnterCompartment enter;
    CHECK(enter.enter(cx, globalB));

    JSObject *customB = JS_NewObject(cx, &CustomClass, NULL, NULL);
    CHECK(customB);
    JS_SetReservedSlot(customB, CUSTOM_SLOT, Int32Value(42));

    JSFunction *customMethodB = JS_NewFunction(cx, CustomMethod, 0, 0, customB, "customMethodB");
    CHECK(customMethodB);

    jsval rval;
    CHECK(JS_CallFunction(cx, customB, customMethodB, 0, NULL, &rval));
    CHECK_SAME(rval, Int32Value(42));

    JSObject *wrappedCustomA = customA;
    CHECK(JS_WrapObject(cx, &wrappedCustomA));

    jsval rval2;
    CHECK(JS_CallFunction(cx, wrappedCustomA, customMethodB, 0, NULL, &rval2));
    CHECK_SAME(rval, Int32Value(42));
  }

  return true;
}
END_TEST(test_CallNonGenericMethodOnProxy)
