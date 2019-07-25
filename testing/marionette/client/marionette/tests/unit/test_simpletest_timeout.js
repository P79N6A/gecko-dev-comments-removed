



MARIONETTE_TIMEOUT = 100;



function do_test() {
  is(1, 1);
  isnot(1, 2);
  ok(1 == 1);
  finish();
}

setTimeout(do_test, 1000);
