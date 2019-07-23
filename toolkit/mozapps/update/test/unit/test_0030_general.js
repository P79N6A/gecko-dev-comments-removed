







































const DIR_DATA = "data"
const URL_PREFIX = "http://localhost:4444/" + DIR_DATA + "/";

const PREF_APP_UPDATE_URL_OVERRIDE = "app.update.url.override";

var gUpdates;
var gUpdateCount;
var gStatus;
var gCheckFunc;
var gNextRunFunc;
var gExpectedResult;

function run_test() {
  do_test_pending();
  startAUS();
  start_httpserver(DIR_DATA);
  do_timeout(0, "run_test_pt1()");
}

function end_test() {
  stop_httpserver(do_test_finished);
}



function run_test_helper_pt1(aUpdateXML, aMsg, aResult, aNextRunFunc) {
  gUpdates = null;
  gUpdateCount = null;
  gStatus = null;
  gCheckFunc = check_test_helper_pt1_1;
  gNextRunFunc = aNextRunFunc;
  gExpectedResult = aResult;
  var url = URL_PREFIX + aUpdateXML;
  dump("Testing: " + aMsg + " - " + url + "\n");
  gPrefs.setCharPref(PREF_APP_UPDATE_URL_OVERRIDE, url);
  gUpdateChecker.checkForUpdates(updateCheckListener, true);
}

function check_test_helper_pt1_1() {
  do_check_eq(gUpdateCount, 1);
  gCheckFunc = check_test_helper_pt1_2;
  var bestUpdate = gAUS.selectUpdate(gUpdates, gUpdateCount);
  var state = gAUS.downloadUpdate(bestUpdate, false);
  if (state == "null" || state == "failed")
    do_throw("nsIApplicationUpdateService:downloadUpdate returned " + state);
  gAUS.addDownloadListener(downloadListener);
}

function check_test_helper_pt1_2() {
  do_check_eq(gStatus, gExpectedResult);
  gAUS.removeDownloadListener(downloadListener);
  gNextRunFunc();
}


function run_test_pt1() {
  run_test_helper_pt1("aus-0030_general-1.xml",
                      "mar download with a valid MD5 hash",
                      AUS_Cr.NS_OK, run_test_pt2);
}


function run_test_pt2() {
  run_test_helper_pt1("aus-0030_general-2.xml",
                      "mar download with an invalid MD5 hash",
                      AUS_Cr.NS_ERROR_UNEXPECTED, run_test_pt3);
}


function run_test_pt3() {
  run_test_helper_pt1("aus-0030_general-3.xml",
                      "mar download with a valid SHA1 hash",
                      AUS_Cr.NS_OK, run_test_pt4);
}


function run_test_pt4() {
  run_test_helper_pt1("aus-0030_general-4.xml",
                      "mar download with an invalid SHA1 hash",
                      AUS_Cr.NS_ERROR_UNEXPECTED, run_test_pt5);
}


function run_test_pt5() {
  run_test_helper_pt1("aus-0030_general-5.xml",
                      "mar download with a valid SHA256 hash",
                      AUS_Cr.NS_OK, run_test_pt6);
}


function run_test_pt6() {
  run_test_helper_pt1("aus-0030_general-6.xml",
                      "mar download with an invalid SHA256 hash",
                      AUS_Cr.NS_ERROR_UNEXPECTED, run_test_pt7);
}


function run_test_pt7() {
  run_test_helper_pt1("aus-0030_general-7.xml",
                      "mar download with a valid SHA384 hash",
                      AUS_Cr.NS_OK, run_test_pt8);
}


function run_test_pt8() {
  run_test_helper_pt1("aus-0030_general-8.xml",
                      "mar download with an invalid SHA384 hash",
                      AUS_Cr.NS_ERROR_UNEXPECTED, run_test_pt9);
}


function run_test_pt9() {
  run_test_helper_pt1("aus-0030_general-9.xml",
                      "mar download with a valid SHA512 hash",
                      AUS_Cr.NS_OK, run_test_pt10);
}


function run_test_pt10() {
  run_test_helper_pt1("aus-0030_general-10.xml",
                      "mar download with an invalid SHA512 hash",
                      AUS_Cr.NS_ERROR_UNEXPECTED, run_test_pt11);
}


function run_test_pt11() {
  run_test_helper_pt1("aus-0030_general-11.xml",
                      "mar download with the mar not found",
                      AUS_Cr.NS_ERROR_UNEXPECTED, end_test);
}


const updateCheckListener = {
  onProgress: function(request, position, totalSize) {
  },

  onCheckComplete: function(request, updates, updateCount) {
    gUpdateCount = updateCount;
    gUpdates = updates;
    dump("onCheckComplete url = " + request.channel.originalURI.spec + "\n\n");
    
    do_timeout(0, "gCheckFunc()");
  },

  onError: function(request, update) {
    dump("onError url = " + request.channel.originalURI.spec + "\n\n");
    
    do_timeout(0, "gCheckFunc()");
  },

  QueryInterface: function(aIID) {
    if (!aIID.equals(AUS_Ci.nsIUpdateCheckListener) &&
        !aIID.equals(AUS_Ci.nsISupports))
      throw AUS_Cr.NS_ERROR_NO_INTERFACE;
    return this;
  }
};


const downloadListener = {
  onStartRequest: function(request, context) {
  },

  onProgress: function(request, context, progress, maxProgress) {
  },

  onStatus: function(request, context, status, statusText) {
  },

  onStopRequest: function(request, context, status) {
    gStatus = status;
    
    do_timeout(0, "gCheckFunc()");
  },

  QueryInterface: function(iid) {
    if (!iid.equals(AUS_Ci.nsIRequestObserver) &&
        !iid.equals(AUS_Ci.nsIProgressEventSink) &&
        !iid.equals(AUS_Ci.nsISupports))
      throw AUS_Cr.NS_ERROR_NO_INTERFACE;
    return this;
  }
};
