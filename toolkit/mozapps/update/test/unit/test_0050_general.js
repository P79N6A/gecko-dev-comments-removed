










































var gNextRunFunc;
var gExpectedStatusCode;
var gExpectedStatusText;

function run_test() {
  do_test_pending();
  removeUpdateDirsAndFiles();
  startAUS();
  startUpdateChecker();
  gPrefs.setCharPref(PREF_APP_UPDATE_URL_OVERRIDE, URL_HOST + "update.xml");
  do_timeout(0, "run_test_pt1()");
}

function end_test() {
  stop_httpserver(do_test_finished);
}


function httpdErrorHandler(metadata, response) {
  response.setStatusLine(metadata.httpVersion, gExpectedStatusCode, "Error");
}


function run_test_helper(aMsg, aExpectedStatusCode, aExpectedStatusTextCode,
                         aNextRunFunc) {
  gStatusCode = null;
  gStatusText = null;
  gCheckFunc = check_test_helper;
  gNextRunFunc = aNextRunFunc;
  gExpectedStatusCode = gResponseStatusCode = aExpectedStatusCode;
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
  toggleOffline(true);
  run_test_helper("run_test_pt1 - network is offline",
                  0, 2152398918, run_test_pt2);
}


function run_test_pt2() {
  toggleOffline(false);
  run_test_helper("run_test_pt2 - connection refused",
                  0, 2152398861, run_test_pt3);
}






function run_test_pt3() {
  start_httpserver(DIR_DATA);
  run_test_helper("run_test_pt3 - file not found",
                  404, 404, run_test_pt4);
}


function run_test_pt4() {
  gTestserver.registerPathHandler("/update.xml", pathHandler);
  gResponseBody = "<html><head></head><body></body></html>\n";
  run_test_helper("run_test_pt4 - file malformed",
                  200, 200, run_test_pt5);
}


function run_test_pt5() {
  gResponseBody = "\n";
  run_test_helper("run_test_pt5 - internal server error",
                  500, 500, run_test_pt6);
}







function run_test_pt6() {
  gTestserver.registerErrorHandler(404, httpdErrorHandler);
  run_test_helper("run_test_pt6 - access denied",
                  403, 403, run_test_pt7);
}


function run_test_pt7() {
  run_test_helper("run_test_pt7 - default onerror error message",
                  399, 404, end_test);
}
