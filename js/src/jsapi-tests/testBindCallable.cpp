



#include "tests.h"

BEGIN_TEST(test_BindCallable)
{
  js::RootedValue v(cx);
  EVAL("({ somename : 1717 })", v.address());
  CHECK(v.isObject());

  js::RootedValue func(cx);
  EVAL("(function() { return this.somename; })", func.address());
  CHECK(func.isObject());

  js::RootedObject funcObj(cx, JSVAL_TO_OBJECT(func));
  js::RootedObject vObj(cx, JSVAL_TO_OBJECT(v));
  JSObject* newCallable = JS_BindCallable(cx, funcObj, vObj);
  CHECK(newCallable);

  js::RootedValue retval(cx);
  bool called = JS_CallFunctionValue(cx, NULL, OBJECT_TO_JSVAL(newCallable), 0, NULL, retval.address());
  CHECK(called);

  CHECK(JSVAL_IS_INT(retval));

  CHECK(JSVAL_TO_INT(retval) == 1717);
  return true;
}
END_TEST(test_BindCallable)
