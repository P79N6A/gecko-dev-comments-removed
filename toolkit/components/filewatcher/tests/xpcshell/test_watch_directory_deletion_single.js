



"use strict";

function run_test() {
  
  do_get_profile();

  
  run_next_test();
}





add_task(function* test_watch_single_path_directory_deletion() {

  let watchedDir = OS.Constants.Path.profileDir;
  let tempDirName = "test";
  let tmpDirPath = OS.Path.join(watchedDir, tempDirName);

  
  let watcher = makeWatcher();
  let deferred = Promise.defer();

  
  yield OS.File.makeDir(tmpDirPath);

  
  
  yield promiseAddPath(watcher, watchedDir, deferred.resolve, deferred.reject);

  
  OS.File.removeDir(tmpDirPath);

  
  let changed = yield deferred.promise;
  do_check_eq(changed, tmpDirPath);

  
  
  yield promiseRemovePath(watcher, watchedDir, deferred.resolve, deferred.reject);
});
