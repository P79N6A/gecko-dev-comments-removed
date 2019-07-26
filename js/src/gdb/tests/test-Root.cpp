#include "gdb-tests.h"

FRAGMENT(Root, null) {
  JS::Rooted<JSObject *> null(cx, NULL);

  breakpoint();

  (void) null;
}

void callee(JS::Handle<JSObject *> obj, JS::MutableHandle<JSObject *> mutableObj)
{
  
  
  fprintf(stderr, "Called " __FILE__ ":callee\n");
  breakpoint();
}

FRAGMENT(Root, handle) {
  JS::Rooted<JSObject *> global(cx, JS_GetGlobalObject(cx));
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
