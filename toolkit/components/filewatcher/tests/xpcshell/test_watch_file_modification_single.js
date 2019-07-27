



"use strict";

function run_test() {
  
  do_get_profile();

  
  run_next_test();
}





add_task(function* test_watch_single_path_file_modification() {

  
  
  let watchedDir = OS.Path.join(OS.Constants.Path.profileDir, "filewatcher_playground");
  yield OS.File.makeDir(watchedDir);

  let tempFileName = "test_filemodification.tmp";

  
  let watcher = makeWatcher();
  let deferred = Promise.defer();

  
  
  let tmpFilePath = OS.Path.join(watchedDir, tempFileName);
  yield OS.File.writeAtomic(tmpFilePath, "some data");

  
  
  yield promiseAddPath(watcher, watchedDir, deferred.resolve, deferred.reject);

  
  yield OS.File.writeAtomic(tmpFilePath, "some new data");

  
  let changed = yield deferred.promise;
  do_check_eq(changed, tmpFilePath);

  
  
  yield promiseRemovePath(watcher, watchedDir, deferred.resolve, deferred.reject);

  
  yield OS.File.removeDir(watchedDir);
});
