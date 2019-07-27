




var gNextRunFunc;
var gExpectedStatusResult;

function run_test() {
  
  
  
  do_get_profile();

  setupTestCommon();

  debugDump("testing mar download and mar hash verification");

  Services.prefs.setBoolPref(PREF_APP_UPDATE_STAGING_ENABLED, false);
  
  start_httpserver();
  setUpdateURLOverride(gURLData + "update.xml");
  
  overrideXHR(callHandleEvent);
  standardInit();
  do_execute_soon(run_test_pt1);
}


function finish_test() {
  stop_httpserver(doTestFinish);
}



function callHandleEvent(aXHR) {
  aXHR.status = 400;
  aXHR.responseText = gResponseBody;
  try {
    let parser = Cc["@mozilla.org/xmlextras/domparser;1"].
                 createInstance(Ci.nsIDOMParser);
    aXHR.responseXML = parser.parseFromString(gResponseBody, "application/xml");
  } catch(e) {
  }
  let e = { target: aXHR };
  aXHR.onload(e);
}



function run_test_helper_pt1(aMsg, aExpectedStatusResult, aNextRunFunc) {
  gUpdates = null;
  gUpdateCount = null;
  gStatusResult = null;
  gCheckFunc = check_test_helper_pt1_1;
  gNextRunFunc = aNextRunFunc;
  gExpectedStatusResult = aExpectedStatusResult;
  debugDump(aMsg, Components.stack.caller);
  gUpdateChecker.checkForUpdates(updateCheckListener, true);
}

function check_test_helper_pt1_1() {
  Assert.equal(gUpdateCount, 1,
               "the update count" + MSG_SHOULD_EQUAL);
  gCheckFunc = check_test_helper_pt1_2;
  let bestUpdate = gAUS.selectUpdate(gUpdates, gUpdateCount);
  let state = gAUS.downloadUpdate(bestUpdate, false);
  if (state == STATE_NONE || state == STATE_FAILED) {
    do_throw("nsIApplicationUpdateService:downloadUpdate returned " + state);
  }
  gAUS.addDownloadListener(downloadListener);
}

function check_test_helper_pt1_2() {
  Assert.equal(gStatusResult, gExpectedStatusResult,
               "the download status result" + MSG_SHOULD_EQUAL);
  gAUS.removeDownloadListener(downloadListener);
  gNextRunFunc();
}




function run_test_helper_bug828858_pt1(aMsg, aExpectedStatusResult, aNextRunFunc) {
  gUpdates = null;
  gUpdateCount = null;
  gStatusResult = null;
  gCheckFunc = check_test_helper_bug828858_pt1_1;
  gNextRunFunc = aNextRunFunc;
  gExpectedStatusResult = aExpectedStatusResult;
  debugDump(aMsg, Components.stack.caller);
  gUpdateChecker.checkForUpdates(updateCheckListener, true);
}

function check_test_helper_bug828858_pt1_1() {
  Assert.equal(gUpdateCount, 1,
               "the update count" + MSG_SHOULD_EQUAL);
  gCheckFunc = check_test_helper_bug828858_pt1_2;
  let bestUpdate = gAUS.selectUpdate(gUpdates, gUpdateCount);
  let state = gAUS.downloadUpdate(bestUpdate, false);
  if (state == STATE_NONE || state == STATE_FAILED) {
    do_throw("nsIApplicationUpdateService:downloadUpdate returned " + state);
  }
  gAUS.addDownloadListener(downloadListener);
}

function check_test_helper_bug828858_pt1_2() {
  if (gStatusResult == Cr.NS_ERROR_CONTENT_CORRUPTED) {
    Assert.ok(true,
              "the status result should equal NS_ERROR_CONTENT_CORRUPTED");
  } else {
    Assert.equal(gStatusResult, gExpectedStatusResult,
                 "the download status result" + MSG_SHOULD_EQUAL);
  }
  gAUS.removeDownloadListener(downloadListener);
  gNextRunFunc();
}

function setResponseBody(aHashFunction, aHashValue, aSize) {
  let patches = getRemotePatchString(null, null,
                                     aHashFunction, aHashValue, aSize);
  let updates = getRemoteUpdateString(patches);
  gResponseBody = getRemoteUpdatesXMLString(updates);
}


function run_test_pt1() {
  setResponseBody("MD5", MD5_HASH_SIMPLE_MAR);
  run_test_helper_pt1("mar download with a valid MD5 hash",
                      Cr.NS_OK, run_test_pt2);
}


function run_test_pt2() {
  setResponseBody("MD5", MD5_HASH_SIMPLE_MAR + "0");
  run_test_helper_pt1("mar download with an invalid MD5 hash",
                      Cr.NS_ERROR_CORRUPTED_CONTENT, run_test_pt3);
}


function run_test_pt3() {
  setResponseBody("SHA1", SHA1_HASH_SIMPLE_MAR);
  run_test_helper_pt1("mar download with a valid SHA1 hash",
                      Cr.NS_OK, run_test_pt4);
}


function run_test_pt4() {
  setResponseBody("SHA1", SHA1_HASH_SIMPLE_MAR + "0");
  run_test_helper_pt1("mar download with an invalid SHA1 hash",
                      Cr.NS_ERROR_CORRUPTED_CONTENT, run_test_pt5);
}


function run_test_pt5() {
  setResponseBody("SHA256", SHA256_HASH_SIMPLE_MAR);
  run_test_helper_pt1("mar download with a valid SHA256 hash",
                      Cr.NS_OK, run_test_pt6);
}


function run_test_pt6() {
  setResponseBody("SHA256", SHA256_HASH_SIMPLE_MAR + "0");
  run_test_helper_pt1("mar download with an invalid SHA256 hash",
                      Cr.NS_ERROR_CORRUPTED_CONTENT, run_test_pt7);
}


function run_test_pt7() {
  setResponseBody("SHA384", SHA384_HASH_SIMPLE_MAR);
  run_test_helper_pt1("mar download with a valid SHA384 hash",
                      Cr.NS_OK, run_test_pt8);
}


function run_test_pt8() {
  setResponseBody("SHA384", SHA384_HASH_SIMPLE_MAR + "0");
  run_test_helper_pt1("mar download with an invalid SHA384 hash",
                      Cr.NS_ERROR_CORRUPTED_CONTENT, run_test_pt9);
}


function run_test_pt9() {
  setResponseBody("SHA512", SHA512_HASH_SIMPLE_MAR);
  run_test_helper_pt1("mar download with a valid SHA512 hash",
                      Cr.NS_OK, run_test_pt10);
}


function run_test_pt10() {
  setResponseBody("SHA512", SHA512_HASH_SIMPLE_MAR + "0");
  run_test_helper_pt1("mar download with an invalid SHA512 hash",
                      Cr.NS_ERROR_CORRUPTED_CONTENT, run_test_pt11);
}


function run_test_pt11() {
  let patches = getRemotePatchString(null, gURLData + "missing.mar");
  let updates = getRemoteUpdateString(patches);
  gResponseBody = getRemoteUpdatesXMLString(updates);
  run_test_helper_pt1("mar download with the mar not found",
                      Cr.NS_ERROR_UNEXPECTED, run_test_pt12);
}


function run_test_pt12() {
  const arbitraryFileSize = 1024000;
  setResponseBody("MD5", MD5_HASH_SIMPLE_MAR, arbitraryFileSize);
  if (IS_TOOLKIT_GONK) {
    
    
    
    
    
    
    run_test_helper_bug828858_pt1("mar download with a valid MD5 hash but invalid file size",
                                  Cr.NS_ERROR_UNEXPECTED, finish_test);
  } else {
    run_test_helper_pt1("mar download with a valid MD5 hash but invalid file size",
                        Cr.NS_ERROR_UNEXPECTED, finish_test);
  }
}
