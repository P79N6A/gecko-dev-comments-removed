



"use strict";

function run_test() {
  
  do_get_profile();

  
  run_next_test();
}




add_task(function* test_remove_not_watched() {
  let nonExistingDir =
    OS.Path.join(OS.Constants.Path.profileDir, "absolutelyNotExisting");

  
  let watcher = makeWatcher();

  
  watcher.removePath(
    nonExistingDir,
    function(changed) {
      do_throw("No change is expected in this test.");
    },
    function(xpcomError, osError) {
      
      
      do_throw("Unexpected exception: "
               + xpcomError + " (XPCOM) "
               + osError + " (OS Error)");
    }
  );
});
