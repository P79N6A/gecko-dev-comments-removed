







































var gNextRunFunc;
var gStatusResult;
var gExpectedStatusResult;

function run_test() {
  do_test_pending();
  removeUpdateDirsAndFiles();
  
  getPrefBranch().setCharPref(PREF_APP_UPDATE_URL_OVERRIDE, URL_HOST + "update.xml");
  overrideXHR(callHandleEvent);
  startAUS();
  startUpdateChecker();
  
  start_httpserver(DIR_DATA);
  do_timeout(0, run_test_pt1);
}

function end_test() {
  stop_httpserver(do_test_finished);
  cleanUp();
}



function callHandleEvent() {
  gXHR.status = 400;
  gXHR.responseText = gResponseBody;
  try {
    var parser = AUS_Cc["@mozilla.org/xmlextras/domparser;1"].
                 createInstance(AUS_Ci.nsIDOMParser);
    gXHR.responseXML = parser.parseFromString(gResponseBody, "application/xml");
  }
  catch(e) {
  }
  var e = { target: gXHR };
  gXHR.onload.handleEvent(e);
}



function run_test_helper_pt1(aMsg, aExpectedStatusResult, aNextRunFunc) {
  gUpdates = null;
  gUpdateCount = null;
  gStatusResult = null;
  gCheckFunc = check_test_helper_pt1_1;
  gNextRunFunc = aNextRunFunc;
  gExpectedStatusResult = aExpectedStatusResult;
  dump("Testing: " + aMsg + "\n");
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

function setResponseBody(aHashFunction, aHashValue) {
  var patches = getRemotePatchString(null, null, aHashFunction, aHashValue);
  var updates = getRemoteUpdateString(patches);
  gResponseBody = getRemoteUpdatesXMLString(updates);
}


function run_test_pt1() {
  setResponseBody("MD5", "6232cd43a1c77e30191c53a329a3f99d");
  run_test_helper_pt1("run_test_pt1 - mar download with a valid MD5 hash",
                      AUS_Cr.NS_OK, run_test_pt2);
}


function run_test_pt2() {
  setResponseBody("MD5", "6232cd43a1c77e30191c53a329a3f99e");
  run_test_helper_pt1("run_test_pt2 - mar download with an invalid MD5 hash",
                      AUS_Cr.NS_ERROR_UNEXPECTED, run_test_pt3);
}


function run_test_pt3() {
  setResponseBody("SHA1", "63A739284A1A73ECB515176B1A9D85B987E789CE");
  run_test_helper_pt1("run_test_pt3 - mar download with a valid SHA1 hash",
                      AUS_Cr.NS_OK, run_test_pt4);
}


function run_test_pt4() {
  setResponseBody("SHA1", "63A739284A1A73ECB515176B1A9D85B987E789CD");
  run_test_helper_pt1("run_test_pt4 - mar download with an invalid SHA1 hash",
                      AUS_Cr.NS_ERROR_UNEXPECTED, run_test_pt5);
}


function run_test_pt5() {
  var hashValue = "a8d9189f3978afd90dc7cd72e887ef22474c178e8314f23df2f779c881" +
                  "b872e2";
  setResponseBody("SHA256", hashValue);
  run_test_helper_pt1("run_test_pt5 - mar download with a valid SHA256 hash",
                      AUS_Cr.NS_OK, run_test_pt6);
}


function run_test_pt6() {
  var hashValue = "a8d9189f3978afd90dc7cd72e887ef22474c178e8314f23df2f779c881" +
                  "b872e1";
  setResponseBody("SHA256", hashValue);
  run_test_helper_pt1("run_test_pt6 - mar download with an invalid SHA256 hash",
                      AUS_Cr.NS_ERROR_UNEXPECTED, run_test_pt7);
}


function run_test_pt7() {
  var hashValue = "802c64f6caa6c356f7a5f8d9a008c08c54fe915c3ec7cf9e215c3bccc9" +
                  "e195c78b2669840d7b1d46ff3c1dfa751d72e1";
  setResponseBody("SHA384", hashValue);
  run_test_helper_pt1("run_test_pt7 - mar download with a valid SHA384 hash",
                      AUS_Cr.NS_OK, run_test_pt8);
}


function run_test_pt8() {
  var hashValue = "802c64f6caa6c356f7a5f8d9a008c08c54fe915c3ec7cf9e215c3bccc9" +
                  "e195c78b2669840d7b1d46ff3c1dfa751d72e2";
  setResponseBody("SHA384", hashValue);
  run_test_helper_pt1("run_test_pt8 - mar download with an invalid SHA384 hash",
                      AUS_Cr.NS_ERROR_UNEXPECTED, run_test_pt9);
}


function run_test_pt9() {
  var hashValue = "1d2307e309587ddd04299423b34762639ce6af3ee17cfdaa8fdd4e66b5" +
                  "a61bfb6555b6e40a82604908d6d68d3e42f318f82e22b6f5e1118b4222" +
                  "e3417a2fa2d0";
  setResponseBody("SHA512", hashValue);
  run_test_helper_pt1("run_test_pt9 - mar download with a valid SHA512 hash",
                      AUS_Cr.NS_OK, run_test_pt10);
}


function run_test_pt10() {
  var hashValue = "1d2307e309587ddd04299423b34762639ce6af3ee17cfdaa8fdd4e66b5" +
                  "a61bfb6555b6e40a82604908d6d68d3e42f318f82e22b6f5e1118b4222" +
                  "e3417a2fa2d1";
  setResponseBody("SHA512", hashValue);
  run_test_helper_pt1("run_test_pt10 - mar download with an invalid SHA512 hash",
                      AUS_Cr.NS_ERROR_UNEXPECTED, run_test_pt11);
}


function run_test_pt11() {
  var patches = getRemotePatchString(null, URL_HOST + DIR_DATA + "/bogus.mar");
  var updates = getRemoteUpdateString(patches);
  gResponseBody = getRemoteUpdatesXMLString(updates);
  run_test_helper_pt1("run_test_pt11 - mar download with the mar not found",
                      AUS_Cr.NS_ERROR_UNEXPECTED, end_test);
}


const downloadListener = {
  onStartRequest: function(request, context) {
  },

  onProgress: function(request, context, progress, maxProgress) {
  },

  onStatus: function(request, context, status, statusText) {
  },

  onStopRequest: function(request, context, status) {
    gStatusResult = status;
    
    do_timeout(0, gCheckFunc);
  },

  QueryInterface: function(iid) {
    if (!iid.equals(AUS_Ci.nsIRequestObserver) &&
        !iid.equals(AUS_Ci.nsIProgressEventSink) &&
        !iid.equals(AUS_Ci.nsISupports))
      throw AUS_Cr.NS_ERROR_NO_INTERFACE;
    return this;
  }
};
