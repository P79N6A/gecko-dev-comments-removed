










































const DIR_DATA = "data"
const URL_PREFIX = "http://localhost:4444/" + DIR_DATA + "/";

const PREF_APP_UPDATE_URL_OVERRIDE = "app.update.url.override";

const URI_UPDATES_PROPERTIES = "chrome://mozapps/locale/update/updates.properties";
const gUpdateBundle = AUS_Cc["@mozilla.org/intl/stringbundle;1"]
                       .getService(AUS_Ci.nsIStringBundleService)
                       .createBundle(URI_UPDATES_PROPERTIES);

var gStatus;
var gStatusText;
var gExpectedStatusText;
var gCheckFunc;
var gNextRunFunc;

function run_test() {
  do_test_pending();
  startAUS();
  do_timeout(0, "run_test_pt1()");
}

function end_test() {
  stop_httpserver(do_test_finished);
}


function getStatusText(aErrCode) {
  try {
    return gUpdateBundle.GetStringFromName("check_error-" + aErrCode);
  }
  catch (e) {
  }
  return null;
}


function httpdErrorHandler(metadata, response) {
  response.setStatusLine(metadata.httpVersion, gExpectedStatus, "Error");
}


function run_test_helper(aUpdateXML, aMsg, aExpectedStatus,
                         aExpectedStatusText, aNextRunFunc) {
  gStatus = null;
  gStatusText = null;
  gCheckFunc = check_test_helper;
  gNextRunFunc = aNextRunFunc;
  gExpectedStatus = aExpectedStatus;
  gExpectedStatusText = aExpectedStatusText;
  var url = URL_PREFIX + aUpdateXML;
  dump("Testing: " + aMsg + " - " + url + "\n");
  gPrefs.setCharPref(PREF_APP_UPDATE_URL_OVERRIDE, url);
  gUpdateChecker.checkForUpdates(updateCheckListener, true);
}

function check_test_helper() {
  do_check_eq(gStatus, gExpectedStatus);
  do_check_eq(gStatusText, gExpectedStatusText);
  gNextRunFunc();
}






function run_test_pt1() {
  toggleOffline(true);
  run_test_helper("aus-0050_general-1.xml", "network is offline",
                  0, getStatusText("2152398918"), run_test_pt2);
}


function run_test_pt2() {
  toggleOffline(false);
  run_test_helper("aus-0050_general-2.xml", "connection refused",
                  0, getStatusText("2152398861"), run_test_pt3);
}






function run_test_pt3() {
  start_httpserver(DIR_DATA);
  run_test_helper("aus-0050_general-3.xml", "file malformed",
                  200, getStatusText("200"), run_test_pt4);
}


function run_test_pt4() {
  run_test_helper("aus-0050_general-4.xml", "file not found",
                  404, getStatusText("404"), run_test_pt5);
}


function run_test_pt5() {
  gTestserver.registerContentType("sjs", "sjs");
  run_test_helper("aus-0050_general-5.sjs", "internal server error",
                  500, getStatusText("500"), run_test_pt6);
}







function run_test_pt6() {
  gTestserver.registerErrorHandler(404, httpdErrorHandler);
  run_test_helper("aus-0050_general-6.xml", "access denied",
                  403, getStatusText("403"), run_test_pt7);
}


function run_test_pt7() {
  run_test_helper("aus-0050_general-7.xml", "default onerror error message",
                  399, getStatusText("404"), end_test);
}



const updateCheckListener = {
  onProgress: function(request, position, totalSize) {
  },

  onCheckComplete: function(request, updates, updateCount) {
    dump("onCheckComplete request.status = " + request.status + "\n\n");
    
    do_timeout(0, "gCheckFunc()");
  },

  onError: function(request, update) {
    gStatus = request.status;
    gStatusText = update.statusText;
    dump("onError: request.status = " + gStatus + ", update.statusText = " + gStatusText + "\n\n");
    
    do_timeout(0, "gCheckFunc()");
  },

  QueryInterface: function(aIID) {
    if (!aIID.equals(AUS_Ci.nsIUpdateCheckListener) &&
        !aIID.equals(AUS_Ci.nsISupports))
      throw AUS_Cr.NS_ERROR_NO_INTERFACE;
    return this;
  }
};
