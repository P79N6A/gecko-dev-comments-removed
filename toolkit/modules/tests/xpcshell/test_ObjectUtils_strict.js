"use strict";

let {ObjectUtils} = Components.utils.import("resource://gre/modules/ObjectUtils.jsm", {});
let {Promise} = Components.utils.import("resource://gre/modules/Promise.jsm", {});

add_task(function* init() {
  
  Promise.Debugging.clearUncaughtErrorObservers();
});

add_task(function* test_strict() {
  let loose = { a: 1 };
  let strict = ObjectUtils.strict(loose);

  loose.a; 
  loose.b || undefined; 

  strict.a; 
  Assert.throws(() => strict.b, /No such property: "b"/);
  "b" in strict; 
  strict.b = 2;
  strict.b; 

  Assert.throws(() => strict.c, /No such property: "c"/);
  "c" in strict; 
  loose.c = 3;
  strict.c; 
});

function run_test() {
  run_next_test();
}
