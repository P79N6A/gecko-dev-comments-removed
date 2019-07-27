





const NETWORK_ERROR_OFFLINE = 111;

function run_test() {
  setupTestCommon();

  debugDump("testing when an update check fails because the network is " +
            "offline that we check again when the network comes online " +
            "(Bug 794211).");

  setUpdateURLOverride();
  Services.prefs.setBoolPref(PREF_APP_UPDATE_AUTO, false);

  overrideXHR(xhr_pt1);
  overrideUpdatePrompt(updatePrompt);
  standardInit();

  do_execute_soon(run_test_pt1);
}

function run_test_pt1() {
  gResponseBody = null;
  gCheckFunc = check_test_pt1;
  gUpdateChecker.checkForUpdates(updateCheckListener, true);
}

function xhr_pt1(aXHR) {
  aXHR.status = Cr.NS_ERROR_OFFLINE;
  aXHR.onerror({ target: aXHR });
}

function check_test_pt1(request, update) {
  do_check_eq(gStatusCode, Cr.NS_ERROR_OFFLINE);
  do_check_eq(update.errorCode, NETWORK_ERROR_OFFLINE);

  
  gAUS.onError(request, update);

  
  overrideXHR(xhr_pt2);
  Services.obs.notifyObservers(gAUS, "network:offline-status-changed", "online");
}

const updatePrompt = {
  showUpdateAvailable: function(update) {
    check_test_pt2(update);
  }
};

function xhr_pt2(aXHR) {
  let patches = getLocalPatchString();
  let updates = getLocalUpdateString(patches);
  let responseBody = getLocalUpdatesXMLString(updates);

  aXHR.status = 200;
  aXHR.responseText = responseBody;
  try {
    let parser = Cc["@mozilla.org/xmlextras/domparser;1"].
                 createInstance(Ci.nsIDOMParser);
    aXHR.responseXML = parser.parseFromString(responseBody, "application/xml");
  } catch (e) {
  }
  aXHR.onload({ target: aXHR });
}

function check_test_pt2(update) {
  
  do_check_neq(update, null);
  do_check_eq(update.name, "App Update Test");

  doTestFinish();
}
