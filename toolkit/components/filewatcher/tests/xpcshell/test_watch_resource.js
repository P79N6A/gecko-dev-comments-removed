



"use strict";

function run_test() {
  
  do_get_profile();

  
  run_next_test();
}




add_task(function* test_watching_non_existing() {
  let notExistingDir =
    OS.Path.join(OS.Constants.Path.profileDir, "absolutelyNotExisting");

  
  let watcher = makeWatcher();
  let deferred = Promise.defer();

  
  watcher.addPath(notExistingDir, deferred.reject, deferred.resolve);

  
  let error = yield deferred.promise;
  do_check_eq(error, Components.results.NS_ERROR_FILE_NOT_FOUND);
});
