


const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/osfile.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/Promise.jsm");

function checkFiles(files) {
  return Task.spawn(function*() {
    for (let file of files) {
      if (!(yield OS.File.exists(file))) {
        info("File doesn't exist: " + file);
        return false;
      }
    }

    return true;
  });
}

function checkDateHigherThan(files, date) {
  return Task.spawn(function*() {
    for (let file of files) {
      if (!(yield OS.File.exists(file))) {
        info("File doesn't exist: " + file);
        return false;
      }

      let stat = yield OS.File.stat(file);
      if (!(stat.lastModificationDate > date)) {
        info("File not newer: " + file);
        return false;
      }
    }

    return true;
  });
}

function wait(time) {
  let deferred = Promise.defer();

  setTimeout(function() {
    deferred.resolve();
  }, time);

  return deferred.promise;
}
