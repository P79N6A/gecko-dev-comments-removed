



"use strict";

function run_test() {
  
  do_get_profile();

  
  run_next_test();
}





add_task(function* test_fill_notification_buffer() {

  
  
  let watchedDir = OS.Path.join(OS.Constants.Path.profileDir, "filewatcher_playground");
  yield OS.File.makeDir(watchedDir);

  
  let numberOfFiles = 100;
  let fileNameBase = "testFile";

  
  
  let detectedChanges = 0;

  
  
  
  let expectedChanges = numberOfFiles * 2;

  
  let watcher = makeWatcher();
  let deferred = Promise.defer();

  
  let changeCallback = function(changed) {
      do_print(changed + " has changed.");

      detectedChanges += 1;

      
      if (detectedChanges >= expectedChanges) {
        deferred.resolve();
      }
    };

  
  
  yield promiseAddPath(watcher, watchedDir, changeCallback, deferred.reject);

  
  for (let i = 0; i < numberOfFiles; i++) {
    let tmpFilePath = OS.Path.join(watchedDir, fileNameBase + i);
    yield OS.File.writeAtomic(tmpFilePath, "test content");
    yield OS.File.remove(tmpFilePath);
  }

  
  
  yield deferred.promise;

  
  
  yield promiseRemovePath(watcher, watchedDir, changeCallback, deferred.reject);
});
