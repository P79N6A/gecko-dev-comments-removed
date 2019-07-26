




var gNextRunFunc;
var gExpectedStatusResult;

function run_test() {
  
  
  gUseTestAppDir = false;
  setupTestCommon();

  logTestInfo("testing mar download and mar hash verification");

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



function callHandleEvent() {
  gXHR.status = 400;
  gXHR.responseText = gResponseBody;
  try {
    var parser = AUS_Cc["@mozilla.org/xmlextras/domparser;1"].
                 createInstance(AUS_Ci.nsIDOMParser);
    gXHR.responseXML = parser.parseFromString(gResponseBody, "application/xml");
  } catch(e) {
  }
  var e = { target: gXHR };
  gXHR.onload(e);
}



function run_test_helper_pt1(aMsg, aExpectedStatusResult, aNextRunFunc) {
  gUpdates = null;
  gUpdateCount = null;
  gStatusResult = null;
  gCheckFunc = check_test_helper_pt1_1;
  gNextRunFunc = aNextRunFunc;
  gExpectedStatusResult = aExpectedStatusResult;
  logTestInfo(aMsg, Components.stack.caller);
  gUpdateChecker.checkForUpdates(updateCheckListener, true);
}

function check_test_helper_pt1_1() {
  do_check_eq(gUpdateCount, 1);
  gCheckFunc = check_test_helper_pt1_2;
  var bestUpdate = gAUS.selectUpdate(gUpdates, gUpdateCount);
  var state = gAUS.downloadUpdate(bestUpdate, false);
  if (state == STATE_NONE || state == STATE_FAILED)
    do_throw("nsIApplicationUpdateService:downloadUpdate returned " + state);
  gAUS.addDownloadListener(downloadListener);
}

function check_test_helper_pt1_2() {
  do_check_eq(gStatusResult, gExpectedStatusResult);
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
  logTestInfo(aMsg, Components.stack.caller);
  gUpdateChecker.checkForUpdates(updateCheckListener, true);
}

function check_test_helper_bug828858_pt1_1() {
  do_check_eq(gUpdateCount, 1);
  gCheckFunc = check_test_helper_bug828858_pt1_2;
  var bestUpdate = gAUS.selectUpdate(gUpdates, gUpdateCount);
  var state = gAUS.downloadUpdate(bestUpdate, false);
  if (state == STATE_NONE || state == STATE_FAILED)
    do_throw("nsIApplicationUpdateService:downloadUpdate returned " + state);
  gAUS.addDownloadListener(downloadListener);
}

function check_test_helper_bug828858_pt1_2() {
  if (gStatusResult == AUS_Cr.NS_ERROR_CONTENT_CORRUPTED) {
    do_check_eq(gStatusResult, AUS_Cr.NS_ERROR_CONTENT_CORRUPTED);
  } else {
    do_check_eq(gStatusResult, gExpectedStatusResult);
  }
  gAUS.removeDownloadListener(downloadListener);
  gNextRunFunc();
}

function setResponseBody(aHashFunction, aHashValue, aSize) {
  var patches = getRemotePatchString(null, null,
                                     aHashFunction, aHashValue, aSize);
  var updates = getRemoteUpdateString(patches);
  gResponseBody = getRemoteUpdatesXMLString(updates);
}


function run_test_pt1() {
  setResponseBody("MD5", MD5_HASH_SIMPLE_MAR);
  run_test_helper_pt1("mar download with a valid MD5 hash",
                      AUS_Cr.NS_OK, run_test_pt2);
}


function run_test_pt2() {
  setResponseBody("MD5", MD5_HASH_SIMPLE_MAR + "0");
  run_test_helper_pt1("mar download with an invalid MD5 hash",
                      AUS_Cr.NS_ERROR_CORRUPTED_CONTENT, run_test_pt3);
}


function run_test_pt3() {
  setResponseBody("SHA1", SHA1_HASH_SIMPLE_MAR);
  run_test_helper_pt1("mar download with a valid SHA1 hash",
                      AUS_Cr.NS_OK, run_test_pt4);
}


function run_test_pt4() {
  setResponseBody("SHA1", SHA1_HASH_SIMPLE_MAR + "0");
  run_test_helper_pt1("mar download with an invalid SHA1 hash",
                      AUS_Cr.NS_ERROR_CORRUPTED_CONTENT, run_test_pt5);
}


function run_test_pt5() {
  setResponseBody("SHA256", SHA256_HASH_SIMPLE_MAR);
  run_test_helper_pt1("mar download with a valid SHA256 hash",
                      AUS_Cr.NS_OK, run_test_pt6);
}


function run_test_pt6() {
  setResponseBody("SHA256", SHA256_HASH_SIMPLE_MAR + "0");
  run_test_helper_pt1("mar download with an invalid SHA256 hash",
                      AUS_Cr.NS_ERROR_CORRUPTED_CONTENT, run_test_pt7);
}


function run_test_pt7() {
  setResponseBody("SHA384", SHA384_HASH_SIMPLE_MAR);
  run_test_helper_pt1("mar download with a valid SHA384 hash",
                      AUS_Cr.NS_OK, run_test_pt8);
}


function run_test_pt8() {
  setResponseBody("SHA384", SHA384_HASH_SIMPLE_MAR + "0");
  run_test_helper_pt1("mar download with an invalid SHA384 hash",
                      AUS_Cr.NS_ERROR_CORRUPTED_CONTENT, run_test_pt9);
}


function run_test_pt9() {
  setResponseBody("SHA512", SHA512_HASH_SIMPLE_MAR);
  run_test_helper_pt1("mar download with a valid SHA512 hash",
                      AUS_Cr.NS_OK, run_test_pt10);
}


function run_test_pt10() {
  setResponseBody("SHA512", SHA512_HASH_SIMPLE_MAR + "0");
  run_test_helper_pt1("mar download with an invalid SHA512 hash",
                      AUS_Cr.NS_ERROR_CORRUPTED_CONTENT, run_test_pt11);
}


function run_test_pt11() {
  var patches = getRemotePatchString(null, gURLData + "missing.mar");
  var updates = getRemoteUpdateString(patches);
  gResponseBody = getRemoteUpdatesXMLString(updates);
  run_test_helper_pt1("mar download with the mar not found",
                      AUS_Cr.NS_ERROR_UNEXPECTED, run_test_pt12);
}


function run_test_pt12() {
  const arbitraryFileSize = 1024000;
  setResponseBody("MD5", MD5_HASH_SIMPLE_MAR, arbitraryFileSize);
  if (IS_TOOLKIT_GONK) {
    
    
    
    
    
    
    run_test_helper_bug828858_pt1("mar download with a valid MD5 hash but invalid file size",
                                  AUS_Cr.NS_ERROR_UNEXPECTED, finish_test);
  } else {
    run_test_helper_pt1("mar download with a valid MD5 hash but invalid file size",
                        AUS_Cr.NS_ERROR_UNEXPECTED, finish_test);
  }
}
