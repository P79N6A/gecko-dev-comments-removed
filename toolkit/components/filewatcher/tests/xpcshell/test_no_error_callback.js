



"use strict";

function run_test() {
  
  do_get_profile();

  
  run_next_test();
}





add_task(function* test_error_with_no_error_callback() {

  let watcher = makeWatcher();
  let testPath = 'someInvalidPath';

  
  
  let dummyFunc = function(changed) {
    do_throw("Not expected in this test.");
  };

  
  
  watcher.addPath(testPath, dummyFunc);
});





add_task(function* test_watch_single_path_file_creation_no_error_cb() {

  
  
  let watchedDir = OS.Path.join(OS.Constants.Path.profileDir, "filewatcher_playground");
  yield OS.File.makeDir(watchedDir);

  let tempFileName = "test_filecreation.tmp";

  
  let watcher = makeWatcher();
  let deferred = Promise.defer();

  
  yield promiseAddPath(watcher, watchedDir, deferred.resolve);

  
  let tmpFilePath = OS.Path.join(watchedDir, tempFileName);
  yield OS.File.writeAtomic(tmpFilePath, "some data");

  
  let changed = yield deferred.promise;
  do_check_eq(changed, tmpFilePath);

  
  
  watcher.removePath(watchedDir, deferred.resolve);

  
  yield OS.File.removeDir(watchedDir);
});
