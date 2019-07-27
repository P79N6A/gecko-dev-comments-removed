



"use strict";

function run_test() {
  
  do_get_profile();

  
  run_next_test();
}





add_task(function* test_watch_recursively() {

  
  
  let watchedDir = OS.Path.join(OS.Constants.Path.profileDir, "filewatcher_playground");
  yield OS.File.makeDir(watchedDir);

  
  let subdirectory = OS.Path.join(watchedDir, "level1");
  yield OS.File.makeDir(subdirectory);

  let tempFileName = "test_filecreation.tmp";

  
  let watcher = makeWatcher();
  let deferred = Promise.defer();

  let tmpFilePath = OS.Path.join(subdirectory, tempFileName);

  
  
  yield promiseAddPath(watcher, watchedDir, deferred.resolve, deferred.reject);

  
  yield OS.File.writeAtomic(tmpFilePath, "some data");

  
  let changed = yield deferred.promise;
  do_check_eq(changed, tmpFilePath);

  
  
  yield promiseRemovePath(watcher, watchedDir, deferred.resolve, deferred.reject);

  
  yield OS.File.removeDir(watchedDir);
});
