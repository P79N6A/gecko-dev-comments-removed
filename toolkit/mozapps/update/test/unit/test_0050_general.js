













































var gNextRunFunc;
var gExpectedStatusCode;
var gExpectedStatusText;

function run_test() {
  do_test_pending();
  removeUpdateDirsAndFiles();
  startAUS();
  startUpdateChecker();
  getPrefBranch().setCharPref(PREF_APP_UPDATE_URL_OVERRIDE,
                              URL_HOST + "update.xml");
  overrideXHR(callHandleEvent);
  do_timeout(0, run_test_pt1);
}

function end_test() {
  do_test_finished();
  cleanUp();
}



function callHandleEvent() {
  gXHR.status = gExpectedStatusCode;
  var e = { target: gXHR };
  gXHR.onload.handleEvent(e);
}


function run_test_helper(aNextRunFunc, aExpectedStatusCode, aMsg) {
  gStatusCode = null;
  gStatusText = null;
  gCheckFunc = check_test_helper;
  gNextRunFunc = aNextRunFunc;
  gExpectedStatusCode = aExpectedStatusCode;
  dump("Testing: " + aMsg + "\n");
  gUpdateChecker.checkForUpdates(updateCheckListener, true);
}

function check_test_helper() {
  do_check_eq(gStatusCode, gExpectedStatusCode);
  var expectedStatusText = getStatusText(gExpectedStatusCode);
  do_check_eq(gStatusText, expectedStatusText);
  gNextRunFunc();
}






function run_test_pt1() {
  gStatusCode = null;
  gStatusText = null;
  gCheckFunc = check_test_pt1;
  gExpectedStatusCode = 399;
  dump("Testing: run_test_pt1 - default onerror error message\n");
  gUpdateChecker.checkForUpdates(updateCheckListener, true);
}

function check_test_pt1() {
  do_check_eq(gStatusCode, gExpectedStatusCode);
  var expectedStatusText = getStatusText(404);
  do_check_eq(gStatusText, expectedStatusText);
  run_test_pt2();
}


function run_test_pt2() {
  run_test_helper(run_test_pt3, 200,
                  "run_test_pt2 - file malformed");
}


function run_test_pt3() {
  run_test_helper(run_test_pt4, 403,
                  "run_test_pt3 - access denied");
}


function run_test_pt4() {
  run_test_helper(run_test_pt5, 404,
                  "run_test_pt4 - file not found");
}


function run_test_pt5() {
  run_test_helper(run_test_pt6, 500,
                  "run_test_pt5 - internal server error");
}


function run_test_pt6() {
  run_test_helper(run_test_pt7, AUS_Cr.NS_BINDING_FAILED,
                  "run_test_pt6 - failed (unknown reason)");
}


function run_test_pt7() {
  run_test_helper(run_test_pt8, AUS_Cr.NS_ERROR_NET_TIMEOUT,
                  "run_test_pt7 - connection timed out");
}


function run_test_pt8() {
  run_test_helper(run_test_pt9, AUS_Cr.NS_ERROR_OFFLINE,
                  "run_test_pt8 - network offline");
}


function run_test_pt9() {
  run_test_helper(run_test_pt10, AUS_Cr.NS_ERROR_PORT_ACCESS_NOT_ALLOWED,
                  "run_test_pt9 - port not allowed");
}


function run_test_pt10() {
  run_test_helper(run_test_pt11, AUS_Cr.NS_ERROR_NET_RESET,
                  "run_test_pt10 - no data was received");
}


function run_test_pt11() {
  run_test_helper(run_test_pt12, AUS_Cr.NS_ERROR_UNKNOWN_HOST,
                  "run_test_pt11 - update server not found");
}


function run_test_pt12() {
  run_test_helper(run_test_pt13, AUS_Cr.NS_ERROR_UNKNOWN_PROXY_HOST,
                  "run_test_pt12 - proxy server not found");
}


function run_test_pt13() {
  run_test_helper(run_test_pt14, AUS_Cr.NS_ERROR_NET_INTERRUPT,
                  "run_test_pt13 - data transfer interrupted");
}


function run_test_pt14() {
  run_test_helper(run_test_pt15, AUS_Cr.NS_ERROR_PROXY_CONNECTION_REFUSED,
                  "run_test_pt14 - proxy server connection refused");
}


function run_test_pt15() {
  run_test_helper(run_test_pt16, 2153390069,
                  "run_test_pt15 - server certificate expired");
}


function run_test_pt16() {
  run_test_helper(run_test_pt17, AUS_Cr.NS_ERROR_DOCUMENT_NOT_CACHED,
                  "run_test_pt16 - network is offline");
}


function run_test_pt17() {
  run_test_helper(end_test, AUS_Cr.NS_ERROR_CONNECTION_REFUSED,
                  "run_test_pt17 - connection refused");
}
