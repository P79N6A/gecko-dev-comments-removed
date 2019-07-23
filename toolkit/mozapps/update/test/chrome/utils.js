







































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

const URL_HOST = "http://example.com/";
const URL_PATH = "chrome/toolkit/mozapps/update/test/chrome";
const URL_UPDATE = URL_HOST + URL_PATH + "/update.sjs";

const URI_UPDATE_PROMPT_DIALOG  = "chrome://mozapps/content/update/updates.xul";

const CRC_ERROR = 4;

var qUpdateChannel;
var gWin;
var gDocElem;
var gPageId;
var gNextFunc;

#include ../shared.js




function runTestDefault() {
  ok(true, "Entering runTestDefault");

  closeUpdateWindow();

  gWW.registerNotification(gWindowObserver);
  setUpdateChannel();
  test01();
}




function finishTestDefault() {
  ok(true, "Entering finishTestDefault");

  gWW.unregisterNotification(gWindowObserver);

  resetPrefs();
  removeUpdateDirsAndFiles();
  reloadUpdateManagerData();
  SimpleTest.finish();
}

__defineGetter__("gWW", function() {
  delete this.gWW;
  return this.gWW = AUS_Cc["@mozilla.org/embedcomp/window-watcher;1"].
                      getService(AUS_Ci.nsIWindowWatcher);
});





function closeUpdateWindow() {
  var updateWindow = getUpdateWindow();
  if (!updateWindow)
    return;

  ok(true, "Found Update Window!!! Attempting to close it.");
  updateWindow.close();
}




function getUpdateWindow() {
  var wm = AUS_Cc["@mozilla.org/appshell/window-mediator;1"].
           getService(AUS_Ci.nsIWindowMediator);
  return wm.getMostRecentWindow("Update:Wizard");
}

var gWindowObserver = {
  observe: function(aSubject, aTopic, aData) {
    var win = aSubject.QueryInterface(AUS_Ci.nsIDOMEventTarget);

    if (aTopic == "domwindowclosed") {
      if (win.location == URI_UPDATE_PROMPT_DIALOG)
        gNextFunc();
      return;
    }

    win.addEventListener("load", function onLoad() {
      if (win.location != URI_UPDATE_PROMPT_DIALOG)
        return;

      gWin = win;
      gWin.removeEventListener("load", onLoad, false);
      gDocElem = gWin.document.documentElement;

      
      
      
      
      
      if (gDocElem.currentPage && gDocElem.currentPage.pageid == gPageId)
        gNextFunc();
      else
        addPageShowListener();
    }, false);
  }
};

function nextFuncListener(aEvent) {
  gNextFunc(aEvent);
}

function addPageShowListener() {
  var page = gDocElem.getPageById(gPageId);
  page.addEventListener("pageshow", function onPageShow(aEvent) {
    page.removeEventListener("pageshow", onPageShow, false);
    
    SimpleTest.executeSoon(gNextFunc);
  }, false);
}

function resetPrefs() {
  if (qUpdateChannel)
    setUpdateChannel(qUpdateChannel);
  if (gPref.prefHasUserValue(PREF_APP_UPDATE_IDLETIME))
    gPref.clearUserPref(PREF_APP_UPDATE_IDLETIME);
  if (gPref.prefHasUserValue(PREF_APP_UPDATE_ENABLED))
    gPref.clearUserPref(PREF_APP_UPDATE_ENABLED);
  if (gPref.prefHasUserValue(PREF_APP_UPDATE_LOG))
    gPref.clearUserPref(PREF_APP_UPDATE_LOG);
  if (gPref.prefHasUserValue(PREF_APP_UPDATE_SHOW_INSTALLED_UI))
    gPref.clearUserPref(PREF_APP_UPDATE_SHOW_INSTALLED_UI);
  if (gPref.prefHasUserValue(PREF_APP_UPDATE_URL_DETAILS))
    gPref.clearUserPref(PREF_APP_UPDATE_URL_DETAILS);
  if (gPref.prefHasUserValue(PREF_APP_UPDATE_URL_OVERRIDE))
    gPref.clearUserPref(PREF_APP_UPDATE_URL_OVERRIDE);
  try {
    gPref.deleteBranch(PREF_APP_UPDATE_NEVER_BRANCH);    
  }
  catch(e) {
  }
}
