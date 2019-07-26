


"use strict";



let imported = {};
Components.utils.import("resource://gre/modules/Timer.jsm", imported);

function run_test(browser, tab, document) {
  do_test_pending();

  let timeout1 = imported.setTimeout(function() do_throw("Should not be called"), 100);
  do_check_eq(typeof timeout1, "number", "setTimeout returns a number");
  do_check_true(timeout1 > 0, "setTimeout returns a positive number");

  let timeout2 = imported.setTimeout(function() {
    do_check_true(true, "Should be called");
    do_test_finished();
  }, 100);

  do_check_eq(typeof timeout2, "number", "setTimeout returns a number");
  do_check_true(timeout2 > 0, "setTimeout returns a positive number");
  do_check_neq(timeout1, timeout2, "Calling setTimeout again returns a different value");

  imported.clearTimeout(timeout1);
}
