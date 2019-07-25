
function run_test() {
  do_check_true(inChildProcess(), "test harness should never call us directly");

  var cps = Cc["@mozilla.org/content-pref/service;1"].
            createInstance(Ci.nsIContentPrefService);

  
  try {
    cps.getPref("group", "name")
    do_check_false(true, "Must have thrown exception on getting general value");
  }
  catch(e) { }

  
  try {
    cps.setPref("group", "name", "someValue2");
    do_check_false(true, "Must have thrown exception on setting general value");
  }
  catch(e) { }

  
  cps.setPref("group", "browser.upload.lastDir", "childValue");
  do_check_eq(cps.getPref("group", "browser.upload.lastDir"), "childValue");

  
  var ioSvc = Cc["@mozilla.org/network/io-service;1"].
              getService(Ci.nsIIOService);
  var uri = ioSvc.newURI("http://mozilla.org", null, null);
  cps.setPref(uri, "browser.upload.lastDir", "childValue2");
  do_check_eq(cps.getPref(uri, "browser.upload.lastDir"), "childValue2");

  
  do_check_eq(cps.getPref("group", "browser.upload.lastDir"), "childValue");

  
  cps.wrappedJSObject.messageManager.sendSyncMessage('ContentPref:QUIT');
}

