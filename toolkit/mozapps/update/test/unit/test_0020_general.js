







































const DIR_DATA = "data"
const URL_PREFIX = "http://localhost:4444/" + DIR_DATA + "/";

const PREF_APP_UPDATE_URL_OVERRIDE = "app.update.url.override";

var gUpdates;
var gUpdateCount;
var gStatus;
var gCheckFunc;
var gNextRunFunc;
var gExpectedCount;

function run_test() {
  do_test_pending();
  startAUS();
  start_httpserver(DIR_DATA);
  do_timeout(0, "run_test_pt1()");
}

function end_test() {
  stop_httpserver(do_test_finished);
}


function run_test_helper_pt1(aUpdateXML, aMsg, aExpectedCount, aNextRunFunc) {
  gUpdates = null;
  gUpdateCount = null;
  gStatus = null;
  gCheckFunc = check_test_helper_pt1;
  gNextRunFunc = aNextRunFunc;
  gExpectedCount = aExpectedCount;
  var url = URL_PREFIX + aUpdateXML;
  dump("Testing: " + aMsg + " - " + url + "\n");
  gPrefs.setCharPref(PREF_APP_UPDATE_URL_OVERRIDE, url);
  gUpdateChecker.checkForUpdates(updateCheckListener, true);
}

function check_test_helper_pt1() {
  do_check_eq(gUpdateCount, gExpectedCount);
  gNextRunFunc();
}


function run_test_pt1() {
  gUpdates = null;
  gUpdateCount = null;
  gStatus = null;
  gCheckFunc = check_test_pt1;
  var url = URL_PREFIX + "aus-0020_general-1.xml";
  dump("Testing: one update available and the update's property values - " +
       url + "\n");
  gPrefs.setCharPref(PREF_APP_UPDATE_URL_OVERRIDE, url);
  var defaults = gPrefs.QueryInterface(AUS_Ci.nsIPrefService)
                   .getDefaultBranch(null);
  defaults.setCharPref("app.update.channel", "bogus_channel");
  gUpdateChecker.checkForUpdates(updateCheckListener, true);
}

function check_test_pt1() {
  do_check_eq(gUpdateCount, 1);
  var bestUpdate = gAUS.selectUpdate(gUpdates, gUpdateCount);
  do_check_eq(bestUpdate.type, "minor");
  do_check_eq(bestUpdate.name, "XPCShell Test");
  do_check_eq(bestUpdate.version, "1.1a1pre");
  do_check_eq(bestUpdate.platformVersion, "2.1a1pre");
  do_check_eq(bestUpdate.extensionVersion, "3.1a1pre");
  do_check_eq(bestUpdate.buildID, "20080811053724");
  do_check_eq(bestUpdate.detailsURL, "http://dummydetails/");
  do_check_eq(bestUpdate.licenseURL, "http://dummylicense/");
  do_check_eq(bestUpdate.serviceURL, URL_PREFIX + "aus-0020_general-1.xml?force=1");
  do_check_eq(bestUpdate.channel, "bogus_channel");
  do_check_false(bestUpdate.isCompleteUpdate);
  do_check_false(bestUpdate.isSecurityUpdate);
  do_check_eq(bestUpdate.installDate, 0);
  do_check_eq(bestUpdate.statusText, null);
  
  
  do_check_eq(bestUpdate.state, "");
  do_check_eq(bestUpdate.errorCode, 0);
  do_check_eq(bestUpdate.patchCount, 2);
  

  var type = "complete";
  var patch = bestUpdate.getPatchAt(0);
  do_check_eq(patch.type, type);
  do_check_eq(patch.URL, "http://" + type + "/");
  do_check_eq(patch.hashFunction, "SHA1");
  do_check_eq(patch.hashValue, "98db9dad8e1d80eda7e1170d0187d6f53e477059");
  do_check_eq(patch.size, 9856459);
  
  
  do_check_eq(patch.state, "null");
  do_check_false(patch.selected);
  

  type = "partial";
  patch = bestUpdate.getPatchAt(1);
  do_check_eq(patch.type, type);
  do_check_eq(patch.URL, "http://" + type + "/");
  do_check_eq(patch.hashFunction, "SHA1");
  do_check_eq(patch.hashValue, "e6678ca40ae7582316acdeddf3c133c9c8577de4");
  do_check_eq(patch.size, 1316138);
  
  
  do_check_eq(patch.state, "null");
  do_check_false(patch.selected);
  

  run_test_pt2();
}


function run_test_pt2() {
  run_test_helper_pt1("aus-0020_general-2.xml",
                      "empty update xml",
                      null, run_test_pt3);
}


function run_test_pt3() {
  run_test_helper_pt1("aus-0020_general-3.xml",
                      "update xml not available",
                      null, run_test_pt4);
}


function run_test_pt4() {
  run_test_helper_pt1("aus-0020_general-4.xml",
                      "no updates available",
                      0, run_test_pt5);
}


function run_test_pt5() {
  run_test_helper_pt1("aus-0020_general-5.xml",
                      "one update available",
                      1, run_test_pt6);
}


function run_test_pt6() {
  run_test_helper_pt1("aus-0020_general-6.xml",
                      "three updates available",
                      3, run_test_pt7);
}



function run_test_pt7() {
  run_test_helper_pt1("aus-0020_general-7.xml",
                      "one update with complete and partial patches with size 0",
                      0, run_test_pt8);
}


function run_test_pt8() {
  run_test_helper_pt1("aus-0020_general-8.xml",
                      "one update with complete patch with size 0",
                      0, run_test_pt9);
}


function run_test_pt9() {
  run_test_helper_pt1("aus-0020_general-9.xml",
                      "one update with partial patch with size 0",
                      0, end_test);
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
