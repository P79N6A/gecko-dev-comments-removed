






















































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

const STATE_STOPPED = 0;
const STATE_RUNNING = 1;
const STATE_QUITTING = -1;

const STATE_STOPPED_STR = "stopped";
const STATE_RUNNING_STR = "running";

const PRIVACY_NONE = 0;
const PRIVACY_ENCRYPTED = 1;
const PRIVACY_FULL = 2;

const NOTIFY_WINDOWS_RESTORED = "sessionstore-windows-restored";


const OBSERVING = [
  "domwindowopened", "domwindowclosed",
  "quit-application-requested", "quit-application-granted",
  "browser-lastwindow-close-granted",
  "quit-application", "browser:purge-session-history",
  "private-browsing", "browser:purge-domain-data",
  "private-browsing-change-granted"
];





const WINDOW_ATTRIBUTES = ["width", "height", "screenX", "screenY", "sizemode"];





const WINDOW_HIDEABLE_FEATURES = [
  "menubar", "toolbar", "locationbar", 
  "personalbar", "statusbar", "scrollbars"
];









const CAPABILITIES = [
  "Subframes", "Plugins", "Javascript", "MetaRedirects", "Images",
  "DNSPrefetch", "Auth"
];

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

function debug(aMsg) {
  aMsg = ("SessionStore: " + aMsg).replace(/\S{80}/g, "$&\n");
  Cc["@mozilla.org/consoleservice;1"].getService(Ci.nsIConsoleService)
                                     .logStringMessage(aMsg);
}

__defineGetter__("NetUtil", function() {
  delete this.NetUtil;
  Cu.import("resource://gre/modules/NetUtil.jsm");
  return NetUtil;
});



function SessionStoreService() {
}

SessionStoreService.prototype = {
  classDescription: "Browser Session Store Service",
  contractID: "@mozilla.org/browser/sessionstore;1",
  classID: Components.ID("{5280606b-2510-4fe0-97ef-9b5a22eafe6b}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsISessionStore,
                                         Ci.nsIDOMEventListener,
                                         Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference]),

  
  
  xulAttributes: ["image"],

  
  _loadState: STATE_STOPPED,

  
  _interval: 10000,

  
  _resume_from_crash: true,

  
  _restoreCount: 0,

  
  _lastSaveTime: 0, 

  
  _windows: {},

  
  _closedWindows: [],

  
  _dirtyWindows: {},

  
  _statesToRestore: {},

  
  _recentCrashes: 0,

  
  _inPrivateBrowsing: false,

  
  _clearingOnShutdown: false,

#ifndef XP_MACOSX
  
  _restoreLastWindow: false,
#endif



  


  init: function sss_init(aWindow) {
    if (!aWindow || this._loadState == STATE_RUNNING) {
      
      
      if (aWindow && (!aWindow.__SSi || !this._windows[aWindow.__SSi]))
        this.onLoad(aWindow);
      return;
    }

    this._prefBranch = Cc["@mozilla.org/preferences-service;1"].
                       getService(Ci.nsIPrefService).getBranch("browser.");
    this._prefBranch.QueryInterface(Ci.nsIPrefBranch2);

    this._observerService = Cc["@mozilla.org/observer-service;1"].
                            getService(Ci.nsIObserverService);

    OBSERVING.forEach(function(aTopic) {
      this._observerService.addObserver(this, aTopic, true);
    }, this);

    var pbs = Cc["@mozilla.org/privatebrowsing;1"].
              getService(Ci.nsIPrivateBrowsingService);
    this._inPrivateBrowsing = pbs.privateBrowsingEnabled;

    
    this._interval = this._prefBranch.getIntPref("sessionstore.interval");
    this._prefBranch.addObserver("sessionstore.interval", this, true);
    
    
    this._resume_from_crash = this._prefBranch.getBoolPref("sessionstore.resume_from_crash");
    this._prefBranch.addObserver("sessionstore.resume_from_crash", this, true);
    
    
    this._prefBranch.addObserver("sessionstore.max_tabs_undo", this, true);
    this._prefBranch.addObserver("sessionstore.max_windows_undo", this, true);
    
    
    this._sessionhistory_max_entries =
      this._prefBranch.getIntPref("sessionhistory.max_entries");

    
    var dirService = Cc["@mozilla.org/file/directory_service;1"].
                     getService(Ci.nsIProperties);
    this._sessionFile = dirService.get("ProfD", Ci.nsILocalFile);
    this._sessionFileBackup = this._sessionFile.clone();
    this._sessionFile.append("sessionstore.js");
    this._sessionFileBackup.append("sessionstore.bak");

    
    var iniString;
    try {
      var ss = Cc["@mozilla.org/browser/sessionstartup;1"].
               getService(Ci.nsISessionStartup);
      if (ss.doRestore())
        iniString = ss.state;
    }
    catch(ex) { dump(ex + "\n"); } 

    if (iniString) {
      try {
        
        this._initialState = this._safeEval("(" + iniString + ")");
        
        let lastSessionCrashed =
          this._initialState.session && this._initialState.session.state &&
          this._initialState.session.state == STATE_RUNNING_STR;
        if (lastSessionCrashed) {
          this._recentCrashes = (this._initialState.session &&
                                 this._initialState.session.recentCrashes || 0) + 1;
          
          if (this._needsRestorePage(this._initialState, this._recentCrashes)) {
            
            let pageData = {
              url: "about:sessionrestore",
              formdata: { "#sessionData": iniString }
            };
            this._initialState = { windows: [{ tabs: [{ entries: [pageData] }] }] };
          }
        }
        
        
        delete this._initialState.windows[0].hidden;
      }
      catch (ex) { debug("The session file is invalid: " + ex); }
    }

    
    if (!this._resume_from_crash)
      this._clearDisk();
    else { 
      try {
        if (this._sessionFileBackup.exists())
          this._sessionFileBackup.remove(false);
        if (this._sessionFile.exists())
          this._sessionFile.copyTo(null, this._sessionFileBackup.leafName);
      }
      catch (ex) { Cu.reportError(ex); } 
    }

    
    
    if (this._loadState != STATE_QUITTING &&
        this._prefBranch.getBoolPref("sessionstore.resume_session_once"))
      this._prefBranch.setBoolPref("sessionstore.resume_session_once", false);
    
    
    this.onLoad(aWindow);
  },

  



  _uninit: function sss_uninit() {
    if (this._doResumeSession()) { 
      this.saveState(true);
    }
    else { 
      this._clearDisk();
    }
    
    if (this._saveTimer) {
      this._saveTimer.cancel();
      this._saveTimer = null;
    }
  },

  


  observe: function sss_observe(aSubject, aTopic, aData) {
    
    var _this = this;

    switch (aTopic) {
    case "domwindowopened": 
      aSubject.addEventListener("load", function(aEvent) {
        aEvent.currentTarget.removeEventListener("load", arguments.callee, false);
        _this.onLoad(aEvent.currentTarget);
        }, false);
      break;
    case "domwindowclosed": 
      this.onClose(aSubject);
      break;
    case "quit-application-requested":
      
      this._forEachBrowserWindow(function(aWindow) {
        this._collectWindowData(aWindow);
      });
      this._dirtyWindows = [];
      break;
    case "quit-application-granted":
      
      this._loadState = STATE_QUITTING;
      break;
#ifndef XP_MACOSX
    case "browser-lastwindow-close-granted":
      
      
      
      
      this._restoreLastWindow = true;
      break;
#endif
    case "quit-application":
      if (aData == "restart") {
        this._prefBranch.setBoolPref("sessionstore.resume_session_once", true);
        this._clearingOnShutdown = false;
      }
      this._loadState = STATE_QUITTING; 
      this._uninit();
      break;
    case "browser:purge-session-history": 
      let openWindows = {};
      this._forEachBrowserWindow(function(aWindow) {
        Array.forEach(aWindow.gBrowser.browsers, function(aBrowser) {
          delete aBrowser.__SS_data;
        });
        openWindows[aWindow.__SSi] = true;
      });
      
      for (let ix in this._windows) {
        if (ix in openWindows)
          this._windows[ix]._closedTabs = [];
        else
          delete this._windows[ix];
      }
      
      this._closedWindows = [];
      this._clearDisk();
      
      var win = this._getMostRecentBrowserWindow();
      if (win)
        win.setTimeout(function() { _this.saveState(true); }, 0);
      else if (this._loadState == STATE_RUNNING)
        this.saveState(true);
      
      if ("_stateBackup" in this)
        delete this._stateBackup;
      if (this._loadState == STATE_QUITTING)
        this._clearingOnShutdown = true;
      break;
    case "browser:purge-domain-data":
      
      function containsDomain(aEntry) {
        try {
          if (this._getURIFromString(aEntry.url).host.hasRootDomain(aData))
            return true;
        }
        catch (ex) {  }
        return aEntry.children && aEntry.children.some(containsDomain, this);
      }
      
      for (let ix in this._windows) {
        let closedTabs = this._windows[ix]._closedTabs;
        for (let i = closedTabs.length - 1; i >= 0; i--) {
          if (closedTabs[i].state.entries.some(containsDomain, this))
            closedTabs.splice(i, 1);
        }
      }
      
      
      for (let ix = this._closedWindows.length - 1; ix >= 0; ix--) {
        let closedTabs = this._closedWindows[ix]._closedTabs;
        let openTabs = this._closedWindows[ix].tabs;
        let openTabCount = openTabs.length;
        for (let i = closedTabs.length - 1; i >= 0; i--)
          if (closedTabs[i].state.entries.some(containsDomain, this))
            closedTabs.splice(i, 1);
        for (let j = openTabs.length - 1; j >= 0; j--) {
          if (openTabs[j].entries.some(containsDomain, this)) {
            openTabs.splice(j, 1);
            if (this._closedWindows[ix].selected > j)
              this._closedWindows[ix].selected--;
          }
        }
        if (openTabs.length == 0) {
          this._closedWindows.splice(ix, 1);
        }
        else if (openTabs.length != openTabCount) {
          
          let selectedTab = openTabs[this._closedWindows[ix].selected - 1];
          
          let activeIndex = (selectedTab.index || selectedTab.entries.length) - 1;
          if (activeIndex >= selectedTab.entries.length)
            activeIndex = selectedTab.entries.length - 1;
          this._closedWindows[ix].title = selectedTab.entries[activeIndex].title;
        }
      }
      if (this._loadState == STATE_RUNNING)
        this.saveState(true);
      break;
    case "nsPref:changed": 
      switch (aData) {
      
      
      case "sessionstore.max_tabs_undo":
        for (let ix in this._windows) {
          this._windows[ix]._closedTabs.splice(this._prefBranch.getIntPref("sessionstore.max_tabs_undo"));
        }
        break;
      case "sessionstore.max_windows_undo":
        this._capClosedWindows();
        break;
      case "sessionstore.interval":
        this._interval = this._prefBranch.getIntPref("sessionstore.interval");
        
        if (this._saveTimer) {
          this._saveTimer.cancel();
          this._saveTimer = null;
        }
        this.saveStateDelayed(null, -1);
        break;
      case "sessionstore.resume_from_crash":
        this._resume_from_crash = this._prefBranch.getBoolPref("sessionstore.resume_from_crash");
        
        
        if (this._resume_from_crash)
          this.saveState(true);
        else if (this._loadState == STATE_RUNNING)
          this._clearDisk();
        break;
      }
      break;
    case "timer-callback": 
      this._saveTimer = null;
      this.saveState();
      break;
    case "private-browsing":
      switch (aData) {
      case "enter":
        this._inPrivateBrowsing = true;
        break;
      case "exit":
        aSubject.QueryInterface(Ci.nsISupportsPRBool);
        let quitting = aSubject.data;
        if (quitting) {
          
          
          if ("_stateBackup" in this) {
            var oState = this._stateBackup;
            oState.session = { state: STATE_STOPPED_STR };

            this._saveStateObject(oState);
          }
          
          this._prefBranch.setBoolPref("sessionstore.resume_session_once", true);
        }
        else
          this._inPrivateBrowsing = false;
        delete this._stateBackup;
        break;
      }
      break;
    case "private-browsing-change-granted":
      if (aData == "enter") {
        this.saveState(true);
        this._stateBackup = this._safeEval(this._getCurrentState(true).toSource());
      }
      break;
    }
  },



  


  handleEvent: function sss_handleEvent(aEvent) {
    var win = aEvent.currentTarget.ownerDocument.defaultView;
    switch (aEvent.type) {
      case "load":
      case "pageshow":
        this.onTabLoad(win, aEvent.currentTarget, aEvent);
        break;
      case "change":
      case "input":
      case "DOMAutoComplete":
        this.onTabInput(win, aEvent.currentTarget);
        break;
      case "scroll":
        this.onTabScroll(win);
        break;
      case "TabOpen":
      case "TabClose":
        let browser = aEvent.originalTarget.linkedBrowser;
        if (aEvent.type == "TabOpen") {
          this.onTabAdd(win, browser);
        }
        else {
          
          if (!aEvent.detail)
            this.onTabClose(win, aEvent.originalTarget);
          this.onTabRemove(win, browser);
        }
        break;
      case "TabSelect":
        this.onTabSelect(win);
        break;
    }
  },

  








  onLoad: function sss_onLoad(aWindow) {
    
    if (aWindow && aWindow.__SSi && this._windows[aWindow.__SSi])
      return;

    
    if (aWindow.document.documentElement.getAttribute("windowtype") != "navigator:browser" ||
      this._loadState == STATE_QUITTING)
      return;

    
    aWindow.__SSi = "window" + Date.now();

    
    this._windows[aWindow.__SSi] = { tabs: [], selected: 0, _closedTabs: [] };
    if (!aWindow.toolbar.visible)
      this._windows[aWindow.__SSi].isPopup = true;
    
    
    if (this._loadState == STATE_STOPPED) {
      this._loadState = STATE_RUNNING;
      this._lastSaveTime = Date.now();
      
      
      if (this._initialState) {
        
        this._initialState._firstTabs = true;
        this._restoreCount = this._initialState.windows ? this._initialState.windows.length : 0;
        this.restoreWindow(aWindow, this._initialState, this._isCmdLineEmpty(aWindow));
        delete this._initialState;
        
        
        
        this.saveState(true);
      }
      else {
        
        this._observerService.notifyObservers(null, NOTIFY_WINDOWS_RESTORED, "");
        
        
        this._lastSaveTime -= this._interval;
      }
    }
    
    else if (!this._isWindowLoaded(aWindow)) {
      let followUp = this._statesToRestore[aWindow.__SS_restoreID].windows.length == 1;
      this.restoreWindow(aWindow, this._statesToRestore[aWindow.__SS_restoreID], true, followUp);
    }
#ifndef XP_MACOSX
    else if (this._restoreLastWindow && aWindow.toolbar.visible &&
             this._closedWindows.length && this._doResumeSession() &&
             !this._inPrivateBrowsing) {

      
      
      let state = null;
      this._closedWindows = this._closedWindows.filter(function(aWinState) {
        if (!state && !aWinState.isPopup) {
          state = aWinState;
          return false;
        }
        return true;
      });
      if (state) {
        delete state.hidden;
        state = { windows: [state] };
        this._restoreCount = 1;
        this.restoreWindow(aWindow, state, this._isCmdLineEmpty(aWindow));
      }
      
      this._prefBranch.setBoolPref("sessionstore.resume_session_once", false);
    }
    if (this._restoreLastWindow && aWindow.toolbar.visible) {
      
      
      
      this._restoreLastWindow = false;
    }
#endif

    var tabbrowser = aWindow.gBrowser;
    
    
    for (let i = 0; i < tabbrowser.browsers.length; i++) {
      this.onTabAdd(aWindow, tabbrowser.browsers[i], true);
    }
    
    tabbrowser.addEventListener("TabOpen", this, true);
    tabbrowser.addEventListener("TabClose", this, true);
    tabbrowser.addEventListener("TabSelect", this, true);
  },

  






  onClose: function sss_onClose(aWindow) {
    
    let isFullyLoaded = this._isWindowLoaded(aWindow);
    if (!isFullyLoaded) {
      if (!aWindow.__SSi)
        aWindow.__SSi = "window" + Date.now();
      this._window[aWindow.__SSi] = this._statesToRestore[aWindow.__SS_restoreID];
      delete this._statesToRestore[aWindow.__SS_restoreID];
      delete aWindow.__SS_restoreID;
    }
    
    
    if (!aWindow.__SSi || !this._windows[aWindow.__SSi]) {
      return;
    }
    
    if (this.windowToFocus && this.windowToFocus == aWindow) {
      delete this.windowToFocus;
    }
    
    var tabbrowser = aWindow.gBrowser;

    tabbrowser.removeEventListener("TabOpen", this, true);
    tabbrowser.removeEventListener("TabClose", this, true);
    tabbrowser.removeEventListener("TabSelect", this, true);
    
    let winData = this._windows[aWindow.__SSi];
    if (this._loadState == STATE_RUNNING) { 
      
      this._collectWindowData(aWindow);
      
      if (isFullyLoaded) {
        winData.title = aWindow.content.document.title || tabbrowser.selectedTab.label;
        winData.title = this._replaceLoadingTitle(winData.title, tabbrowser,
                                                  tabbrowser.selectedTab);
        this._updateCookies([winData]);
      }
      
      
      if (winData.tabs.length > 1 ||
          (winData.tabs.length == 1 && winData.tabs[0].entries.length > 0)) {
        this._closedWindows.unshift(winData);
        this._capClosedWindows();
      }
      
      
      delete this._windows[aWindow.__SSi];
      
      
      this.saveStateDelayed();
    }
    
    for (let i = 0; i < tabbrowser.browsers.length; i++) {
      this.onTabRemove(aWindow, tabbrowser.browsers[i], true);
    }
    
    
    aWindow.__SS_dyingCache = winData;
    
    delete aWindow.__SSi;
  },

  








  onTabAdd: function sss_onTabAdd(aWindow, aBrowser, aNoNotification) {
    aBrowser.addEventListener("load", this, true);
    aBrowser.addEventListener("pageshow", this, true);
    aBrowser.addEventListener("change", this, true);
    aBrowser.addEventListener("input", this, true);
    aBrowser.addEventListener("DOMAutoComplete", this, true);
    aBrowser.addEventListener("scroll", this, true);
    
    if (!aNoNotification) {
      this.saveStateDelayed(aWindow);
    }
  },

  








  onTabRemove: function sss_onTabRemove(aWindow, aBrowser, aNoNotification) {
    aBrowser.removeEventListener("load", this, true);
    aBrowser.removeEventListener("pageshow", this, true);
    aBrowser.removeEventListener("change", this, true);
    aBrowser.removeEventListener("input", this, true);
    aBrowser.removeEventListener("DOMAutoComplete", this, true);
    aBrowser.removeEventListener("scroll", this, true);
    
    delete aBrowser.__SS_data;
    
    if (!aNoNotification) {
      this.saveStateDelayed(aWindow);
    }
  },

  






  onTabClose: function sss_onTabClose(aWindow, aTab) {
    
    
    var event = aWindow.document.createEvent("Events");
    event.initEvent("SSTabClosing", true, false);
    aTab.dispatchEvent(event);
    
    var maxTabsUndo = this._prefBranch.getIntPref("sessionstore.max_tabs_undo");
    
    if (maxTabsUndo == 0) {
      return;
    }
    
    
    var tabState = this._collectTabData(aTab);
    this._updateTextAndScrollDataForTab(aWindow, aTab.linkedBrowser, tabState);

    
    if (tabState.entries.length > 0) {
      let tabTitle = aTab.label;
      let tabbrowser = aWindow.gBrowser;
      tabTitle = this._replaceLoadingTitle(tabTitle, tabbrowser, aTab);
      
      this._windows[aWindow.__SSi]._closedTabs.unshift({
        state: tabState,
        title: tabTitle,
        image: aTab.getAttribute("image"),
        pos: aTab._tPos
      });
      var length = this._windows[aWindow.__SSi]._closedTabs.length;
      if (length > maxTabsUndo)
        this._windows[aWindow.__SSi]._closedTabs.splice(maxTabsUndo, length - maxTabsUndo);
    }
  },

  








  onTabLoad: function sss_onTabLoad(aWindow, aBrowser, aEvent) { 
    
    
    if (aEvent.type != "load" && !aEvent.persisted) {
      return;
    }
    
    delete aBrowser.__SS_data;
    this.saveStateDelayed(aWindow);
    
    
    this._updateCrashReportURL(aWindow);
  },

  






  onTabInput: function sss_onTabInput(aWindow, aBrowser) {
    if (aBrowser.__SS_data)
      delete aBrowser.__SS_data._formDataSaved;
    
    this.saveStateDelayed(aWindow, 3000);
  },

  




  onTabScroll: function sss_onTabScroll(aWindow) {
    this.saveStateDelayed(aWindow, 3000);
  },

  




  onTabSelect: function sss_onTabSelect(aWindow) {
    if (this._loadState == STATE_RUNNING) {
      this._windows[aWindow.__SSi].selected = aWindow.gBrowser.tabContainer.selectedIndex;
      this.saveStateDelayed(aWindow);

      
      this._updateCrashReportURL(aWindow);
    }
  },



  getBrowserState: function sss_getBrowserState() {
    return this._toJSONString(this._getCurrentState());
  },

  setBrowserState: function sss_setBrowserState(aState) {
    try {
      var state = this._safeEval("(" + aState + ")");
    }
    catch (ex) {  }
    if (!state || !state.windows)
      throw (Components.returnCode = Cr.NS_ERROR_INVALID_ARG);
    
    var window = this._getMostRecentBrowserWindow();
    if (!window) {
      this._openWindowWithState(state);
      return;
    }

    
    this._forEachBrowserWindow(function(aWindow) {
      if (aWindow != window) {
        aWindow.close();
      }
    });

    
    this._closedWindows = [];

    
    this.restoreWindow(window, state, true);
  },

  getWindowState: function sss_getWindowState(aWindow) {
    if (!aWindow.__SSi && !aWindow.__SS_dyingCache)
      throw (Components.returnCode = Cr.NS_ERROR_INVALID_ARG);
    
    if (!aWindow.__SSi)
      return this._toJSONString({ windows: [aWindow.__SS_dyingCache] });
    return this._toJSONString(this._getWindowState(aWindow));
  },

  setWindowState: function sss_setWindowState(aWindow, aState, aOverwrite) {
    if (!aWindow.__SSi)
      throw (Components.returnCode = Cr.NS_ERROR_INVALID_ARG);
    
    this.restoreWindow(aWindow, "(" + aState + ")", aOverwrite);
  },

  getTabState: function sss_getTabState(aTab) {
    if (!aTab.ownerDocument || !aTab.ownerDocument.defaultView.__SSi)
      throw (Components.returnCode = Cr.NS_ERROR_INVALID_ARG);
    
    var tabState = this._collectTabData(aTab);
    
    var window = aTab.ownerDocument.defaultView;
    this._updateTextAndScrollDataForTab(window, aTab.linkedBrowser, tabState);
    
    return this._toJSONString(tabState);
  },

  setTabState: function sss_setTabState(aTab, aState) {
    var tabState = this._safeEval("(" + aState + ")");
    if (!tabState.entries || !aTab.ownerDocument || !aTab.ownerDocument.defaultView.__SSi)
      throw (Components.returnCode = Cr.NS_ERROR_INVALID_ARG);
    
    var window = aTab.ownerDocument.defaultView;
    this.restoreHistoryPrecursor(window, [aTab], [tabState], 0, 0, 0);
  },

  duplicateTab: function sss_duplicateTab(aWindow, aTab) {
    if (!aTab.ownerDocument || !aTab.ownerDocument.defaultView.__SSi ||
        !aWindow.getBrowser)
      throw (Components.returnCode = Cr.NS_ERROR_INVALID_ARG);
    
    var tabState = this._collectTabData(aTab, true);
    var sourceWindow = aTab.ownerDocument.defaultView;
    this._updateTextAndScrollDataForTab(sourceWindow, aTab.linkedBrowser, tabState, true);
    
    var newTab = aWindow.gBrowser.addTab();
    this.restoreHistoryPrecursor(aWindow, [newTab], [tabState], 0, 0, 0);
    
    return newTab;
  },

  getClosedTabCount: function sss_getClosedTabCount(aWindow) {
    if (!aWindow.__SSi && aWindow.__SS_dyingCache)
      return aWindow.__SS_dyingCache._closedTabs.length;
    if (!aWindow.__SSi)
      
      return 0; 
    
    return this._windows[aWindow.__SSi]._closedTabs.length;
  },

  getClosedTabData: function sss_getClosedTabDataAt(aWindow) {
    if (!aWindow.__SSi && !aWindow.__SS_dyingCache)
      throw (Components.returnCode = Cr.NS_ERROR_INVALID_ARG);
    
    if (!aWindow.__SSi)
      return this._toJSONString(aWindow.__SS_dyingCache._closedTabs);
    return this._toJSONString(this._windows[aWindow.__SSi]._closedTabs);
  },

  undoCloseTab: function sss_undoCloseTab(aWindow, aIndex) {
    if (!aWindow.__SSi)
      throw (Components.returnCode = Cr.NS_ERROR_INVALID_ARG);
    
    var closedTabs = this._windows[aWindow.__SSi]._closedTabs;

    
    aIndex = aIndex || 0;
    if (!(aIndex in closedTabs))
      throw (Components.returnCode = Cr.NS_ERROR_INVALID_ARG);
    
    
    let closedTab = closedTabs.splice(aIndex, 1).shift();
    let closedTabState = closedTab.state;

    
    let browser = aWindow.gBrowser;
    let tab = browser.addTab();
      
    
    browser.moveTabTo(tab, closedTab.pos);

    
    this.restoreHistoryPrecursor(aWindow, [tab], [closedTabState], 1, 0, 0);

    
    let content = browser.getBrowserForTab(tab).contentWindow;
    aWindow.setTimeout(function() { content.focus(); }, 0);
    
    return tab;
  },

  forgetClosedTab: function sss_forgetClosedTab(aWindow, aIndex) {
    if (!aWindow.__SSi)
      throw (Components.returnCode = Cr.NS_ERROR_INVALID_ARG);
    
    var closedTabs = this._windows[aWindow.__SSi]._closedTabs;

    
    aIndex = aIndex || 0;
    if (!(aIndex in closedTabs))
      throw (Components.returnCode = Cr.NS_ERROR_INVALID_ARG);
    
    
    closedTabs.splice(aIndex, 1);
  },

  getClosedWindowCount: function sss_getClosedWindowCount() {
    return this._closedWindows.length;
  },

  getClosedWindowData: function sss_getClosedWindowData() {
    return this._toJSONString(this._closedWindows);
  },

  undoCloseWindow: function sss_undoCloseWindow(aIndex) {
    if (!(aIndex in this._closedWindows))
      throw (Components.returnCode = Cr.NS_ERROR_INVALID_ARG);

    
    let state = { windows: this._closedWindows.splice(aIndex, 1) };
    let window = this._openWindowWithState(state);
    this.windowToFocus = window;
    return window;
  },

  forgetClosedWindow: function sss_forgetClosedWindow(aIndex) {
    
    aIndex = aIndex || 0;
    if (!(aIndex in this._closedWindows))
      throw (Components.returnCode = Cr.NS_ERROR_INVALID_ARG);
    
    
    this._closedWindows.splice(aIndex, 1);
  },

  getWindowValue: function sss_getWindowValue(aWindow, aKey) {
    if (aWindow.__SSi) {
      var data = this._windows[aWindow.__SSi].extData || {};
      return data[aKey] || "";
    }
    if (aWindow.__SS_dyingCache) {
      data = aWindow.__SS_dyingCache.extData || {};
      return data[aKey] || "";
    }
    throw (Components.returnCode = Cr.NS_ERROR_INVALID_ARG);
  },

  setWindowValue: function sss_setWindowValue(aWindow, aKey, aStringValue) {
    if (aWindow.__SSi) {
      if (!this._windows[aWindow.__SSi].extData) {
        this._windows[aWindow.__SSi].extData = {};
      }
      this._windows[aWindow.__SSi].extData[aKey] = aStringValue;
      this.saveStateDelayed(aWindow);
    }
    else {
      throw (Components.returnCode = Cr.NS_ERROR_INVALID_ARG);
    }
  },

  deleteWindowValue: function sss_deleteWindowValue(aWindow, aKey) {
    if (aWindow.__SSi && this._windows[aWindow.__SSi].extData &&
        this._windows[aWindow.__SSi].extData[aKey])
      delete this._windows[aWindow.__SSi].extData[aKey];
    else
      throw (Components.returnCode = Cr.NS_ERROR_INVALID_ARG);
  },

  getTabValue: function sss_getTabValue(aTab, aKey) {
    var data = aTab.__SS_extdata || {};
    return data[aKey] || "";
  },

  setTabValue: function sss_setTabValue(aTab, aKey, aStringValue) {
    if (!aTab.__SS_extdata) {
      aTab.__SS_extdata = {};
    }
    aTab.__SS_extdata[aKey] = aStringValue;
    this.saveStateDelayed(aTab.ownerDocument.defaultView);
  },

  deleteTabValue: function sss_deleteTabValue(aTab, aKey) {
    if (aTab.__SS_extdata && aTab.__SS_extdata[aKey])
      delete aTab.__SS_extdata[aKey];
    else
      throw (Components.returnCode = Cr.NS_ERROR_INVALID_ARG);
  },

  persistTabAttribute: function sss_persistTabAttribute(aName) {
    if (this.xulAttributes.indexOf(aName) != -1)
      return; 
    
    this.xulAttributes.push(aName);
    this.saveStateDelayed();
  },



  




  _saveWindowHistory: function sss_saveWindowHistory(aWindow) {
    var tabbrowser = aWindow.gBrowser;
    var tabs = tabbrowser.mTabs;
    var tabsData = this._windows[aWindow.__SSi].tabs = [];
    
    for (var i = 0; i < tabs.length; i++)
      tabsData.push(this._collectTabData(tabs[i]));
    
    this._windows[aWindow.__SSi].selected = tabbrowser.mTabBox.selectedIndex + 1;
  },

  







  _collectTabData: function sss_collectTabData(aTab, aFullData) {
    var tabData = { entries: [] };
    var browser = aTab.linkedBrowser;
    
    if (!browser || !browser.currentURI)
      
      return tabData;
    else if (browser.__SS_data && browser.__SS_data._tabStillLoading)
      
      return browser.__SS_data;
    
    var history = null;
    try {
      history = browser.sessionHistory;
    }
    catch (ex) { } 
    
    
    
    if (history && browser.__SS_data &&
        browser.__SS_data.entries[history.index] &&
        history.index < this._sessionhistory_max_entries - 1 && !aFullData) {
      tabData = browser.__SS_data;
      tabData.index = history.index + 1;
    }
    else if (history && history.count > 0) {
      for (var j = 0; j < history.count; j++)
        tabData.entries.push(this._serializeHistoryEntry(history.getEntryAtIndex(j, false),
                                                         aFullData));
      tabData.index = history.index + 1;

      
      if (!aFullData)
        browser.__SS_data = tabData;
    }
    else if (browser.currentURI.spec != "about:blank" ||
             browser.contentDocument.body.hasChildNodes()) {
      tabData.entries[0] = { url: browser.currentURI.spec };
      tabData.index = 1;
    }
    else if (browser.currentURI.spec == "about:blank" &&
             browser.userTypedValue) {
      
      
      
      tabData.entries[0] = { url: browser.userTypedValue };
      tabData.index = 1;
    }
    
    var disallow = [];
    for (var i = 0; i < CAPABILITIES.length; i++)
      if (!browser.docShell["allow" + CAPABILITIES[i]])
        disallow.push(CAPABILITIES[i]);
    if (disallow.length > 0)
      tabData.disallow = disallow.join(",");
    else if (tabData.disallow)
      delete tabData.disallow;
    
    if (this.xulAttributes.length > 0) {
      tabData.attributes = {};
      Array.forEach(aTab.attributes, function(aAttr) {
        if (this.xulAttributes.indexOf(aAttr.name) > -1)
          tabData.attributes[aAttr.name] = aAttr.value;
      }, this);
    }
    
    if (aTab.__SS_extdata)
      tabData.extData = aTab.__SS_extdata;
    else if (tabData.extData)
      delete tabData.extData;
    
    if (history && browser.docShell instanceof Ci.nsIDocShell)
      this._serializeSessionStorage(tabData, history, browser.docShell, aFullData);
    
    return tabData;
  },

  








  _serializeHistoryEntry: function sss_serializeHistoryEntry(aEntry, aFullData) {
    var entry = { url: aEntry.URI.spec };
    
    if (aEntry.title && aEntry.title != entry.url) {
      entry.title = aEntry.title;
    }
    if (aEntry.isSubFrame) {
      entry.subframe = true;
    }
    if (!(aEntry instanceof Ci.nsISHEntry)) {
      return entry;
    }
    
    var cacheKey = aEntry.cacheKey;
    if (cacheKey && cacheKey instanceof Ci.nsISupportsPRUint32 &&
        cacheKey.data != 0) {
      
      
      entry.cacheKey = cacheKey.data;
    }
    entry.ID = aEntry.ID;
    
    if (aEntry.referrerURI)
      entry.referrer = aEntry.referrerURI.spec;

    if (aEntry.contentType)
      entry.contentType = aEntry.contentType;
    
    var x = {}, y = {};
    aEntry.getScrollPosition(x, y);
    if (x.value != 0 || y.value != 0)
      entry.scroll = x.value + "," + y.value;
    
    try {
      var prefPostdata = this._prefBranch.getIntPref("sessionstore.postdata");
      if (aEntry.postData && (aFullData ||
            prefPostdata && this._checkPrivacyLevel(aEntry.URI.schemeIs("https")))) {
        aEntry.postData.QueryInterface(Ci.nsISeekableStream).
                        seek(Ci.nsISeekableStream.NS_SEEK_SET, 0);
        var stream = Cc["@mozilla.org/binaryinputstream;1"].
                     createInstance(Ci.nsIBinaryInputStream);
        stream.setInputStream(aEntry.postData);
        var postBytes = stream.readByteArray(stream.available());
        var postdata = String.fromCharCode.apply(null, postBytes);
        if (aFullData || prefPostdata == -1 ||
            postdata.replace(/^(Content-.*\r\n)+(\r\n)*/, "").length <=
              prefPostdata) {
          
          
          
          entry.postdata_b64 = btoa(postdata);
        }
      }
    }
    catch (ex) { debug(ex); } 

    if (aEntry.owner) {
      
      
      try {
        var binaryStream = Cc["@mozilla.org/binaryoutputstream;1"].
                           createInstance(Ci.nsIObjectOutputStream);
        var pipe = Cc["@mozilla.org/pipe;1"].createInstance(Ci.nsIPipe);
        pipe.init(false, false, 0, 0xffffffff, null);
        binaryStream.setOutputStream(pipe.outputStream);
        binaryStream.writeCompoundObject(aEntry.owner, Ci.nsISupports, true);
        binaryStream.close();

        
        var scriptableStream = Cc["@mozilla.org/binaryinputstream;1"].
                               createInstance(Ci.nsIBinaryInputStream);
        scriptableStream.setInputStream(pipe.inputStream);
        var ownerBytes =
          scriptableStream.readByteArray(scriptableStream.available());
        
        
        
        entry.owner_b64 = btoa(String.fromCharCode.apply(null, ownerBytes));
      }
      catch (ex) { debug(ex); }
    }
    
    if (!(aEntry instanceof Ci.nsISHContainer)) {
      return entry;
    }
    
    if (aEntry.childCount > 0) {
      entry.children = [];
      for (var i = 0; i < aEntry.childCount; i++) {
        var child = aEntry.GetChildAt(i);
        if (child) {
          entry.children.push(this._serializeHistoryEntry(child, aFullData));
        }
        else { 
          entry.children.push({ url: "about:blank" });
        }
        
        if (/^wyciwyg:\/\//.test(entry.children[i].url)) {
          delete entry.children;
          break;
        }
      }
    }
    
    return entry;
  },

  










  _serializeSessionStorage:
    function sss_serializeSessionStorage(aTabData, aHistory, aDocShell, aFullData) {
    let storageData = {};
    let hasContent = false;

    for (let i = 0; i < aHistory.count; i++) {
      let uri = aHistory.getEntryAtIndex(i, false).URI;
      
      let domain = uri.spec;
      try {
        if (uri.host)
          domain = uri.prePath;
      }
      catch (ex) {  }
      if (storageData[domain] || !(aFullData || this._checkPrivacyLevel(uri.schemeIs("https"))))
        continue;

      let storage, storageItemCount = 0;
      try {
        var principal = Cc["@mozilla.org/scriptsecuritymanager;1"].
                        getService(Ci.nsIScriptSecurityManager).
                        getCodebasePrincipal(uri);

        
        
        
        
        
        storage = aDocShell.getSessionStorageForPrincipal(principal, false);
        if (storage)
          storageItemCount = storage.length;
      }
      catch (ex) {  }
      if (storageItemCount == 0)
        continue;

      let data = storageData[domain] = {};
      for (let j = 0; j < storageItemCount; j++) {
        try {
          let key = storage.key(j);
          let item = storage.getItem(key);
          data[key] = item;
        }
        catch (ex) {  }
      }
      hasContent = true;
    }

    if (hasContent)
      aTabData.storage = storageData;
  },

  





  _updateTextAndScrollData: function sss_updateTextAndScrollData(aWindow) {
    var browsers = aWindow.gBrowser.browsers;
    for (var i = 0; i < browsers.length; i++) {
      try {
        var tabData = this._windows[aWindow.__SSi].tabs[i];
        if (browsers[i].__SS_data &&
            browsers[i].__SS_data._tabStillLoading)
          continue; 
        this._updateTextAndScrollDataForTab(aWindow, browsers[i], tabData);
      }
      catch (ex) { debug(ex); } 
    }
  },

  











  _updateTextAndScrollDataForTab:
    function sss_updateTextAndScrollDataForTab(aWindow, aBrowser, aTabData, aFullData) {
    var tabIndex = (aTabData.index || aTabData.entries.length) - 1;
    
    if (!aTabData.entries[tabIndex])
      return;
    
    let selectedPageStyle = aBrowser.markupDocumentViewer.authorStyleDisabled ? "_nostyle" :
                            this._getSelectedPageStyle(aBrowser.contentWindow);
    if (selectedPageStyle)
      aTabData.pageStyle = selectedPageStyle;
    else if (aTabData.pageStyle)
      delete aTabData.pageStyle;
    
    this._updateTextAndScrollDataForFrame(aWindow, aBrowser.contentWindow,
                                          aTabData.entries[tabIndex],
                                          !aTabData._formDataSaved, aFullData);
    aTabData._formDataSaved = true;
    if (aBrowser.currentURI.spec == "about:config")
      aTabData.entries[tabIndex].formdata = {
        "#textbox": aBrowser.contentDocument.getElementById("textbox").wrappedJSObject.value
      };
  },

  













  _updateTextAndScrollDataForFrame:
    function sss_updateTextAndScrollDataForFrame(aWindow, aContent, aData,
                                                 aUpdateFormData, aFullData) {
    for (var i = 0; i < aContent.frames.length; i++) {
      if (aData.children && aData.children[i])
        this._updateTextAndScrollDataForFrame(aWindow, aContent.frames[i],
                                              aData.children[i], aUpdateFormData, aFullData);
    }
    var isHTTPS = this._getURIFromString((aContent.parent || aContent).
                                         document.location.href).schemeIs("https");
    if (aFullData || this._checkPrivacyLevel(isHTTPS) ||
        aContent.top.document.location.href == "about:sessionrestore") {
      if (aFullData || aUpdateFormData) {
        let formData = this._collectFormDataForFrame(aContent.document);
        if (formData)
          aData.formdata = formData;
        else if (aData.formdata)
          delete aData.formdata;
      }
      
      
      if ((aContent.document.designMode || "") == "on") {
        if (aData.innerHTML === undefined && !aFullData) {
          
          let _this = this;
          aContent.addEventListener("keypress", function(aEvent) {
            _this.saveStateDelayed(aWindow, 3000);
          }, true);
        }
        aData.innerHTML = aContent.document.body.innerHTML;
      }
    }

    
    
    let domWindowUtils = aContent.QueryInterface(Ci.nsIInterfaceRequestor)
                                 .getInterface(Ci.nsIDOMWindowUtils);
    let scrollX = {}, scrollY = {};
    domWindowUtils.getScrollXY(false, scrollX, scrollY);
    aData.scroll = scrollX.value + "," + scrollY.value;
  },

  





  _getSelectedPageStyle: function sss_getSelectedPageStyle(aContent) {
    const forScreen = /(?:^|,)\s*(?:all|screen)\s*(?:,|$)/i;
    for (let i = 0; i < aContent.document.styleSheets.length; i++) {
      let ss = aContent.document.styleSheets[i];
      let media = ss.media.mediaText;
      if (!ss.disabled && ss.title && (!media || forScreen.test(media)))
        return ss.title
    }
    for (let i = 0; i < aContent.frames.length; i++) {
      let selectedPageStyle = this._getSelectedPageStyle(aContent.frames[i]);
      if (selectedPageStyle)
        return selectedPageStyle;
    }
    return "";
  },

  




  _collectFormDataForFrame: function sss_collectFormDataForFrame(aDocument) {
    let formNodes = aDocument.evaluate(XPathHelper.restorableFormNodes, aDocument,
                                       XPathHelper.resolveNS,
                                       Ci.nsIDOMXPathResult.UNORDERED_NODE_ITERATOR_TYPE, null);
    let node = formNodes.iterateNext();
    if (!node)
      return null;
    
    const MAX_GENERATED_XPATHS = 100;
    let generatedCount = 0;
    
    let data = {};
    do {
      
      if (!node.id && ++generatedCount > MAX_GENERATED_XPATHS)
        continue;
      
      let id = node.id ? "#" + node.id : XPathHelper.generate(node);
      if (node instanceof Ci.nsIDOMHTMLInputElement) {
        if (node.type != "file")
          data[id] = node.type == "checkbox" || node.type == "radio" ? node.checked : node.value;
        else
          data[id] = { type: "file", value: node.value };
      }
      else if (node instanceof Ci.nsIDOMHTMLTextAreaElement)
        data[id] = node.value;
      else if (!node.multiple)
        data[id] = node.selectedIndex;
      else {
        let options = Array.map(node.options, function(aOpt, aIx) aOpt.selected ? aIx : -1);
        data[id] = options.filter(function(aIx) aIx >= 0);
      }
    } while ((node = formNodes.iterateNext()));
    
    return data;
  },

  




  _updateCookieHosts: function sss_updateCookieHosts(aWindow) {
    var hosts = this._windows[aWindow.__SSi]._hosts = {};
    
    
    function extractHosts(aEntry) {
      if (/^https?:\/\/(?:[^@\/\s]+@)?([\w.-]+)/.test(aEntry.url)) {
        if (!hosts[RegExp.$1] && _this._checkPrivacyLevel(_this._getURIFromString(aEntry.url).schemeIs("https"))) {
          hosts[RegExp.$1] = true;
        }
      }
      else if (/^file:\/\/([^\/]*)/.test(aEntry.url)) {
        hosts[RegExp.$1] = true;
      }
      if (aEntry.children) {
        aEntry.children.forEach(extractHosts);
      }
    }

    var _this = this;
    this._windows[aWindow.__SSi].tabs.forEach(function(aTabData) { aTabData.entries.forEach(extractHosts); });
  },

  




  _updateCookies: function sss_updateCookies(aWindows) {
    function addCookieToHash(aHash, aHost, aPath, aName, aCookie) {
      
      
      if (!aHash[aHost])
        aHash[aHost] = {};
      if (!aHash[aHost][aPath])
        aHash[aHost][aPath] = {};
      aHash[aHost][aPath][aName] = aCookie;
    }

    var cm = Cc["@mozilla.org/cookiemanager;1"].getService(Ci.nsICookieManager2);
    
    for (var i = 0; i < aWindows.length; i++)
      aWindows[i].cookies = [];

    var jscookies = {};
    var _this = this;
    
    var MAX_EXPIRY = Math.pow(2, 62);
    aWindows.forEach(function(aWindow) {
      for (var host in aWindow._hosts) {
        var list = cm.getCookiesFromHost(host);
        while (list.hasMoreElements()) {
          var cookie = list.getNext().QueryInterface(Ci.nsICookie2);
          if (cookie.isSession && _this._checkPrivacyLevel(cookie.isSecure)) {
            
            
            if (!(cookie.host in jscookies &&
                  cookie.path in jscookies[cookie.host] &&
                  cookie.name in jscookies[cookie.host][cookie.path])) {
              var jscookie = { "host": cookie.host, "value": cookie.value };
              
              if (cookie.path) jscookie.path = cookie.path;
              if (cookie.name) jscookie.name = cookie.name;
              if (cookie.isSecure) jscookie.secure = true;
              if (cookie.isHttpOnly) jscookie.httponly = true;
              if (cookie.expiry < MAX_EXPIRY) jscookie.expiry = cookie.expiry;

              addCookieToHash(jscookies, cookie.host, cookie.path, cookie.name, jscookie);
            }
            aWindow.cookies.push(jscookies[cookie.host][cookie.path][cookie.name]);
          }
        }
      }
    });

    
    for (i = 0; i < aWindows.length; i++)
      if (aWindows[i].cookies.length == 0)
        delete aWindows[i].cookies;
  },

  




  _updateWindowFeatures: function sss_updateWindowFeatures(aWindow) {
    var winData = this._windows[aWindow.__SSi];
    
    WINDOW_ATTRIBUTES.forEach(function(aAttr) {
      winData[aAttr] = this._getWindowDimension(aWindow, aAttr);
    }, this);
    
    var hidden = WINDOW_HIDEABLE_FEATURES.filter(function(aItem) {
      return aWindow[aItem] && !aWindow[aItem].visible;
    });
    if (hidden.length != 0)
      winData.hidden = hidden.join(",");
    else if (winData.hidden)
      delete winData.hidden;

    var sidebar = aWindow.document.getElementById("sidebar-box").getAttribute("sidebarcommand");
    if (sidebar)
      winData.sidebar = sidebar;
    else if (winData.sidebar)
      delete winData.sidebar;
  },

  





  _getCurrentState: function sss_getCurrentState(aUpdateAll) {
    var activeWindow = this._getMostRecentBrowserWindow();
    
    if (this._loadState == STATE_RUNNING) {
      
      this._forEachBrowserWindow(function(aWindow) {
        if (!this._isWindowLoaded(aWindow)) 
          return;
        if (aUpdateAll || this._dirtyWindows[aWindow.__SSi] || aWindow == activeWindow) {
          this._collectWindowData(aWindow);
        }
        else { 
          this._updateWindowFeatures(aWindow);
        }
      }, this);
      this._dirtyWindows = [];
    }
    
    
    var total = [], windows = [];
    var nonPopupCount = 0;
    var ix;
    for (ix in this._windows) {
      total.push(this._windows[ix]);
      windows.push(ix);
      if (!this._windows[ix].isPopup)
        nonPopupCount++;
    }
    this._updateCookies(total);

    
    for (ix in this._statesToRestore) {
      for each (let winData in this._statesToRestore[ix].windows) {
        total.push(winData);
        if (!winData.isPopup)
          nonPopupCount++;
      }
    }

    
    let lastClosedWindowsCopy = this._closedWindows.slice();

#ifndef XP_MACOSX
    
    if (nonPopupCount == 0 && lastClosedWindowsCopy.length > 0) {
      
      
      do {
        total.unshift(lastClosedWindowsCopy.shift())
      } while (total[0].isPopup)
    }
#endif

    if (activeWindow) {
      this.activeWindowSSiCache = activeWindow.__SSi || "";
    }
    ix = this.activeWindowSSiCache ? windows.indexOf(this.activeWindowSSiCache) : -1;

    return { windows: total, selectedWindow: ix + 1, _closedWindows: lastClosedWindowsCopy };
  },

  





  _getWindowState: function sss_getWindowState(aWindow) {
    if (!this._isWindowLoaded(aWindow))
      return this._statesToRestore[aWindow.__SS_restoreID];
    
    if (this._loadState == STATE_RUNNING) {
      this._collectWindowData(aWindow);
    }
    
    var total = [this._windows[aWindow.__SSi]];
    this._updateCookies(total);
    
    return { windows: total };
  },

  _collectWindowData: function sss_collectWindowData(aWindow) {
    if (!this._isWindowLoaded(aWindow))
      return;
    
    
    this._saveWindowHistory(aWindow);
    this._updateTextAndScrollData(aWindow);
    this._updateCookieHosts(aWindow);
    this._updateWindowFeatures(aWindow);
    
    this._dirtyWindows[aWindow.__SSi] = false;
  },



  










  restoreWindow: function sss_restoreWindow(aWindow, aState, aOverwriteTabs, aFollowUp) {
    if (!aFollowUp) {
      this.windowToFocus = aWindow;
    }
    
    if (aWindow && (!aWindow.__SSi || !this._windows[aWindow.__SSi]))
      this.onLoad(aWindow);

    try {
      var root = typeof aState == "string" ? this._safeEval(aState) : aState;
      if (!root.windows[0]) {
        this._notifyIfAllWindowsRestored();
        return; 
      }
    }
    catch (ex) { 
      debug(ex);
      this._notifyIfAllWindowsRestored();
      return;
    }

    if (root._closedWindows)
      this._closedWindows = root._closedWindows;

    var winData;
    if (!aState.selectedWindow) {
      aState.selectedWindow = 0;
    }
    
    
    for (var w = 1; w < root.windows.length; w++) {
      winData = root.windows[w];
      if (winData && winData.tabs && winData.tabs[0]) {
        var window = this._openWindowWithState({ windows: [winData] });
        if (w == aState.selectedWindow - 1) {
          this.windowToFocus = window;
        }
      }
    }
    winData = root.windows[0];
    if (!winData.tabs) {
      winData.tabs = [];
    }
    
    
    else if (root._firstTabs && !aOverwriteTabs && winData.tabs.length == 1 &&
             (!winData.tabs[0].entries || winData.tabs[0].entries.length == 0)) {
      winData.tabs = [];
    }
    
    var tabbrowser = aWindow.gBrowser;
    var openTabCount = aOverwriteTabs ? tabbrowser.browsers.length : -1;
    var newTabCount = winData.tabs.length;
    var tabs = [];

    
    var tabstrip = tabbrowser.tabContainer.mTabstrip;
    var smoothScroll = tabstrip.smoothScroll;
    tabstrip.smoothScroll = false;
    
    
    
    if (aOverwriteTabs && tabbrowser.selectedTab._tPos >= newTabCount)
      tabbrowser.moveTabTo(tabbrowser.selectedTab, newTabCount - 1);
    
    for (var t = 0; t < newTabCount; t++) {
      tabs.push(t < openTabCount ? tabbrowser.mTabs[t] : tabbrowser.addTab());
      
      if (!aOverwriteTabs && root._firstTabs) {
        tabbrowser.moveTabTo(tabs[t], t);
      }
    }

    
    if (aOverwriteTabs && newTabCount < openTabCount) {
      Array.slice(tabbrowser.mTabs, newTabCount, openTabCount)
           .forEach(tabbrowser.removeTab, tabbrowser);
    }
    
    if (aOverwriteTabs) {
      this.restoreWindowFeatures(aWindow, winData);
      delete this._windows[aWindow.__SSi].extData;
    }
    if (winData.cookies) {
      this.restoreCookies(winData.cookies);
    }
    if (winData.extData) {
      if (!this._windows[aWindow.__SSi].extData) {
        this._windows[aWindow.__SSi].extData = {};
      }
      for (var key in winData.extData) {
        this._windows[aWindow.__SSi].extData[key] = winData.extData[key];
      }
    }
    if (aOverwriteTabs || root._firstTabs) {
      this._windows[aWindow.__SSi]._closedTabs = winData._closedTabs || [];
    }
    
    this.restoreHistoryPrecursor(aWindow, tabs, winData.tabs,
      (aOverwriteTabs ? (parseInt(winData.selected) || 1) : 0), 0, 0);

    
    tabstrip.smoothScroll = smoothScroll;

    this._notifyIfAllWindowsRestored();
  },

  














  restoreHistoryPrecursor:
    function sss_restoreHistoryPrecursor(aWindow, aTabs, aTabData, aSelectTab, aIx, aCount) {
    var tabbrowser = aWindow.gBrowser;
    
    
    
    for (var t = aIx; t < aTabs.length; t++) {
      try {
        if (!tabbrowser.getBrowserForTab(aTabs[t]).webNavigation.sessionHistory) {
          throw new Error();
        }
      }
      catch (ex) { 
        if (aCount < 10) {
          var restoreHistoryFunc = function(self) {
            self.restoreHistoryPrecursor(aWindow, aTabs, aTabData, aSelectTab, aIx, aCount + 1);
          }
          aWindow.setTimeout(restoreHistoryFunc, 100, this);
          return;
        }
      }
    }
    
    
    for (t = 0; t < aTabs.length; t++) {
      var tab = aTabs[t];
      var browser = tabbrowser.getBrowserForTab(tab);
      
      aTabData[t]._tabStillLoading = true;
      if (!aTabData[t].entries || aTabData[t].entries.length == 0) {
        
        
        browser.contentDocument.location = "about:blank";
        continue;
      }
      
      browser.stop(); 
      
      tab.setAttribute("busy", "true");
      tabbrowser.updateIcon(tab);
      tabbrowser.setTabTitleLoading(tab);
      
      
      
      let activeIndex = (aTabData[t].index || aTabData[t].entries.length) - 1;
      let activePageData = aTabData[t].entries[activeIndex] || null;
      browser.userTypedValue = activePageData ? activePageData.url || null : null;
      
      
      
      browser.__SS_data = aTabData[t];
    }
    
    if (aTabs.length > 0) {
      
      let maxVisibleTabs = Math.ceil(tabbrowser.tabContainer.mTabstrip.scrollClientSize /
                                     aTabs[0].clientWidth);

      
      if (maxVisibleTabs < aTabs.length && aSelectTab > 1) {
        let firstVisibleTab = 0;
        if (aTabs.length - maxVisibleTabs > aSelectTab) {
          
          firstVisibleTab = aSelectTab - 1;
        } else {
          
          firstVisibleTab = aTabs.length - maxVisibleTabs;
        }
        aTabs = aTabs.splice(firstVisibleTab, maxVisibleTabs).concat(aTabs);
        aTabData = aTabData.splice(firstVisibleTab, maxVisibleTabs).concat(aTabData);
        aSelectTab -= firstVisibleTab;
      }

      
      if (aSelectTab-- && aTabs[aSelectTab]) {
        aTabs.unshift(aTabs.splice(aSelectTab, 1)[0]);
        aTabData.unshift(aTabData.splice(aSelectTab, 1)[0]);
        tabbrowser.selectedTab = aTabs[0];
      }
    }

    if (!this._isWindowLoaded(aWindow)) {
      
      delete this._statesToRestore[aWindow.__SS_restoreID];
      delete aWindow.__SS_restoreID;
    }
    
    
    var idMap = { used: {} };
    this.restoreHistory(aWindow, aTabs, aTabData, idMap);
  },

  










  restoreHistory: function sss_restoreHistory(aWindow, aTabs, aTabData, aIdMap) {
    var _this = this;
    while (aTabs.length > 0 && (!aTabData[0]._tabStillLoading || !aTabs[0].parentNode)) {
      aTabs.shift(); 
      aTabData.shift();
    }
    if (aTabs.length == 0) {
      return; 
    }
    
    var tab = aTabs.shift();
    var tabData = aTabData.shift();

    var browser = aWindow.gBrowser.getBrowserForTab(tab);
    var history = browser.webNavigation.sessionHistory;
    
    if (history.count > 0) {
      history.PurgeHistory(history.count);
    }
    history.QueryInterface(Ci.nsISHistoryInternal);
    
    if (!tabData.entries) {
      tabData.entries = [];
    }
    if (tabData.extData) {
      tab.__SS_extdata = {};
      for (let key in tabData.extData)
        tab.__SS_extdata[key] = tabData.extData[key];
    }
    else
      delete tab.__SS_extdata;
    
    for (var i = 0; i < tabData.entries.length; i++) {
      
      if (!tabData.entries[i].url)
        continue;
      history.addEntry(this._deserializeHistoryEntry(tabData.entries[i], aIdMap), true);
    }
    
    
    var disallow = (tabData.disallow)?tabData.disallow.split(","):[];
    CAPABILITIES.forEach(function(aCapability) {
      browser.docShell["allow" + aCapability] = disallow.indexOf(aCapability) == -1;
    });
    Array.filter(tab.attributes, function(aAttr) {
      return (_this.xulAttributes.indexOf(aAttr.name) > -1);
    }).forEach(tab.removeAttribute, tab);
    if (tabData.xultab) {
      
      tabData.xultab.split(" ").forEach(function(aAttr) {
        if (/^([^\s=]+)=(.*)/.test(aAttr)) {
          tab.setAttribute(RegExp.$1, decodeURI(RegExp.$2));
        }
      });
    }
    for (let name in tabData.attributes)
      tab.setAttribute(name, tabData.attributes[name]);
    
    if (tabData.storage && browser.docShell instanceof Ci.nsIDocShell)
      this._deserializeSessionStorage(tabData.storage, browser.docShell);
    
    
    var event = aWindow.document.createEvent("Events");
    event.initEvent("SSTabRestoring", true, false);
    tab.dispatchEvent(event);
    
    let activeIndex = (tabData.index || tabData.entries.length) - 1;
    if (activeIndex >= tabData.entries.length)
      activeIndex = tabData.entries.length - 1;
    try {
      if (activeIndex >= 0)
        browser.webNavigation.gotoIndex(activeIndex);
    }
    catch (ex) {
      
      tab.removeAttribute("busy");
    }
    
    if (tabData.entries.length > 0) {
      
      
      
      browser.__SS_restore_data = tabData.entries[activeIndex] || {};
      browser.__SS_restore_text = tabData.text || "";
      browser.__SS_restore_pageStyle = tabData.pageStyle || "";
      browser.__SS_restore_tab = tab;
      browser.__SS_restore = this.restoreDocument_proxy;
      browser.addEventListener("load", browser.__SS_restore, true);
    }
    
    aWindow.setTimeout(function(){ _this.restoreHistory(aWindow, aTabs, aTabData, aIdMap); }, 0);
  },

  







  _deserializeHistoryEntry: function sss_deserializeHistoryEntry(aEntry, aIdMap) {
    var shEntry = Cc["@mozilla.org/browser/session-history-entry;1"].
                  createInstance(Ci.nsISHEntry);
    
    var ioService = Cc["@mozilla.org/network/io-service;1"].
                    getService(Ci.nsIIOService);
    shEntry.setURI(ioService.newURI(aEntry.url, null, null));
    shEntry.setTitle(aEntry.title || aEntry.url);
    if (aEntry.subframe)
      shEntry.setIsSubFrame(aEntry.subframe || false);
    shEntry.loadType = Ci.nsIDocShellLoadInfo.loadHistory;
    if (aEntry.contentType)
      shEntry.contentType = aEntry.contentType;
    if (aEntry.referrer) 
      shEntry.referrerURI = ioService.newURI(aEntry.referrer, null, null);
    
    if (aEntry.cacheKey) {
      var cacheKey = Cc["@mozilla.org/supports-PRUint32;1"].
                     createInstance(Ci.nsISupportsPRUint32);
      cacheKey.data = aEntry.cacheKey;
      shEntry.cacheKey = cacheKey;
    }

    if (aEntry.ID) {
      
      
      var id = aIdMap[aEntry.ID] || 0;
      if (!id) {
        for (id = Date.now(); id in aIdMap.used; id++);
        aIdMap[aEntry.ID] = id;
        aIdMap.used[id] = true;
      }
      shEntry.ID = id;
    }
    
    if (aEntry.scroll) {
      var scrollPos = (aEntry.scroll || "0,0").split(",");
      scrollPos = [parseInt(scrollPos[0]) || 0, parseInt(scrollPos[1]) || 0];
      shEntry.setScrollPosition(scrollPos[0], scrollPos[1]);
    }

    var postdata;
    if (aEntry.postdata_b64) {  
      postdata = atob(aEntry.postdata_b64);
    } else if (aEntry.postdata) { 
      postdata = aEntry.postdata;
    }

    if (postdata) {
      var stream = Cc["@mozilla.org/io/string-input-stream;1"].
                   createInstance(Ci.nsIStringInputStream);
      stream.setData(postdata, postdata.length);
      shEntry.postData = stream;
    }

    if (aEntry.owner_b64) {  
      var ownerInput = Cc["@mozilla.org/io/string-input-stream;1"].
                       createInstance(Ci.nsIStringInputStream);
      var binaryData = atob(aEntry.owner_b64);
      ownerInput.setData(binaryData, binaryData.length);
      var binaryStream = Cc["@mozilla.org/binaryinputstream;1"].
                         createInstance(Ci.nsIObjectInputStream);
      binaryStream.setInputStream(ownerInput);
      try { 
        shEntry.owner = binaryStream.readObject(true);
      } catch (ex) { debug(ex); }
    } else if (aEntry.ownerURI) { 
      var uriObj = ioService.newURI(aEntry.ownerURI, null, null);
      shEntry.owner = Cc["@mozilla.org/scriptsecuritymanager;1"].
                      getService(Ci.nsIScriptSecurityManager).
                      getCodebasePrincipal(uriObj);
    }
    
    if (aEntry.children && shEntry instanceof Ci.nsISHContainer) {
      for (var i = 0; i < aEntry.children.length; i++) {
        
        if (!aEntry.children[i].url)
          continue;
        shEntry.AddChild(this._deserializeHistoryEntry(aEntry.children[i], aIdMap), i);
      }
    }
    
    return shEntry;
  },

  






  _deserializeSessionStorage: function sss_deserializeSessionStorage(aStorageData, aDocShell) {
    let ioService = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);
    for (let url in aStorageData) {
      let uri = ioService.newURI(url, null, null);
      let storage = aDocShell.getSessionStorageForURI(uri);
      for (let key in aStorageData[url]) {
        try {
          storage.setItem(key, aStorageData[url][key]);
        }
        catch (ex) { Cu.reportError(ex); } 
      }
    }
  },

  


  restoreDocument_proxy: function sss_restoreDocument_proxy(aEvent) {
    
    if (!aEvent || !aEvent.originalTarget || !aEvent.originalTarget.defaultView || aEvent.originalTarget.defaultView != aEvent.originalTarget.defaultView.top) {
      return;
    }
    
    
    function hasExpectedURL(aDocument, aURL)
      !aURL || aURL.replace(/#.*/, "") == aDocument.location.href.replace(/#.*/, "");
    
    
    var textArray = this.__SS_restore_text ? this.__SS_restore_text.split(" ") : [];
    function restoreTextData(aContent, aPrefix, aURL) {
      textArray.forEach(function(aEntry) {
        if (/^((?:\d+\|)*)(#?)([^\s=]+)=(.*)$/.test(aEntry) &&
            RegExp.$1 == aPrefix && hasExpectedURL(aContent.document, aURL)) {
          var document = aContent.document;
          var node = RegExp.$2 ? document.getElementById(RegExp.$3) : document.getElementsByName(RegExp.$3)[0] || null;
          if (node && "value" in node && node.type != "file") {
            node.value = decodeURI(RegExp.$4);
            
            var event = document.createEvent("UIEvents");
            event.initUIEvent("input", true, true, aContent, 0);
            node.dispatchEvent(event);
          }
        }
      });
    }
    
    function restoreFormData(aDocument, aData, aURL) {
      for (let key in aData) {
        if (!hasExpectedURL(aDocument, aURL))
          return;
        
        let node = key.charAt(0) == "#" ? aDocument.getElementById(key.slice(1)) :
                                          XPathHelper.resolve(aDocument, key);
        if (!node)
          continue;
        
        let value = aData[key];
        if (typeof value == "string" && node.type != "file") {
          if (node.value == value)
            continue; 
          
          node.value = value;
          
          let event = aDocument.createEvent("UIEvents");
          event.initUIEvent("input", true, true, aDocument.defaultView, 0);
          node.dispatchEvent(event);
        }
        else if (typeof value == "boolean")
          node.checked = value;
        else if (typeof value == "number")
          try {
            node.selectedIndex = value;
          } catch (ex) {  }
        else if (value && value.type && value.type == node.type)
          node.value = value.value;
        else if (value && typeof value.indexOf == "function" && node.options) {
          Array.forEach(node.options, function(aOpt, aIx) {
            aOpt.selected = value.indexOf(aIx) > -1;
          });
        }
        
      }
    }
    
    let selectedPageStyle = this.__SS_restore_pageStyle;
    let window = this.ownerDocument.defaultView;
    function restoreTextDataAndScrolling(aContent, aData, aPrefix) {
      if (aData.formdata)
        restoreFormData(aContent.document, aData.formdata, aData.url);
      else
        restoreTextData(aContent, aPrefix, aData.url);
      if (aData.innerHTML) {
        window.setTimeout(function() {
          if (aContent.document.designMode == "on" &&
              hasExpectedURL(aContent.document, aData.url)) {
            aContent.document.body.innerHTML = aData.innerHTML;
          }
        }, 0);
      }
      if (aData.scroll && /(\d+),(\d+)/.test(aData.scroll)) {
        aContent.scrollTo(RegExp.$1, RegExp.$2);
      }
      Array.forEach(aContent.document.styleSheets, function(aSS) {
        aSS.disabled = aSS.title && aSS.title != selectedPageStyle;
      });
      for (var i = 0; i < aContent.frames.length; i++) {
        if (aData.children && aData.children[i] &&
          hasExpectedURL(aContent.document, aData.url)) {
          restoreTextDataAndScrolling(aContent.frames[i], aData.children[i], aPrefix + i + "|");
        }
      }
    }
    
    
    
    if (hasExpectedURL(aEvent.originalTarget, this.__SS_restore_data.url)) {
      var content = aEvent.originalTarget.defaultView;
      if (this.currentURI.spec == "about:config") {
        
        
        content = content.wrappedJSObject;
      }
      restoreTextDataAndScrolling(content, this.__SS_restore_data, "");
      this.markupDocumentViewer.authorStyleDisabled = selectedPageStyle == "_nostyle";
      
      
      var event = this.ownerDocument.createEvent("Events");
      event.initEvent("SSTabRestored", true, false);
      this.__SS_restore_tab.dispatchEvent(event);
    }
    
    this.removeEventListener("load", this.__SS_restore, true);
    delete this.__SS_restore_data;
    delete this.__SS_restore_text;
    delete this.__SS_restore_pageStyle;
    delete this.__SS_restore_tab;
    delete this.__SS_restore;
  },

  






  restoreWindowFeatures: function sss_restoreWindowFeatures(aWindow, aWinData) {
    var hidden = (aWinData.hidden)?aWinData.hidden.split(","):[];
    WINDOW_HIDEABLE_FEATURES.forEach(function(aItem) {
      aWindow[aItem].visible = hidden.indexOf(aItem) == -1;
    });
    
    if (aWinData.isPopup) {
      this._windows[aWindow.__SSi].isPopup = true;
      if (aWindow.gURLBar) {
        aWindow.gURLBar.readOnly = true;
        aWindow.gURLBar.setAttribute("enablehistory", "false");
      }
    }
    else {
      delete this._windows[aWindow.__SSi].isPopup;
      if (aWindow.gURLBar) {
        aWindow.gURLBar.readOnly = false;
        aWindow.gURLBar.setAttribute("enablehistory", "true");
      }
    }

    var _this = this;
    aWindow.setTimeout(function() {
      _this.restoreDimensions.apply(_this, [aWindow, aWinData.width || 0, 
        aWinData.height || 0, "screenX" in aWinData ? aWinData.screenX : NaN,
        "screenY" in aWinData ? aWinData.screenY : NaN,
        aWinData.sizemode || "", aWinData.sidebar || ""]);
    }, 0);
  },

  














  restoreDimensions: function sss_restoreDimensions(aWindow, aWidth, aHeight, aLeft, aTop, aSizeMode, aSidebar) {
    var win = aWindow;
    var _this = this;
    function win_(aName) { return _this._getWindowDimension(win, aName); }
    
    
    if (aWidth && aHeight && (aWidth != win_("width") || aHeight != win_("height"))) {
      aWindow.resizeTo(aWidth, aHeight);
    }
    if (!isNaN(aLeft) && !isNaN(aTop) && (aLeft != win_("screenX") || aTop != win_("screenY"))) {
      aWindow.moveTo(aLeft, aTop);
    }
    if (aSizeMode && win_("sizemode") != aSizeMode)
    {
      switch (aSizeMode)
      {
      case "maximized":
        aWindow.maximize();
        break;
      case "minimized":
        aWindow.minimize();
        break;
      case "normal":
        aWindow.restore();
        break;
      }
    }
    var sidebar = aWindow.document.getElementById("sidebar-box");
    if (sidebar.getAttribute("sidebarcommand") != aSidebar) {
      aWindow.toggleSidebar(aSidebar);
    }
    
    
    if (this.windowToFocus && this.windowToFocus.content) {
      this.windowToFocus.content.focus();
    }
  },

  




  restoreCookies: function sss_restoreCookies(aCookies) {
    if (aCookies.count && aCookies.domain1) {
      
      var converted = [];
      for (var i = 1; i <= aCookies.count; i++) {
        
        var parsed = aCookies["value" + i].match(/^([^=;]+)=([^;]*);(?:domain=[^;]+;)?(?:path=([^;]*);)?(secure;)?(httponly;)?/);
        if (parsed && /^https?:\/\/([^\/]+)/.test(aCookies["domain" + i]))
          converted.push({
            host: RegExp.$1, path: parsed[3], name: parsed[1], value: parsed[2],
            secure: parsed[4], httponly: parsed[5]
          });
      }
      aCookies = converted;
    }
    
    var cookieManager = Cc["@mozilla.org/cookiemanager;1"].
                        getService(Ci.nsICookieManager2);
    
    var MAX_EXPIRY = Math.pow(2, 62);
    for (i = 0; i < aCookies.length; i++) {
      var cookie = aCookies[i];
      try {
        cookieManager.add(cookie.host, cookie.path || "", cookie.name || "", cookie.value, !!cookie.secure, !!cookie.httponly, true, "expiry" in cookie ? cookie.expiry : MAX_EXPIRY);
      }
      catch (ex) { Cu.reportError(ex); } 
    }
  },



  







  saveStateDelayed: function sss_saveStateDelayed(aWindow, aDelay) {
    if (aWindow) {
      this._dirtyWindows[aWindow.__SSi] = true;
    }

    if (!this._saveTimer && this._resume_from_crash &&
        !this._inPrivateBrowsing) {
      
      var minimalDelay = this._lastSaveTime + this._interval - Date.now();
      
      
      aDelay = Math.max(minimalDelay, aDelay || 2000);
      if (aDelay > 0) {
        this._saveTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
        this._saveTimer.init(this, aDelay, Ci.nsITimer.TYPE_ONE_SHOT);
      }
      else {
        this.saveState();
      }
    }
  },

  




  saveState: function sss_saveState(aUpdateAll) {
    
    if (!this._resume_from_crash && this._loadState == STATE_RUNNING)
      return;

    
    if (this._inPrivateBrowsing)
      return;

    var oState = this._getCurrentState(aUpdateAll);
    oState.session = {
      state: this._loadState == STATE_RUNNING ? STATE_RUNNING_STR : STATE_STOPPED_STR,
      lastUpdate: Date.now()
    };
    if (this._recentCrashes)
      oState.session.recentCrashes = this._recentCrashes;

    this._saveStateObject(oState);
  },

  


  _saveStateObject: function sss_saveStateObject(aStateObj) {
    var stateString = Cc["@mozilla.org/supports-string;1"].
                        createInstance(Ci.nsISupportsString);
    
    stateString.data = "(" + this._toJSONString(aStateObj) + ")";

    this._observerService.notifyObservers(stateString,
                                          "sessionstore-state-write", "");

    
    if (stateString.data)
      this._writeFile(this._sessionFile, stateString.data);

    this._lastSaveTime = Date.now();
  },

  


  _clearDisk: function sss_clearDisk() {
    if (this._sessionFile.exists()) {
      try {
        this._sessionFile.remove(false);
      }
      catch (ex) { dump(ex + '\n'); } 
    }
    if (this._sessionFileBackup.exists()) {
      try {
        this._sessionFileBackup.remove(false);
      }
      catch (ex) { dump(ex + '\n'); } 
    }
  },



  





  _forEachBrowserWindow: function sss_forEachBrowserWindow(aFunc) {
    var windowMediator = Cc["@mozilla.org/appshell/window-mediator;1"].
                         getService(Ci.nsIWindowMediator);
    var windowsEnum = windowMediator.getEnumerator("navigator:browser");
    
    while (windowsEnum.hasMoreElements()) {
      var window = windowsEnum.getNext();
      if (window.__SSi) {
        aFunc.call(this, window);
      }
    }
  },

  



  _getMostRecentBrowserWindow: function sss_getMostRecentBrowserWindow() {
    var windowMediator = Cc["@mozilla.org/appshell/window-mediator;1"].
                         getService(Ci.nsIWindowMediator);
    return windowMediator.getMostRecentWindow("navigator:browser");
  },

  





  _openWindowWithState: function sss_openWindowWithState(aState) {
    var argString = Cc["@mozilla.org/supports-string;1"].
                    createInstance(Ci.nsISupportsString);
    argString.data = "";

    
    var window = Cc["@mozilla.org/embedcomp/window-watcher;1"].
                 getService(Ci.nsIWindowWatcher).
                 openWindow(null, this._prefBranch.getCharPref("chromeURL"), "_blank",
                            "chrome,dialog=no,all", argString);
    
    do {
      var ID = "window" + Math.random();
    } while (ID in this._statesToRestore);
    this._statesToRestore[(window.__SS_restoreID = ID)] = aState;
    
    return window;
  },

  



  _doResumeSession: function sss_doResumeSession() {
    if (this._clearingOnShutdown)
      return false;

    return this._prefBranch.getIntPref("startup.page") == 3 ||
           this._prefBranch.getBoolPref("sessionstore.resume_session_once");
  },

  





  _isCmdLineEmpty: function sss_isCmdLineEmpty(aWindow) {
    var defaultArgs = Cc["@mozilla.org/browser/clh;1"].
                      getService(Ci.nsIBrowserHandler).defaultArgs;
    if (aWindow.arguments && aWindow.arguments[0] &&
        aWindow.arguments[0] == defaultArgs)
      aWindow.arguments[0] = null;

    return !aWindow.arguments || !aWindow.arguments[0];
  },

  






  _checkPrivacyLevel: function sss_checkPrivacyLevel(aIsHTTPS) {
    return this._prefBranch.getIntPref("sessionstore.privacy_level") < (aIsHTTPS ? PRIVACY_ENCRYPTED : PRIVACY_FULL);
  },

  










  _getWindowDimension: function sss_getWindowDimension(aWindow, aAttribute) {
    if (aAttribute == "sizemode") {
      switch (aWindow.windowState) {
      case aWindow.STATE_FULLSCREEN:
      case aWindow.STATE_MAXIMIZED:
        return "maximized";
      case aWindow.STATE_MINIMIZED:
        return "minimized";
      default:
        return "normal";
      }
    }
    
    var dimension;
    switch (aAttribute) {
    case "width":
      dimension = aWindow.outerWidth;
      break;
    case "height":
      dimension = aWindow.outerHeight;
      break;
    default:
      dimension = aAttribute in aWindow ? aWindow[aAttribute] : "";
      break;
    }
    
    if (aWindow.windowState == aWindow.STATE_NORMAL) {
      return dimension;
    }
    return aWindow.document.documentElement.getAttribute(aAttribute) || dimension;
  },

  




  _getURIFromString: function sss_getURIFromString(aString) {
    var ioService = Cc["@mozilla.org/network/io-service;1"].
                    getService(Ci.nsIIOService);
    return ioService.newURI(aString, null, null);
  },

  


  _updateCrashReportURL: function sss_updateCrashReportURL(aWindow) {
    if (!Ci.nsICrashReporter) {
      
      this._updateCrashReportURL = function(aWindow) {};
      return;
    }
    try {
      var currentURI = aWindow.gBrowser.currentURI.clone();
      
      try { 
        currentURI.userPass = ""; 
      } 
      catch (ex) { } 

      var cr = Cc["@mozilla.org/xre/app-info;1"].getService(Ci.nsICrashReporter);
      cr.annotateCrashReport("URL", currentURI.spec);
    }
    catch (ex) {
      
      if (ex.result != Components.results.NS_ERROR_NOT_INITIALIZED)
        debug(ex);
    }
  },

  




  _needsRestorePage: function sss_needsRestorePage(aState, aRecentCrashes) {
    const SIX_HOURS_IN_MS = 6 * 60 * 60 * 1000;
    
    
    let winData = aState.windows || null;
    if (!winData || winData.length == 0)
      return false;
    
    
    if (winData.length == 1 && winData[0].tabs &&
        winData[0].tabs.length == 1 && winData[0].tabs[0].entries &&
        winData[0].tabs[0].entries.length == 1 &&
        winData[0].tabs[0].entries[0].url == "about:sessionrestore")
      return false;
    
    
    let XRE = Cc["@mozilla.org/xre/app-info;1"].getService(Ci.nsIXULRuntime);
    if (XRE.inSafeMode)
      return true;
    
    let max_resumed_crashes =
      this._prefBranch.getIntPref("sessionstore.max_resumed_crashes");
    let sessionAge = aState.session && aState.session.lastUpdate &&
                     (Date.now() - aState.session.lastUpdate);
    
    return max_resumed_crashes != -1 &&
           (aRecentCrashes > max_resumed_crashes ||
            sessionAge && sessionAge >= SIX_HOURS_IN_MS);
  },

  


  _safeEval: function sss_safeEval(aStr) {
    return Cu.evalInSandbox(aStr, new Cu.Sandbox("about:blank"));
  },

  








  _toJSONString: function sss_toJSONString(aJSObject) {
    
    
    let jsonString = JSON.stringify(aJSObject);
    
    if (/[\u2028\u2029]/.test(jsonString)) {
      
      
      jsonString = jsonString.replace(/[\u2028\u2029]/g,
                                      function($0) "\\u" + $0.charCodeAt(0).toString(16));
    }
    
    return jsonString;
  },

  _notifyIfAllWindowsRestored: function sss_notifyIfAllWindowsRestored() {
    if (this._restoreCount) {
      this._restoreCount--;
      if (this._restoreCount == 0) {
        
        this._observerService.notifyObservers(null, NOTIFY_WINDOWS_RESTORED, "");
      }
    }
  },

  





  _isWindowLoaded: function sss_isWindowLoaded(aWindow) {
    return !aWindow.__SS_restoreID;
  },

  







  _replaceLoadingTitle : function sss_replaceLoadingTitle(aString, aTabbrowser, aTab) {
    if (aString == aTabbrowser.mStringBundle.getString("tabs.loading")) {
      aTabbrowser.setTabTitle(aTab);
      [aString, aTab.label] = [aTab.label, aString];
    }
    return aString;
  },

  




  _capClosedWindows : function sss_capClosedWindows() {
    let maxWindowsUndo = this._prefBranch.getIntPref("sessionstore.max_windows_undo");
    if (this._closedWindows.length <= maxWindowsUndo)
      return;
    let spliceTo = maxWindowsUndo;
#ifndef XP_MACOSX
    let normalWindowIndex = 0;
    
    while (normalWindowIndex < this._closedWindows.length &&
           !!this._closedWindows[normalWindowIndex].isPopup)
      normalWindowIndex++;
    if (normalWindowIndex >= maxWindowsUndo)
      spliceTo = normalWindowIndex + 1;
#endif
    this._closedWindows.splice(spliceTo);
  },



  






  _writeFile: function sss_writeFile(aFile, aData) {
    
    var ostream = Cc["@mozilla.org/network/safe-file-output-stream;1"].
                  createInstance(Ci.nsIFileOutputStream);
    ostream.init(aFile, 0x02 | 0x08 | 0x20, 0600, 0);

    
    var converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"].
                    createInstance(Ci.nsIScriptableUnicodeConverter);
    converter.charset = "UTF-8";

    
    var istream = converter.convertToInputStream(aData);
    var self = this;
    NetUtil.asyncCopy(istream, ostream, function(rc) {
      if (Components.isSuccessCode(rc)) {
        self._observerService.notifyObservers(null,
                                              "sessionstore-state-write-complete",
                                              "");
      }
    });
  }
};

let XPathHelper = {
  
  namespaceURIs:     { "xhtml": "http://www.w3.org/1999/xhtml" },
  namespacePrefixes: { "http://www.w3.org/1999/xhtml": "xhtml" },

  


  generate: function sss_xph_generate(aNode) {
    
    if (!aNode.parentNode)
      return "";
    
    let prefix = this.namespacePrefixes[aNode.namespaceURI] || null;
    let tag = (prefix ? prefix + ":" : "") + this.escapeName(aNode.localName);
    
    
    if (aNode.id)
      return "//" + tag + "[@id=" + this.quoteArgument(aNode.id) + "]";
    
    
    
    let count = 0;
    let nName = aNode.name || null;
    for (let n = aNode; (n = n.previousSibling); )
      if (n.localName == aNode.localName && n.namespaceURI == aNode.namespaceURI &&
          (!nName || n.name == nName))
        count++;
    
    
    return this.generate(aNode.parentNode) + "/" + tag +
           (nName ? "[@name=" + this.quoteArgument(nName) + "]" : "") +
           (count ? "[" + (count + 1) + "]" : "");
  },

  


  resolve: function sss_xph_resolve(aDocument, aQuery) {
    let xptype = Ci.nsIDOMXPathResult.FIRST_ORDERED_NODE_TYPE;
    return aDocument.evaluate(aQuery, aDocument, this.resolveNS, xptype, null).singleNodeValue;
  },

  


  resolveNS: function sss_xph_resolveNS(aPrefix) {
    return XPathHelper.namespaceURIs[aPrefix] || null;
  },

  


  escapeName: function sss_xph_escapeName(aName) {
    
    
    return /^\w+$/.test(aName) ? aName :
           "*[local-name()=" + this.quoteArgument(aName) + "]";
  },

  


  quoteArgument: function sss_xph_quoteArgument(aArg) {
    return !/'/.test(aArg) ? "'" + aArg + "'" :
           !/"/.test(aArg) ? '"' + aArg + '"' :
           "concat('" + aArg.replace(/'+/g, "',\"$&\",'") + "')";
  },

  


  get restorableFormNodes() {
    
    
    let ignoreTypes = ["password", "hidden", "button", "image", "submit", "reset"];
    
    let toLowerCase = '"ABCDEFGHIJKLMNOPQRSTUVWXYZ", "abcdefghijklmnopqrstuvwxyz"';
    let ignore = "not(translate(@type, " + toLowerCase + ")='" +
      ignoreTypes.join("' or translate(@type, " + toLowerCase + ")='") + "')";
    let formNodesXPath = "//textarea|//select|//xhtml:textarea|//xhtml:select|" +
      "//input[" + ignore + "]|//xhtml:input[" + ignore + "]";
    
    delete this.restorableFormNodes;
    return (this.restorableFormNodes = formNodesXPath);
  }
};


String.prototype.hasRootDomain = function hasRootDomain(aDomain)
{
  let index = this.indexOf(aDomain);
  if (index == -1)
    return false;

  if (this == aDomain)
    return true;

  let prevChar = this[index - 1];
  return (index == (this.length - aDomain.length)) &&
         (prevChar == "." || prevChar == "/");
}

function NSGetModule(aComMgr, aFileSpec)
  XPCOMUtils.generateModule([SessionStoreService]);
