



"use strict";

function run_test() {
  
  do_get_profile();

  
  run_next_test();
}





add_task(function* test_watch_single_path_directory_creation() {

  
  
  let watchedDir = OS.Path.join(OS.Constants.Path.profileDir, "filewatcher_playground");
  yield OS.File.makeDir(watchedDir);

  let tmpDirPath = OS.Path.join(watchedDir, "testdir");

  
  let watcher = makeWatcher();
  let deferred = Promise.defer();

  
  
  yield promiseAddPath(watcher, watchedDir, deferred.resolve, deferred.reject);

  
  yield OS.File.makeDir(tmpDirPath);

  
  let changed = yield deferred.promise;
  do_check_eq(changed, tmpDirPath);

  
  
  yield promiseRemovePath(watcher, watchedDir, deferred.resolve, deferred.reject);

  
  yield OS.File.removeDir(watchedDir);
});
