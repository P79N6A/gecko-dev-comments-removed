







































const URL_PREFIX = URL_HOST + URL_PATH + "/";

var gAppInfo;

function run_test() {
  do_test_pending();
  do_register_cleanup(end_test);
  removeUpdateDirsAndFiles();
  
  overrideXHR(callHandleEvent);
  standardInit();
  gAppInfo = AUS_Cc["@mozilla.org/xre/app-info;1"].
             getService(AUS_Ci.nsIXULAppInfo).
             QueryInterface(AUS_Ci.nsIXULRuntime);
  do_execute_soon(run_test_pt1);
}

function end_test() {
  cleanUp();
}



function callHandleEvent() {
  var e = { target: gXHR };
  gXHR.onload.handleEvent(e);
}


function getResult(url) {
  return url.substr(URL_PREFIX.length).split("/")[0];
}


function run_test_pt1() {
  gCheckFunc = check_test_pt1;
  var url = URL_PREFIX + "%PRODUCT%/";
  logTestInfo("testing url constructed with %PRODUCT% - " + url);
  setUpdateURLOverride(url);
  gUpdateChecker.checkForUpdates(updateCheckListener, true);
}

function check_test_pt1() {
  do_check_eq(getResult(gRequestURL), gAppInfo.name);
  run_test_pt2();
}


function run_test_pt2() {
  gCheckFunc = check_test_pt2;
  var url = URL_PREFIX + "%VERSION%/";
  logTestInfo("testing url constructed with %VERSION% - " + url);
  setUpdateURLOverride(url);
  gUpdateChecker.checkForUpdates(updateCheckListener, true);
}

function check_test_pt2() {
  do_check_eq(getResult(gRequestURL), gAppInfo.version);
  run_test_pt3();
}


function run_test_pt3() {
  gCheckFunc = check_test_pt3;
  var url = URL_PREFIX + "%BUILD_ID%/";
  logTestInfo("testing url constructed with %BUILD_ID% - " + url);
  setUpdateURLOverride(url);
  gUpdateChecker.checkForUpdates(updateCheckListener, true);
}

function check_test_pt3() {
  do_check_eq(getResult(gRequestURL), gAppInfo.appBuildID);
  run_test_pt4();
}



function run_test_pt4() {
  gCheckFunc = check_test_pt4;
  var url = URL_PREFIX + "%BUILD_TARGET%/";
  logTestInfo("testing url constructed with %BUILD_TARGET% - " + url);
  setUpdateURLOverride(url);
  gUpdateChecker.checkForUpdates(updateCheckListener, true);
}

function check_test_pt4() {
  var abi;
  try {
    abi = gAppInfo.XPCOMABI;
  }
  catch (e) {
    do_throw("nsIXULAppInfo:XPCOMABI not defined\n");
  }

  if (IS_MACOSX) {
    
    
    
    var macutils = AUS_Cc["@mozilla.org/xpcom/mac-utils;1"].
                   getService(AUS_Ci.nsIMacUtils);

    if (macutils.isUniversalBinary)
      abi += "-u-" + macutils.architecturesInBinary;
  }

  do_check_eq(getResult(gRequestURL), gAppInfo.OS + "_" + abi);
  run_test_pt5();
}



function run_test_pt5() {
  gCheckFunc = check_test_pt5;
  var url = URL_PREFIX + "%LOCALE%/";
  logTestInfo("testing url constructed with %LOCALE% - " + url);
  setUpdateURLOverride(url);
  try {
    gUpdateChecker.checkForUpdates(updateCheckListener, true);
  }
  catch (e) {
    logTestInfo("The following error is most likely due to a missing " +
                "update.locale file");
    do_throw(e);
  }
}

function check_test_pt5() {
  do_check_eq(getResult(gRequestURL), INSTALL_LOCALE);
  run_test_pt6();
}


function run_test_pt6() {
  gCheckFunc = check_test_pt6;
  var url = URL_PREFIX + "%CHANNEL%/";
  logTestInfo("testing url constructed with %CHANNEL% - " + url);
  setUpdateURLOverride(url);
  setUpdateChannel();
  gUpdateChecker.checkForUpdates(updateCheckListener, true);
}

function check_test_pt6() {
  do_check_eq(getResult(gRequestURL), "test_channel");
  run_test_pt7();
}


function run_test_pt7() {
  gCheckFunc = check_test_pt7;
  var url = URL_PREFIX + "%CHANNEL%/";
  logTestInfo("testing url constructed with %CHANNEL% - " + url);
  setUpdateURLOverride(url);
  gDefaultPrefBranch.setCharPref(PREF_APP_PARTNER_BRANCH + "test_partner1", "test_partner1");
  gDefaultPrefBranch.setCharPref(PREF_APP_PARTNER_BRANCH + "test_partner2", "test_partner2");
  gUpdateChecker.checkForUpdates(updateCheckListener, true);
}

function check_test_pt7() {
  do_check_eq(getResult(gRequestURL), "test_channel-cck-test_partner1-test_partner2");
  run_test_pt8();
}


function run_test_pt8() {
  gCheckFunc = check_test_pt8;
  var url = URL_PREFIX + "%PLATFORM_VERSION%/";
  logTestInfo("testing url constructed with %PLATFORM_VERSION% - " + url);
  setUpdateURLOverride(url);
  gUpdateChecker.checkForUpdates(updateCheckListener, true);
}

function check_test_pt8() {
  do_check_eq(getResult(gRequestURL), gAppInfo.platformVersion);
  run_test_pt9();
}


function run_test_pt9() {
  gCheckFunc = check_test_pt9;
  var url = URL_PREFIX + "%OS_VERSION%/";
  logTestInfo("testing url constructed with %OS_VERSION% - " + url);
  setUpdateURLOverride(url);
  gUpdateChecker.checkForUpdates(updateCheckListener, true);
}

function check_test_pt9() {
  var osVersion;
  var sysInfo = AUS_Cc["@mozilla.org/system-info;1"].
                getService(AUS_Ci.nsIPropertyBag2);
  osVersion = sysInfo.getProperty("name") + " " + sysInfo.getProperty("version");

  if (osVersion) {
    try {
      osVersion += " (" + sysInfo.getProperty("secondaryLibrary") + ")";
    }
    catch (e) {
      
      
    }
    osVersion = encodeURIComponent(osVersion);
  }

  do_check_eq(getResult(gRequestURL), osVersion);
  run_test_pt10();
}


function run_test_pt10() {
  gCheckFunc = check_test_pt10;
  var url = URL_PREFIX + "%DISTRIBUTION%/";
  logTestInfo("testing url constructed with %DISTRIBUTION% - " + url);
  setUpdateURLOverride(url);
  gDefaultPrefBranch.setCharPref(PREF_DISTRIBUTION_ID, "test_distro");
  gUpdateChecker.checkForUpdates(updateCheckListener, true);
}

function check_test_pt10() {
  do_check_eq(getResult(gRequestURL), "test_distro");
  run_test_pt11();
}


function run_test_pt11() {
  gCheckFunc = check_test_pt11;
  var url = URL_PREFIX + "%DISTRIBUTION_VERSION%/";
  logTestInfo("testing url constructed with %DISTRIBUTION_VERSION% - " + url);
  setUpdateURLOverride(url);
  gDefaultPrefBranch.setCharPref(PREF_DISTRIBUTION_VERSION, "test_distro_version");
  gUpdateChecker.checkForUpdates(updateCheckListener, true);
}

function check_test_pt11() {
  do_check_eq(getResult(gRequestURL), "test_distro_version");
  run_test_pt12();
}


function run_test_pt12() {
  gCheckFunc = check_test_pt12;
  var url = URL_PREFIX;
  logTestInfo("testing url with force param that doesn't already have a " +
              "param - " + url);
  setUpdateURLOverride(url);
  gUpdateChecker.checkForUpdates(updateCheckListener, true);
}

function check_test_pt12() {
  do_check_eq(getResult(gRequestURL), "?force=1");
  run_test_pt13();
}


function run_test_pt13() {
  gCheckFunc = check_test_pt13;
  var url = URL_PREFIX + "?extra=param";
  logTestInfo("testing url with force param that already has a param - " + url);
  logTestInfo("testing url constructed that has a parameter - " + url);
  setUpdateURLOverride(url);
  gUpdateChecker.checkForUpdates(updateCheckListener, true);
}

function check_test_pt13() {
  do_check_eq(getResult(gRequestURL), "?extra=param&force=1");
  run_test_pt14();
}


function run_test_pt14() {
  gCheckFunc = check_test_pt14;
  Services.prefs.setCharPref(PREF_APP_UPDATE_DESIREDCHANNEL, "testchannel");
  var url = URL_PREFIX;
  logTestInfo("testing url with newchannel param that doesn't already have a " +
              "param - " + url);
  setUpdateURLOverride(url);
  gUpdateChecker.checkForUpdates(updateCheckListener, false);
}

function check_test_pt14() {
  do_check_eq(getResult(gRequestURL), "?newchannel=testchannel");
  run_test_pt15();
}


function run_test_pt15() {
  gCheckFunc = check_test_pt15;
  Services.prefs.setCharPref(PREF_APP_UPDATE_DESIREDCHANNEL, "testchannel");
  var url = URL_PREFIX + "?extra=param";
  logTestInfo("testing url with newchannel param that already has a " +
              "param - " + url);
  setUpdateURLOverride(url);
  gUpdateChecker.checkForUpdates(updateCheckListener, false);
}

function check_test_pt15() {
  do_check_eq(getResult(gRequestURL), "?extra=param&newchannel=testchannel");
  run_test_pt16();
}


function run_test_pt16() {
  gCheckFunc = check_test_pt16;
  Services.prefs.setCharPref(PREF_APP_UPDATE_DESIREDCHANNEL, "testchannel");
  var url = URL_PREFIX;
  logTestInfo("testing url with force and newchannel params - " + url);
  setUpdateURLOverride(url);
  gUpdateChecker.checkForUpdates(updateCheckListener, true);
}

function check_test_pt16() {
  do_check_eq(getResult(gRequestURL), "?newchannel=testchannel&force=1");
  do_test_finished();
}
