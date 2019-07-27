



"use strict";

function run_test() {
  
  do_get_profile();

  
  run_next_test();
}









add_task(function* test_watch_multi_paths() {

  
  
  let resourcesToWatch = 5;
  let watchedDir = OS.Constants.Path.profileDir;

  
  let tempDirNameBase = "FileWatcher_Test_";
  let tempFileName = "test.tmp";

  
  let watcher = makeWatcher();

  
  
  let detectedChanges = 0;
  let watchedResources = 0;
  let unwatchedResources = 0;

  let deferredChanges = Promise.defer();
  let deferredSuccesses = Promise.defer();
  let deferredShutdown = Promise.defer();

  
  let changeCallback = function(changed) {
      do_print(changed + " has changed.");

      detectedChanges += 1;

      
      if (detectedChanges === resourcesToWatch) {
        deferredChanges.resolve();
      }
    };

  
  let watchSuccessCallback = function(resourcePath) {
      do_print(resourcePath + " is being watched.");

      watchedResources += 1;

      
      
      if (watchedResources === resourcesToWatch) {
        deferredSuccesses.resolve();
      }
    };

  
  let unwatchSuccessCallback = function(resourcePath) {
      do_print(resourcePath + " is being un-watched.");

      unwatchedResources += 1;

      
      
      if (unwatchedResources === resourcesToWatch) {
        deferredShutdown.resolve();
      }
    };

  
  for (let i = 0; i < resourcesToWatch; i++) {
    let tmpSubDirPath = OS.Path.join(watchedDir, tempDirNameBase + i);
    do_print("Creating the " + tmpSubDirPath + " directory.");
    yield OS.File.makeDir(tmpSubDirPath);
    watcher.addPath(tmpSubDirPath, changeCallback, deferredChanges.reject, watchSuccessCallback);
  }

  
  
  yield deferredSuccesses.promise;

  
  for (let i = 0; i < resourcesToWatch; i++) {
    let tmpFilePath = OS.Path.join(watchedDir, tempDirNameBase + i, tempFileName);
    yield OS.File.writeAtomic(tmpFilePath, "test content");
  }

  
  yield deferredChanges.promise;

  
  for (let i = 0; i < resourcesToWatch; i++) {
    let tmpSubDirPath = OS.Path.join(watchedDir, tempDirNameBase + i);
    watcher.removePath(tmpSubDirPath, changeCallback, deferredChanges.reject, unwatchSuccessCallback);
  }

  
  yield deferredShutdown.promise;
});
