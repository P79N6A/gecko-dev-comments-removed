







































const URL_PREFIX = URL_HOST + DIR_DATA + "/";

const PREF_APP_UPDATE_CHANNEL        = "app.update.channel";
const PREF_PARTNER_BRANCH            = "app.partner.";
const PREF_APP_DISTRIBUTION          = "distribution.id";
const PREF_APP_DISTRIBUTION_VERSION  = "distribution.version";

var gAppInfo;

function run_test() {
  do_test_pending();
  removeUpdateDirsAndFiles();
  
  overrideXHR(callHandleEvent);
  startAUS();
  startUpdateChecker();
  gAppInfo = AUS_Cc["@mozilla.org/xre/app-info;1"].
             getService(AUS_Ci.nsIXULAppInfo).
             QueryInterface(AUS_Ci.nsIXULRuntime);
  do_timeout(0, "run_test_pt1()");
}

function end_test() {
  do_test_finished();
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
  dump("Testing: url constructed with %PRODUCT% - " + url + "\n");
  getPrefBranch().setCharPref(PREF_APP_UPDATE_URL_OVERRIDE, url);
  gUpdateChecker.checkForUpdates(updateCheckListener, true);
}

function check_test_pt1() {
  do_check_eq(getResult(gRequestURL), gAppInfo.name);
  run_test_pt2();
}


function run_test_pt2() {
  gCheckFunc = check_test_pt2;
  var url = URL_PREFIX + "%VERSION%/";
  dump("Testing: url constructed with %VERSION% - " + url + "\n");
  getPrefBranch().setCharPref(PREF_APP_UPDATE_URL_OVERRIDE, url);
  gUpdateChecker.checkForUpdates(updateCheckListener, true);
}

function check_test_pt2() {
  do_check_eq(getResult(gRequestURL), gAppInfo.version);
  run_test_pt3();
}


function run_test_pt3() {
  gCheckFunc = check_test_pt3;
  var url = URL_PREFIX + "%BUILD_ID%/";
  dump("Testing: url constructed with %BUILD_ID% - " + url + "\n");
  getPrefBranch().setCharPref(PREF_APP_UPDATE_URL_OVERRIDE, url);
  gUpdateChecker.checkForUpdates(updateCheckListener, true);
}

function check_test_pt3() {
  do_check_eq(getResult(gRequestURL), gAppInfo.appBuildID);
  run_test_pt4();
}



function run_test_pt4() {
  gCheckFunc = check_test_pt4;
  var url = URL_PREFIX + "%BUILD_TARGET%/";
  dump("Testing: url constructed with %BUILD_TARGET% - " + url + "\n");
  getPrefBranch().setCharPref(PREF_APP_UPDATE_URL_OVERRIDE, url);
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
      abi = "Universal-gcc3";
  }

  do_check_eq(getResult(gRequestURL), gAppInfo.OS + "_" + abi);
  run_test_pt5();
}



function run_test_pt5() {
  gCheckFunc = check_test_pt5;
  var url = URL_PREFIX + "%LOCALE%/";
  dump("Testing: url constructed with %LOCALE% - " + url + "\n");
  getPrefBranch().setCharPref(PREF_APP_UPDATE_URL_OVERRIDE, url);
  try {
    gUpdateChecker.checkForUpdates(updateCheckListener, true);
  }
  catch (e) {
    dump("***\n*** The following error is most likely due to a missing " +
         "update.locale file\n***\n");
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
  dump("Testing: url constructed with %CHANNEL% - " + url + "\n");
  var pb = getPrefBranch();
  pb.setCharPref(PREF_APP_UPDATE_URL_OVERRIDE, url);
  var defaults = pb.QueryInterface(AUS_Ci.nsIPrefService).getDefaultBranch(null);
  defaults.setCharPref(PREF_APP_UPDATE_CHANNEL, "bogus_channel");
  gUpdateChecker.checkForUpdates(updateCheckListener, true);
}

function check_test_pt6() {
  do_check_eq(getResult(gRequestURL), "bogus_channel");
  run_test_pt7();
}


function run_test_pt7() {
  gCheckFunc = check_test_pt7;
  var url = URL_PREFIX + "%CHANNEL%/";
  dump("Testing: url constructed with %CHANNEL% - " + url + "\n");
  var pb = getPrefBranch();
  pb.setCharPref(PREF_APP_UPDATE_URL_OVERRIDE, url);
  var defaults = pb.QueryInterface(AUS_Ci.nsIPrefService).getDefaultBranch(null);
  defaults.setCharPref(PREF_PARTNER_BRANCH + "bogus_partner1", "bogus_partner1");
  defaults.setCharPref(PREF_PARTNER_BRANCH + "bogus_partner2", "bogus_partner2");
  gUpdateChecker.checkForUpdates(updateCheckListener, true);
}

function check_test_pt7() {
  do_check_eq(getResult(gRequestURL), "bogus_channel-cck-bogus_partner1-bogus_partner2");
  run_test_pt8();
}


function run_test_pt8() {
  gCheckFunc = check_test_pt8;
  var url = URL_PREFIX + "%PLATFORM_VERSION%/";
  dump("Testing: url constructed with %PLATFORM_VERSION% - " + url + "\n");
  getPrefBranch().setCharPref(PREF_APP_UPDATE_URL_OVERRIDE, url);
  gUpdateChecker.checkForUpdates(updateCheckListener, true);
}

function check_test_pt8() {
  do_check_eq(getResult(gRequestURL), gAppInfo.platformVersion);
  run_test_pt9();
}


function run_test_pt9() {
  gCheckFunc = check_test_pt9;
  var url = URL_PREFIX + "%OS_VERSION%/";
  dump("Testing: url constructed with %OS_VERSION% - " + url + "\n");
  getPrefBranch().setCharPref(PREF_APP_UPDATE_URL_OVERRIDE, url);
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
  dump("Testing: url constructed with %DISTRIBUTION% - " + url + "\n");
  var pb = getPrefBranch();
  pb.setCharPref(PREF_APP_UPDATE_URL_OVERRIDE, url);
  var defaults = pb.QueryInterface(AUS_Ci.nsIPrefService).getDefaultBranch(null);
  defaults.setCharPref(PREF_APP_DISTRIBUTION, "bogus_distro");
  gUpdateChecker.checkForUpdates(updateCheckListener, true);
}

function check_test_pt10() {
  do_check_eq(getResult(gRequestURL), "bogus_distro");
  run_test_pt11();
}


function run_test_pt11() {
  gCheckFunc = check_test_pt11;
  var url = URL_PREFIX + "%DISTRIBUTION_VERSION%/";
  dump("Testing: url constructed with %DISTRIBUTION_VERSION% - " + url + "\n");
  var pb = getPrefBranch();
  pb.setCharPref(PREF_APP_UPDATE_URL_OVERRIDE, url);
  var defaults = pb.QueryInterface(AUS_Ci.nsIPrefService).getDefaultBranch(null);
  defaults.setCharPref(PREF_APP_DISTRIBUTION_VERSION, "bogus_distro_version");
  gUpdateChecker.checkForUpdates(updateCheckListener, true);
}

function check_test_pt11() {
  do_check_eq(getResult(gRequestURL), "bogus_distro_version");
  run_test_pt12();
}


function run_test_pt12() {
  gCheckFunc = check_test_pt12;
  var url = URL_PREFIX;
  dump("Testing: url constructed that doesn't have a parameter - " + url + "\n");
  getPrefBranch().setCharPref(PREF_APP_UPDATE_URL_OVERRIDE, url);
  gUpdateChecker.checkForUpdates(updateCheckListener, true);
}

function check_test_pt12() {
  do_check_eq(getResult(gRequestURL), "?force=1");
  run_test_pt13();
}


function run_test_pt13() {
  gCheckFunc = check_test_pt13;
  var url = URL_PREFIX + "?bogus=param";
  dump("Testing: url constructed that has a parameter - " + url + "\n");
  getPrefBranch().setCharPref(PREF_APP_UPDATE_URL_OVERRIDE, url);
  gUpdateChecker.checkForUpdates(updateCheckListener, true);
}

function check_test_pt13() {
  do_check_eq(getResult(gRequestURL), "?bogus=param&force=1");
  end_test();
}
