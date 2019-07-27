

























































































































'use strict';

const { classes: Cc, interfaces: Ci, manager: Cm, results: Cr,
        utils: Cu } = Components;

Cu.import("resource://gre/modules/AddonManager.jsm", this);
Cu.import("resource://gre/modules/Services.jsm", this);

const IS_MACOSX = ("nsILocalFileMac" in Ci);
const IS_WIN = ("@mozilla.org/windows-registry-key;1" in Cc);



const PAGEID_DUMMY            = "dummy";                 
const PAGEID_CHECKING         = "checking";              
const PAGEID_PLUGIN_UPDATES   = "pluginupdatesfound";
const PAGEID_NO_UPDATES_FOUND = "noupdatesfound";        
const PAGEID_MANUAL_UPDATE    = "manualUpdate"; 
const PAGEID_UNSUPPORTED      = "unsupported";           
const PAGEID_INCOMPAT_CHECK   = "incompatibleCheck";     
const PAGEID_FOUND_BASIC      = "updatesfoundbasic";     
const PAGEID_FOUND_BILLBOARD  = "updatesfoundbillboard"; 
const PAGEID_LICENSE          = "license";               
const PAGEID_INCOMPAT_LIST    = "incompatibleList";      
const PAGEID_DOWNLOADING      = "downloading";           
const PAGEID_ERRORS           = "errors";                
const PAGEID_ERROR_EXTRA      = "errorextra";            
const PAGEID_ERROR_PATCHING   = "errorpatching";         
const PAGEID_FINISHED         = "finished";              
const PAGEID_FINISHED_BKGRD   = "finishedBackground";    
const PAGEID_INSTALLED        = "installed";             

const UPDATE_WINDOW_NAME = "Update:Wizard";

const URL_HOST = "http://example.com";
const URL_PATH_UPDATE_XML = "/chrome/toolkit/mozapps/update/tests/chrome/update.sjs";
const REL_PATH_DATA = "chrome/toolkit/mozapps/update/tests/data";



const URL_HTTP_UPDATE_XML = URL_HOST + URL_PATH_UPDATE_XML;
const URL_HTTPS_UPDATE_XML = "https://example.com" + URL_PATH_UPDATE_XML;

const URI_UPDATE_PROMPT_DIALOG  = "chrome://mozapps/content/update/updates.xul";

const ADDON_ID_SUFFIX = "@appupdatetest.mozilla.org";
const ADDON_PREP_DIR = "appupdateprep";

const PREF_APP_UPDATE_INTERVAL = "app.update.interval";
const PREF_APP_UPDATE_LASTUPDATETIME = "app.update.lastUpdateTime.background-update-timer";



const PREF_DISABLEDADDONS = "app.update.test.disabledAddons";
const PREF_EM_HOTFIX_ID = "extensions.hotfix.id";
const TEST_ADDONS = [ "appdisabled_1", "appdisabled_2",
                      "compatible_1", "compatible_2",
                      "noupdate_1", "noupdate_2",
                      "updatecompatibility_1", "updatecompatibility_2",
                      "updateversion_1", "updateversion_2",
                      "userdisabled_1", "userdisabled_2", "hotfix" ];

const LOG_FUNCTION = info;

const BIN_SUFFIX = (IS_WIN ? ".exe" : "");
const FILE_UPDATER_BIN = "updater" + (IS_MACOSX ? ".app" : BIN_SUFFIX);
const FILE_UPDATER_BIN_BAK = FILE_UPDATER_BIN + ".bak";

var gURLData = URL_HOST + "/" + REL_PATH_DATA + "/";

var gTestTimeout = 240000; 
var gTimeoutTimer;



const CLOSE_WINDOW_TIMEOUT_MAXCOUNT = 10;


var gCloseWindowTimeoutCounter = 0;



var gAppUpdateEnabled;            
var gAppUpdateServiceEnabled;     
var gAppUpdateStagingEnabled;     
var gAppUpdateURLDefault;         
var gAppUpdateURL;                
var gExtUpdateURL;                

var gTestCounter = -1;
var gWin;
var gDocElem;
var gPrefToCheck;
var gDisableNoUpdateAddon = false;
var gDisableUpdateCompatibilityAddon = false;
var gDisableUpdateVersionAddon = false;




var DEBUG_AUS_TEST = false;

const DATA_URI_SPEC = "chrome://mochitests/content/chrome/toolkit/mozapps/update/tests/data/";
Services.scriptloader.loadSubScript(DATA_URI_SPEC + "shared.js", this);




this.__defineGetter__("gTest", function() {
  return TESTS[gTestCounter];
});






this.__defineGetter__("gCallback", function() {
  return gTest.overrideCallback ? gTest.overrideCallback
                                : defaultCallback;
});





this.__defineGetter__("gRemoteContent", function() {
  switch (gTest.pageid) {
    case PAGEID_FOUND_BILLBOARD:
      return gWin.document.getElementById("updateMoreInfoContent");
    case PAGEID_LICENSE:
      return gWin.document.getElementById("licenseContent");
  }
  return null;
});





this.__defineGetter__("gRemoteContentState", function() {
  if (gRemoteContent) {
    return gRemoteContent.getAttribute("state");
  }
  return null;
});




this.__defineGetter__("gAcceptDeclineLicense", function() {
  return gWin.document.getElementById("acceptDeclineLicense");
});




this.__defineGetter__("gIncompatibleListbox", function() {
  return gWin.document.getElementById("incompatibleListbox");
});







function runTestDefault() {
  debugDump("entering");

  if (!("@mozilla.org/zipwriter;1" in Cc)) {
    ok(false, "nsIZipWriter is required to run these tests");
    return;
  }

  SimpleTest.waitForExplicitFinish();

  runTestDefaultWaitForWindowClosed();
}








function runTestDefaultWaitForWindowClosed() {
  gCloseWindowTimeoutCounter++;
  if (gCloseWindowTimeoutCounter > CLOSE_WINDOW_TIMEOUT_MAXCOUNT) {
    try {
      finishTest();
    }
    catch (e) {
      finishTestDefault();
    }
    return;
  }

  
  
  if (closeUpdateWindow()) {
    SimpleTest.executeSoon(runTestDefaultWaitForWindowClosed);
  } else {
    Services.ww.registerNotification(gWindowObserver);

    gCloseWindowTimeoutCounter = 0;

    setupFiles();
    setupPrefs();
    removeUpdateDirsAndFiles();
    reloadUpdateManagerData();
    setupAddons(runTest);
  }
}







function finishTestDefault() {
  debugDump("entering");
  if (gTimeoutTimer) {
    gTimeoutTimer.cancel();
    gTimeoutTimer = null;
  }

  if (gChannel) {
    debugDump("channel = " + gChannel);
    gChannel = null;
    gPrefRoot.removeObserver(PREF_APP_UPDATE_CHANNEL, observer);
  }

  verifyTestsRan();

  resetPrefs();
  resetFiles();
  removeUpdateDirsAndFiles();
  reloadUpdateManagerData();

  Services.ww.unregisterNotification(gWindowObserver);
  if (gDocElem) {
    gDocElem.removeEventListener("pageshow", onPageShowDefault, false);
  }

  finishTestDefaultWaitForWindowClosed();
}









function finishTestTimeout(aTimer) {
  ok(false, "Test timed out. Maximum time allowed is " + (gTestTimeout / 1000) +
     " seconds");

  try {
    finishTest();
  }
  catch (e) {
    finishTestDefault();
  }
}








function finishTestDefaultWaitForWindowClosed() {
  gCloseWindowTimeoutCounter++;
  if (gCloseWindowTimeoutCounter > CLOSE_WINDOW_TIMEOUT_MAXCOUNT) {
    SimpleTest.finish();
    return;
  }

  
  
  if (closeUpdateWindow()) {
    SimpleTest.executeSoon(finishTestDefaultWaitForWindowClosed);
  } else {
    SimpleTest.finish();
  }
}






function onPageShowDefault(aEvent) {
  if (!gTimeoutTimer) {
    debugDump("gTimeoutTimer is null... returning early");
    return;
  }

  
  
  if (aEvent.originalTarget.nodeName != "wizardpage") {
    debugDump("only handles events with an originalTarget nodeName of " +
              "|wizardpage|. aEvent.originalTarget.nodeName = " +
              aEvent.originalTarget.nodeName + "... returning early");
    return;
  }

  gTestCounter++;
  gCallback(aEvent);
}




function defaultCallback(aEvent) {
  if (!gTimeoutTimer) {
    debugDump("gTimeoutTimer is null... returning early");
    return;
  }

  debugDump("entering - TESTS[" + gTestCounter + "], pageid: " + gTest.pageid +
            ", aEvent.originalTarget.nodeName: " +
            aEvent.originalTarget.nodeName);

  if (gTest && gTest.extraStartFunction) {
    debugDump("calling extraStartFunction " + gTest.extraStartFunction.name);
    if (gTest.extraStartFunction(aEvent)) {
      debugDump("extraStartFunction early return");
      return;
    }
  }

  is(gDocElem.currentPage.pageid, gTest.pageid,
     "Checking currentPage.pageid equals " + gTest.pageid + " in pageshow");

  
  if (gTest.extraCheckFunction) {
    debugDump("calling extraCheckFunction " + gTest.extraCheckFunction.name);
    gTest.extraCheckFunction();
  }

  
  
  
  SimpleTest.executeSoon(delayedDefaultCallback);
}






function delayedDefaultCallback() {
  if (!gTimeoutTimer) {
    debugDump("gTimeoutTimer is null... returning early");
    return;
  }

  if (!gTest) {
    debugDump("gTest is null... returning early");
    return;
  }

  debugDump("entering - TESTS[" + gTestCounter + "], pageid: " + gTest.pageid);

  
  is(gDocElem.currentPage.pageid, gTest.pageid,
     "Checking currentPage.pageid equals " + gTest.pageid + " after " +
     "executeSoon");

  checkButtonStates();

  
  if (gTest.extraDelayedCheckFunction) {
    debugDump("calling extraDelayedCheckFunction " +
              gTest.extraDelayedCheckFunction.name);
    gTest.extraDelayedCheckFunction();
  }

  
  gTest.ranTest = true;

  if (gTest.buttonClick) {
    debugDump("clicking " + gTest.buttonClick + " button");
    if (gTest.extraDelayedFinishFunction) {
      throw("Tests cannot have a buttonClick and an extraDelayedFinishFunction property");
    }
    gDocElem.getButton(gTest.buttonClick).click();
  } else if (gTest.extraDelayedFinishFunction) {
    debugDump("calling extraDelayedFinishFunction " +
              gTest.extraDelayedFinishFunction.name);
    gTest.extraDelayedFinishFunction();
  }
}







function getContinueFile() {
  let continueFile = Cc["@mozilla.org/file/directory_service;1"].
                     getService(Ci.nsIProperties).
                     get("CurWorkD", Ci.nsILocalFile);
  let continuePath = REL_PATH_DATA + "/continue";
  let continuePathParts = continuePath.split("/");
  for (let i = 0; i < continuePathParts.length; ++i) {
    continueFile.append(continuePathParts[i]);
  }
  return continueFile;
}





function createContinueFile() {
  debugDump("creating 'continue' file for slow mar downloads");
  writeFile(getContinueFile(), "");
}





function removeContinueFile() {
  let continueFile = getContinueFile();
  if (continueFile.exists()) {
    debugDump("removing 'continue' file for slow mar downloads");
    continueFile.remove(false);
  }
}






function checkButtonStates() {
  debugDump("entering - TESTS[" + gTestCounter + "], pageid: " + gTest.pageid);

  const buttonNames = ["extra1", "extra2", "back", "next", "finish", "cancel"];
  let buttonStates = getExpectedButtonStates();
  buttonNames.forEach(function(aButtonName) {
    let button = gDocElem.getButton(aButtonName);
    let hasHidden = aButtonName in buttonStates &&
                    "hidden" in buttonStates[aButtonName];
    let hidden = hasHidden ? buttonStates[aButtonName].hidden : true;
    let hasDisabled = aButtonName in buttonStates &&
                      "disabled" in buttonStates[aButtonName];
    let disabled = hasDisabled ? buttonStates[aButtonName].disabled : true;
    is(button.hidden, hidden, "Checking " + aButtonName + " button " +
       "hidden attribute value equals " + (hidden ? "true" : "false"));
    is(button.disabled, disabled, "Checking " + aButtonName + " button " +
       "disabled attribute value equals " + (disabled ? "true" : "false"));
  });
}





function getExpectedButtonStates() {
  
  if (gTest.buttonStates) {
    return gTest.buttonStates;
  }

  switch (gTest.pageid) {
    case PAGEID_CHECKING:
    case PAGEID_INCOMPAT_CHECK:
      return { cancel: { disabled: false, hidden: false } };
    case PAGEID_FOUND_BASIC:
    case PAGEID_FOUND_BILLBOARD:
      if (gTest.neverButton) {
        return { extra1: { disabled: false, hidden: false },
                 extra2: { disabled: false, hidden: false },
                 next  : { disabled: false, hidden: false } }
      }
      return { extra1: { disabled: false, hidden: false },
               next  : { disabled: false, hidden: false } };
    case PAGEID_LICENSE:
      if (gRemoteContentState != "loaded" ||
          gAcceptDeclineLicense.selectedIndex != 0) {
        return { extra1: { disabled: false, hidden: false },
                 next  : { disabled: true, hidden: false } };
      }
      return { extra1: { disabled: false, hidden: false },
               next  : { disabled: false, hidden: false } };
    case PAGEID_INCOMPAT_LIST:
      return { extra1: { disabled: false, hidden: false },
               next  : { disabled: false, hidden: false } };
    case PAGEID_DOWNLOADING:
      return { extra1: { disabled: false, hidden: false } };
    case PAGEID_NO_UPDATES_FOUND:
    case PAGEID_MANUAL_UPDATE:
    case PAGEID_UNSUPPORTED:
    case PAGEID_ERRORS:
    case PAGEID_ERROR_EXTRA:
    case PAGEID_INSTALLED:
      return { finish: { disabled: false, hidden: false } };
    case PAGEID_ERROR_PATCHING:
      return { next  : { disabled: false, hidden: false } };
    case PAGEID_FINISHED:
    case PAGEID_FINISHED_BKGRD:
      return { extra1: { disabled: false, hidden: false },
               finish: { disabled: false, hidden: false } };
  }
  return null;
}




function addRemoteContentLoadListener() {
  debugDump("entering - TESTS[" + gTestCounter + "], pageid: " + gTest.pageid);

  gRemoteContent.addEventListener("load", remoteContentLoadListener, false);
}




function remoteContentLoadListener(aEvent) {
  
  if (aEvent.originalTarget.nodeName != "remotecontent") {
    debugDump("only handles events with an originalTarget nodeName of " +
              "|remotecontent|. aEvent.originalTarget.nodeName = " +
              aEvent.originalTarget.nodeName);
    return;
  }

  gTestCounter++;
  gCallback(aEvent);
}













function waitForRemoteContentLoaded(aEvent) {
  
  
  if (gRemoteContentState != gTest.expectedRemoteContentState ||
      aEvent.originalTarget != gRemoteContent) {
    debugDump("returning early. " +
              "gRemoteContentState: " +
              gRemoteContentState + ", " +
              "expectedRemoteContentState: " +
              gTest.expectedRemoteContentState + ", " +
              "aEvent.originalTarget.nodeName: " +
              aEvent.originalTarget.nodeName);
    return true;
  }

  gRemoteContent.removeEventListener("load", remoteContentLoadListener, false);
  return false;
}





function checkRemoteContentState() {
  is(gRemoteContentState, gTest.expectedRemoteContentState, "Checking remote " +
     "content state equals " + gTest.expectedRemoteContentState + " - pageid " +
     gTest.pageid);
}





function addRadioGroupSelectListenerAndClick() {
  debugDump("entering - TESTS[" + gTestCounter + "], pageid: " + gTest.pageid);

  gAcceptDeclineLicense.addEventListener("select", radioGroupSelectListener,
                                         false);
  gWin.document.getElementById(gTest.radioClick).click();
}




function radioGroupSelectListener(aEvent) {
  
  if (aEvent.originalTarget.nodeName != "radiogroup") {
    debugDump("only handles events with an originalTarget nodeName of " +
              "|radiogroup|. aEvent.originalTarget.nodeName = " +
              aEvent.originalTarget.nodeName);
    return;
  }

  gAcceptDeclineLicense.removeEventListener("select", radioGroupSelectListener,
                                            false);
  gTestCounter++;
  gCallback(aEvent);
}





function checkRadioGroupSelectedIndex() {
  is(gAcceptDeclineLicense.selectedIndex, gTest.expectedRadioGroupSelectedIndex,
     "Checking license radiogroup selectedIndex equals " +
     gTest.expectedRadioGroupSelectedIndex);
}





function checkIncompatbleList() {
  for (let i = 0; i < gIncompatibleListbox.itemCount; i++) {
    let label = gIncompatibleListbox.getItemAtIndex(i).label;
    
    ok(label.indexOf("noupdate") != -1, "Checking that only incompatible " + 
       "add-ons that don't have an update are listed in the incompatible list");
  }
}












function checkPrefHasUserValue(aPrefHasValue) {
  let prefHasUserValue = aPrefHasValue === undefined ? gTest.prefHasUserValue
                                                     : aPrefHasValue;
  is(Services.prefs.prefHasUserValue(gPrefToCheck), prefHasUserValue,
     "Checking prefHasUserValue for preference " + gPrefToCheck + " equals " +
     (prefHasUserValue ? "true" : "false"));
}













function checkErrorExtraPage(aShouldBeHidden) {
  let shouldBeHidden = aShouldBeHidden === undefined ? gTest.shouldBeHidden
                                                     : aShouldBeHidden;
  is(gWin.document.getElementById("errorExtraLinkLabel").hidden, shouldBeHidden,
     "Checking errorExtraLinkLabel hidden attribute equals " +
     (shouldBeHidden ? "true" : "false"));

  is(gWin.document.getElementById(gTest.displayedTextElem).hidden, false,
     "Checking " + gTest.displayedTextElem + " should not be hidden");

  ok(!Services.prefs.prefHasUserValue(PREF_APP_UPDATE_CERT_ERRORS),
     "Preference " + PREF_APP_UPDATE_CERT_ERRORS + " should not have a " +
     "user value");

  ok(!Services.prefs.prefHasUserValue(PREF_APP_UPDATE_BACKGROUNDERRORS),
     "Preference " + PREF_APP_UPDATE_BACKGROUNDERRORS + " should not have a " +
     "user value");
}














function getVersionParams(aAppVersion, aPlatformVersion) {
  let appInfo = Services.appinfo;
  return "&appVersion=" + (aAppVersion ? aAppVersion : appInfo.version) +
         "&platformVersion=" + (aPlatformVersion ? aPlatformVersion
                                                 : appInfo.platformVersion);
}








function getNewerAppVersion() {
  let appVersion = Services.appinfo.version.split(".")[0];
  appVersion++;
  return appVersion;
}








function getNewerPlatformVersion() {
  let platformVersion = Services.appinfo.platformVersion.split(".")[0];
  platformVersion++;
  return platformVersion;
}




function verifyTestsRan() {
  debugDump("entering");

  
  if (!TESTS) {
    return;
  }

  gTestCounter = -1;
  for (let i = 0; i < TESTS.length; ++i) {
    gTestCounter++;
    let test = TESTS[i];
    let msg = "Checking if TESTS[" + i + "] test was performed... " +
              "callback function name = " + gCallback.name + ", " +
              "pageid = " + test.pageid;
    ok(test.ranTest, msg);
  }
}







function resetUpdaterBackup() {
  let baseAppDir = getAppBaseDir();
  let updater = baseAppDir.clone();
  let updaterBackup = baseAppDir.clone();
  updater.append(FILE_UPDATER_BIN);
  updaterBackup.append(FILE_UPDATER_BIN_BAK);
  if (updaterBackup.exists()) {
    if (updater.exists()) {
      updater.remove(true);
    }
    updaterBackup.moveTo(baseAppDir, FILE_UPDATER_BIN);
  }
}





function setupFiles() {
  
  let baseAppDir = getAppBaseDir();
  let updateSettingsIni = baseAppDir.clone();
  updateSettingsIni.append(FILE_UPDATE_SETTINGS_INI);
  if (updateSettingsIni.exists()) {
    updateSettingsIni.moveTo(baseAppDir, FILE_UPDATE_SETTINGS_INI_BAK);
  }
  updateSettingsIni = baseAppDir.clone();
  updateSettingsIni.append(FILE_UPDATE_SETTINGS_INI);
  writeFile(updateSettingsIni, UPDATE_SETTINGS_CONTENTS);

  
  resetUpdaterBackup();

  
  let updater = baseAppDir.clone();
  updater.append(FILE_UPDATER_BIN);
  updater.moveTo(baseAppDir, FILE_UPDATER_BIN_BAK);

  
  let testUpdaterDir = Cc["@mozilla.org/file/directory_service;1"].
    getService(Ci.nsIProperties).
    get("CurWorkD", Ci.nsILocalFile);

  let relPath = REL_PATH_DATA;
  let pathParts = relPath.split("/");
  for (let i = 0; i < pathParts.length; ++i) {
    testUpdaterDir.append(pathParts[i]);
  }

  let testUpdater = testUpdaterDir.clone();
  testUpdater.append(FILE_UPDATER_BIN);
  if (testUpdater.exists()) {
    testUpdater.copyToFollowingLinks(baseAppDir, FILE_UPDATER_BIN);
  }
}







function setupPrefs() {
  if (DEBUG_AUS_TEST) {
    Services.prefs.setBoolPref(PREF_APP_UPDATE_LOG, true);
  }

  
  
  
  
  let now = Math.round(Date.now() / 1000) - 60;
  Services.prefs.setIntPref(PREF_APP_UPDATE_LASTUPDATETIME, now);
  Services.prefs.setIntPref(PREF_APP_UPDATE_INTERVAL, 43200);

  if (Services.prefs.prefHasUserValue(PREF_APP_UPDATE_URL_OVERRIDE)) {
    gAppUpdateURL = Services.prefs.getCharPref(PREF_APP_UPDATE_URL_OVERRIDE);
  }

  if (Services.prefs.prefHasUserValue(PREF_APP_UPDATE_ENABLED)) {
    gAppUpdateEnabled = Services.prefs.getBoolPref(PREF_APP_UPDATE_ENABLED);
  }
  Services.prefs.setBoolPref(PREF_APP_UPDATE_ENABLED, true);

  if (Services.prefs.prefHasUserValue(PREF_APP_UPDATE_SERVICE_ENABLED)) {
    gAppUpdateServiceEnabled = Services.prefs.getBoolPref(PREF_APP_UPDATE_SERVICE_ENABLED);
  }
  Services.prefs.setBoolPref(PREF_APP_UPDATE_SERVICE_ENABLED, false);

  if (Services.prefs.prefHasUserValue(PREF_APP_UPDATE_STAGING_ENABLED)) {
    gAppUpdateStagingEnabled = Services.prefs.getBoolPref(PREF_APP_UPDATE_STAGING_ENABLED);
  }
  Services.prefs.setBoolPref(PREF_APP_UPDATE_STAGING_ENABLED, false);

  if (Services.prefs.prefHasUserValue(PREF_EXTENSIONS_UPDATE_URL)) {
    gExtUpdateURL = Services.prefs.getCharPref(PREF_EXTENSIONS_UPDATE_URL);
  }
  let extUpdateUrl = URL_HTTP_UPDATE_XML + "?addonID=%ITEM_ID%" +
                     "&platformVersion=" + Services.appinfo.platformVersion +
                     "&newerPlatformVersion=" + getNewerPlatformVersion();
  Services.prefs.setCharPref(PREF_EXTENSIONS_UPDATE_URL, extUpdateUrl);

  Services.prefs.setIntPref(PREF_APP_UPDATE_IDLETIME, 0);
  Services.prefs.setIntPref(PREF_APP_UPDATE_PROMPTWAITTIME, 0);
  Services.prefs.setBoolPref(PREF_APP_UPDATE_SILENT, false);
  Services.prefs.setBoolPref(PREF_EXTENSIONS_STRICT_COMPAT, true);
  Services.prefs.setCharPref(PREF_EM_HOTFIX_ID, "hotfix" + ADDON_ID_SUFFIX);
}




function resetFiles() {
  
  let baseAppDir = getAppBaseDir();
  let updateSettingsIni = baseAppDir.clone();
  updateSettingsIni.append(FILE_UPDATE_SETTINGS_INI_BAK);
  if (updateSettingsIni.exists()) {
    updateSettingsIni.moveTo(baseAppDir, FILE_UPDATE_SETTINGS_INI);
  }

  
  
  
  let updatedDir;
  if (IS_MACOSX) {
    updatedDir = getUpdatesDir();
    updatedDir.append(DIR_PATCH);
  } else {
    updatedDir = getAppBaseDir();
  }
  updatedDir.append(DIR_UPDATED);
  if (updatedDir.exists()) {
    try {
      removeDirRecursive(updatedDir);
    }
    catch (e) {
      logTestInfo("Unable to remove directory. Path: " + updatedDir.path +
                  ", Exception: " + e);
    }
  }
  resetUpdaterBackup();
}




function resetPrefs() {
  if (gAppUpdateURL !== undefined) {
    Services.prefs.setCharPref(PREF_APP_UPDATE_URL_OVERRIDE, gAppUpdateURL);
  } else if (Services.prefs.prefHasUserValue(PREF_APP_UPDATE_URL_OVERRIDE)) {
    Services.prefs.clearUserPref(PREF_APP_UPDATE_URL_OVERRIDE);
  }

  if (gAppUpdateURLDefault) {
    gDefaultPrefBranch.setCharPref(PREF_APP_UPDATE_URL, gAppUpdateURLDefault);
  }

  if (gAppUpdateEnabled !== undefined) {
    Services.prefs.setBoolPref(PREF_APP_UPDATE_ENABLED, gAppUpdateEnabled);
  } else if (Services.prefs.prefHasUserValue(PREF_APP_UPDATE_ENABLED)) {
    Services.prefs.clearUserPref(PREF_APP_UPDATE_ENABLED);
  }

  if (gAppUpdateServiceEnabled !== undefined) {
    Services.prefs.setBoolPref(PREF_APP_UPDATE_SERVICE_ENABLED, gAppUpdateServiceEnabled);
  } else if (Services.prefs.prefHasUserValue(PREF_APP_UPDATE_SERVICE_ENABLED)) {
    Services.prefs.clearUserPref(PREF_APP_UPDATE_SERVICE_ENABLED);
  }

  if (gAppUpdateStagingEnabled !== undefined) {
    Services.prefs.setBoolPref(PREF_APP_UPDATE_STAGING_ENABLED, gAppUpdateStagingEnabled);
  } else if (Services.prefs.prefHasUserValue(PREF_APP_UPDATE_STAGING_ENABLED)) {
    Services.prefs.clearUserPref(PREF_APP_UPDATE_STAGING_ENABLED);
  }

  if (gExtUpdateURL !== undefined) {
    Services.prefs.setCharPref(PREF_EXTENSIONS_UPDATE_URL, gExtUpdateURL);
  } else if (Services.prefs.prefHasUserValue(PREF_EXTENSIONS_UPDATE_URL)) {
    Services.prefs.clearUserPref(PREF_EXTENSIONS_UPDATE_URL);
  }

  if (Services.prefs.prefHasUserValue(PREF_APP_UPDATE_IDLETIME)) {
    Services.prefs.clearUserPref(PREF_APP_UPDATE_IDLETIME);
  }

  if (Services.prefs.prefHasUserValue(PREF_APP_UPDATE_PROMPTWAITTIME)) {
    Services.prefs.clearUserPref(PREF_APP_UPDATE_PROMPTWAITTIME);
  }

  if (Services.prefs.prefHasUserValue(PREF_APP_UPDATE_URL_DETAILS)) {
    Services.prefs.clearUserPref(PREF_APP_UPDATE_URL_DETAILS);
  }

  if (Services.prefs.prefHasUserValue(PREF_APP_UPDATE_SHOW_INSTALLED_UI)) {
    Services.prefs.clearUserPref(PREF_APP_UPDATE_SHOW_INSTALLED_UI);
  }

  if (Services.prefs.prefHasUserValue(PREF_APP_UPDATE_NOTIFIEDUNSUPPORTED)) {
    Services.prefs.clearUserPref(PREF_APP_UPDATE_NOTIFIEDUNSUPPORTED);
  }

  if (Services.prefs.prefHasUserValue(PREF_APP_UPDATE_LOG)) {
    Services.prefs.clearUserPref(PREF_APP_UPDATE_LOG);
  }

  if (Services.prefs.prefHasUserValue(PREF_APP_UPDATE_CERT_ERRORS)) {
    Services.prefs.clearUserPref(PREF_APP_UPDATE_CERT_ERRORS);
  }

  if (Services.prefs.prefHasUserValue(PREF_APP_UPDATE_CERT_MAXERRORS)) {
    Services.prefs.clearUserPref(PREF_APP_UPDATE_CERT_MAXERRORS);
  }

  if (Services.prefs.prefHasUserValue(PREF_APP_UPDATE_BACKGROUNDERRORS)) {
    Services.prefs.clearUserPref(PREF_APP_UPDATE_BACKGROUNDERRORS);
  }

  if (Services.prefs.prefHasUserValue(PREF_APP_UPDATE_BACKGROUNDMAXERRORS)) {
    Services.prefs.clearUserPref(PREF_APP_UPDATE_BACKGROUNDMAXERRORS);
  }

  if (Services.prefs.prefHasUserValue(PREF_APP_UPDATE_CERT_INVALID_ATTR_NAME)) {
    Services.prefs.clearUserPref(PREF_APP_UPDATE_CERT_INVALID_ATTR_NAME);
  }

  if (Services.prefs.prefHasUserValue(PREF_APP_UPDATE_CERT_REQUIREBUILTIN)) {
    Services.prefs.clearUserPref(PREF_APP_UPDATE_CERT_REQUIREBUILTIN);
  }

  if (Services.prefs.prefHasUserValue(PREF_APP_UPDATE_CERT_CHECKATTRS)) {
    Services.prefs.clearUserPref(PREF_APP_UPDATE_CERT_CHECKATTRS);
  }

  try {
    CERT_ATTRS.forEach(function(aCertAttrName) {
      Services.prefs.clearUserPref(PREF_APP_UPDATE_CERTS_BRANCH + "1." +
                                   aCertAttrName);
    });
  }
  catch (e) {
  }

  try {
    Services.prefs.deleteBranch(PREF_APP_UPDATE_NEVER_BRANCH);
  }
  catch(e) {
  }

  if (Services.prefs.prefHasUserValue(PREF_APP_UPDATE_SILENT)) {
    Services.prefs.clearUserPref(PREF_APP_UPDATE_SILENT);
  }

  if (Services.prefs.prefHasUserValue(PREF_EXTENSIONS_STRICT_COMPAT)) {
		Services.prefs.clearUserPref(PREF_EXTENSIONS_STRICT_COMPAT);
  }

  if (Services.prefs.prefHasUserValue(PREF_EM_HOTFIX_ID)) {
    Services.prefs.clearUserPref(PREF_EM_HOTFIX_ID);
  }
}

function setupTimer(aTestTimeout) {
  gTestTimeout = aTestTimeout;
  if (gTimeoutTimer) {
    gTimeoutTimer.cancel();
    gTimeoutTimer = null;
  }
  gTimeoutTimer = Cc["@mozilla.org/timer;1"].
                  createInstance(Ci.nsITimer);
  gTimeoutTimer.initWithCallback(finishTestTimeout, gTestTimeout,
                                 Ci.nsITimer.TYPE_ONE_SHOT);
}











function setupAddons(aCallback) {
  debugDump("entering");

  
  
  
  function setNoUpdateAddonsDisabledState() {
    AddonManager.getAllAddons(function(aAddons) {
      aAddons.forEach(function(aAddon) {
        if (aAddon.name.startsWith("appdisabled")) {
          if (!aAddon.userDisabled) {
            aAddon.userDisabled = true;
          }
        }

        if (aAddon.name.startsWith("noupdate")) {
          if (aAddon.userDisabled != gDisableNoUpdateAddon) {
            aAddon.userDisabled = gDisableNoUpdateAddon;
          }
        }

        if (aAddon.name.startsWith("updatecompatibility")) {
          if (aAddon.userDisabled != gDisableUpdateCompatibilityAddon) {
            aAddon.userDisabled = gDisableUpdateCompatibilityAddon;
          }
        }

        if (aAddon.name.startsWith("updateversion")) {
          if (aAddon.userDisabled != gDisableUpdateVersionAddon) {
            aAddon.userDisabled = gDisableUpdateVersionAddon;
          }
        }
      });
      
      
      setupTimer(gTestTimeout);
      aCallback();
    });
  }

  
  
  
  
  
  if (Services.prefs.prefHasUserValue(PREF_DISABLEDADDONS)) {
    setNoUpdateAddonsDisabledState();
    return;
  }

  
  
  AddonManager.getAllAddons(function(aAddons) {
    let disabledAddons = [];
    aAddons.forEach(function(aAddon) {
      
      
      
      
      if (aAddon.type != "plugin" && !aAddon.appDisabled &&
          !aAddon.userDisabled &&
          aAddon.scope != AddonManager.SCOPE_APPLICATION) {
        disabledAddons.push(aAddon);
        aAddon.userDisabled = true;
      }
    });
    
    
    Services.prefs.setCharPref(PREF_DISABLEDADDONS, disabledAddons.join(" "));

    
    let xpiFiles = getTestAddonXPIFiles();
    let xpiCount = xpiFiles.length;
    let installs = [];
    xpiFiles.forEach(function(aFile) {
      AddonManager.getInstallForFile(aFile, function(aInstall) {
        if (!aInstall) {
          throw "No AddonInstall created for " + aFile.path;
        }

        installs.push(aInstall);

        if (--xpiCount == 0) {
          let installCount = installs.length;
          let installCompleted = function(aInstall) {
            aInstall.removeListener(listener);

            if (getAddonTestType(aInstall.addon.name) == "userdisabled") {
              aInstall.addon.userDisabled = true;
            }
            if (--installCount == 0) {
              setNoUpdateAddonsDisabledState();
            }
          };

          let listener = {
            onDownloadFailed: installCompleted,
            onDownloadCancelled: installCompleted,
            onInstallFailed: installCompleted,
            onInstallCancelled: installCompleted,
            onInstallEnded: installCompleted
          };

          installs.forEach(function(aInstall) {
            aInstall.addListener(listener);
            aInstall.install();
          });
        }
      });
    });
  });
}








function resetAddons(aCallback) {
  debugDump("entering");
  
  
  
  if (!Services.prefs.prefHasUserValue(PREF_DISABLEDADDONS)) {
    debugDump("preference " + PREF_DISABLEDADDONS + " doesn't exist... " +
              "returning early");
    aCallback();
    return;
  }

  
  let count = TEST_ADDONS.length;
  function uninstallCompleted(aAddon) {
    if (--count == 0) {
      AddonManager.removeAddonListener(listener);

      
      
      let disabledAddons = Services.prefs.getCharPref(PREF_DISABLEDADDONS).split(" ");
      Services.prefs.clearUserPref(PREF_DISABLEDADDONS);
      AddonManager.getAllAddons(function(aAddons) {
        aAddons.forEach(function(aAddon) {
          if (disabledAddons.indexOf(aAddon.id)) {
            aAddon.userDisabled = false;
          }
        });
        aCallback();
      });
    }
  }

  let listener = {
    onUninstalled: uninstallCompleted
  };

  AddonManager.addAddonListener(listener);
  TEST_ADDONS.forEach(function(aName) {
    AddonManager.getAddonByID(aName + ADDON_ID_SUFFIX, function(aAddon) {
      aAddon.uninstall();
    });
  });
}










function getAddonTestType(aName) {
  return aName.split("_")[0];
}







function getTestAddonXPIFiles() {
  let addonPrepDir = Services.dirsvc.get(NS_APP_USER_PROFILE_50_DIR,
                                         Ci.nsILocalFile);
  addonPrepDir.append(ADDON_PREP_DIR);

  let bootstrap = addonPrepDir.clone();
  bootstrap.append("bootstrap.js");
  
  if (!bootstrap.exists()) {
    let bootstrapContents = "function install(data, reason){ }\n" +
                            "function startup(data, reason){ }\n" +
                            "function shutdown(data, reason){ }\n" +
                            "function uninstall(data, reason){ }\n";
    writeFile(bootstrap, bootstrapContents);
  }

  let installRDF = addonPrepDir.clone();
  installRDF.append("install.rdf");

  let xpiFiles = [];
  TEST_ADDONS.forEach(function(aName) {
    let xpiFile = addonPrepDir.clone();
    xpiFile.append(aName + ".xpi");

    if (installRDF.exists()) {
      installRDF.remove(false);
    }
    writeFile(installRDF, getInstallRDFString(aName));
    gZipW.open(xpiFile, PR_RDWR | PR_CREATE_FILE | PR_TRUNCATE);
    gZipW.addEntryFile(installRDF.leafName,
                       Ci.nsIZipWriter.COMPRESSION_DEFAULT, installRDF,
                       false);
    gZipW.addEntryFile(bootstrap.leafName,
                       Ci.nsIZipWriter.COMPRESSION_DEFAULT, bootstrap,
                       false);
    gZipW.close();
    xpiFiles.push(xpiFile);
  });

  return xpiFiles;
}











function getInstallRDFString(aName) {
  let maxVersion = Services.appinfo.platformVersion;
  switch (getAddonTestType(aName)) {
    case "compatible":
      maxVersion = getNewerPlatformVersion();
      break;
    case "appdisabled":
      maxVersion = "0.1";
      break;
  }

  return "<?xml version=\"1.0\"?>\n" +
         "<RDF xmlns=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\"\n" +
         "  xmlns:em=\"http://www.mozilla.org/2004/em-rdf#\">\n" +
         "  <Description about=\"urn:mozilla:install-manifest\">\n" +
         "    <em:id>" + aName + ADDON_ID_SUFFIX + "</em:id>\n" +
         "    <em:version>1.0</em:version>\n" +
         "    <em:bootstrap>true</em:bootstrap>\n" +
         "    <em:name>" + aName + "</em:name>\n" +
         "    <em:description>Test Description</em:description>\n" +
         "    <em:targetApplication>\n" +
         "      <Description>\n" +
         "        <em:id>toolkit@mozilla.org</em:id>\n" +
         "        <em:minVersion>undefined</em:minVersion>\n" +
         "        <em:maxVersion>" + maxVersion + "</em:maxVersion>\n" +
         "      </Description>\n" +
         "    </em:targetApplication>\n" +
         "  </Description>\n" +
         "</RDF>";
}







function closeUpdateWindow() {
  let updateWindow = getUpdateWindow();
  if (!updateWindow) {
    return false;
  }

  ok(false, "Found an existing Update Window from the current or a previous " +
            "test... attempting to close it.");
  updateWindow.close();
  return true;
}







function getUpdateWindow() {
  return Services.wm.getMostRecentWindow(UPDATE_WINDOW_NAME);
}




const errorsPrefObserver = {
  observedPref: null,
  maxErrorPref: null,

  










  init: function(aObservePref, aMaxErrorPref, aMaxErrorCount) {
    this.observedPref = aObservePref;
    this.maxErrorPref = aMaxErrorPref;

    let maxErrors = aMaxErrorCount ? aMaxErrorCount : 2;
    Services.prefs.setIntPref(aMaxErrorPref, maxErrors);
    Services.prefs.addObserver(aObservePref, this, false);
  },

  


  observe: function XPI_observe(aSubject, aTopic, aData) {
    if (aData == this.observedPref) {
      let errCount = Services.prefs.getIntPref(this.observedPref);
      let errMax = Services.prefs.getIntPref(this.maxErrorPref);
      if (errCount >= errMax) {
        debugDump("removing pref observer");
        Services.prefs.removeObserver(this.observedPref, this);
      } else {
        debugDump("notifying AUS");
        SimpleTest.executeSoon(function() {
          gAUS.notify(null);
        });
      }
    }
  }
};




const gWindowObserver = {
  observe: function WO_observe(aSubject, aTopic, aData) {
    let win = aSubject.QueryInterface(Ci.nsIDOMEventTarget);

    if (aTopic == "domwindowclosed") {
      if (win.location != URI_UPDATE_PROMPT_DIALOG) {
        debugDump("domwindowclosed event for window not being tested - " +
                  "location: " + win.location + "... returning early");
        return;
      }
      
      
      try {
        finishTest();
      }
      catch (e) {
        finishTestDefault();
      }
      return;
    }

    win.addEventListener("load", function WO_observe_onLoad() {
      win.removeEventListener("load", WO_observe_onLoad, false);
      
      if (win.location != URI_UPDATE_PROMPT_DIALOG) {
        debugDump("load event for window not being tested - location: " +
                  win.location + "... returning early");
        return;
      }

      
      let pageid = win.document.documentElement.currentPage.pageid;
      if (pageid != PAGEID_DUMMY) {
        
        
        ok(false, "Unexpected load event - pageid got: " + pageid +
           ", expected: " + PAGEID_DUMMY + "... returning early");
        return;
      }

      gWin = win;
      gDocElem = gWin.document.documentElement;
      gDocElem.addEventListener("pageshow", onPageShowDefault, false);
    }, false);
  }
};
