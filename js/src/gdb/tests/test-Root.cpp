#include "gdb-tests.h"
#include "jsapi.h"

FRAGMENT(Root, null) {
  JS::Rooted<JSObject *> null(cx, nullptr);

  breakpoint();

  (void) null;
}

void callee(JS::Handle<JSObject *> obj, JS::MutableHandle<JSObject *> mutableObj)
{
  
  
  fprintf(stderr, "Called " __FILE__ ":callee\n");
  breakpoint();
}

FRAGMENT(Root, handle) {
  JS::Rooted<JSObject *> global(cx, JS::CurrentGlobalOrNull(cx));
  callee(global, &global);
  (void) global;
}

FRAGMENT(Root, HeapSlot) {
  JS::Rooted<jsval> plinth(cx, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, "plinth")));
  JS::Rooted<JSObject *> array(cx, JS_NewArrayObject(cx, 1, plinth.address()));

  breakpoint();

  (void) plinth;
  (void) array;
}
