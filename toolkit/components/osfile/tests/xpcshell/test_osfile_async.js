"use strict";

Components.utils.import("resource://gre/modules/osfile.jsm");






function run_test() {
  do_test_pending();
  OS.File.getCurrentDirectory().then(
    do_test_finished,
    do_test_finished
  );
}
