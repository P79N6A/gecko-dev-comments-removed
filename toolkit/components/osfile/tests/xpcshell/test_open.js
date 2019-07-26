


"use strict";

Components.utils.import("resource://gre/modules/osfile.jsm");

function run_test() {
  do_test_pending();
  run_next_test();
}







add_task(function() {
  
  
  try {
    let fd = yield OS.File.open(OS.Path.join(".", "This file does not exist"));
    do_check_true(false, "File opening 1 succeeded (it should fail)");
  } catch (err if err instanceof OS.File.Error && err.becauseNoSuchFile) {
    do_print("File opening 1 failed " + err);
  }

  
  
  do_print("Attempting to open a file with wrong arguments");
  try {
    let fd = yield OS.File.open(1, 2, 3);
    do_check_true(false, "File opening 2 succeeded (it should fail)" + fd);
  } catch (err) {
    do_print("File opening 2 failed " + err);
    do_check_false(err instanceof OS.File.Error,
                   "File opening 2 returned something that is not a file error");
    do_check_true(err.constructor.name == "TypeError",
                  "File opening 2 returned a TypeError");
  }

  
  do_print("Attempting to open a file correctly");
  let openedFile = yield OS.File.open(OS.Path.join(do_get_cwd().path, "test_open.js"));
  do_print("File opened correctly");

  do_print("Attempting to close a file correctly");
  yield openedFile.close();

  do_print("Attempting to close a file again");
  yield openedFile.close();
});





add_task(function test_error_attributes () {

  let dir = OS.Path.join(do_get_profile().path, "test_osfileErrorAttrs");
  let fpath = OS.Path.join(dir, "test_error_attributes.txt");

  try {
    yield OS.File.open(fpath, {truncate: true}, {});
    do_check_true(false, "Opening path suceeded (it should fail) " + fpath);
  } catch (err) {
    do_check_true(err instanceof OS.File.Error);
    do_check_true(err.becauseNoSuchFile);
  }
});

add_task(function() {
  do_test_finished();
});
