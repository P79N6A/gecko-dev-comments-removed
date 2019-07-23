











































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
  do_timeout(0, "run_test_pt1()");
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


function run_test_helper(aMsg, aExpectedStatusCode, aExpectedStatusTextCode,
                         aNextRunFunc) {
  gStatusCode = null;
  gStatusText = null;
  gCheckFunc = check_test_helper;
  gNextRunFunc = aNextRunFunc;
  gExpectedStatusCode = aExpectedStatusCode;
  gExpectedStatusText = getStatusText(aExpectedStatusTextCode);
  dump("Testing: " + aMsg + "\n");
  gUpdateChecker.checkForUpdates(updateCheckListener, true);
}

function check_test_helper() {
  do_check_eq(gStatusCode, gExpectedStatusCode);
  do_check_eq(gStatusText, gExpectedStatusText);
  gNextRunFunc();
}






function run_test_pt1() {
  run_test_helper("run_test_pt1 - failed (unknown reason)",
                  2152398849, 2152398849, run_test_pt2);
}


function run_test_pt2() {
  run_test_helper("run_test_pt2 - connection timed out",
                  2152398862, 2152398862, run_test_pt3);
}


function run_test_pt3() {
  run_test_helper("run_test_pt3 - network offline",
                  2152398864, 2152398864, run_test_pt4);
}


function run_test_pt4() {
  run_test_helper("run_test_pt4 - port not allowed",
                  2152398867, 2152398867, run_test_pt5);
}


function run_test_pt5() {
  run_test_helper("run_test_pt5 - no data was received",
                  2152398868, 2152398868, run_test_pt6);
}


function run_test_pt6() {
  run_test_helper("run_test_pt6 - update server not found",
                  2152398878, 2152398878, run_test_pt7);
}


function run_test_pt7() {
  run_test_helper("run_test_pt7 - proxy server not found",
                  2152398890, 2152398890, run_test_pt8);
}


function run_test_pt8() {
  run_test_helper("run_test_pt8 - data transfer interrupted",
                  2152398919, 2152398919, run_test_pt9);
}


function run_test_pt9() {
  run_test_helper("run_test_pt9 - proxy server connection refused",
                  2152398920, 2152398920, run_test_pt10);
}


function run_test_pt10() {
  run_test_helper("run_test_pt10 - server certificate expired",
                  2153390069, 2153390069, run_test_pt11);
}


function run_test_pt11() {
  run_test_helper("run_test_pt11 - default onload error message",
                  1152398920, 404, end_test);
}
