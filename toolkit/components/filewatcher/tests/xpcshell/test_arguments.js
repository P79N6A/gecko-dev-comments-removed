



"use strict";

function run_test() {
  
  do_get_profile();

  
  run_next_test();
}




add_task(function* test_null_args_addPath() {

  let watcher = makeWatcher();
  let testPath = 'someInvalidPath';

  
  
  let dummyFunc = function(changed) {
    do_throw("Not expected in this test.");
  };

  
  try {
    watcher.addPath(testPath, null, dummyFunc);
  } catch (ex if ex.result == Cr.NS_ERROR_NULL_POINTER) {
    do_print("Initialisation thrown NS_ERROR_NULL_POINTER as expected.");
  }

  
  try {
    watcher.addPath(testPath, null, null);
  } catch (ex if ex.result == Cr.NS_ERROR_NULL_POINTER) {
    do_print("Initialisation thrown NS_ERROR_NULL_POINTER as expected.");
  }
});




add_task(function* test_null_args_removePath() {

  let watcher = makeWatcher();
  let testPath = 'someInvalidPath';

  
  
  let dummyFunc = function(changed) {
    do_throw("Not expected in this test.");
  };

  
  try {
    watcher.removePath(testPath, null, dummyFunc);
  } catch (ex if ex.result == Cr.NS_ERROR_NULL_POINTER) {
    do_print("Initialisation thrown NS_ERROR_NULL_POINTER as expected.");
  }

  
  try {
    watcher.removePath(testPath, null, null);
  } catch (ex if ex.result == Cr.NS_ERROR_NULL_POINTER) {
    do_print("Initialisation thrown NS_ERROR_NULL_POINTER as expected.");
  }
});
