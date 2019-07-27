



Cu.import("resource://gre/modules/Promise.jsm");

let cs = Cc["@mozilla.org/consoleservice;1"].
  getService(Ci.nsIConsoleService);
let ps = Cc["@mozilla.org/preferences-service;1"].
  getService(Ci.nsIPrefService);

function makeBuffer(length) {
  return new Array(length + 1).join('x');
}





function checkWarning(pref, buffer) {
  let deferred = Promise.defer();
  let complete = false;
  let listener = {
    observe: function(event) {
      let message = event.message;
      if (!(message.startsWith("Warning: attempting to write")
            && message.includes(pref))) {
        return;
      }
      if (complete) {
        return;
      }
      complete = true;
      do_print("Warning while setting " + pref);
      cs.unregisterListener(listener);
      deferred.resolve(true);
    }
  };
  do_timeout(1000, function() {
    if (complete) {
      return;
    }
    complete = true;
    do_print("No warning while setting " + pref);
    cs.unregisterListener(listener);
    deferred.resolve(false);
  });
  cs.registerListener(listener);
  ps.setCharPref(pref, buffer);
  return deferred.promise;
}

function run_test() {
  run_next_test();
}

add_task(function() {
  
  do_print("Checking that a simple change doesn't cause a warning");
  let buf = makeBuffer(100);
  let warned = yield checkWarning("string.accept", buf);
  do_check_false(warned);

  
  do_print("Checking that a large change causes a warning");
  buf = makeBuffer(32 * 1024);
  warned = yield checkWarning("string.warn", buf);
  do_check_true(warned);
});
