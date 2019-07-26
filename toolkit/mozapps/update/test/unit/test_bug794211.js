







const NETWORK_ERROR_OFFLINE = 111;

function run_test() {
  do_test_pending();
  do_register_cleanup(end_test);

  logTestInfo("test when an update check fails because the network is " +
              "offline that we check again when the network comes online. " +
              "(Bug 794211)");
  removeUpdateDirsAndFiles();
  setUpdateURLOverride();
  Services.prefs.setBoolPref(PREF_APP_UPDATE_AUTO, false);

  overrideXHR(null);
  overrideUpdatePrompt(updatePrompt);
  standardInit();

  do_execute_soon(run_test_pt1);
}

function run_test_pt1() {
  gResponseBody = null;
  gCheckFunc = check_test_pt1;
  gXHRCallback = xhr_pt1;
  gUpdateChecker.checkForUpdates(updateCheckListener, true);
}

function xhr_pt1() {
  gXHR.status = AUS_Cr.NS_ERROR_OFFLINE;
  gXHR.onerror({ target: gXHR });
}

function check_test_pt1(request, update) {
  do_check_eq(gStatusCode, AUS_Cr.NS_ERROR_OFFLINE);
  do_check_eq(update.errorCode, NETWORK_ERROR_OFFLINE);

  
  gAUS.onError(request, update);

  
  gXHRCallback = xhr_pt2;
  Services.obs.notifyObservers(gAUS, "network:offline-status-changed", "online");
}

var updatePrompt = {
  showUpdateAvailable: function(update) {
    check_test_pt2(update);
  }
};

function xhr_pt2() {
  var patches = getLocalPatchString();
  var updates = getLocalUpdateString(patches);
  var responseBody = getLocalUpdatesXMLString(updates);

  gXHR.status = 200;
  gXHR.responseText = responseBody;
  try {
    var parser = AUS_Cc["@mozilla.org/xmlextras/domparser;1"].
                 createInstance(AUS_Ci.nsIDOMParser);
    gXHR.responseXML = parser.parseFromString(responseBody, "application/xml");
  }
  catch(e) { }
  gXHR.onload({ target: gXHR });
}

function check_test_pt2(update) {
  
  do_check_neq(update, null);
  do_check_eq(update.name, "App Update Test");

  do_test_finished();
}

function end_test() {
  cleanUp();
}
