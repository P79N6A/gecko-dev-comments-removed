



#include "jsapi-tests/tests.h"

using namespace JS;

static const JSClass CustomClass = {
  "CustomClass",
  JSCLASS_HAS_RESERVED_SLOTS(1)
};

static const uint32_t CUSTOM_SLOT = 0;

static bool
IsCustomClass(JS::Handle<JS::Value> v)
{
  return v.isObject() && JS_GetClass(&v.toObject()) == &CustomClass;
}

static bool
CustomMethodImpl(JSContext *cx, CallArgs args)
{
  MOZ_ASSERT(IsCustomClass(args.thisv()));
  args.rval().set(JS_GetReservedSlot(&args.thisv().toObject(), CUSTOM_SLOT));
  return true;
}

static bool
CustomMethod(JSContext *cx, unsigned argc, Value *vp)
{
  CallArgs args = CallArgsFromVp(argc, vp);
  return CallNonGenericMethod(cx, IsCustomClass, CustomMethodImpl, args);
}

BEGIN_TEST(test_CallNonGenericMethodOnProxy)
{
  
  JS::RootedObject globalA(cx, JS_NewGlobalObject(cx, getGlobalClass(), nullptr, JS::FireOnNewGlobalHook));
  CHECK(globalA);

  JS::RootedObject customA(cx, JS_NewObject(cx, &CustomClass, JS::NullPtr(), JS::NullPtr()));
  CHECK(customA);
  JS_SetReservedSlot(customA, CUSTOM_SLOT, Int32Value(17));

  JS::RootedFunction customMethodA(cx, JS_NewFunction(cx, CustomMethod, 0, 0,
                                                      customA, "customMethodA"));
  CHECK(customMethodA);

  JS::RootedValue rval(cx);
  CHECK(JS_CallFunction(cx, customA, customMethodA, JS::HandleValueArray::empty(),
                        &rval));
  CHECK_SAME(rval, Int32Value(17));

  
  {
    JS::RootedObject globalB(cx, JS_NewGlobalObject(cx, getGlobalClass(), nullptr, JS::FireOnNewGlobalHook));
    CHECK(globalB);

    
    JSAutoCompartment enter(cx, globalB);
    JS::RootedObject customB(cx, JS_NewObject(cx, &CustomClass, JS::NullPtr(), JS::NullPtr()));
    CHECK(customB);
    JS_SetReservedSlot(customB, CUSTOM_SLOT, Int32Value(42));

    JS::RootedFunction customMethodB(cx, JS_NewFunction(cx, CustomMethod, 0, 0, customB, "customMethodB"));
    CHECK(customMethodB);

    JS::RootedValue rval(cx);
    CHECK(JS_CallFunction(cx, customB, customMethodB, JS::HandleValueArray::empty(),
                          &rval));
    CHECK_SAME(rval, Int32Value(42));

    JS::RootedObject wrappedCustomA(cx, customA);
    CHECK(JS_WrapObject(cx, &wrappedCustomA));

    JS::RootedValue rval2(cx);
    CHECK(JS_CallFunction(cx, wrappedCustomA, customMethodB, JS::HandleValueArray::empty(),
                          &rval2));
    CHECK_SAME(rval, Int32Value(42));
  }

  return true;
}
END_TEST(test_CallNonGenericMethodOnProxy)
