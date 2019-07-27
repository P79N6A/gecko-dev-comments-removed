#include "gdb-tests.h"
#include "jsatom.h"
#include "jscntxt.h"



#include "vm/String.h"

FRAGMENT(JSString, simple) {
  JS::Rooted<JSString*> empty(cx, JS_NewStringCopyN(cx, nullptr, 0));
  JS::Rooted<JSString*> x(cx, JS_NewStringCopyN(cx, "x", 1));
  JS::Rooted<JSString*> z(cx, JS_NewStringCopyZ(cx, "z"));

  
  JS::Rooted<JSString*> stars(cx, JS_NewStringCopyZ(cx,
                                                     "*************************"
                                                     "*************************"
                                                     "*************************"
                                                     "*************************"));

  
  JS::Rooted<JSString*> xz(cx, JS_ConcatStrings(cx, x, z));

  
  JS::Rooted<JSString*> doubleStars(cx, JS_ConcatStrings(cx, stars, stars));

  
  JSString* xRaw = x;

  breakpoint();

  (void) empty;
  (void) x;
  (void) z;
  (void) stars;
  (void) xz;
  (void) doubleStars;
  (void) xRaw;
}

FRAGMENT(JSString, null) {
  JS::Rooted<JSString*> null(cx, nullptr);
  JSString* nullRaw = null;

  breakpoint();

  (void) null;
  (void) nullRaw;
}

FRAGMENT(JSString, subclasses) {
  JS::Rooted<JSFlatString*> flat(cx, JS_FlattenString(cx, JS_NewStringCopyZ(cx, "Hi!")));

  breakpoint();

  (void) flat;
}

FRAGMENT(JSString, atom) {
  JSAtom* molybdenum = js::Atomize(cx, "molybdenum", 10);
  breakpoint();

  (void) molybdenum;
}
