Components.utils.import("resource://gre/modules/Services.jsm", this);
Components.utils.import("resource://gre/modules/Promise.jsm", this);
Components.utils.import("resource://gre/modules/Task.jsm", this);
Components.utils.import("resource://gre/modules/osfile.jsm", this);

add_task(function init() {
  do_get_profile();
});




add_task(function system_shutdown() {

  
  
  
  
  function testLeaksOf(resource, topic) {
    return Task.spawn(function() {
      let deferred = Promise.defer();

      
      Services.prefs.setBoolPref("toolkit.asyncshutdown.testing", true);
      Services.prefs.setBoolPref("toolkit.osfile.log", true);
      Services.prefs.setBoolPref("toolkit.osfile.log.redirect", true);
      Services.prefs.setCharPref("toolkit.osfile.test.shutdown.observer", topic);

      let observer = function(aMessage) {
        try {
          do_print("Got message: " + aMessage);
          if (!(aMessage instanceof Components.interfaces.nsIConsoleMessage)) {
            return;
          }
          let message = aMessage.message;
          do_print("Got message: " + message);
          if (message.indexOf("TEST OS Controller WARNING") < 0) {
            return;
          }
          do_print("Got message: " + message + ", looking for resource " + resource);
          if (message.indexOf(resource) < 0) {
            return;
          }
          do_print("Resource: " + resource + " found");
          do_execute_soon(deferred.resolve);
        } catch (ex) {
          do_execute_soon(function() {
            deferred.reject(ex);
          });
        }
      };
      Services.console.registerListener(observer);
      Services.obs.notifyObservers(null, topic, null);
      do_timeout(1000, function() {
        do_print("Timeout while waiting for resource: " + resource);
        deferred.reject("timeout");
      });

      let resolved = false;
      try {
        yield deferred.promise;
        resolved = true;
      } catch (ex if ex == "timeout") {
        resolved = false;
      }
      Services.console.unregisterListener(observer);
      Services.prefs.clearUserPref("toolkit.osfile.log");
      Services.prefs.clearUserPref("toolkit.osfile.log.redirect");
      Services.prefs.clearUserPref("toolkit.osfile.test.shutdown.observer");
      Services.prefs.clearUserPref("toolkit.async_shutdown.testing", true);

      throw new Task.Result(resolved);
    });
  }

  let TEST_DIR = OS.Path.join((yield OS.File.getCurrentDirectory()), "..");
  do_print("Testing for leaks of directory iterator " + TEST_DIR);
  let iterator = new OS.File.DirectoryIterator(TEST_DIR);
  do_print("At this stage, we leak the directory");
  do_check_true((yield testLeaksOf(TEST_DIR, "test.shutdown.dir.leak")));
  yield iterator.close();
  do_print("At this stage, we don't leak the directory anymore");
  do_check_false((yield testLeaksOf(TEST_DIR, "test.shutdown.dir.noleak")));

  let TEST_FILE = OS.Path.join(OS.Constants.Path.profileDir, "test");
  do_print("Testing for leaks of file descriptor: " + TEST_FILE);
  let openedFile = yield OS.File.open(TEST_FILE, { create: true} );
  do_print("At this stage, we leak the file");
  do_check_true((yield testLeaksOf(TEST_FILE, "test.shutdown.file.leak")));
  yield openedFile.close();
  do_print("At this stage, we don't leak the file anymore");
  do_check_false((yield testLeaksOf(TEST_FILE, "test.shutdown.file.leak")));
});


function run_test() {
  run_next_test();
}
