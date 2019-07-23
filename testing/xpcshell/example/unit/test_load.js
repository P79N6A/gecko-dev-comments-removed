






































var subscriptLoaded = false;

function run_test() {
  load("load_subscript.js");
  do_check_true(subscriptLoaded);
  subscriptLoaded = false;
  try {
    load("file_that_does_not_exist.js");
    subscriptLoaded = true;
  }
  catch (ex) {
    do_check_eq(ex.message.substring(0,16), "cannot open file");
  }
  do_check_false(subscriptLoaded, "load() should throw an error");
}
