









var gNextRunFunc;
var gExpectedStatusCode;
var gExpectedStatusText;

function run_test() {
  setupTestCommon();

  debugDump("testing nsIUpdateCheckListener onload and onerror error code " +
            "and statusText values");

  setUpdateURLOverride();
  standardInit();
  
  overrideXHR(callHandleEvent);
  do_execute_soon(run_test_pt1);
}



function callHandleEvent(aXHR) {
  aXHR.status = gExpectedStatusCode;
  let e = { target: aXHR };
  aXHR.onload(e);
}


function run_test_helper(aNextRunFunc, aExpectedStatusCode, aMsg) {
  gStatusCode = null;
  gStatusText = null;
  gCheckFunc = check_test_helper;
  gNextRunFunc = aNextRunFunc;
  gExpectedStatusCode = aExpectedStatusCode;
  debugDump(aMsg, Components.stack.caller);
  gUpdateChecker.checkForUpdates(updateCheckListener, true);
}

function check_test_helper() {
  Assert.equal(gStatusCode, gExpectedStatusCode,
               "the download status code" + MSG_SHOULD_EQUAL);
  let expectedStatusText = getStatusText(gExpectedStatusCode);
  Assert.equal(gStatusText, expectedStatusText,
               "the update status text" + MSG_SHOULD_EQUAL);
  gNextRunFunc();
}






function run_test_pt1() {
  gStatusCode = null;
  gStatusText = null;
  gCheckFunc = check_test_pt1;
  gExpectedStatusCode = 399;
  debugDump("testing default onerror error message");
  gUpdateChecker.checkForUpdates(updateCheckListener, true);
}

function check_test_pt1() {
  Assert.equal(gStatusCode, gExpectedStatusCode,
               "the download status code" + MSG_SHOULD_EQUAL);
  let expectedStatusText = getStatusText(404);
  Assert.equal(gStatusText, expectedStatusText,
               "the update status text" + MSG_SHOULD_EQUAL);
  run_test_pt2();
}


function run_test_pt2() {
  run_test_helper(run_test_pt3, 200,
                  "testing file malformed");
}


function run_test_pt3() {
  run_test_helper(run_test_pt4, 403,
                  "testing access denied");
}


function run_test_pt4() {
  run_test_helper(run_test_pt5, 404,
                  "testing file not found");
}


function run_test_pt5() {
  run_test_helper(run_test_pt6, 500,
                  "testing internal server error");
}


function run_test_pt6() {
  run_test_helper(run_test_pt7, Cr.NS_BINDING_FAILED,
                  "testing failed (unknown reason)");
}


function run_test_pt7() {
  run_test_helper(run_test_pt8, Cr.NS_ERROR_NET_TIMEOUT,
                  "testing connection timed out");
}


function run_test_pt8() {
  run_test_helper(run_test_pt9, Cr.NS_ERROR_OFFLINE,
                  "testing network offline");
}


function run_test_pt9() {
  run_test_helper(run_test_pt10, Cr.NS_ERROR_PORT_ACCESS_NOT_ALLOWED,
                  "testing port not allowed");
}


function run_test_pt10() {
  run_test_helper(run_test_pt11, Cr.NS_ERROR_NET_RESET,
                  "testing no data was received");
}


function run_test_pt11() {
  run_test_helper(run_test_pt12, Cr.NS_ERROR_UNKNOWN_HOST,
                  "testing update server not found");
}


function run_test_pt12() {
  run_test_helper(run_test_pt13, Cr.NS_ERROR_UNKNOWN_PROXY_HOST,
                  "testing proxy server not found");
}


function run_test_pt13() {
  run_test_helper(run_test_pt14, Cr.NS_ERROR_NET_INTERRUPT,
                  "testing data transfer interrupted");
}


function run_test_pt14() {
  run_test_helper(run_test_pt15, Cr.NS_ERROR_PROXY_CONNECTION_REFUSED,
                  "testing proxy server connection refused");
}


function run_test_pt15() {
  run_test_helper(run_test_pt16, 2153390069,
                  "testing server certificate expired");
}


function run_test_pt16() {
  run_test_helper(run_test_pt17, Cr.NS_ERROR_DOCUMENT_NOT_CACHED,
                  "testing network is offline");
}


function run_test_pt17() {
  run_test_helper(doTestFinish, Cr.NS_ERROR_CONNECTION_REFUSED,
                  "testing connection refused");
}
