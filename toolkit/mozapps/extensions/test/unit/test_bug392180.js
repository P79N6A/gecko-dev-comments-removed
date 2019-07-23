






































gPrefs.setBoolPref("extensions.checkUpdateSecurity", false);

do_load_httpd_js();

var server;

var updateListener = {
  onUpdateStarted: function() {
  },

  onUpdateEnded: function() {
    gNext();
  },

  onAddonUpdateStarted: function(addon) {
  },

  onAddonUpdateEnded: function(addon, status) {
    
    do_check_eq(status, Ci.nsIAddonUpdateCheckListener.STATUS_FAILURE);
  }
}

var requestHandler = {
  handle: function(metadata, response) {
    var updateType = metadata.queryString;
    do_check_eq(updateType, gType);
    response.setStatusLine(metadata.httpVersion, 404, "Not Found");
  }
}

var gAddon;
var gNext;
var gType;

function run_test() {
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "1.9");

  startupEM();
  gEM.installItemFromFile(do_get_addon("test_bug392180"), NS_INSTALL_LOCATION_APPPROFILE);
  restartEM();

  gAddon = gEM.getItemForID("bug392180@tests.mozilla.org");
  do_check_neq(gAddon, null);

  server = new nsHttpServer();
  server.registerPathHandler("/update.rdf", requestHandler);
  server.start(4444);
  do_test_pending();

  run_test_1();
}

function end_test() {
  server.stop(do_test_finished);
}

function run_test_1() {
  
  gType = 96;
  gNext = run_test_2;
  gEM.update([gAddon], 1, Ci.nsIExtensionManager.UPDATE_CHECK_NEWVERSION, updateListener);
}

function run_test_2() {
  
  gType = 32;
  gNext = run_test_3;
  gEM.update([gAddon], 1, Ci.nsIExtensionManager.UPDATE_CHECK_COMPATIBILITY, updateListener);
}

function run_test_3() {
  
  gType = 97;
  gNext = run_test_4;
  gEM.update([gAddon], 1, Ci.nsIExtensionManager.UPDATE_CHECK_NEWVERSION, updateListener,
             Ci.nsIExtensionManager.UPDATE_WHEN_USER_REQUESTED);
}

function run_test_4() {
  
  gType = 98;
  gNext = run_test_5;
  gEM.update([gAddon], 1, Ci.nsIExtensionManager.UPDATE_CHECK_NEWVERSION, updateListener,
             Ci.nsIExtensionManager.UPDATE_WHEN_NEW_APP_DETECTED);
}

function run_test_5() {
  
  gType = 35;
  gNext = run_test_6;
  gEM.update([gAddon], 1, Ci.nsIExtensionManager.UPDATE_CHECK_COMPATIBILITY, updateListener,
             Ci.nsIExtensionManager.UPDATE_WHEN_NEW_APP_INSTALLED);
}

function run_test_6() {
  
  gType = 35;
  gNext = end_test;
  try {
    gEM.update([gAddon], 1, Ci.nsIExtensionManager.UPDATE_CHECK_COMPATIBILITY, updateListener,
               16);
    do_throw("Should have thrown an exception");
  }
  catch (e) {
    do_check_eq(e.result, Components.results.NS_ERROR_ILLEGAL_VALUE);
    end_test();
  }
}
