











































const DIR_DATA = "data"
const URL_PREFIX = "http://localhost:4444/" + DIR_DATA + "/";

const PREF_APP_UPDATE_URL_OVERRIDE = "app.update.url.override";

const URI_UPDATES_PROPERTIES = "chrome://mozapps/locale/update/updates.properties";
const gUpdateBundle = AUS_Cc["@mozilla.org/intl/stringbundle;1"]
                       .getService(AUS_Ci.nsIStringBundleService)
                       .createBundle(URI_UPDATES_PROPERTIES);

var gStatusCode;
var gStatusText;
var gExpectedStatusCode;
var gExpectedStatusText;
var gCheckFunc;
var gNextRunFunc;

function run_test() {
  do_test_pending();
  startAUS();
  overrideXHR(callHandleEvent);
  do_timeout(0, "run_test_pt1()");
}

function end_test() {
  do_test_finished();
}


function getStatusText(aErrCode) {
  try {
    return gUpdateBundle.GetStringFromName("check_error-" + aErrCode);
  }
  catch (e) {
  }
  return null;
}



function callHandleEvent() {
  gXHR.status = gExpectedStatusCode;
  var e = { target: gXHR };
  gXHR.onload.handleEvent(e);
}


function run_test_helper(aUpdateXML, aMsg, aExpectedStatusCode,
                         aExpectedStatusText, aNextRunFunc) {
  gStatusCode = null;
  gStatusText = null;
  gCheckFunc = check_test_helper;
  gNextRunFunc = aNextRunFunc;
  gExpectedStatusCode = aExpectedStatusCode;
  gExpectedStatusText = aExpectedStatusText;
  var url = URL_PREFIX + aUpdateXML;
  dump("Testing: " + aMsg + " - " + url + "\n");
  gPrefs.setCharPref(PREF_APP_UPDATE_URL_OVERRIDE, url);
  gUpdateChecker.checkForUpdates(updateCheckListener, true);
}

function check_test_helper() {
  do_check_eq(gStatusCode, gExpectedStatusCode);
  do_check_eq(gStatusText, gExpectedStatusText);
  gNextRunFunc();
}






function run_test_pt1() {
  run_test_helper("aus-0051_general-1.xml", "failed (unknown reason)",
                  2152398849, getStatusText("2152398849"), run_test_pt2);
}


function run_test_pt2() {
  run_test_helper("aus-0051_general-2.xml", "connection timed out",
                  2152398862, getStatusText("2152398862"), run_test_pt3);
}


function run_test_pt3() {
  run_test_helper("aus-0051_general-3.xml", "network offline",
                  2152398864, getStatusText("2152398864"), run_test_pt4);
}


function run_test_pt4() {
  run_test_helper("aus-0051_general-4.xml", "port not allowed",
                  2152398867, getStatusText("2152398867"), run_test_pt5);
}


function run_test_pt5() {
  run_test_helper("aus-0051_general-5.xml", "no data was received",
                  2152398868, getStatusText("2152398868"), run_test_pt6);
}


function run_test_pt6() {
  run_test_helper("aus-0051_general-6.xml", "update server not found",
                  2152398878, getStatusText("2152398878"), run_test_pt7);
}


function run_test_pt7() {
  run_test_helper("aus-0051_general-7.xml", "proxy server not found",
                  2152398890, getStatusText("2152398890"), run_test_pt8);
}


function run_test_pt8() {
  run_test_helper("aus-0051_general-8.xml", "data transfer interrupted",
                  2152398919, getStatusText("2152398919"), run_test_pt9);
}


function run_test_pt9() {
  run_test_helper("aus-0051_general-9.xml", "proxy server connection refused",
                  2152398920, getStatusText("2152398920"), run_test_pt10);
}


function run_test_pt10() {
  run_test_helper("aus-0051_general-10.xml", "server certificate expired",
                  2153390069, getStatusText("2153390069"), run_test_pt11);
}


function run_test_pt11() {
  run_test_helper("aus-0051_general-11.xml", "default onload error message",
                  1152398920, getStatusText("404"), end_test);
}


const updateCheckListener = {
  onProgress: function(request, position, totalSize) {
  },

  onCheckComplete: function(request, updates, updateCount) {
    dump("onCheckComplete request.status = " + request.status + "\n\n");
    
    do_timeout(0, "gCheckFunc()");
  },

  onError: function(request, update) {
    gStatusCode = request.status;
    gStatusText = update.statusText;
    dump("onError: request.status = " + gStatusCode + ", update.statusText = " + gStatusText + "\n\n");
    
    do_timeout(0, "gCheckFunc()");
  },

  QueryInterface: function(aIID) {
    if (!aIID.equals(AUS_Ci.nsIUpdateCheckListener) &&
        !aIID.equals(AUS_Ci.nsISupports))
      throw AUS_Cr.NS_ERROR_NO_INTERFACE;

    return this;
  }
};
