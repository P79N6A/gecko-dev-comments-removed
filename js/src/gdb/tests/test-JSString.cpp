#include "gdb-tests.h"
#include "jsatom.h"



#include "vm/String.h"

FRAGMENT(JSString, simple) {
  js::Rooted<JSString *> empty(cx, JS_NewStringCopyN(cx, NULL, 0));
  js::Rooted<JSString *> x(cx, JS_NewStringCopyN(cx, "x", 1));
  js::Rooted<JSString *> z(cx, JS_NewStringCopyZ(cx, "z"));

  
  js::Rooted<JSString *> stars(cx, JS_NewStringCopyZ(cx,
                                                     "*************************"
                                                     "*************************"
                                                     "*************************"
                                                     "*************************"));

  
  js::Rooted<JSString *> xz(cx, JS_ConcatStrings(cx, x, z));

  
  js::Rooted<JSString *> doubleStars(cx, JS_ConcatStrings(cx, stars, stars));

  
  js::RawString xRaw = x;

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
  js::Rooted<JSString *> null(cx, NULL);
  js::RawString nullRaw = null;

  breakpoint();

  (void) null;
  (void) nullRaw;
}

FRAGMENT(JSString, subclasses) {
  js::Rooted<JSFlatString *> flat(cx, JS_FlattenString(cx, JS_NewStringCopyZ(cx, "Hi!")));

  breakpoint();

  (void) flat;
}

FRAGMENT(JSString, atom) {
  JSAtom *molybdenum = js::Atomize(cx, "molybdenum", 10);
  breakpoint();

  (void) molybdenum;
}
