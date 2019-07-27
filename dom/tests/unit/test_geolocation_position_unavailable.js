const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;


function successCallback() {
  do_check_true(false);
  do_test_finished();
}

function errorCallback(err) {
  do_check_eq(Ci.nsIDOMGeoPositionError.POSITION_UNAVAILABLE, err.code);
  do_test_finished();
}

function run_test()
{
  do_test_pending();

  if (Cc["@mozilla.org/xre/app-info;1"].getService(Ci.nsIXULRuntime)
        .processType == Ci.nsIXULRuntime.PROCESS_TYPE_DEFAULT) {
    
    
    
    do_get_profile();

    var prefs = Cc["@mozilla.org/preferences-service;1"].getService(Ci.nsIPrefBranch);
    prefs.setBoolPref("geo.wifi.scan", false);
    prefs.setCharPref("geo.wifi.uri", "UrlNotUsedHere:");
    prefs.setBoolPref("dom.testing.ignore_ipc_principal", true);
  }

  geolocation = Cc["@mozilla.org/geolocation;1"].getService(Ci.nsISupports);
  geolocation.getCurrentPosition(successCallback, errorCallback);
}
