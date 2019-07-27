


"use strict";



function setup_crash() {
  Components.utils.import("resource://gre/modules/Services.jsm");

  Services.prefs.setBoolPref("toolkit.terminator.testing", true);
  Services.prefs.setIntPref("toolkit.asyncshutdown.crash_timeout", 10);

  
  
  
  let terminator = Components.classes["@mozilla.org/toolkit/shutdown-terminator;1"].
    createInstance(Components.interfaces.nsIObserver);
  terminator.observe(null, "profile-after-change", null);

  
  
  terminator.observe(null, "xpcom-will-shutdown", null);
  terminator.observe(null, "profile-before-change", null);

  dump("Waiting (actively) for the crash\n");
  while(true) {
    Services.tm.currentThread.processNextEvent(true);
  }
};


function after_crash(mdump, extra) {
  do_print("Crash signature: " + JSON.stringify(extra, null, "\t"));
  Assert.equal(extra.ShutdownProgress, "profile-before-change");
}

function run_test() {
  do_crash(setup_crash, after_crash);
}
