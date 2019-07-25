











































































const PAGEID_DUMMY            = "dummy";                 
const PAGEID_CHECKING         = "checking";              
const PAGEID_PLUGIN_UPDATES   = "pluginupdatesfound";
const PAGEID_NO_UPDATES_FOUND = "noupdatesfound";        
const PAGEID_MANUAL_UPDATE    = "manualUpdate"; 
const PAGEID_INCOMPAT_CHECK   = "incompatibleCheck"; 
const PAGEID_FOUND_BASIC      = "updatesfoundbasic";     
const PAGEID_FOUND_BILLBOARD  = "updatesfoundbillboard"; 
const PAGEID_LICENSE          = "license";               
const PAGEID_INCOMPAT_LIST    = "incompatibleList"; 
const PAGEID_DOWNLOADING      = "downloading";           
const PAGEID_ERRORS           = "errors";                
const PAGEID_ERROR_PATCHING   = "errorpatching";         
const PAGEID_FINISHED         = "finished";              
const PAGEID_FINISHED_BKGRD   = "finishedBackground";    
const PAGEID_INSTALLED        = "installed";             

const UPDATE_WINDOW_NAME = "Update:Wizard";

const URL_HOST   = "http://example.com/";
const URL_PATH   = "chrome/toolkit/mozapps/update/test/chrome";
const URL_UPDATE = URL_HOST + URL_PATH + "/update.sjs";

const URI_UPDATE_PROMPT_DIALOG  = "chrome://mozapps/content/update/updates.xul";

const CRC_ERROR = 4;

const DEBUG_DUMP = false;

const TEST_TIMEOUT = 30000; 
var gTimeoutTimer;

var gTestCounter = -1;
var gUpdateChannel;
var gDocElem;
var gWin;
var gPrefToCheck;

#include ../shared.js

function debugDump(msg) {
  if (DEBUG_DUMP) {
    dump("*** " + msg + "\n");
  }
}




__defineGetter__("gTest", function() {
  return gTests[gTestCounter];
});






__defineGetter__("gCallback", function() {
    return gTest.overrideCallback ? gTest.overrideCallback
                                  : defaultCallback;
});





__defineGetter__("gRemoteContent", function() {
  switch (gTest.pageid) {
    case PAGEID_FOUND_BILLBOARD:
      return gWin.document.getElementById("updateMoreInfoContent");
    case PAGEID_LICENSE:
      return gWin.document.getElementById("licenseContent");
  }
  return null;
});





__defineGetter__("gRemoteContentState", function() {
  if (gRemoteContent) {
    return gRemoteContent.getAttribute("state");
  }
  return null;
});




__defineGetter__("gAcceptDeclineLicense", function() {
  return gWin.document.getElementById("acceptDeclineLicense");
});




function runTestDefault() {
  debugDump("Entering runTestDefault");

  SimpleTest.waitForExplicitFinish();

  Services.ww.registerNotification(gWindowObserver);
  setUpdateChannel();
  Services.prefs.setIntPref(PREF_APP_UPDATE_IDLETIME, 0);

  removeUpdateDirsAndFiles
  reloadUpdateManagerData();

  runTest();
}




function finishTestDefault() {
  debugDump("Entering finishTestDefault");

  gDocElem.removeEventListener("pageshow", onPageShowDefault, false);

  if (gTimeoutTimer) {
    gTimeoutTimer.cancel();
    gTimeoutTimer = null;
  }

  verifyTestsRan();

  Services.ww.unregisterNotification(gWindowObserver);

  resetPrefs();
  removeUpdateDirsAndFiles();
  reloadUpdateManagerData();
  SimpleTest.finish();
}









function finishTestTimeout(aTimer) {
  gTimeoutTimer = null;
  ok(false, "Test timed out. Maximum time allowed is " + (TEST_TIMEOUT / 1000) +
     " seconds");
  gWin.close();
}






function onPageShowDefault(aEvent) {
  
  
  if (aEvent.originalTarget.nodeName != "wizardpage") {
    debugDump("onPageShowDefault - only handles events with an " +
              "originalTarget nodeName of |wizardpage|. " +
              "aEvent.originalTarget.nodeName = " +
              aEvent.originalTarget.nodeName + "... returning early");
    return;
  }

  gTestCounter++;
  gCallback(aEvent);
}




function defaultCallback(aEvent) {
  debugDump("Entering defaultCallback - gTests[" + gTestCounter + "], " +
            "pageid: " + gTest.pageid + ", " +
            "aEvent.originalTarget.nodeName: " + aEvent.originalTarget.nodeName);

  if (gTest && gTest.extraStartFunction) {
    debugDump("defaultCallback - calling extraStartFunction " +
              gTest.extraStartFunction.name);
    if (gTest.extraStartFunction(aEvent)) {
      debugDump("defaultCallback - extraStartFunction early return");
      return;
    }
  }

  is(gDocElem.currentPage.pageid, gTest.pageid,
     "Checking currentPage.pageid equals " + gTest.pageid + " in onPageShow");

  
  if (gTest.extraCheckFunction) {
    debugDump("delayedCallback - calling extraCheckFunction " +
              gTest.extraCheckFunction.name);
    gTest.extraCheckFunction();
  }

  
  
  
  SimpleTest.executeSoon(delayedDefaultCallback);
}






function delayedDefaultCallback() {
  debugDump("Entering delayedDefaultCallback - gTests[" + gTestCounter + "], " +
            "pageid: " + gTest.pageid);

  
  is(gDocElem.currentPage.pageid, gTest.pageid,
     "Checking currentPage.pageid equals " + gTest.pageid + " after " +
     "executeSoon");

  checkButtonStates();

  
  if (gTest.extraDelayedCheckFunction) {
    debugDump("delayedDefaultCallback - calling extraDelayedCheckFunction " +
              gTest.extraDelayedCheckFunction.name);
    gTest.extraDelayedCheckFunction();
  }

  
  gTest.ranTest = true;

  if (gTest.buttonClick) {
    debugDump("delayedDefaultCallback - clicking " + gTest.buttonClick +
              " button");
    if(gTest.extraDelayedFinishFunction) {
      throw("Tests cannot have a buttonClick and an extraDelayedFinishFunction property");
    }
    gDocElem.getButton(gTest.buttonClick).click();
  }
  else if (gTest.extraDelayedFinishFunction) {
    debugDump("delayedDefaultCallback - calling extraDelayedFinishFunction " +
              gTest.extraDelayedFinishFunction.name);
    gTest.extraDelayedFinishFunction();
  }
}






function checkButtonStates() {
  debugDump("Entering checkButtonStates - gTests[" + gTestCounter + "], " +
            "pageid: " + gTest.pageid);

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
    case PAGEID_ERRORS:
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
  debugDump("Entering addRemoteContentLoadListener - gTests[" + gTestCounter +
            "], pageid: " + gTest.pageid);

  gRemoteContent.addEventListener("load", remoteContentLoadListener, false);
}




function remoteContentLoadListener(aEvent) {
  
  if (aEvent.originalTarget.nodeName != "remotecontent") {
    debugDump("remoteContentLoadListener - only handles events with an " +
              "originalTarget nodeName of |remotecontent|. " +
              "aEvent.originalTarget.nodeName = " +
              aEvent.originalTarget.nodeName);
    return;
  }

  gTestCounter++;
  gCallback(aEvent);
}













function waitForRemoteContentLoaded(aEvent) {
  
  
  if (gRemoteContentState != gTest.expectedRemoteContentState ||
      !aEvent.originalTarget.isSameNode(gRemoteContent)) {
    debugDump("waitForRemoteContentLoaded - returning early\n" +
              "gRemoteContentState: " + gRemoteContentState + "\n" +
              "expectedRemoteContentState: " +
              gTest.expectedRemoteContentState + "\n" +
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
  debugDump("Entering addRadioGroupSelectListenerAndClick - gTests[" +
            gTestCounter + "], pageid: " + gTest.pageid);

  gAcceptDeclineLicense.addEventListener("select", radioGroupSelectListener,
                                         false);
  gWin.document.getElementById(gTest.radioClick).click();
}




function radioGroupSelectListener(aEvent) {
  
  if (aEvent.originalTarget.nodeName != "radiogroup") {
    debugDump("remoteContentLoadListener - only handles events with an " +
              "originalTarget nodeName of |radiogroup|. " +
              "aEvent.originalTarget.nodeName = " +
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












function checkPrefHasUserValue(aPrefHasValue) {
  var prefHasUserValue = aPrefHasValue === undefined ? gTest.prefHasUserValue
                                                     : aPrefHasValue;
  is(Services.prefs.prefHasUserValue(gPrefToCheck), prefHasUserValue,
     "Checking prefHasUserValue for preference " + gPrefToCheck + " equals " +
     (prefHasUserValue ? "true" : "false"));
}













function getVersionParams(aAppVersion, aPlatformVersion) {
  let appInfo = Services.appinfo;
  return "&appVersion=" + (aAppVersion ? aAppVersion : appInfo.version) +
         "&platformVersion=" + (aPlatformVersion ? aPlatformVersion
                                                 : appInfo.platformVersion);
}




function verifyTestsRan() {
  debugDump("Entering verifyTestsRan");

  
  if (!gTests) {
    return;
  }

  gTestCounter = -1;
  for (let i = 0; i < gTests.length; ++i) {
    gTestCounter++;
    let test = gTests;
    let msg = "Checking if gTests[" + i + "] test was performed... " +
              "callback function name = " + gCallback.name + "," +
              "pageid = " + (gTest.pageid ? gTest.pageid : "N/A");
    ok(gTest.ranTest, msg);
  }
}




function resetPrefs() {
  if (gUpdateChannel) {
    setUpdateChannel(gUpdateChannel);
  }
  if (Services.prefs.prefHasUserValue(PREF_APP_UPDATE_IDLETIME)) {
    Services.prefs.clearUserPref(PREF_APP_UPDATE_IDLETIME);
  }
  if (Services.prefs.prefHasUserValue(PREF_APP_UPDATE_ENABLED)) {
    Services.prefs.clearUserPref(PREF_APP_UPDATE_ENABLED);
  }
  if (Services.prefs.prefHasUserValue(PREF_APP_UPDATE_LOG)) {
    Services.prefs.clearUserPref(PREF_APP_UPDATE_LOG);
  }
  if (Services.prefs.prefHasUserValue(PREF_APP_UPDATE_SHOW_INSTALLED_UI)) {
    Services.prefs.clearUserPref(PREF_APP_UPDATE_SHOW_INSTALLED_UI);
  }
  if (Services.prefs.prefHasUserValue(PREF_APP_UPDATE_URL_DETAILS)) {
    Services.prefs.clearUserPref(PREF_APP_UPDATE_URL_DETAILS);
  }
  if (Services.prefs.prefHasUserValue(PREF_APP_UPDATE_URL_OVERRIDE)) {
    Services.prefs.clearUserPref(PREF_APP_UPDATE_URL_OVERRIDE);
  }
  if (Services.prefs.prefHasUserValue(PREF_APP_UPDATE_IDLETIME)) {
    Services.prefs.clearUserPref(PREF_APP_UPDATE_IDLETIME);
  }
  try {
    Services.prefs.deleteBranch(PREF_APP_UPDATE_NEVER_BRANCH);
  }
  catch(e) {
  }
}




function closeUpdateWindow() {
  let updateWindow = getUpdateWindow();
  if (!updateWindow)
    return;

  ok(false, "Found an existing Update Window from a previous test... " +
            "attempting to close it.");
  updateWindow.close();
}







function getUpdateWindow() {
  return Services.wm.getMostRecentWindow(UPDATE_WINDOW_NAME);
}




var gWindowObserver = {
  loaded: false,

  observe: function WO_observe(aSubject, aTopic, aData) {
    let win = aSubject.QueryInterface(AUS_Ci.nsIDOMEventTarget);

    if (aTopic == "domwindowclosed") {
      if (win.location == URI_UPDATE_PROMPT_DIALOG) {
        
        
        try {
          finishTest();
        }
        catch (e) {
          finishTestDefault();
        }
      }
      return;
    }

    
    if (this.loaded) {
      
      
      ok(false, "Unexpected gWindowObserver:observe - called with aTopic = " +
         aTopic + "... returning early");
      return;
    }

    win.addEventListener("load", function onLoad() {
      
      
      if (win.location != URI_UPDATE_PROMPT_DIALOG) {
        
        ok(false, "Unexpected load event - win.location got: " + location +
           ", expected: " + URI_UPDATE_PROMPT_DIALOG + "... returning early");
        return;
      }

      
      
      let pageid = win.document.documentElement.currentPage.pageid;
      if (pageid != PAGEID_DUMMY) {
        
        
        ok(false, "Unexpected load event - pageid got: " + pageid +
           ", expected: " + PAGEID_DUMMY + "... returning early");
        return;
      }

      win.removeEventListener("load", onLoad, false);
      gTimeoutTimer = AUS_Cc["@mozilla.org/timer;1"].
                      createInstance(AUS_Ci.nsITimer);
      gTimeoutTimer.initWithCallback(finishTestTimeout, TEST_TIMEOUT,
                                     AUS_Ci.nsITimer.TYPE_ONE_SHOT);

      gWin = win;
      gDocElem = gWin.document.documentElement;
      gDocElem.addEventListener("pageshow", onPageShowDefault, false);
    }, false);

    this.loaded = true;
  }
};
