



#include "jsapi-tests/tests.h"

BEGIN_TEST(test_GetPropertyDescriptor)
{
  JS::RootedValue v(cx);
  EVAL("({ somename : 123 })", &v);
  CHECK(v.isObject());

  JS::RootedObject obj(cx, &v.toObject());
  JS::Rooted<JSPropertyDescriptor> desc(cx);

  CHECK(JS_GetPropertyDescriptor(cx, obj, "somename", &desc));
  CHECK_EQUAL(desc.object(), obj);
  CHECK_SAME(desc.value(), JS::Int32Value(123));

  CHECK(JS_GetPropertyDescriptor(cx, obj, "not-here", &desc));
  CHECK_EQUAL(desc.object(), nullptr);

  CHECK(JS_GetPropertyDescriptor(cx, obj, "toString", &desc));
  JS::RootedObject objectProto(cx, JS_GetObjectPrototype(cx, obj));
  CHECK(objectProto);
  CHECK_EQUAL(desc.object(), objectProto);
  CHECK(desc.value().isObject());
  CHECK(JS::IsCallable(&desc.value().toObject()));

  return true;
}
END_TEST(test_GetPropertyDescriptor)
