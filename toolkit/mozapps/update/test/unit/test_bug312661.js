







































const PREF_APP_UPDATE_URL_OVERRIDE = "app.update.url.override";

var gStatusText;
var gCheckFunc;

function run_test() {
  do_test_pending();
  var ioService = AUS_Cc["@mozilla.org/network/io-service;1"]
                    .getService(AUS_Ci.nsIIOService);
  try {
    ioService.manageOfflineStatus = false;
  }
  catch (e) {
  }
  ioService.offline = true;
  startAUS();
  do_timeout(0, "run_test_pt1()");
}

function end_test() {
  do_test_finished();
}


function run_test_pt1() {
  gStatusText = null;
  gCheckFunc = check_test_pt1;
  dump("Testing: update,statusText when xml is not cached and network is offline\n");
  gPrefs.setCharPref(PREF_APP_UPDATE_URL_OVERRIDE, "http://localhost:4444/");
  gUpdateChecker.checkForUpdates(updateCheckListener, true);
}

function check_test_pt1() {
  const URI_UPDATES_PROPERTIES = "chrome://mozapps/locale/update/updates.properties";
  const updateBundle = AUS_Cc["@mozilla.org/intl/stringbundle;1"]
                         .getService(AUS_Ci.nsIStringBundleService)
                         .createBundle(URI_UPDATES_PROPERTIES);
  var statusText = updateBundle.GetStringFromName("checker_error-2152398918");
  do_check_eq(statusText, gStatusText);
  end_test();
}


const updateCheckListener = {
  onProgress: function(request, position, totalSize) {
  },

  onCheckComplete: function(request, updates, updateCount) {
    dump("onCheckComplete request.status = " + request.status + "\n\n");
    
    do_timeout(0, "gCheckFunc()");
  },

  onError: function(request, update) {
    gStatusText = update.statusText;
    dump("onError update.statusText = " + update.statusText + "\n\n");
    
    do_timeout(0, "gCheckFunc()");
  },

  QueryInterface: function(aIID) {
    if (!aIID.equals(AUS_Ci.nsIUpdateCheckListener) &&
        !aIID.equals(AUS_Ci.nsISupports))
      throw AUS_Cr.NS_ERROR_NO_INTERFACE;
    return this;
  }
};
