#include "gdb-tests.h"

#include "jsapi.h"
#include "jsfun.h"

#include "gc/Barrier.h"

FRAGMENT(Root, null) {
  JS::Rooted<JSObject*> null(cx, nullptr);

  breakpoint();

  (void) null;
}

void callee(JS::Handle<JSObject*> obj, JS::MutableHandle<JSObject*> mutableObj)
{
  
  
  fprintf(stderr, "Called " __FILE__ ":callee\n");
  breakpoint();
}

FRAGMENT(Root, handle) {
  JS::Rooted<JSObject*> global(cx, JS::CurrentGlobalOrNull(cx));
  callee(global, &global);
  (void) global;
}

FRAGMENT(Root, HeapSlot) {
  JS::Rooted<jsval> plinth(cx, JS::StringValue(JS_NewStringCopyZ(cx, "plinth")));
  JS::Rooted<JSObject*> array(cx, JS_NewArrayObject(cx, JS::HandleValueArray(plinth)));

  breakpoint();

  (void) plinth;
  (void) array;
}

FRAGMENT(Root, barriers) {
  JSObject* obj = JS_NewPlainObject(cx);
  js::PreBarriered<JSObject*> prebarriered(obj);
  js::HeapPtr<JSObject*> heapptr(obj);
  js::RelocatablePtr<JSObject*> relocatable(obj);

  JS::Value val = JS::ObjectValue(*obj);
  js::PreBarrieredValue prebarrieredValue(JS::ObjectValue(*obj));
  js::HeapValue heapValue(JS::ObjectValue(*obj));
  js::RelocatableValue relocatableValue(JS::ObjectValue(*obj));

  breakpoint();

  (void) prebarriered;
  (void) heapptr;
  (void) relocatable;
  (void) val;
  (void) prebarrieredValue;
  (void) heapValue;
  (void) relocatableValue;
}

