



#include "jsapi-tests/tests.h"

BEGIN_TEST(test_BindCallable)
{
  JS::RootedValue v(cx);
  EVAL("({ somename : 1717 })", v.address());
  CHECK(v.isObject());

  JS::RootedValue func(cx);
  EVAL("(function() { return this.somename; })", func.address());
  CHECK(func.isObject());

  JS::RootedObject funcObj(cx, JSVAL_TO_OBJECT(func));
  JS::RootedObject vObj(cx, JSVAL_TO_OBJECT(v));
  JSObject* newCallable = JS_BindCallable(cx, funcObj, vObj);
  CHECK(newCallable);

  JS::RootedValue retval(cx);
  JS::RootedValue fun(cx, JS::ObjectValue(*newCallable));
  bool called = JS_CallFunctionValue(cx, JS::NullPtr(), fun, JS::HandleValueArray::empty(), &retval);
  CHECK(called);

  CHECK(JSVAL_IS_INT(retval));

  CHECK(JSVAL_TO_INT(retval) == 1717);
  return true;
}
END_TEST(test_BindCallable)
