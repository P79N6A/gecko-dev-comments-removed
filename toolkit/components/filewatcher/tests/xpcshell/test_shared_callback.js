



"use strict";

function run_test() {
  
  do_get_profile();

  
  run_next_test();
}





add_task(function* test_watch_with_shared_callback() {

  
  
  let watchedDirs =
    [
      OS.Path.join(OS.Constants.Path.profileDir, "filewatcher_playground"),
      OS.Path.join(OS.Constants.Path.profileDir, "filewatcher_playground2")
    ];

  yield OS.File.makeDir(watchedDirs[0]);
  yield OS.File.makeDir(watchedDirs[1]);

  let tempFileName = "test_filecreation.tmp";

  
  let watcher = makeWatcher();
  let deferred = Promise.defer();

  
  yield promiseAddPath(watcher, watchedDirs[0], deferred.resolve, deferred.reject);
  yield promiseAddPath(watcher, watchedDirs[1], deferred.resolve, deferred.reject);

  
  
  
  watcher.removePath(watchedDirs[0], deferred.resolve, deferred.reject);

  
  let tmpFilePath = OS.Path.join(watchedDirs[1], tempFileName);
  yield OS.File.writeAtomic(tmpFilePath, "some data");

  
  let changed = yield deferred.promise;
  do_check_eq(changed, tmpFilePath);

  
  
  watcher.removePath(watchedDirs[1], deferred.resolve, deferred.reject);

  
  yield OS.File.removeDir(watchedDirs[0]);
  yield OS.File.removeDir(watchedDirs[1]);
});
