



"use strict";

this.EXPORTED_SYMBOLS = ["SessionStore"];

const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

const STATE_STOPPED = 0;
const STATE_RUNNING = 1;
const STATE_QUITTING = -1;

const STATE_STOPPED_STR = "stopped";
const STATE_RUNNING_STR = "running";

const TAB_STATE_NEEDS_RESTORE = 1;
const TAB_STATE_RESTORING = 2;

const NOTIFY_WINDOWS_RESTORED = "sessionstore-windows-restored";
const NOTIFY_BROWSER_STATE_RESTORED = "sessionstore-browser-state-restored";



const MAX_CONCURRENT_TAB_RESTORES = 3;


const OBSERVING = [
  "domwindowopened", "domwindowclosed",
  "quit-application-requested", "quit-application-granted",
  "browser-lastwindow-close-granted",
  "quit-application", "browser:purge-session-history",
  "browser:purge-domain-data"
];



const WINDOW_ATTRIBUTES = ["width", "height", "screenX", "screenY", "sizemode"];



const WINDOW_HIDEABLE_FEATURES = [
  "menubar", "toolbar", "locationbar", "personalbar", "statusbar", "scrollbars"
];

const MESSAGES = [
  
  
  
  "SessionStore:input",

  
  
  
  "SessionStore:pageshow",

  
  
  "SessionStore:MozStorageChanged",

  
  
  "SessionStore:loadStart"
];


const TAB_EVENTS = [
  "TabOpen", "TabClose", "TabSelect", "TabShow", "TabHide", "TabPinned",
  "TabUnpinned"
];


const MS_PER_DAY = 1000.0 * 60.0 * 60.0 * 24.0;

#ifndef XP_WIN
#define BROKEN_WM_Z_ORDER
#endif

Cu.import("resource://gre/modules/Services.jsm", this);
Cu.import("resource://gre/modules/XPCOMUtils.jsm", this);
Cu.import("resource://gre/modules/TelemetryTimestamps.jsm", this);
Cu.import("resource://gre/modules/TelemetryStopwatch.jsm", this);
Cu.import("resource://gre/modules/osfile.jsm", this);
Cu.import("resource://gre/modules/PrivateBrowsingUtils.jsm", this);
Cu.import("resource://gre/modules/Promise.jsm", this);
Cu.import("resource://gre/modules/Task.jsm", this);

XPCOMUtils.defineLazyServiceGetter(this, "gSessionStartup",
  "@mozilla.org/browser/sessionstartup;1", "nsISessionStartup");
XPCOMUtils.defineLazyServiceGetter(this, "gScreenManager",
  "@mozilla.org/gfx/screenmanager;1", "nsIScreenManager");





let gDocShellCapabilities = (function () {
  let caps;

  return docShell => {
    if (!caps) {
      let keys = Object.keys(docShell);
      caps = keys.filter(k => k.startsWith("allow")).map(k => k.slice(5));
    }

    return caps;
  };
})();






function makeURI(aString) {
  return Services.io.newURI(aString, null, null);
}

XPCOMUtils.defineLazyModuleGetter(this, "ScratchpadManager",
  "resource:///modules/devtools/scratchpad-manager.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "DocumentUtils",
  "resource:///modules/sessionstore/DocumentUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PrivacyLevel",
  "resource:///modules/sessionstore/PrivacyLevel.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "SessionSaver",
  "resource:///modules/sessionstore/SessionSaver.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "SessionStorage",
  "resource:///modules/sessionstore/SessionStorage.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "SessionCookies",
  "resource:///modules/sessionstore/SessionCookies.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "_SessionFile",
  "resource:///modules/sessionstore/_SessionFile.jsm");

#ifdef MOZ_CRASHREPORTER
XPCOMUtils.defineLazyServiceGetter(this, "CrashReporter",
  "@mozilla.org/xre/app-info;1", "nsICrashReporter");
#endif





let gDebuggingEnabled = false;
function debug(aMsg) {
  if (gDebuggingEnabled) {
    aMsg = ("SessionStore: " + aMsg).replace(/\S{80}/g, "$&\n");
    Services.console.logStringMessage(aMsg);
  }
}

this.SessionStore = {
  get promiseInitialized() {
    return SessionStoreInternal.promiseInitialized;
  },

  get canRestoreLastSession() {
    return SessionStoreInternal.canRestoreLastSession;
  },

  set canRestoreLastSession(val) {
    SessionStoreInternal.canRestoreLastSession = val;
  },

  init: function ss_init(aWindow) {
    SessionStoreInternal.init(aWindow);
  },

  getBrowserState: function ss_getBrowserState() {
    return SessionStoreInternal.getBrowserState();
  },

  setBrowserState: function ss_setBrowserState(aState) {
    SessionStoreInternal.setBrowserState(aState);
  },

  getWindowState: function ss_getWindowState(aWindow) {
    return SessionStoreInternal.getWindowState(aWindow);
  },

  setWindowState: function ss_setWindowState(aWindow, aState, aOverwrite) {
    SessionStoreInternal.setWindowState(aWindow, aState, aOverwrite);
  },

  getTabState: function ss_getTabState(aTab) {
    return SessionStoreInternal.getTabState(aTab);
  },

  setTabState: function ss_setTabState(aTab, aState) {
    SessionStoreInternal.setTabState(aTab, aState);
  },

  duplicateTab: function ss_duplicateTab(aWindow, aTab, aDelta) {
    return SessionStoreInternal.duplicateTab(aWindow, aTab, aDelta);
  },

  getNumberOfTabsClosedLast: function ss_getNumberOfTabsClosedLast(aWindow) {
    return SessionStoreInternal.getNumberOfTabsClosedLast(aWindow);
  },

  setNumberOfTabsClosedLast: function ss_setNumberOfTabsClosedLast(aWindow, aNumber) {
    return SessionStoreInternal.setNumberOfTabsClosedLast(aWindow, aNumber);
  },

  getClosedTabCount: function ss_getClosedTabCount(aWindow) {
    return SessionStoreInternal.getClosedTabCount(aWindow);
  },

  getClosedTabData: function ss_getClosedTabDataAt(aWindow) {
    return SessionStoreInternal.getClosedTabData(aWindow);
  },

  undoCloseTab: function ss_undoCloseTab(aWindow, aIndex) {
    return SessionStoreInternal.undoCloseTab(aWindow, aIndex);
  },

  forgetClosedTab: function ss_forgetClosedTab(aWindow, aIndex) {
    return SessionStoreInternal.forgetClosedTab(aWindow, aIndex);
  },

  getClosedWindowCount: function ss_getClosedWindowCount() {
    return SessionStoreInternal.getClosedWindowCount();
  },

  getClosedWindowData: function ss_getClosedWindowData() {
    return SessionStoreInternal.getClosedWindowData();
  },

  undoCloseWindow: function ss_undoCloseWindow(aIndex) {
    return SessionStoreInternal.undoCloseWindow(aIndex);
  },

  forgetClosedWindow: function ss_forgetClosedWindow(aIndex) {
    return SessionStoreInternal.forgetClosedWindow(aIndex);
  },

  getWindowValue: function ss_getWindowValue(aWindow, aKey) {
    return SessionStoreInternal.getWindowValue(aWindow, aKey);
  },

  setWindowValue: function ss_setWindowValue(aWindow, aKey, aStringValue) {
    SessionStoreInternal.setWindowValue(aWindow, aKey, aStringValue);
  },

  deleteWindowValue: function ss_deleteWindowValue(aWindow, aKey) {
    SessionStoreInternal.deleteWindowValue(aWindow, aKey);
  },

  getTabValue: function ss_getTabValue(aTab, aKey) {
    return SessionStoreInternal.getTabValue(aTab, aKey);
  },

  setTabValue: function ss_setTabValue(aTab, aKey, aStringValue) {
    SessionStoreInternal.setTabValue(aTab, aKey, aStringValue);
  },

  deleteTabValue: function ss_deleteTabValue(aTab, aKey) {
    SessionStoreInternal.deleteTabValue(aTab, aKey);
  },

  persistTabAttribute: function ss_persistTabAttribute(aName) {
    SessionStoreInternal.persistTabAttribute(aName);
  },

  restoreLastSession: function ss_restoreLastSession() {
    SessionStoreInternal.restoreLastSession();
  },

  getCurrentState: function (aUpdateAll) {
    return SessionStoreInternal.getCurrentState(aUpdateAll);
  },

  



  get _internal() {
    if (Services.prefs.getBoolPref("browser.sessionstore.debug")) {
      return SessionStoreInternal;
    }
    return undefined;
  },
};


Object.freeze(SessionStore);

let SessionStoreInternal = {
  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsIDOMEventListener,
    Ci.nsIObserver,
    Ci.nsISupportsWeakReference
  ]),

  
  _loadState: STATE_STOPPED,

  
  
  _restoreCount: -1,

  
  _browserSetState: false,

  
  
  _sessionStartTime: Date.now(),

  
  _windows: {},

  
  _closedWindows: [],

  
  _statesToRestore: {},

  
  _recentCrashes: 0,

  
  _restoreLastWindow: false,

  
  _tabsRestoringCount: 0,

  
  
  
  
  _lastSessionState: null,

  
  
  
  _deferredInitialState: null,

  
  _deferredInitialized: Promise.defer(),

  
  _sessionInitialized: false,

  
  
  _disabledForMultiProcess: false,

  


  get promiseInitialized() {
    return this._deferredInitialized.promise;
  },

  
  get canRestoreLastSession() {
    return this._lastSessionState;
  },

  set canRestoreLastSession(val) {
    
    if (val)
      return;
    this._lastSessionState = null;
  },

  


  init: function (aWindow) {
    if (this._initialized) {
      throw new Error("SessionStore.init() must only be called once!");
    }

    if (!aWindow) {
      throw new Error("SessionStore.init() must be called with a valid window.");
    }

    this._disabledForMultiProcess = Services.prefs.getBoolPref("browser.tabs.remote");
    if (this._disabledForMultiProcess) {
      this._deferredInitialized.resolve();
      return;
    }

    TelemetryTimestamps.add("sessionRestoreInitialized");
    OBSERVING.forEach(function(aTopic) {
      Services.obs.addObserver(this, aTopic, true);
    }, this);

    this._initPrefs();
    this._initialized = true;

    
    this._sessionhistory_max_entries =
      this._prefBranch.getIntPref("sessionhistory.max_entries");

    
    gSessionStartup.onceInitialized.then(() => {
      
      let initialState = this.initSession();

      
      if (!aWindow.closed) {
        this.onLoad(aWindow, initialState);
      }

      
      this._deferredInitialized.resolve();
    });
  },

  initSession: function ssi_initSession() {
    let state;
    let ss = gSessionStartup;

    try {
      if (ss.doRestore() ||
          ss.sessionType == Ci.nsISessionStartup.DEFER_SESSION)
        state = ss.state;
    }
    catch(ex) { dump(ex + "\n"); } 

    if (state) {
      try {
        
        
        if (ss.sessionType == Ci.nsISessionStartup.DEFER_SESSION) {
          let [iniState, remainingState] = this._prepDataForDeferredRestore(state);
          
          
          if (iniState.windows.length)
            state = iniState;
          else
            state = null;
          if (remainingState.windows.length)
            this._lastSessionState = remainingState;
        }
        else {
          
          
          this._lastSessionState = state.lastSessionState;

          let lastSessionCrashed =
            state.session && state.session.state &&
            state.session.state == STATE_RUNNING_STR;
          if (lastSessionCrashed) {
            this._recentCrashes = (state.session &&
                                   state.session.recentCrashes || 0) + 1;

            if (this._needsRestorePage(state, this._recentCrashes)) {
              
              let pageData = {
                url: "about:sessionrestore",
                formdata: {
                  id: { "sessionData": state },
                  xpath: {}
                }
              };
              state = { windows: [{ tabs: [{ entries: [pageData] }] }] };
            } else if (this._hasSingleTabWithURL(state.windows,
                                                 "about:welcomeback")) {
              
              
              state.windows[0].tabs[0].entries[0].url = "about:sessionrestore";
            }
          }

          
          this._updateSessionStartTime(state);

          
          delete state.windows[0].hidden;
          
          delete state.windows[0].isPopup;
          
          if (state.windows[0].sizemode == "minimized")
            state.windows[0].sizemode = "normal";
          
          
          state.windows.forEach(function(aWindow) {
            delete aWindow.__lastSessionWindowID;
          });
        }
      }
      catch (ex) { debug("The session file is invalid: " + ex); }
    }

    
    
    if (this._loadState != STATE_QUITTING &&
        this._prefBranch.getBoolPref("sessionstore.resume_session_once"))
      this._prefBranch.setBoolPref("sessionstore.resume_session_once", false);

    this._performUpgradeBackup();
    this._sessionInitialized = true;

    return state;
  },

  



  _performUpgradeBackup: function ssi_performUpgradeBackup() {
    
    const PREF_UPGRADE = "sessionstore.upgradeBackup.latestBuildID";

    let buildID = Services.appinfo.platformBuildID;
    let latestBackup = this._prefBranch.getCharPref(PREF_UPGRADE);
    if (latestBackup == buildID) {
      return Promise.resolve();
    }
    return Task.spawn(function task() {
      try {
        
        yield _SessionFile.createBackupCopy("-" + buildID);

        this._prefBranch.setCharPref(PREF_UPGRADE, buildID);

        
        yield _SessionFile.removeBackupCopy("-" + latestBackup);
      } catch (ex) {
        debug("Could not perform upgrade backup " + ex);
        debug(ex.stack);
      }
    }.bind(this));
  },

  _initPrefs : function() {
    this._prefBranch = Services.prefs.getBranch("browser.");

    gDebuggingEnabled = this._prefBranch.getBoolPref("sessionstore.debug");

    Services.prefs.addObserver("browser.sessionstore.debug", () => {
      gDebuggingEnabled = this._prefBranch.getBoolPref("sessionstore.debug");
    }, false);

    XPCOMUtils.defineLazyGetter(this, "_max_tabs_undo", function () {
      this._prefBranch.addObserver("sessionstore.max_tabs_undo", this, true);
      return this._prefBranch.getIntPref("sessionstore.max_tabs_undo");
    });

    XPCOMUtils.defineLazyGetter(this, "_max_windows_undo", function () {
      this._prefBranch.addObserver("sessionstore.max_windows_undo", this, true);
      return this._prefBranch.getIntPref("sessionstore.max_windows_undo");
    });
  },

  



  _uninit: function ssi_uninit() {
    if (!this._initialized) {
      throw new Error("SessionStore is not initialized.");
    }

    
    if (this._sessionInitialized) {
      SessionSaver.run();
    }

    
    TabRestoreQueue.reset();

    
    SessionSaver.cancel();
  },

  


  observe: function ssi_observe(aSubject, aTopic, aData) {
    switch (aTopic) {
      case "domwindowopened": 
        this.onOpen(aSubject);
        break;
      case "domwindowclosed": 
        this.onClose(aSubject);
        break;
      case "quit-application-requested":
        this.onQuitApplicationRequested();
        break;
      case "quit-application-granted":
        this.onQuitApplicationGranted();
        break;
      case "browser-lastwindow-close-granted":
        this.onLastWindowCloseGranted();
        break;
      case "quit-application":
        this.onQuitApplication(aData);
        break;
      case "browser:purge-session-history": 
        this.onPurgeSessionHistory();
        break;
      case "browser:purge-domain-data":
        this.onPurgeDomainData(aData);
        break;
      case "nsPref:changed": 
        this.onPrefChange(aData);
        break;
      case "timer-callback": 
        this.onTimerCallback();
        break;
    }
  },

  



  receiveMessage: function ssi_receiveMessage(aMessage) {
    var browser = aMessage.target;
    var win = browser.ownerDocument.defaultView;

    switch (aMessage.name) {
      case "SessionStore:pageshow":
        this.onTabLoad(win, browser);
        break;
      case "SessionStore:input":
        this.onTabInput(win, browser);
        break;
      case "SessionStore:MozStorageChanged":
        TabStateCache.delete(browser);
        this.saveStateDelayed(win);
        break;
      case "SessionStore:loadStart":
        TabStateCache.delete(browser);
        break;
      default:
        debug("received unknown message '" + aMessage.name + "'");
        break;
    }

    this._clearRestoringWindows();
  },

  

  


  handleEvent: function ssi_handleEvent(aEvent) {
    if (this._disabledForMultiProcess)
      return;

    var win = aEvent.currentTarget.ownerDocument.defaultView;
    switch (aEvent.type) {
      case "load":
        
        
        
        let browser = aEvent.currentTarget;
        TabStateCache.delete(browser);
        if (browser.__SS_restore_data)
          this.restoreDocument(win, browser, aEvent);
        this.onTabLoad(win, browser);
        break;
      case "TabOpen":
        this.onTabAdd(win, aEvent.originalTarget);
        break;
      case "TabClose":
        
        if (!aEvent.detail)
          this.onTabClose(win, aEvent.originalTarget);
        this.onTabRemove(win, aEvent.originalTarget);
        break;
      case "TabSelect":
        this.onTabSelect(win);
        break;
      case "TabShow":
        this.onTabShow(win, aEvent.originalTarget);
        break;
      case "TabHide":
        this.onTabHide(win, aEvent.originalTarget);
        break;
      case "TabPinned":
        
        TabStateCache.updateField(aEvent.originalTarget, "pinned", true);
        this.saveStateDelayed(win);
        break;
      case "TabUnpinned":
        
        TabStateCache.updateField(aEvent.originalTarget, "pinned", false);
        this.saveStateDelayed(win);
        break;
    }
    this._clearRestoringWindows();
  },

  










  onLoad: function ssi_onLoad(aWindow, aInitialState = null) {
    
    if (aWindow && aWindow.__SSi && this._windows[aWindow.__SSi])
      return;

    
    if (aWindow.document.documentElement.getAttribute("windowtype") != "navigator:browser" ||
        this._loadState == STATE_QUITTING)
      return;

    
    aWindow.__SSi = "window" + Date.now();

    
    this._windows[aWindow.__SSi] = { tabs: [], selected: 0, _closedTabs: [], busy: false };

    let isPrivateWindow = false;
    if (PrivateBrowsingUtils.isWindowPrivate(aWindow))
      this._windows[aWindow.__SSi].isPrivate = isPrivateWindow = true;
    if (!this._isWindowLoaded(aWindow))
      this._windows[aWindow.__SSi]._restoring = true;
    if (!aWindow.toolbar.visible)
      this._windows[aWindow.__SSi].isPopup = true;

    
    if (this._loadState == STATE_STOPPED) {
      this._loadState = STATE_RUNNING;
      SessionSaver.updateLastSaveTime();

      
      if (aInitialState) {
        if (isPrivateWindow) {
          
          
          
          this._deferredInitialState = aInitialState;

          
          Services.obs.notifyObservers(null, NOTIFY_WINDOWS_RESTORED, "");
        } else {
          TelemetryTimestamps.add("sessionRestoreRestoring");
          this._restoreCount = aInitialState.windows ? aInitialState.windows.length : 0;

          let overwrite = this._isCmdLineEmpty(aWindow, aInitialState);
          let options = {firstWindow: true, overwriteTabs: overwrite};
          this.restoreWindow(aWindow, aInitialState, options);

          
          
          
          _SessionFile.writeLoadStateOnceAfterStartup(STATE_RUNNING_STR);
        }
      }
      else {
        
        Services.obs.notifyObservers(null, NOTIFY_WINDOWS_RESTORED, "");

        
        SessionSaver.clearLastSaveTime();
      }
    }
    
    else if (!this._isWindowLoaded(aWindow)) {
      let state = this._statesToRestore[aWindow.__SS_restoreID];
      let options = {overwriteTabs: true, isFollowUp: state.windows.length == 1};
      this.restoreWindow(aWindow, state, options);
    }
    
    
    
    else if (this._deferredInitialState && !isPrivateWindow &&
             aWindow.toolbar.visible) {

      this._restoreCount = this._deferredInitialState.windows ?
        this._deferredInitialState.windows.length : 0;
      this.restoreWindow(aWindow, this._deferredInitialState, {firstWindow: true});
      this._deferredInitialState = null;
    }
    else if (this._restoreLastWindow && aWindow.toolbar.visible &&
             this._closedWindows.length && !isPrivateWindow) {

      
      
      let closedWindowState = null;
      let closedWindowIndex;
      for (let i = 0; i < this._closedWindows.length; i++) {
        
        if (!this._closedWindows[i].isPopup) {
          closedWindowState = this._closedWindows[i];
          closedWindowIndex = i;
          break;
        }
      }

      if (closedWindowState) {
        let newWindowState;
#ifndef XP_MACOSX
        if (!this._doResumeSession()) {
#endif
          
          
          
          
          
          let [appTabsState, normalTabsState] =
            this._prepDataForDeferredRestore({ windows: [closedWindowState] });

          
          if (appTabsState.windows.length) {
            newWindowState = appTabsState.windows[0];
            delete newWindowState.__lastSessionWindowID;
          }

          
          if (!normalTabsState.windows.length) {
            this._closedWindows.splice(closedWindowIndex, 1);
          }
          
          else {
            delete normalTabsState.windows[0].__lastSessionWindowID;
            this._closedWindows[closedWindowIndex] = normalTabsState.windows[0];
          }
#ifndef XP_MACOSX
        }
        else {
          
          
          this._closedWindows.splice(closedWindowIndex, 1);
          newWindowState = closedWindowState;
          delete newWindowState.hidden;
        }
#endif
        if (newWindowState) {
          
          this._restoreCount = 1;
          let state = { windows: [newWindowState] };
          let options = {overwriteTabs: this._isCmdLineEmpty(aWindow, state)};
          this.restoreWindow(aWindow, state, options);
        }
      }
      
      this._prefBranch.setBoolPref("sessionstore.resume_session_once", false);
    }
    if (this._restoreLastWindow && aWindow.toolbar.visible) {
      
      
      
      this._restoreLastWindow = false;
    }

    var tabbrowser = aWindow.gBrowser;

    
    for (let i = 0; i < tabbrowser.tabs.length; i++) {
      this.onTabAdd(aWindow, tabbrowser.tabs[i], true);
    }
    
    TAB_EVENTS.forEach(function(aEvent) {
      tabbrowser.tabContainer.addEventListener(aEvent, this, true);
    }, this);
  },

  




  onOpen: function ssi_onOpen(aWindow) {
    let onload = () => {
      aWindow.removeEventListener("load", onload);
      this.onLoad(aWindow);
    };

    aWindow.addEventListener("load", onload);
  },

  






  onClose: function ssi_onClose(aWindow) {
    
    let isFullyLoaded = this._isWindowLoaded(aWindow);
    if (!isFullyLoaded) {
      if (!aWindow.__SSi)
        aWindow.__SSi = "window" + Date.now();
      this._windows[aWindow.__SSi] = this._statesToRestore[aWindow.__SS_restoreID];
      delete this._statesToRestore[aWindow.__SS_restoreID];
      delete aWindow.__SS_restoreID;
    }

    
    if (!aWindow.__SSi || !this._windows[aWindow.__SSi]) {
      return;
    }

    
    
    
    let event = aWindow.document.createEvent("Events");
    event.initEvent("SSWindowClosing", true, false);
    aWindow.dispatchEvent(event);

    if (this.windowToFocus && this.windowToFocus == aWindow) {
      delete this.windowToFocus;
    }

    var tabbrowser = aWindow.gBrowser;

    TAB_EVENTS.forEach(function(aEvent) {
      tabbrowser.tabContainer.removeEventListener(aEvent, this, true);
    }, this);

    
    tabbrowser.removeTabsProgressListener(gRestoreTabsProgressListener);

    let winData = this._windows[aWindow.__SSi];
    if (this._loadState == STATE_RUNNING) { 
      
      this._collectWindowData(aWindow);

      if (isFullyLoaded) {
        winData.title = aWindow.content.document.title || tabbrowser.selectedTab.label;
        winData.title = this._replaceLoadingTitle(winData.title, tabbrowser,
                                                  tabbrowser.selectedTab);
        SessionCookies.update([winData]);
      }

#ifndef XP_MACOSX
      
      
      winData._shouldRestore = true;
#endif

      
      
      if (!winData.isPrivate && (winData.tabs.length > 1 ||
          (winData.tabs.length == 1 && this._shouldSaveTabState(winData.tabs[0])))) {
        
        delete winData.busy;

        this._closedWindows.unshift(winData);
        this._capClosedWindows();
      }

      
      delete this._windows[aWindow.__SSi];

      
      this.saveStateDelayed();
    }

    for (let i = 0; i < tabbrowser.tabs.length; i++) {
      this.onTabRemove(aWindow, tabbrowser.tabs[i], true);
    }

    
    DyingWindowCache.set(aWindow, winData);

    delete aWindow.__SSi;
  },

  


  onQuitApplicationRequested: function ssi_onQuitApplicationRequested() {
    
    this._forEachBrowserWindow(function(aWindow) {
      this._collectWindowData(aWindow);
    });
    
    
    var activeWindow = this._getMostRecentBrowserWindow();
    if (activeWindow)
      this.activeWindowSSiCache = activeWindow.__SSi || "";
    DirtyWindows.clear();
  },

  


  onQuitApplicationGranted: function ssi_onQuitApplicationGranted() {
    
    this._loadState = STATE_QUITTING;
  },

  


  onLastWindowCloseGranted: function ssi_onLastWindowCloseGranted() {
    
    
    
    
    this._restoreLastWindow = true;
  },

  




  onQuitApplication: function ssi_onQuitApplication(aData) {
    if (aData == "restart") {
      this._prefBranch.setBoolPref("sessionstore.resume_session_once", true);
      
      
      
      
      
      
      Services.obs.removeObserver(this, "browser:purge-session-history");
    }

    if (aData != "restart") {
      
      this._lastSessionState = null;
    }

    this._loadState = STATE_QUITTING; 
    this._uninit();
  },

  


  onPurgeSessionHistory: function ssi_onPurgeSessionHistory() {
    _SessionFile.wipe();
    
    
    
    if (this._loadState == STATE_QUITTING)
      return;
    this._lastSessionState = null;
    let openWindows = {};
    this._forEachBrowserWindow(function(aWindow) {
      Array.forEach(aWindow.gBrowser.tabs, function(aTab) {
        TabStateCache.delete(aTab);
        delete aTab.linkedBrowser.__SS_data;
        delete aTab.linkedBrowser.__SS_tabStillLoading;
        if (aTab.linkedBrowser.__SS_restoreState)
          this._resetTabRestoringState(aTab);
      }, this);
      openWindows[aWindow.__SSi] = true;
    });
    
    for (let ix in this._windows) {
      if (ix in openWindows) {
        this._windows[ix]._closedTabs = [];
      } else {
        delete this._windows[ix];
      }
    }
    
    this._closedWindows = [];
    
    var win = this._getMostRecentBrowserWindow();
    if (win) {
      win.setTimeout(() => SessionSaver.run(), 0);
    } else if (this._loadState == STATE_RUNNING) {
      SessionSaver.run();
    }

    this._clearRestoringWindows();
  },

  




  onPurgeDomainData: function ssi_onPurgeDomainData(aData) {
    
    function containsDomain(aEntry) {
      try {
        if (makeURI(aEntry.url).host.hasRootDomain(aData)) {
          return true;
        }
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

    if (this._loadState == STATE_RUNNING) {
      SessionSaver.run();
    }

    this._clearRestoringWindows();
  },

  




  onPrefChange: function ssi_onPrefChange(aData) {
    switch (aData) {
      
      
      case "sessionstore.max_tabs_undo":
        this._max_tabs_undo = this._prefBranch.getIntPref("sessionstore.max_tabs_undo");
        for (let ix in this._windows) {
          this._windows[ix]._closedTabs.splice(this._max_tabs_undo, this._windows[ix]._closedTabs.length);
        }
        break;
      case "sessionstore.max_windows_undo":
        this._max_windows_undo = this._prefBranch.getIntPref("sessionstore.max_windows_undo");
        this._capClosedWindows();
        break;
    }
  },

  








  onTabAdd: function ssi_onTabAdd(aWindow, aTab, aNoNotification) {
    let browser = aTab.linkedBrowser;
    browser.addEventListener("load", this, true);

    let mm = browser.messageManager;
    MESSAGES.forEach(msg => mm.addMessageListener(msg, this));

    if (!aNoNotification) {
      this.saveStateDelayed(aWindow);
    }

    this._updateCrashReportURL(aWindow);
  },

  








  onTabRemove: function ssi_onTabRemove(aWindow, aTab, aNoNotification) {
    let browser = aTab.linkedBrowser;
    browser.removeEventListener("load", this, true);

    let mm = browser.messageManager;
    MESSAGES.forEach(msg => mm.removeMessageListener(msg, this));

    delete browser.__SS_data;
    delete browser.__SS_tabStillLoading;

    
    
    
    let previousState = browser.__SS_restoreState;
    if (previousState) {
      this._resetTabRestoringState(aTab);
      if (previousState == TAB_STATE_RESTORING)
        this.restoreNextTab();
    }

    if (!aNoNotification) {
      this.saveStateDelayed(aWindow);
    }
  },

  






  onTabClose: function ssi_onTabClose(aWindow, aTab) {
    
    
    var event = aWindow.document.createEvent("Events");
    event.initEvent("SSTabClosing", true, false);
    aTab.dispatchEvent(event);

    
    if (this._max_tabs_undo == 0) {
      return;
    }

    
    let tabState = TabState.collectSync(aTab);

    
    if (this._shouldSaveTabState(tabState)) {
      let tabTitle = aTab.label;
      let tabbrowser = aWindow.gBrowser;
      tabTitle = this._replaceLoadingTitle(tabTitle, tabbrowser, aTab);

      this._windows[aWindow.__SSi]._closedTabs.unshift({
        state: tabState,
        title: tabTitle,
        image: tabbrowser.getIcon(aTab),
        pos: aTab._tPos
      });
      var length = this._windows[aWindow.__SSi]._closedTabs.length;
      if (length > this._max_tabs_undo)
        this._windows[aWindow.__SSi]._closedTabs.splice(this._max_tabs_undo, length - this._max_tabs_undo);
    }
  },

  







  onTabLoad: function ssi_onTabLoad(aWindow, aBrowser) {
    
    
    
    
    if (aBrowser.__SS_restoreState &&
        aBrowser.__SS_restoreState == TAB_STATE_NEEDS_RESTORE) {
      return;
    }

    TabStateCache.delete(aBrowser);

    delete aBrowser.__SS_data;
    delete aBrowser.__SS_tabStillLoading;
    this.saveStateDelayed(aWindow);

    
    this._updateCrashReportURL(aWindow);
  },

  






  onTabInput: function ssi_onTabInput(aWindow, aBrowser) {
    TabStateCache.delete(aBrowser);
    this.saveStateDelayed(aWindow);
  },

  




  onTabSelect: function ssi_onTabSelect(aWindow) {
    if (this._loadState == STATE_RUNNING) {
      this._windows[aWindow.__SSi].selected = aWindow.gBrowser.tabContainer.selectedIndex;

      let tab = aWindow.gBrowser.selectedTab;
      
      
      
      if (tab.linkedBrowser.__SS_restoreState &&
          tab.linkedBrowser.__SS_restoreState == TAB_STATE_NEEDS_RESTORE)
        this.restoreTab(tab);

      
      this._updateCrashReportURL(aWindow);
    }
  },

  onTabShow: function ssi_onTabShow(aWindow, aTab) {
    
    if (aTab.linkedBrowser.__SS_restoreState &&
        aTab.linkedBrowser.__SS_restoreState == TAB_STATE_NEEDS_RESTORE) {
      TabRestoreQueue.hiddenToVisible(aTab);

      
      
      this.restoreNextTab();
    }

    
    TabStateCache.updateField(aTab, "hidden", false);

    
    
    this.saveStateDelayed(aWindow);
  },

  onTabHide: function ssi_onTabHide(aWindow, aTab) {
    
    if (aTab.linkedBrowser.__SS_restoreState &&
        aTab.linkedBrowser.__SS_restoreState == TAB_STATE_NEEDS_RESTORE) {
      TabRestoreQueue.visibleToHidden(aTab);
    }

    
    TabStateCache.updateField(aTab, "hidden", true);

    
    
    this.saveStateDelayed(aWindow);
  },

  

  getBrowserState: function ssi_getBrowserState() {
    let state = this.getCurrentState();

    
    delete state.lastSessionState;

    return this._toJSONString(state);
  },

  setBrowserState: function ssi_setBrowserState(aState) {
    this._handleClosedWindows();

    try {
      var state = JSON.parse(aState);
    }
    catch (ex) {  }
    if (!state || !state.windows)
      throw (Components.returnCode = Cr.NS_ERROR_INVALID_ARG);

    this._browserSetState = true;

    
    this._resetRestoringState();

    var window = this._getMostRecentBrowserWindow();
    if (!window) {
      this._restoreCount = 1;
      this._openWindowWithState(state);
      return;
    }

    
    this._forEachBrowserWindow(function(aWindow) {
      if (aWindow != window) {
        aWindow.close();
        this.onClose(aWindow);
      }
    });

    
    this._closedWindows = [];

    
    this._restoreCount = state.windows ? state.windows.length : 0;

    
    this.restoreWindow(window, state, {overwriteTabs: true});
  },

  getWindowState: function ssi_getWindowState(aWindow) {
    if ("__SSi" in aWindow) {
      return this._toJSONString(this._getWindowState(aWindow));
    }

    if (DyingWindowCache.has(aWindow)) {
      let data = DyingWindowCache.get(aWindow);
      return this._toJSONString({ windows: [data] });
    }

    throw (Components.returnCode = Cr.NS_ERROR_INVALID_ARG);
  },

  setWindowState: function ssi_setWindowState(aWindow, aState, aOverwrite) {
    if (!aWindow.__SSi)
      throw (Components.returnCode = Cr.NS_ERROR_INVALID_ARG);

    this.restoreWindow(aWindow, aState, {overwriteTabs: aOverwrite});
  },

  getTabState: function ssi_getTabState(aTab) {
    if (!aTab.ownerDocument || !aTab.ownerDocument.defaultView.__SSi)
      throw (Components.returnCode = Cr.NS_ERROR_INVALID_ARG);

    let tabState = TabState.collectSync(aTab);

    return this._toJSONString(tabState);
  },

  setTabState: function ssi_setTabState(aTab, aState) {
    
    
    
    
    let tabState = JSON.parse(aState);
    if (!tabState) {
      debug("Empty state argument");
      throw (Components.returnCode = Cr.NS_ERROR_INVALID_ARG);
    }
    if (typeof tabState != "object") {
      debug("State argument does not represent an object");
      throw (Components.returnCode = Cr.NS_ERROR_INVALID_ARG);
    }
    if (!("entries" in tabState)) {
      debug("State argument must contain field 'entries'");
      throw (Components.returnCode = Cr.NS_ERROR_INVALID_ARG);
    }
    if (!aTab.ownerDocument) {
      debug("Tab argument must have an owner document");
      throw (Components.returnCode = Cr.NS_ERROR_INVALID_ARG);
    }

    let window = aTab.ownerDocument.defaultView;
    if (!("__SSi" in window)) {
      debug("Default view of ownerDocument must have a unique identifier");
      throw (Components.returnCode = Cr.NS_ERROR_INVALID_ARG);
    }

    TabStateCache.delete(aTab);
    this._setWindowStateBusy(window);
    this.restoreHistoryPrecursor(window, [aTab], [tabState], 0, 0, 0);
  },

  duplicateTab: function ssi_duplicateTab(aWindow, aTab, aDelta) {
    if (!aTab.ownerDocument || !aTab.ownerDocument.defaultView.__SSi ||
        !aWindow.getBrowser)
      throw (Components.returnCode = Cr.NS_ERROR_INVALID_ARG);

    
    let tabState = TabState.clone(aTab);

    tabState.index += aDelta;
    tabState.index = Math.max(1, Math.min(tabState.index, tabState.entries.length));
    tabState.pinned = false;

    this._setWindowStateBusy(aWindow);
    let newTab = aTab == aWindow.gBrowser.selectedTab ?
      aWindow.gBrowser.addTab(null, {relatedToCurrent: true, ownerTab: aTab}) :
      aWindow.gBrowser.addTab();

    this.restoreHistoryPrecursor(aWindow, [newTab], [tabState], 0, 0, 0,
                                 true );

    return newTab;
  },

  setNumberOfTabsClosedLast: function ssi_setNumberOfTabsClosedLast(aWindow, aNumber) {
    if (this._disabledForMultiProcess)
      return;

    if ("__SSi" in aWindow) {
      return NumberOfTabsClosedLastPerWindow.set(aWindow, aNumber);
    }

    throw (Components.returnCode = Cr.NS_ERROR_INVALID_ARG);
  },

  
  getNumberOfTabsClosedLast: function ssi_getNumberOfTabsClosedLast(aWindow) {
    if (this._disabledForMultiProcess)
      return 0;

    if ("__SSi" in aWindow) {
      
      
      
      
      return Math.min(NumberOfTabsClosedLastPerWindow.get(aWindow) || 1,
                      this.getClosedTabCount(aWindow));
    }

    throw (Components.returnCode = Cr.NS_ERROR_INVALID_ARG);
  },

  getClosedTabCount: function ssi_getClosedTabCount(aWindow) {
    if ("__SSi" in aWindow) {
      return this._windows[aWindow.__SSi]._closedTabs.length;
    }

    if (DyingWindowCache.has(aWindow)) {
      return DyingWindowCache.get(aWindow)._closedTabs.length;
    }

    throw (Components.returnCode = Cr.NS_ERROR_INVALID_ARG);
  },

  getClosedTabData: function ssi_getClosedTabDataAt(aWindow) {
    if ("__SSi" in aWindow) {
      return this._toJSONString(this._windows[aWindow.__SSi]._closedTabs);
    }

    if (DyingWindowCache.has(aWindow)) {
      let data = DyingWindowCache.get(aWindow);
      return this._toJSONString(data._closedTabs);
    }

    throw (Components.returnCode = Cr.NS_ERROR_INVALID_ARG);
  },

  undoCloseTab: function ssi_undoCloseTab(aWindow, aIndex) {
    if (!aWindow.__SSi)
      throw (Components.returnCode = Cr.NS_ERROR_INVALID_ARG);

    var closedTabs = this._windows[aWindow.__SSi]._closedTabs;

    
    aIndex = aIndex || 0;
    if (!(aIndex in closedTabs))
      throw (Components.returnCode = Cr.NS_ERROR_INVALID_ARG);

    
    let closedTab = closedTabs.splice(aIndex, 1).shift();
    let closedTabState = closedTab.state;

    this._setWindowStateBusy(aWindow);
    
    let tabbrowser = aWindow.gBrowser;
    let tab = tabbrowser.addTab();

    
    this.restoreHistoryPrecursor(aWindow, [tab], [closedTabState], 1, 0, 0);

    
    tabbrowser.moveTabTo(tab, closedTab.pos);

    
    tab.linkedBrowser.focus();

    return tab;
  },

  forgetClosedTab: function ssi_forgetClosedTab(aWindow, aIndex) {
    if (!aWindow.__SSi)
      throw (Components.returnCode = Cr.NS_ERROR_INVALID_ARG);

    var closedTabs = this._windows[aWindow.__SSi]._closedTabs;

    
    aIndex = aIndex || 0;
    if (!(aIndex in closedTabs))
      throw (Components.returnCode = Cr.NS_ERROR_INVALID_ARG);

    
    closedTabs.splice(aIndex, 1);
  },

  getClosedWindowCount: function ssi_getClosedWindowCount() {
    return this._closedWindows.length;
  },

  getClosedWindowData: function ssi_getClosedWindowData() {
    return this._toJSONString(this._closedWindows);
  },

  undoCloseWindow: function ssi_undoCloseWindow(aIndex) {
    if (!(aIndex in this._closedWindows))
      throw (Components.returnCode = Cr.NS_ERROR_INVALID_ARG);

    
    let state = { windows: this._closedWindows.splice(aIndex, 1) };
    let window = this._openWindowWithState(state);
    this.windowToFocus = window;
    return window;
  },

  forgetClosedWindow: function ssi_forgetClosedWindow(aIndex) {
    
    aIndex = aIndex || 0;
    if (!(aIndex in this._closedWindows))
      throw (Components.returnCode = Cr.NS_ERROR_INVALID_ARG);

    
    this._closedWindows.splice(aIndex, 1);
  },

  getWindowValue: function ssi_getWindowValue(aWindow, aKey) {
    if (this._disabledForMultiProcess)
      return "";

    if ("__SSi" in aWindow) {
      var data = this._windows[aWindow.__SSi].extData || {};
      return data[aKey] || "";
    }

    if (DyingWindowCache.has(aWindow)) {
      let data = DyingWindowCache.get(aWindow).extData || {};
      return data[aKey] || "";
    }

    throw (Components.returnCode = Cr.NS_ERROR_INVALID_ARG);
  },

  setWindowValue: function ssi_setWindowValue(aWindow, aKey, aStringValue) {
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

  deleteWindowValue: function ssi_deleteWindowValue(aWindow, aKey) {
    if (aWindow.__SSi && this._windows[aWindow.__SSi].extData &&
        this._windows[aWindow.__SSi].extData[aKey])
      delete this._windows[aWindow.__SSi].extData[aKey];
    this.saveStateDelayed(aWindow);
  },

  getTabValue: function ssi_getTabValue(aTab, aKey) {
    let data = {};
    if (aTab.__SS_extdata) {
      data = aTab.__SS_extdata;
    }
    else if (aTab.linkedBrowser.__SS_data && aTab.linkedBrowser.__SS_data.extData) {
      
      data = aTab.linkedBrowser.__SS_data.extData;
    }
    return data[aKey] || "";
  },

  setTabValue: function ssi_setTabValue(aTab, aKey, aStringValue) {
    
    
    let saveTo;
    if (aTab.__SS_extdata) {
      saveTo = aTab.__SS_extdata;
    }
    else if (aTab.linkedBrowser.__SS_data && aTab.linkedBrowser.__SS_data.extData) {
      saveTo = aTab.linkedBrowser.__SS_data.extData;
    }
    else {
      aTab.__SS_extdata = {};
      saveTo = aTab.__SS_extdata;
    }

    saveTo[aKey] = aStringValue;
    TabStateCache.updateField(aTab, "extData", saveTo);
    this.saveStateDelayed(aTab.ownerDocument.defaultView);
  },

  deleteTabValue: function ssi_deleteTabValue(aTab, aKey) {
    
    
    
    let deleteFrom;
    if (aTab.__SS_extdata) {
      deleteFrom = aTab.__SS_extdata;
    }
    else if (aTab.linkedBrowser.__SS_data && aTab.linkedBrowser.__SS_data.extData) {
      deleteFrom = aTab.linkedBrowser.__SS_data.extData;
    }

    if (deleteFrom && aKey in deleteFrom) {
      delete deleteFrom[aKey];

      
      
      if (Object.keys(deleteFrom).length) {
        TabStateCache.updateField(aTab, "extData", deleteFrom);
      } else {
        TabStateCache.removeField(aTab, "extData");
      }

      this.saveStateDelayed(aTab.ownerDocument.defaultView);
    }
  },

  persistTabAttribute: function ssi_persistTabAttribute(aName) {
    if (TabAttributes.persist(aName)) {
      TabStateCache.clear();
      this.saveStateDelayed();
    }
  },

  






  restoreLastSession: function ssi_restoreLastSession() {
    
    if (!this.canRestoreLastSession)
      throw (Components.returnCode = Cr.NS_ERROR_FAILURE);

    
    let windows = {};
    this._forEachBrowserWindow(function(aWindow) {
      if (aWindow.__SS_lastSessionWindowID)
        windows[aWindow.__SS_lastSessionWindowID] = aWindow;
    });

    let lastSessionState = this._lastSessionState;

    
    if (!lastSessionState.windows.length)
      throw (Components.returnCode = Cr.NS_ERROR_UNEXPECTED);

    
    
    this._restoreCount = lastSessionState.windows.length;
    this._browserSetState = true;

    
    
    
    
    let lastWindow = this._getMostRecentBrowserWindow();
    let canUseLastWindow = lastWindow &&
                           !lastWindow.__SS_lastSessionWindowID;

    
    for (let i = 0; i < lastSessionState.windows.length; i++) {
      let winState = lastSessionState.windows[i];
      let lastSessionWindowID = winState.__lastSessionWindowID;
      
      delete winState.__lastSessionWindowID;

      
      
      
      let windowToUse = windows[lastSessionWindowID];
      if (!windowToUse && canUseLastWindow) {
        windowToUse = lastWindow;
        canUseLastWindow = false;
      }

      let [canUseWindow, canOverwriteTabs] = this._prepWindowToRestoreInto(windowToUse);

      
      if (canUseWindow) {
        
        
        if (winState._closedTabs && winState._closedTabs.length) {
          let curWinState = this._windows[windowToUse.__SSi];
          curWinState._closedTabs = curWinState._closedTabs.concat(winState._closedTabs);
          curWinState._closedTabs.splice(this._prefBranch.getIntPref("sessionstore.max_tabs_undo"), curWinState._closedTabs.length);
        }

        
        
        
        
        
        
        
        let options = {overwriteTabs: canOverwriteTabs, isFollowUp: true};
        this.restoreWindow(windowToUse, { windows: [winState] }, options);
      }
      else {
        this._openWindowWithState({ windows: [winState] });
      }
    }

    
    if (lastSessionState._closedWindows) {
      this._closedWindows = this._closedWindows.concat(lastSessionState._closedWindows);
      this._capClosedWindows();
    }

    if (lastSessionState.scratchpads) {
      ScratchpadManager.restoreSession(lastSessionState.scratchpads);
    }

    
    this._recentCrashes = lastSessionState.session &&
                          lastSessionState.session.recentCrashes || 0;

    
    this._updateSessionStartTime(lastSessionState);

    this._lastSessionState = null;
  },

  










  _prepWindowToRestoreInto: function ssi_prepWindowToRestoreInto(aWindow) {
    if (!aWindow)
      return [false, false];

    
    
    let canOverwriteTabs = false;

    
    
    
    
    let groupsData = this.getWindowValue(aWindow, "tabview-groups");
    if (groupsData) {
      groupsData = JSON.parse(groupsData);

      
      if (groupsData.totalNumber > 1)
        return [false, false];
    }

    
    
    
    
    
    
    let homePages = ["about:blank"];
    let removableTabs = [];
    let tabbrowser = aWindow.gBrowser;
    let normalTabsLen = tabbrowser.tabs.length - tabbrowser._numPinnedTabs;
    let startupPref = this._prefBranch.getIntPref("startup.page");
    if (startupPref == 1)
      homePages = homePages.concat(aWindow.gHomeButton.getHomePage().split("|"));

    for (let i = tabbrowser._numPinnedTabs; i < tabbrowser.tabs.length; i++) {
      let tab = tabbrowser.tabs[i];
      if (homePages.indexOf(tab.linkedBrowser.currentURI.spec) != -1) {
        removableTabs.push(tab);
      }
    }

    if (tabbrowser.tabs.length == removableTabs.length) {
      canOverwriteTabs = true;
    }
    else {
      
      for (let i = removableTabs.length - 1; i >= 0; i--) {
        tabbrowser.removeTab(removableTabs.pop(), { animate: false });
      }
    }

    return [true, canOverwriteTabs];
  },

  

  




  _updateWindowFeatures: function ssi_updateWindowFeatures(aWindow) {
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

  





  getCurrentState: function (aUpdateAll) {
    this._handleClosedWindows();

    var activeWindow = this._getMostRecentBrowserWindow();

    if (this._loadState == STATE_RUNNING) {
      
      this._forEachBrowserWindow(function(aWindow) {
        if (!this._isWindowLoaded(aWindow)) 
          return;
        if (aUpdateAll || DirtyWindows.has(aWindow) || aWindow == activeWindow) {
          this._collectWindowData(aWindow);
        }
        else { 
          this._updateWindowFeatures(aWindow);
        }
      });
      DirtyWindows.clear();
    }

    
    var total = [];
    
    var ids = [];
    
    var nonPopupCount = 0;
    var ix;

    
    for (ix in this._windows) {
      if (this._windows[ix]._restoring) 
        continue;
      total.push(this._windows[ix]);
      ids.push(ix);
      if (!this._windows[ix].isPopup)
        nonPopupCount++;
    }
    SessionCookies.update(total);

    
    for (ix in this._statesToRestore) {
      for each (let winData in this._statesToRestore[ix].windows) {
        total.push(winData);
        if (!winData.isPopup)
          nonPopupCount++;
      }
    }

    
    let lastClosedWindowsCopy = this._closedWindows.slice();

#ifndef XP_MACOSX
    
    
    
    
    
    if (nonPopupCount == 0 && lastClosedWindowsCopy.length > 0 &&
        this._loadState == STATE_QUITTING) {
      
      
      do {
        total.unshift(lastClosedWindowsCopy.shift())
      } while (total[0].isPopup && lastClosedWindowsCopy.length > 0)
    }
#endif

    if (activeWindow) {
      this.activeWindowSSiCache = activeWindow.__SSi || "";
    }
    ix = ids.indexOf(this.activeWindowSSiCache);
    
    
    if (ix != -1 && total[ix] && total[ix].sizemode == "minimized")
      ix = -1;

    let session = {
      state: this._loadState == STATE_RUNNING ? STATE_RUNNING_STR : STATE_STOPPED_STR,
      lastUpdate: Date.now(),
      startTime: this._sessionStartTime,
      recentCrashes: this._recentCrashes
    };

    
    var scratchpads = ScratchpadManager.getSessionState();

    let state = {
      windows: total,
      selectedWindow: ix + 1,
      _closedWindows: lastClosedWindowsCopy,
      session: session,
      scratchpads: scratchpads
    };

    
    if (this._lastSessionState) {
      state.lastSessionState = this._lastSessionState;
    }

    return state;
  },

  





  _getWindowState: function ssi_getWindowState(aWindow) {
    if (!this._isWindowLoaded(aWindow))
      return this._statesToRestore[aWindow.__SS_restoreID];

    if (this._loadState == STATE_RUNNING) {
      this._collectWindowData(aWindow);
    }

    let windows = [this._windows[aWindow.__SSi]];
    SessionCookies.update(windows);

    return { windows: windows };
  },

  _collectWindowData: function ssi_collectWindowData(aWindow) {
    if (!this._isWindowLoaded(aWindow))
      return;

    let tabbrowser = aWindow.gBrowser;
    let tabs = tabbrowser.tabs;
    let winData = this._windows[aWindow.__SSi];
    let tabsData = winData.tabs = [];

    
    for (let tab of tabs) {
      tabsData.push(TabState.collectSync(tab));
    }
    winData.selected = tabbrowser.mTabBox.selectedIndex + 1;

    this._updateWindowFeatures(aWindow);

    
    
    if (aWindow.__SS_lastSessionWindowID)
      this._windows[aWindow.__SSi].__lastSessionWindowID =
        aWindow.__SS_lastSessionWindowID;

    DirtyWindows.remove(aWindow);
  },

  

  












  restoreWindow: function ssi_restoreWindow(aWindow, aState, aOptions = {}) {
    let overwriteTabs = aOptions && aOptions.overwriteTabs;
    let isFollowUp = aOptions && aOptions.isFollowUp;
    let firstWindow = aOptions && aOptions.firstWindow;

    if (isFollowUp) {
      this.windowToFocus = aWindow;
    }
    
    if (aWindow && (!aWindow.__SSi || !this._windows[aWindow.__SSi]))
      this.onLoad(aWindow);

    try {
      var root = typeof aState == "string" ? JSON.parse(aState) : aState;
      if (!root.windows[0]) {
        this._sendRestoreCompletedNotifications();
        return; 
      }
    }
    catch (ex) { 
      debug(ex);
      this._sendRestoreCompletedNotifications();
      return;
    }

    TelemetryStopwatch.start("FX_SESSION_RESTORE_RESTORE_WINDOW_MS");

    
    
    this._setWindowStateBusy(aWindow);

    if (root._closedWindows)
      this._closedWindows = root._closedWindows;

    var winData;
    if (!root.selectedWindow || root.selectedWindow > root.windows.length) {
      root.selectedWindow = 0;
    }

    
    
    for (var w = 1; w < root.windows.length; w++) {
      winData = root.windows[w];
      if (winData && winData.tabs && winData.tabs[0]) {
        var window = this._openWindowWithState({ windows: [winData] });
        if (w == root.selectedWindow - 1) {
          this.windowToFocus = window;
        }
      }
    }
    winData = root.windows[0];
    if (!winData.tabs) {
      winData.tabs = [];
    }
    
    
    else if (firstWindow && !overwriteTabs && winData.tabs.length == 1 &&
             (!winData.tabs[0].entries || winData.tabs[0].entries.length == 0)) {
      winData.tabs = [];
    }

    var tabbrowser = aWindow.gBrowser;
    var openTabCount = overwriteTabs ? tabbrowser.browsers.length : -1;
    var newTabCount = winData.tabs.length;
    var tabs = [];

    
    var tabstrip = tabbrowser.tabContainer.mTabstrip;
    var smoothScroll = tabstrip.smoothScroll;
    tabstrip.smoothScroll = false;

    
    if (overwriteTabs) {
      for (let t = tabbrowser._numPinnedTabs - 1; t > -1; t--)
        tabbrowser.unpinTab(tabbrowser.tabs[t]);
    }

    
    
    if (overwriteTabs && tabbrowser.selectedTab._tPos >= newTabCount)
      tabbrowser.moveTabTo(tabbrowser.selectedTab, newTabCount - 1);

    let numVisibleTabs = 0;

    for (var t = 0; t < newTabCount; t++) {
      tabs.push(t < openTabCount ?
                tabbrowser.tabs[t] :
                tabbrowser.addTab("about:blank", {skipAnimation: true}));
      
      if (!overwriteTabs && firstWindow) {
        tabbrowser.moveTabTo(tabs[t], t);
      }

      if (winData.tabs[t].pinned)
        tabbrowser.pinTab(tabs[t]);

      if (winData.tabs[t].hidden) {
        tabbrowser.hideTab(tabs[t]);
      }
      else {
        tabbrowser.showTab(tabs[t]);
        numVisibleTabs++;
      }
    }

    
    if (!numVisibleTabs && winData.tabs.length) {
      winData.tabs[0].hidden = false;
      tabbrowser.showTab(tabs[0]);
    }

    
    
    
    
    
    if (overwriteTabs) {
      for (let i = 0; i < tabbrowser.tabs.length; i++) {
        let tab = tabbrowser.tabs[i];
        TabStateCache.delete(tab);
        if (tabbrowser.browsers[i].__SS_restoreState)
          this._resetTabRestoringState(tab);
      }
    }

    
    
    
    
    
    if (!aWindow.__SS_tabsToRestore)
      aWindow.__SS_tabsToRestore = 0;
    if (overwriteTabs)
      aWindow.__SS_tabsToRestore = newTabCount;
    else
      aWindow.__SS_tabsToRestore += newTabCount;

    
    
    
    delete aWindow.__SS_lastSessionWindowID;
    if (winData.__lastSessionWindowID)
      aWindow.__SS_lastSessionWindowID = winData.__lastSessionWindowID;

    
    if (overwriteTabs && newTabCount < openTabCount) {
      Array.slice(tabbrowser.tabs, newTabCount, openTabCount)
           .forEach(tabbrowser.removeTab, tabbrowser);
    }

    if (overwriteTabs) {
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
    if (overwriteTabs || firstWindow) {
      this._windows[aWindow.__SSi]._closedTabs = winData._closedTabs || [];
    }

    this.restoreHistoryPrecursor(aWindow, tabs, winData.tabs,
      (overwriteTabs ? (parseInt(winData.selected) || 1) : 0), 0, 0);

    if (aState.scratchpads) {
      ScratchpadManager.restoreSession(aState.scratchpads);
    }

    
    tabstrip.smoothScroll = smoothScroll;

    TelemetryStopwatch.finish("FX_SESSION_RESTORE_RESTORE_WINDOW_MS");

    this._sendRestoreCompletedNotifications();
  },

  












  _setTabsRestoringOrder : function ssi__setTabsRestoringOrder(
    aTabBrowser, aTabs, aTabData, aSelectedTab) {

    
    let selectedTab;
    if (aSelectedTab > 0 && aTabs[aSelectedTab - 1]) {
      selectedTab = aTabs[aSelectedTab - 1];
    }

    
    let pinnedTabs = [];
    let pinnedTabsData = [];
    let hiddenTabs = [];
    let hiddenTabsData = [];
    if (aTabs.length > 1) {
      for (let t = aTabs.length - 1; t >= 0; t--) {
        if (aTabData[t].pinned) {
          pinnedTabs.unshift(aTabs.splice(t, 1)[0]);
          pinnedTabsData.unshift(aTabData.splice(t, 1)[0]);
        } else if (aTabData[t].hidden) {
          hiddenTabs.unshift(aTabs.splice(t, 1)[0]);
          hiddenTabsData.unshift(aTabData.splice(t, 1)[0]);
        }
      }
    }

    
    if (selectedTab) {
      let selectedTabIndex = aTabs.indexOf(selectedTab);
      if (selectedTabIndex > 0) {
        let scrollSize = aTabBrowser.tabContainer.mTabstrip.scrollClientSize;
        let tabWidth = aTabs[0].getBoundingClientRect().width;
        let maxVisibleTabs = Math.ceil(scrollSize / tabWidth);
        if (maxVisibleTabs < aTabs.length) {
          let firstVisibleTab = 0;
          let nonVisibleTabsCount = aTabs.length - maxVisibleTabs;
          if (nonVisibleTabsCount >= selectedTabIndex) {
            
            firstVisibleTab = selectedTabIndex;
          } else {
            
            firstVisibleTab = nonVisibleTabsCount;
          }
          aTabs = aTabs.splice(firstVisibleTab, maxVisibleTabs).concat(aTabs);
          aTabData =
            aTabData.splice(firstVisibleTab, maxVisibleTabs).concat(aTabData);
        }
      }
    }

    
    aTabs = pinnedTabs.concat(aTabs, hiddenTabs);
    aTabData = pinnedTabsData.concat(aTabData, hiddenTabsData);

    
    if (selectedTab) {
      let selectedTabIndex = aTabs.indexOf(selectedTab);
      if (selectedTabIndex > 0) {
        aTabs = aTabs.splice(selectedTabIndex, 1).concat(aTabs);
        aTabData = aTabData.splice(selectedTabIndex, 1).concat(aTabData);
      }
      aTabBrowser.selectedTab = selectedTab;
    }

    return [aTabs, aTabData];
  },
  
  

















  restoreHistoryPrecursor:
    function ssi_restoreHistoryPrecursor(aWindow, aTabs, aTabData, aSelectTab,
                                         aIx, aCount, aRestoreImmediately = false) {

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
            self.restoreHistoryPrecursor(aWindow, aTabs, aTabData, aSelectTab,
                                         aIx, aCount + 1, aRestoreImmediately);
          };
          aWindow.setTimeout(restoreHistoryFunc, 100, this);
          return;
        }
      }
    }

    if (!this._isWindowLoaded(aWindow)) {
      
      delete this._statesToRestore[aWindow.__SS_restoreID];
      delete aWindow.__SS_restoreID;
      delete this._windows[aWindow.__SSi]._restoring;

      
      
      DirtyWindows.add(aWindow);
    }

    if (aTabs.length == 0) {
      
      
      this._setWindowStateReady(aWindow);
      return;
    }

    
    [aTabs, aTabData] =
      this._setTabsRestoringOrder(tabbrowser, aTabs, aTabData, aSelectTab);

    
    
    
    
    for (t = 0; t < aTabs.length; t++) {
      let tab = aTabs[t];
      let browser = tabbrowser.getBrowserForTab(tab);
      let tabData = aTabData[t];

      if (tabData.pinned)
        tabbrowser.pinTab(tab);
      else
        tabbrowser.unpinTab(tab);

      if (tabData.hidden)
        tabbrowser.hideTab(tab);
      else
        tabbrowser.showTab(tab);

      if ("attributes" in tabData) {
        
        Object.keys(tabData.attributes).forEach(a => TabAttributes.persist(a));
      }

      browser.__SS_tabStillLoading = true;

      
      
      browser.__SS_data = tabData;
      browser.__SS_restoreState = TAB_STATE_NEEDS_RESTORE;
      browser.setAttribute("pending", "true");
      tab.setAttribute("pending", "true");

      
      
      delete tab.__SS_extdata;

      if (!tabData.entries || tabData.entries.length == 0) {
        
        
        browser.contentDocument.location = "about:blank";
        continue;
      }

      browser.stop(); 

      
      
      let activeIndex = (tabData.index || tabData.entries.length) - 1;
      let activePageData = tabData.entries[activeIndex] || null;
      let uri = activePageData ? activePageData.url || null : null;
      browser.userTypedValue = uri;

      
      
      
      if (uri) {
        browser.docShell.setCurrentURI(makeURI(uri));
      }

      
      if (activePageData) {
        if (activePageData.title) {
          tab.label = activePageData.title;
          tab.crop = "end";
        } else if (activePageData.url != "about:blank") {
          tab.label = activePageData.url;
          tab.crop = "center";
        }
      }
    }

    
    
    var idMap = { used: {} };
    var docIdentMap = {};
    this.restoreHistory(aWindow, aTabs, aTabData, idMap, docIdentMap,
                        aRestoreImmediately);
  },

  













  restoreHistory:
    function ssi_restoreHistory(aWindow, aTabs, aTabData, aIdMap, aDocIdentMap,
                                aRestoreImmediately) {
    var _this = this;
    
    while (aTabs.length > 0 && !(this._canRestoreTabHistory(aTabs[0]))) {
      aTabs.shift();
      aTabData.shift();
    }
    if (aTabs.length == 0) {
      
      
      this._setWindowStateReady(aWindow);
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

    browser.__SS_shistoryListener = new SessionStoreSHistoryListener(tab);
    history.addSHistoryListener(browser.__SS_shistoryListener);

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
      history.addEntry(this._deserializeHistoryEntry(tabData.entries[i],
                                                     aIdMap, aDocIdentMap), true);
    }

    
    let disallow = new Set(tabData.disallow && tabData.disallow.split(","));
    for (let cap of gDocShellCapabilities(browser.docShell))
      browser.docShell["allow" + cap] = !disallow.has(cap);

    
    if ("attributes" in tabData) {
      TabAttributes.set(tab, tabData.attributes);
    }

    
    if ("image" in tabData) {
      aWindow.gBrowser.setIcon(tab, tabData.image);
    }

    if (tabData.storage && browser.docShell instanceof Ci.nsIDocShell)
      SessionStorage.deserialize(browser.docShell, tabData.storage);

    
    var event = aWindow.document.createEvent("Events");
    event.initEvent("SSTabRestoring", true, false);
    tab.dispatchEvent(event);

    
    aWindow.setTimeout(function(){
      _this.restoreHistory(aWindow, aTabs, aTabData, aIdMap, aDocIdentMap,
                           aRestoreImmediately);
    }, 0);

    
    
    if (aRestoreImmediately || aWindow.gBrowser.selectedBrowser == browser) {
      this.restoreTab(tab);
    }
    else {
      TabRestoreQueue.add(tab);
      this.restoreNextTab();
    }
  },

  
















  restoreTab: function ssi_restoreTab(aTab) {
    let window = aTab.ownerDocument.defaultView;
    let browser = aTab.linkedBrowser;
    let tabData = browser.__SS_data;

    
    
    
    let didStartLoad = false;

    
    this._ensureTabsProgressListener(window);

    
    TabRestoreQueue.remove(aTab);

    
    this._tabsRestoringCount++;

    
    browser.__SS_restoreState = TAB_STATE_RESTORING;
    browser.removeAttribute("pending");
    aTab.removeAttribute("pending");

    
    this._removeSHistoryListener(aTab);

    let activeIndex = (tabData.index || tabData.entries.length) - 1;
    if (activeIndex >= tabData.entries.length)
      activeIndex = tabData.entries.length - 1;
    
    
    
    browser.webNavigation.setCurrentURI(makeURI("about:blank"));
    
    if (activeIndex > -1) {
      
      
      browser.__SS_restore_data = tabData.entries[activeIndex] || {};
      browser.__SS_restore_pageStyle = tabData.pageStyle || "";
      browser.__SS_restore_tab = aTab;
      didStartLoad = true;
      try {
        
        
        
        browser.webNavigation.sessionHistory.getEntryAtIndex(activeIndex, true);
        browser.webNavigation.sessionHistory.reloadCurrentEntry();
      }
      catch (ex) {
        
        aTab.removeAttribute("busy");
        didStartLoad = false;
      }
    }

    
    
    if (tabData.userTypedValue) {
      browser.userTypedValue = tabData.userTypedValue;
      if (tabData.userTypedClear) {
        
        
        
        browser.__SS_restore_data = { url: null };
        browser.__SS_restore_tab = aTab;
        if (didStartLoad)
          browser.stop();
        didStartLoad = true;
        browser.loadURIWithFlags(tabData.userTypedValue,
                                 Ci.nsIWebNavigation.LOAD_FLAGS_ALLOW_THIRD_PARTY_FIXUP);
      }
    }

    
    
    
    if (!didStartLoad) {
      this._sendTabRestoredNotification(aTab);
      this._resetTabRestoringState(aTab);
    }

    return didStartLoad;
  },

  







  restoreNextTab: function ssi_restoreNextTab() {
    
    if (this._loadState == STATE_QUITTING)
      return;

    
    if (this._tabsRestoringCount >= MAX_CONCURRENT_TAB_RESTORES)
      return;

    let tab = TabRestoreQueue.shift();
    if (tab) {
      let didStartLoad = this.restoreTab(tab);
      
      
      if (!didStartLoad)
        this.restoreNextTab();
    }
  },

  







  _deserializeHistoryEntry:
    function ssi_deserializeHistoryEntry(aEntry, aIdMap, aDocIdentMap) {

    var shEntry = Cc["@mozilla.org/browser/session-history-entry;1"].
                  createInstance(Ci.nsISHEntry);

    shEntry.setURI(makeURI(aEntry.url));
    shEntry.setTitle(aEntry.title || aEntry.url);
    if (aEntry.subframe)
      shEntry.setIsSubFrame(aEntry.subframe || false);
    shEntry.loadType = Ci.nsIDocShellLoadInfo.loadHistory;
    if (aEntry.contentType)
      shEntry.contentType = aEntry.contentType;
    if (aEntry.referrer)
      shEntry.referrerURI = makeURI(aEntry.referrer);
    if (aEntry.isSrcdocEntry)
      shEntry.srcdocData = aEntry.srcdocData;

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

    if (aEntry.docshellID)
      shEntry.docshellID = aEntry.docshellID;

    if (aEntry.structuredCloneState && aEntry.structuredCloneVersion) {
      shEntry.stateData =
        Cc["@mozilla.org/docshell/structured-clone-container;1"].
        createInstance(Ci.nsIStructuredCloneContainer);

      shEntry.stateData.initFromBase64(aEntry.structuredCloneState,
                                       aEntry.structuredCloneVersion);
    }

    if (aEntry.scroll) {
      var scrollPos = (aEntry.scroll || "0,0").split(",");
      scrollPos = [parseInt(scrollPos[0]) || 0, parseInt(scrollPos[1]) || 0];
      shEntry.setScrollPosition(scrollPos[0], scrollPos[1]);
    }

    if (aEntry.postdata_b64) {
      var postdata = atob(aEntry.postdata_b64);
      var stream = Cc["@mozilla.org/io/string-input-stream;1"].
                   createInstance(Ci.nsIStringInputStream);
      stream.setData(postdata, postdata.length);
      shEntry.postData = stream;
    }

    let childDocIdents = {};
    if (aEntry.docIdentifier) {
      
      
      
      
      let matchingEntry = aDocIdentMap[aEntry.docIdentifier];
      if (!matchingEntry) {
        matchingEntry = {shEntry: shEntry, childDocIdents: childDocIdents};
        aDocIdentMap[aEntry.docIdentifier] = matchingEntry;
      }
      else {
        shEntry.adoptBFCacheEntry(matchingEntry.shEntry);
        childDocIdents = matchingEntry.childDocIdents;
      }
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
    }

    if (aEntry.children && shEntry instanceof Ci.nsISHContainer) {
      for (var i = 0; i < aEntry.children.length; i++) {
        
        if (!aEntry.children[i].url)
          continue;

        
        
        
        
        
        
        
        
        
        
        
        
        
        

        shEntry.AddChild(this._deserializeHistoryEntry(aEntry.children[i], aIdMap,
                                                       childDocIdents), i);
      }
    }

    return shEntry;
  },

  


  restoreDocument: function ssi_restoreDocument(aWindow, aBrowser, aEvent) {
    
    if (!aEvent || !aEvent.originalTarget || !aEvent.originalTarget.defaultView ||
        aEvent.originalTarget.defaultView != aEvent.originalTarget.defaultView.top) {
      return;
    }

    
    function hasExpectedURL(aDocument, aURL)
      !aURL || aURL.replace(/#.*/, "") == aDocument.location.href.replace(/#.*/, "");

    let selectedPageStyle = aBrowser.__SS_restore_pageStyle;
    function restoreTextDataAndScrolling(aContent, aData, aPrefix) {
      if (aData.formdata && hasExpectedURL(aContent.document, aData.url)) {
        let formdata = aData.formdata;

        
        
        if (!("xpath" in formdata || "id" in formdata)) {
          formdata = { xpath: {}, id: {} };

          for each (let [key, value] in Iterator(aData.formdata)) {
            if (key.charAt(0) == "#") {
              formdata.id[key.slice(1)] = value;
            } else {
              formdata.xpath[key] = value;
            }
          }
        }

        
        
        
        if ((aData.url == "about:sessionrestore" || aData.url == "about:welcomeback") &&
            "sessionData" in formdata.id &&
            typeof formdata.id["sessionData"] == "object") {
          formdata.id["sessionData"] =
            JSON.stringify(formdata.id["sessionData"]);
        }

        
        aData.formdata = formdata;
        
        DocumentUtils.mergeFormData(aContent.document, formdata);
      }

      if (aData.innerHTML) {
        aWindow.setTimeout(function() {
          if (aContent.document.designMode == "on" &&
              hasExpectedURL(aContent.document, aData.url) &&
              aContent.document.body) {
            aContent.document.body.innerHTML = aData.innerHTML;
          }
        }, 0);
      }
      var match;
      if (aData.scroll && (match = /(\d+),(\d+)/.exec(aData.scroll)) != null) {
        aContent.scrollTo(match[1], match[2]);
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

    
    
    if (hasExpectedURL(aEvent.originalTarget, aBrowser.__SS_restore_data.url)) {
      var content = aEvent.originalTarget.defaultView;
      restoreTextDataAndScrolling(content, aBrowser.__SS_restore_data, "");
      aBrowser.markupDocumentViewer.authorStyleDisabled = selectedPageStyle == "_nostyle";
    }

    
    this._sendTabRestoredNotification(aBrowser.__SS_restore_tab);

    delete aBrowser.__SS_restore_data;
    delete aBrowser.__SS_restore_pageStyle;
    delete aBrowser.__SS_restore_tab;
  },

  






  restoreWindowFeatures: function ssi_restoreWindowFeatures(aWindow, aWinData) {
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
      _this.restoreDimensions.apply(_this, [aWindow,
        +aWinData.width || 0,
        +aWinData.height || 0,
        "screenX" in aWinData ? +aWinData.screenX : NaN,
        "screenY" in aWinData ? +aWinData.screenY : NaN,
        aWinData.sizemode || "", aWinData.sidebar || ""]);
    }, 0);
  },

  














  restoreDimensions: function ssi_restoreDimensions(aWindow, aWidth, aHeight, aLeft, aTop, aSizeMode, aSidebar) {
    var win = aWindow;
    var _this = this;
    function win_(aName) { return _this._getWindowDimension(win, aName); }

    
    let screen = gScreenManager.screenForRect(aLeft, aTop, aWidth, aHeight);
    if (screen) {
      let screenLeft = {}, screenTop = {}, screenWidth = {}, screenHeight = {};
      screen.GetAvailRectDisplayPix(screenLeft, screenTop, screenWidth, screenHeight);
      
      if (aWidth > screenWidth.value) {
        aWidth = screenWidth.value;
      }
      if (aHeight > screenHeight.value) {
        aHeight = screenHeight.value;
      }
      
      if (aLeft < screenLeft.value) {
        aLeft = screenLeft.value;
      } else if (aLeft + aWidth > screenLeft.value + screenWidth.value) {
        aLeft = screenLeft.value + screenWidth.value - aWidth;
      }
      if (aTop < screenTop.value) {
        aTop = screenTop.value;
      } else if (aTop + aHeight > screenTop.value + screenHeight.value) {
        aTop = screenTop.value + screenHeight.value - aHeight;
      }
    }

    
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
    
    
    if (this.windowToFocus) {
      this.windowToFocus.focus();
    }
  },

  




  restoreCookies: function ssi_restoreCookies(aCookies) {
    
    var MAX_EXPIRY = Math.pow(2, 62);
    for (let i = 0; i < aCookies.length; i++) {
      var cookie = aCookies[i];
      try {
        Services.cookies.add(cookie.host, cookie.path || "", cookie.name || "",
                             cookie.value, !!cookie.secure, !!cookie.httponly, true,
                             "expiry" in cookie ? cookie.expiry : MAX_EXPIRY);
      }
      catch (ex) { Cu.reportError(ex); } 
    }
  },

  

  






  saveStateDelayed: function (aWindow = null) {
    if (aWindow) {
      DirtyWindows.add(aWindow);
    }

    SessionSaver.runDelayed();
  },

  

  






  _updateSessionStartTime: function ssi_updateSessionStartTime(state) {
    
    if (state.session && state.session.startTime) {
      this._sessionStartTime = state.session.startTime;

      
      let sessionLength = (Date.now() - this._sessionStartTime) / MS_PER_DAY;

      if (sessionLength > 0) {
        
        Services.telemetry.getHistogramById("FX_SESSION_RESTORE_SESSION_LENGTH").add(sessionLength);
      }
    }
  },

  





  _forEachBrowserWindow: function ssi_forEachBrowserWindow(aFunc) {
    var windowsEnum = Services.wm.getEnumerator("navigator:browser");

    while (windowsEnum.hasMoreElements()) {
      var window = windowsEnum.getNext();
      if (window.__SSi && !window.closed) {
        aFunc.call(this, window);
      }
    }
  },

  



  _getMostRecentBrowserWindow: function ssi_getMostRecentBrowserWindow() {
    var win = Services.wm.getMostRecentWindow("navigator:browser");
    if (!win)
      return null;
    if (!win.closed)
      return win;

#ifdef BROKEN_WM_Z_ORDER
    win = null;
    var windowsEnum = Services.wm.getEnumerator("navigator:browser");
    
    while (windowsEnum.hasMoreElements()) {
      let nextWin = windowsEnum.getNext();
      if (!nextWin.closed)
        win = nextWin;
    }
    return win;
#else
    var windowsEnum =
      Services.wm.getZOrderDOMWindowEnumerator("navigator:browser", true);
    while (windowsEnum.hasMoreElements()) {
      win = windowsEnum.getNext();
      if (!win.closed)
        return win;
    }
    return null;
#endif
  },

  




  _handleClosedWindows: function ssi_handleClosedWindows() {
    var windowsEnum = Services.wm.getEnumerator("navigator:browser");

    while (windowsEnum.hasMoreElements()) {
      var window = windowsEnum.getNext();
      if (window.closed) {
        this.onClose(window);
      }
    }
  },

  





  _openWindowWithState: function ssi_openWindowWithState(aState) {
    var argString = Cc["@mozilla.org/supports-string;1"].
                    createInstance(Ci.nsISupportsString);
    argString.data = "";

    
    let features = "chrome,dialog=no,macsuppressanimation,all";
    let winState = aState.windows[0];
    WINDOW_ATTRIBUTES.forEach(function(aFeature) {
      
      if (aFeature in winState && !isNaN(winState[aFeature]))
        features += "," + aFeature + "=" + winState[aFeature];
    });

    if (winState.isPrivate) {
      features += ",private";
    }

    var window =
      Services.ww.openWindow(null, this._prefBranch.getCharPref("chromeURL"),
                             "_blank", features, argString);

    do {
      var ID = "window" + Math.random();
    } while (ID in this._statesToRestore);
    this._statesToRestore[(window.__SS_restoreID = ID)] = aState;

    return window;
  },

  







  _getTabForBrowser: function ssi_getTabForBrowser(aBrowser) {
    let window = aBrowser.ownerDocument.defaultView;
    for (let i = 0; i < window.gBrowser.tabs.length; i++) {
      let tab = window.gBrowser.tabs[i];
      if (tab.linkedBrowser == aBrowser)
        return tab;
    }
    return undefined;
  },

  



  _doResumeSession: function ssi_doResumeSession() {
    return this._prefBranch.getIntPref("startup.page") == 3 ||
           this._prefBranch.getBoolPref("sessionstore.resume_session_once");
  },

  





  _isCmdLineEmpty: function ssi_isCmdLineEmpty(aWindow, aState) {
    var pinnedOnly = aState.windows &&
                     aState.windows.every(function (win)
                       win.tabs.every(function (tab) tab.pinned));

    let hasFirstArgument = aWindow.arguments && aWindow.arguments[0];
    if (!pinnedOnly) {
      let defaultArgs = Cc["@mozilla.org/browser/clh;1"].
                        getService(Ci.nsIBrowserHandler).defaultArgs;
      if (aWindow.arguments &&
          aWindow.arguments[0] &&
          aWindow.arguments[0] == defaultArgs)
        hasFirstArgument = false;
    }

    return !hasFirstArgument;
  },

  










  _getWindowDimension: function ssi_getWindowDimension(aWindow, aAttribute) {
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

  




  _getURIFromString: function ssi_getURIFromString(aString) {
    return Services.io.newURI(aString, null, null);
  },

  


  _updateCrashReportURL: function ssi_updateCrashReportURL(aWindow) {
#ifdef MOZ_CRASHREPORTER
    try {
      var currentURI = aWindow.gBrowser.currentURI.clone();
      
      try {
        currentURI.userPass = "";
      }
      catch (ex) { } 

      CrashReporter.annotateCrashReport("URL", currentURI.spec);
    }
    catch (ex) {
      
      if (ex.result != Components.results.NS_ERROR_NOT_INITIALIZED)
        debug(ex);
    }
#endif
  },

  




  _needsRestorePage: function ssi_needsRestorePage(aState, aRecentCrashes) {
    const SIX_HOURS_IN_MS = 6 * 60 * 60 * 1000;

    
    let winData = aState.windows || null;
    if (!winData || winData.length == 0)
      return false;

    
    if (this._hasSingleTabWithURL(winData, "about:sessionrestore") ||
        this._hasSingleTabWithURL(winData, "about:welcomeback")) {
      return false;
    }

    
    if (Services.appinfo.inSafeMode)
      return true;

    let max_resumed_crashes =
      this._prefBranch.getIntPref("sessionstore.max_resumed_crashes");
    let sessionAge = aState.session && aState.session.lastUpdate &&
                     (Date.now() - aState.session.lastUpdate);

    return max_resumed_crashes != -1 &&
           (aRecentCrashes > max_resumed_crashes ||
            sessionAge && sessionAge >= SIX_HOURS_IN_MS);
  },

  




  _hasSingleTabWithURL: function(aWinData, aURL) {
    if (aWinData &&
        aWinData.length == 1 &&
        aWinData[0].tabs &&
        aWinData[0].tabs.length == 1 &&
        aWinData[0].tabs[0].entries &&
        aWinData[0].tabs[0].entries.length == 1) {
      return aURL == aWinData[0].tabs[0].entries[0].url;
    }
    return false;
  },

  







  _shouldSaveTabState: function ssi_shouldSaveTabState(aTabState) {
    
    
    
    return aTabState.entries.length &&
           !(aTabState.entries.length == 1 &&
                (aTabState.entries[0].url == "about:blank" ||
                 aTabState.entries[0].url == "about:newtab") &&
                 !aTabState.userTypedValue);
  },

  








  _canRestoreTabHistory: function ssi_canRestoreTabHistory(aTab) {
    return aTab.parentNode && aTab.linkedBrowser &&
           aTab.linkedBrowser.__SS_tabStillLoading;
  },

  
















  _prepDataForDeferredRestore: function ssi_prepDataForDeferredRestore(state) {
    let defaultState = { windows: [], selectedWindow: 1 };

    state.selectedWindow = state.selectedWindow || 1;

    
    
    for (let wIndex = 0; wIndex < state.windows.length;) {
      let window = state.windows[wIndex];
      window.selected = window.selected || 1;
      
      let pinnedWindowState = { tabs: [], cookies: []};
      for (let tIndex = 0; tIndex < window.tabs.length;) {
        if (window.tabs[tIndex].pinned) {
          
          if (tIndex + 1 < window.selected)
            window.selected -= 1;
          else if (tIndex + 1 == window.selected)
            pinnedWindowState.selected = pinnedWindowState.tabs.length + 2;
            

          
          pinnedWindowState.tabs =
            pinnedWindowState.tabs.concat(window.tabs.splice(tIndex, 1));
          
          continue;
        }
        tIndex++;
      }

      
      
      if (pinnedWindowState.tabs.length) {
        
        WINDOW_ATTRIBUTES.forEach(function(attr) {
          if (attr in window) {
            pinnedWindowState[attr] = window[attr];
            delete window[attr];
          }
        });
        
        
        
        
        
        

        
        
        window.__lastSessionWindowID = pinnedWindowState.__lastSessionWindowID
                                     = "" + Date.now() + Math.random();

        
        this._splitCookiesFromWindow(window, pinnedWindowState);

        
        defaultState.windows.push(pinnedWindowState);
        
        if (!window.tabs.length) {
          if (wIndex + 1 <= state.selectedWindow)
            state.selectedWindow -= 1;
          else if (wIndex + 1 == state.selectedWindow)
            defaultState.selectedIndex = defaultState.windows.length + 1;

          state.windows.splice(wIndex, 1);
          
          continue;
        }


      }
      wIndex++;
    }

    return [defaultState, state];
  },

  




  _splitCookiesFromWindow:
    function ssi_splitCookiesFromWindow(aWinState, aTargetWinState) {
    if (!aWinState.cookies || !aWinState.cookies.length)
      return;

    
    let cookieHosts = SessionCookies.getHostsForWindow(aTargetWinState);

    
    
    let hosts = Object.keys(cookieHosts).join("|").replace("\\.", "\\.", "g");
    
    if (!hosts.length)
      return;
    let cookieRegex = new RegExp(".*(" + hosts + ")");
    for (let cIndex = 0; cIndex < aWinState.cookies.length;) {
      if (cookieRegex.test(aWinState.cookies[cIndex].host)) {
        aTargetWinState.cookies =
          aTargetWinState.cookies.concat(aWinState.cookies.splice(cIndex, 1));
        continue;
      }
      cIndex++;
    }
  },

  








  _toJSONString: function ssi_toJSONString(aJSObject) {
    return JSON.stringify(aJSObject);
  },

  _sendRestoreCompletedNotifications: function ssi_sendRestoreCompletedNotifications() {
    
    if (this._restoreCount > 1) {
      this._restoreCount--;
      return;
    }

    
    if (this._restoreCount == -1)
      return;

    
    Services.obs.notifyObservers(null,
      this._browserSetState ? NOTIFY_BROWSER_STATE_RESTORED : NOTIFY_WINDOWS_RESTORED,
      "");

    this._browserSetState = false;
    this._restoreCount = -1;
  },

   




  _setWindowStateBusyValue:
    function ssi_changeWindowStateBusyValue(aWindow, aValue) {

    this._windows[aWindow.__SSi].busy = aValue;

    
    
    if (!this._isWindowLoaded(aWindow)) {
      let stateToRestore = this._statesToRestore[aWindow.__SS_restoreID].windows[0];
      stateToRestore.busy = aValue;
    }
  },

  



  _setWindowStateReady: function ssi_setWindowStateReady(aWindow) {
    this._setWindowStateBusyValue(aWindow, false);
    this._sendWindowStateEvent(aWindow, "Ready");
  },

  



  _setWindowStateBusy: function ssi_setWindowStateBusy(aWindow) {
    this._setWindowStateBusyValue(aWindow, true);
    this._sendWindowStateEvent(aWindow, "Busy");
  },

  




  _sendWindowStateEvent: function ssi_sendWindowStateEvent(aWindow, aType) {
    let event = aWindow.document.createEvent("Events");
    event.initEvent("SSWindowState" + aType, true, false);
    aWindow.dispatchEvent(event);
  },

  



  _sendTabRestoredNotification: function ssi_sendTabRestoredNotification(aTab) {
      let event = aTab.ownerDocument.createEvent("Events");
      event.initEvent("SSTabRestored", true, false);
      aTab.dispatchEvent(event);
  },

  





  _isWindowLoaded: function ssi_isWindowLoaded(aWindow) {
    return !aWindow.__SS_restoreID;
  },

  







  _replaceLoadingTitle : function ssi_replaceLoadingTitle(aString, aTabbrowser, aTab) {
    if (aString == aTabbrowser.mStringBundle.getString("tabs.connecting")) {
      aTabbrowser.setTabTitle(aTab);
      [aString, aTab.label] = [aTab.label, aString];
    }
    return aString;
  },

  




  _capClosedWindows : function ssi_capClosedWindows() {
    if (this._closedWindows.length <= this._max_windows_undo)
      return;
    let spliceTo = this._max_windows_undo;
#ifndef XP_MACOSX
    let normalWindowIndex = 0;
    
    while (normalWindowIndex < this._closedWindows.length &&
           !!this._closedWindows[normalWindowIndex].isPopup)
      normalWindowIndex++;
    if (normalWindowIndex >= this._max_windows_undo)
      spliceTo = normalWindowIndex + 1;
#endif
    this._closedWindows.splice(spliceTo, this._closedWindows.length);
  },

  _clearRestoringWindows: function ssi_clearRestoringWindows() {
    for (let i = 0; i < this._closedWindows.length; i++) {
      delete this._closedWindows[i]._shouldRestore;
    }
  },

  


  _resetRestoringState: function ssi_initRestoringState() {
    TabRestoreQueue.reset();
    this._tabsRestoringCount = 0;
  },

  






  _resetTabRestoringState: function ssi_resetTabRestoringState(aTab) {
    let window = aTab.ownerDocument.defaultView;
    let browser = aTab.linkedBrowser;

    
    let previousState = browser.__SS_restoreState;

    
    delete browser.__SS_restoreState;

    aTab.removeAttribute("pending");
    browser.removeAttribute("pending");

    
    
    window.__SS_tabsToRestore--;

    
    this._removeTabsProgressListener(window);

    if (previousState == TAB_STATE_RESTORING) {
      if (this._tabsRestoringCount)
        this._tabsRestoringCount--;
    }
    else if (previousState == TAB_STATE_NEEDS_RESTORE) {
      
      
      this._removeSHistoryListener(aTab);

      
      
      
      TabRestoreQueue.remove(aTab);
    }
  },

  





  _ensureTabsProgressListener: function ssi_ensureTabsProgressListener(aWindow) {
    let tabbrowser = aWindow.gBrowser;
    if (tabbrowser.mTabsProgressListeners.indexOf(gRestoreTabsProgressListener) == -1)
      tabbrowser.addTabsProgressListener(gRestoreTabsProgressListener);
  },

  





  _removeTabsProgressListener: function ssi_removeTabsProgressListener(aWindow) {
    
    
    if (!aWindow.__SS_tabsToRestore)
      aWindow.gBrowser.removeTabsProgressListener(gRestoreTabsProgressListener);
  },

  





  _removeSHistoryListener: function ssi_removeSHistoryListener(aTab) {
    let browser = aTab.linkedBrowser;
    if (browser.__SS_shistoryListener) {
      browser.webNavigation.sessionHistory.
                            removeSHistoryListener(browser.__SS_shistoryListener);
      delete browser.__SS_shistoryListener;
    }
  }
};







let TabRestoreQueue = {
  
  tabs: {priority: [], visible: [], hidden: []},

  
  
  prefs: {
    
    get restoreOnDemand() {
      let updateValue = () => {
        let value = Services.prefs.getBoolPref(PREF);
        let definition = {value: value, configurable: true};
        Object.defineProperty(this, "restoreOnDemand", definition);
        return value;
      }

      const PREF = "browser.sessionstore.restore_on_demand";
      Services.prefs.addObserver(PREF, updateValue, false);
      return updateValue();
    },

    
    get restorePinnedTabsOnDemand() {
      let updateValue = () => {
        let value = Services.prefs.getBoolPref(PREF);
        let definition = {value: value, configurable: true};
        Object.defineProperty(this, "restorePinnedTabsOnDemand", definition);
        return value;
      }

      const PREF = "browser.sessionstore.restore_pinned_tabs_on_demand";
      Services.prefs.addObserver(PREF, updateValue, false);
      return updateValue();
    },

    
    get restoreHiddenTabs() {
      let updateValue = () => {
        let value = Services.prefs.getBoolPref(PREF);
        let definition = {value: value, configurable: true};
        Object.defineProperty(this, "restoreHiddenTabs", definition);
        return value;
      }

      const PREF = "browser.sessionstore.restore_hidden_tabs";
      Services.prefs.addObserver(PREF, updateValue, false);
      return updateValue();
    }
  },

  
  reset: function () {
    this.tabs = {priority: [], visible: [], hidden: []};
  },

  
  add: function (tab) {
    let {priority, hidden, visible} = this.tabs;

    if (tab.pinned) {
      priority.push(tab);
    } else if (tab.hidden) {
      hidden.push(tab);
    } else {
      visible.push(tab);
    }
  },

  
  remove: function (tab) {
    let {priority, hidden, visible} = this.tabs;

    
    
    let set = priority;
    let index = set.indexOf(tab);

    if (index == -1) {
      set = tab.hidden ? hidden : visible;
      index = set.indexOf(tab);
    }

    if (index > -1) {
      set.splice(index, 1);
    }
  },

  
  shift: function () {
    let set;
    let {priority, hidden, visible} = this.tabs;

    let {restoreOnDemand, restorePinnedTabsOnDemand} = this.prefs;
    let restorePinned = !(restoreOnDemand && restorePinnedTabsOnDemand);
    if (restorePinned && priority.length) {
      set = priority;
    } else if (!restoreOnDemand) {
      if (visible.length) {
        set = visible;
      } else if (this.prefs.restoreHiddenTabs && hidden.length) {
        set = hidden;
      }
    }

    return set && set.shift();
  },

  
  hiddenToVisible: function (tab) {
    let {hidden, visible} = this.tabs;
    let index = hidden.indexOf(tab);

    if (index > -1) {
      hidden.splice(index, 1);
      visible.push(tab);
    } else {
      throw new Error("restore queue: hidden tab not found");
    }
  },

  
  visibleToHidden: function (tab) {
    let {visible, hidden} = this.tabs;
    let index = visible.indexOf(tab);

    if (index > -1) {
      visible.splice(index, 1);
      hidden.push(tab);
    } else {
      throw new Error("restore queue: visible tab not found");
    }
  }
};




let DyingWindowCache = {
  _data: new WeakMap(),

  has: function (window) {
    return this._data.has(window);
  },

  get: function (window) {
    return this._data.get(window);
  },

  set: function (window, data) {
    this._data.set(window, data);
  },

  remove: function (window) {
    this._data.delete(window);
  }
};



let DirtyWindows = {
  _data: new WeakMap(),

  has: function (window) {
    return this._data.has(window);
  },

  add: function (window) {
    return this._data.set(window, true);
  },

  remove: function (window) {
    this._data.delete(window);
  },

  clear: function (window) {
    this._data.clear();
  }
};




let NumberOfTabsClosedLastPerWindow = new WeakMap();




let TabAttributes = {
  _attrs: new Set(),

  
  
  
  
  _skipAttrs: new Set(["image", "pending"]),

  persist: function (name) {
    if (this._attrs.has(name) || this._skipAttrs.has(name)) {
      return false;
    }

    this._attrs.add(name);
    return true;
  },

  get: function (tab) {
    let data = {};

    for (let name of this._attrs) {
      if (tab.hasAttribute(name)) {
        data[name] = tab.getAttribute(name);
      }
    }

    return data;
  },

  set: function (tab, data = {}) {
    
    for (let name of this._attrs) {
      tab.removeAttribute(name);
    }

    
    for (let name in data) {
      tab.setAttribute(name, data[name]);
    }
  }
};




let gRestoreTabsProgressListener = {
  onStateChange: function(aBrowser, aWebProgress, aRequest, aStateFlags, aStatus) {
    
    
    if (aBrowser.__SS_restoreState &&
        aBrowser.__SS_restoreState == TAB_STATE_RESTORING &&
        aStateFlags & Ci.nsIWebProgressListener.STATE_STOP &&
        aStateFlags & Ci.nsIWebProgressListener.STATE_IS_NETWORK &&
        aStateFlags & Ci.nsIWebProgressListener.STATE_IS_WINDOW) {
      
      let tab = SessionStoreInternal._getTabForBrowser(aBrowser);
      SessionStoreInternal._resetTabRestoringState(tab);
      SessionStoreInternal.restoreNextTab();
    }
  }
};




function SessionStoreSHistoryListener(aTab) {
  this.tab = aTab;
}
SessionStoreSHistoryListener.prototype = {
  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsISHistoryListener,
    Ci.nsISupportsWeakReference
  ]),
  browser: null,



  OnHistoryNewEntry: function(aNewURI) {

  },
  OnHistoryGoBack: function(aBackURI) {
    return true;
  },
  OnHistoryGoForward: function(aForwardURI) {
    return true;
  },
  OnHistoryGotoIndex: function(aIndex, aGotoURI) {
    return true;
  },
  OnHistoryPurge: function(aNumEntries) {
    TabStateCache.delete(this.tab);
    return true;
  },
  OnHistoryReload: function(aReloadURI, aReloadFlags) {
    
    
    
    SessionStoreInternal.restoreTab(this.tab);
    
    return false;
  }
}


String.prototype.hasRootDomain = function hasRootDomain(aDomain) {
  let index = this.indexOf(aDomain);
  if (index == -1)
    return false;

  if (this == aDomain)
    return true;

  let prevChar = this[index - 1];
  return (index == (this.length - aDomain.length)) &&
         (prevChar == "." || prevChar == "/");
};

function TabData(obj = null) {
  if (obj) {
    if (obj instanceof TabData) {
      
      return obj;
    }
    for (let [key, value] in Iterator(obj)) {
      this[key] = value;
    }
  }
  return this;
}











let TabStateCache = {
  _data: new WeakMap(),

  







  set: function(aTab, aValue) {
    let key = this._normalizeToBrowser(aTab);
    if (!(aValue instanceof TabData)) {
      throw new TypeError("Attempting to cache a non TabData");
    }
    this._data.set(key, aValue);
  },

  







  get: function(aKey) {
    let key = this._normalizeToBrowser(aKey);
    let result = this._data.get(key);
    TabStateCacheTelemetry.recordAccess(!!result);
    return result;
  },

  






  delete: function(aKey) {
    let key = this._normalizeToBrowser(aKey);
    this._data.delete(key);
  },

  


  clear: function() {
    TabStateCacheTelemetry.recordClear();
    this._data.clear();
  },

  







  updateField: function(aKey, aField, aValue) {
    let key = this._normalizeToBrowser(aKey);
    let data = this._data.get(key);
    if (data) {
      data[aField] = aValue;
    }
    TabStateCacheTelemetry.recordAccess(!!data);
  },

  






  removeField: function(aKey, aField) {
    let key = this._normalizeToBrowser(aKey);
    let data = this._data.get(key);
    if (data && aField in data) {
      delete data[aField];
    }
    TabStateCacheTelemetry.recordAccess(!!data);
  },

  _normalizeToBrowser: function(aKey) {
    let nodeName = aKey.localName;
    if (nodeName == "tab") {
      return aKey.linkedBrowser;
    }
    if (nodeName == "browser") {
      return aKey;
    }
    throw new TypeError("Key is neither a tab nor a browser: " + nodeName);
  }
};




let TabState = {
  







  collect: function (tab) {
    return Promise.resolve(this.collectSync(tab));
  },

  









  collectSync: function (tab) {
    if (!tab) {
      throw new TypeError("Expecting a tab");
    }
    let tabData;
    if ((tabData = TabStateCache.get(tab))) {
      return tabData;
    }
    tabData = new TabData(this._collectBaseTabData(tab));
    if (this._updateTextAndScrollDataForTab(tab, tabData)) {
      TabStateCache.set(tab, tabData);
    }
    return tabData;
  },

  










  clone: function (tab) {
    let options = { includePrivateData: true };
    let tabData = this._collectBaseTabData(tab, options);
    this._updateTextAndScrollDataForTab(tab, tabData, options);
    return tabData;
  },

  










  _collectBaseTabData: function (tab, options = {}) {
    let tabData = {entries: [], lastAccessed: tab.lastAccessed };
    let browser = tab.linkedBrowser;
    if (!browser || !browser.currentURI) {
      
      return tabData;
    }
    if (browser.__SS_data && browser.__SS_tabStillLoading) {
      
      tabData = browser.__SS_data;
      if (tab.pinned)
        tabData.pinned = true;
      else
        delete tabData.pinned;
      tabData.hidden = tab.hidden;

      
      if (tab.__SS_extdata)
        tabData.extData = tab.__SS_extdata;
      
      
      if (tabData.extData && !Object.keys(tabData.extData).length)
        delete tabData.extData;
      return tabData;
    }

    
    this._collectTabHistory(tab, tabData, options);

    
    
    
    
    if (browser.userTypedValue) {
      tabData.userTypedValue = browser.userTypedValue;
      tabData.userTypedClear = browser.userTypedClear;
    } else {
      delete tabData.userTypedValue;
      delete tabData.userTypedClear;
    }

    if (tab.pinned)
      tabData.pinned = true;
    else
      delete tabData.pinned;
    tabData.hidden = tab.hidden;

    let disallow = [];
    for (let cap of gDocShellCapabilities(browser.docShell))
      if (!browser.docShell["allow" + cap])
        disallow.push(cap);
    if (disallow.length > 0)
      tabData.disallow = disallow.join(",");
    else if (tabData.disallow)
      delete tabData.disallow;

    
    tabData.attributes = TabAttributes.get(tab);

    
    let tabbrowser = tab.ownerDocument.defaultView.gBrowser;
    tabData.image = tabbrowser.getIcon(tab);

    if (tab.__SS_extdata)
      tabData.extData = tab.__SS_extdata;
    else if (tabData.extData)
      delete tabData.extData;

    
    this._collectTabSessionStorage(tab, tabData, options);

    return tabData;
  },

  









  _collectTabHistory: function (tab, tabData, options = {}) {
    let includePrivateData = options && options.includePrivateData;
    let browser = tab.linkedBrowser;
    let history = null;
    try {
      history = browser.sessionHistory;
    } catch (ex) {
      
    }

    
    
    if (history && browser.__SS_data &&
        browser.__SS_data.entries[history.index] &&
        browser.__SS_data.entries[history.index].url == browser.currentURI.spec &&
        history.index < this._sessionhistory_max_entries - 1 && !includePrivateData) {
      tabData = browser.__SS_data;
      tabData.index = history.index + 1;
    }
    else if (history && history.count > 0) {
      try {
        for (let j = 0; j < history.count; j++) {
          let entry = this._serializeHistoryEntry(history.getEntryAtIndex(j, false),
                                                  includePrivateData, tab.pinned);
          tabData.entries.push(entry);
        }
        
        
        delete tab.__SS_broken_history;
      }
      catch (ex) {
        
        
        
        
        
        
        if (!tab.__SS_broken_history) {
          
          tab.ownerDocument.defaultView.focus();
          tab.ownerDocument.defaultView.gBrowser.selectedTab = tab;
          debug("SessionStore failed gathering complete history " +
                "for the focused window/tab. See bug 669196.");
          tab.__SS_broken_history = true;
        }
      }
      tabData.index = history.index + 1;

      
      if (!includePrivateData)
        browser.__SS_data = tabData;
    }
    else if (browser.currentURI.spec != "about:blank" ||
             browser.contentDocument.body.hasChildNodes()) {
      tabData.entries[0] = { url: browser.currentURI.spec };
      tabData.index = 1;
    }
  },

  









  _collectTabSessionStorage: function (tab, tabData, options = {}) {
    let includePrivateData = options && options.includePrivateData;
    let browser = tab.linkedBrowser;
    let history = null;
    try {
      history = browser.sessionHistory;
    } catch (ex) {
      
    }

    if (history && browser.docShell instanceof Ci.nsIDocShell) {
      let storageData = SessionStorage.serialize(browser.docShell, includePrivateData)
      if (Object.keys(storageData).length) {
        tabData.storage = storageData;
      }
    }
  },

  










  _serializeHistoryEntry: function (shEntry, includePrivateData, isPinned) {
    let entry = { url: shEntry.URI.spec };

    if (shEntry.title && shEntry.title != entry.url) {
      entry.title = shEntry.title;
    }
    if (shEntry.isSubFrame) {
      entry.subframe = true;
    }
    if (!(shEntry instanceof Ci.nsISHEntry)) {
      return entry;
    }

    let cacheKey = shEntry.cacheKey;
    if (cacheKey && cacheKey instanceof Ci.nsISupportsPRUint32 &&
        cacheKey.data != 0) {
      
      
      entry.cacheKey = cacheKey.data;
    }
    entry.ID = shEntry.ID;
    entry.docshellID = shEntry.docshellID;

    
    
    if (shEntry.referrerURI)
      entry.referrer = shEntry.referrerURI.spec;

    if (shEntry.srcdocData)
      entry.srcdocData = shEntry.srcdocData;

    if (shEntry.isSrcdocEntry)
      entry.isSrcdocEntry = shEntry.isSrcdocEntry;

    if (shEntry.contentType)
      entry.contentType = shEntry.contentType;

    let x = {}, y = {};
    shEntry.getScrollPosition(x, y);
    if (x.value != 0 || y.value != 0)
      entry.scroll = x.value + "," + y.value;

    try {
      let isHttps = shEntry.URI.schemeIs("https");
      let prefPostdata = Services.prefs.getIntPref("browser.sessionstore.postdata");
      if (shEntry.postData && (includePrivateData || prefPostdata &&
          PrivacyLevel.canSave({isHttps: isHttps, isPinned: isPinned}))) {
        shEntry.postData.QueryInterface(Ci.nsISeekableStream).
                        seek(Ci.nsISeekableStream.NS_SEEK_SET, 0);
        let stream = Cc["@mozilla.org/binaryinputstream;1"].
                     createInstance(Ci.nsIBinaryInputStream);
        stream.setInputStream(shEntry.postData);
        let postBytes = stream.readByteArray(stream.available());
        let postdata = String.fromCharCode.apply(null, postBytes);
        if (includePrivateData || prefPostdata == -1 ||
            postdata.replace(/^(Content-.*\r\n)+(\r\n)*/, "").length <=
              prefPostdata) {
          
          
          
          entry.postdata_b64 = btoa(postdata);
        }
      }
    }
    catch (ex) { debug(ex); } 

    if (shEntry.owner) {
      
      
      try {
        let binaryStream = Cc["@mozilla.org/binaryoutputstream;1"].
                           createInstance(Ci.nsIObjectOutputStream);
        let pipe = Cc["@mozilla.org/pipe;1"].createInstance(Ci.nsIPipe);
        pipe.init(false, false, 0, 0xffffffff, null);
        binaryStream.setOutputStream(pipe.outputStream);
        binaryStream.writeCompoundObject(shEntry.owner, Ci.nsISupports, true);
        binaryStream.close();

        
        let scriptableStream = Cc["@mozilla.org/binaryinputstream;1"].
                               createInstance(Ci.nsIBinaryInputStream);
        scriptableStream.setInputStream(pipe.inputStream);
        let ownerBytes =
          scriptableStream.readByteArray(scriptableStream.available());
        
        
        
        entry.owner_b64 = btoa(String.fromCharCode.apply(null, ownerBytes));
      }
      catch (ex) { debug(ex); }
    }

    entry.docIdentifier = shEntry.BFCacheEntry.ID;

    if (shEntry.stateData != null) {
      entry.structuredCloneState = shEntry.stateData.getDataAsBase64();
      entry.structuredCloneVersion = shEntry.stateData.formatVersion;
    }

    if (!(shEntry instanceof Ci.nsISHContainer)) {
      return entry;
    }

    if (shEntry.childCount > 0) {
      let children = [];
      for (let i = 0; i < shEntry.childCount; i++) {
        let child = shEntry.GetChildAt(i);

        if (child) {
          
          if (child.URI.schemeIs("wyciwyg")) {
            children = [];
            break;
          }

          children.push(this._serializeHistoryEntry(child, includePrivateData,
                                                    isPinned));
        }
      }

      if (children.length)
        entry.children = children;
    }

    return entry;
  },

  














  _updateTextAndScrollDataForTab: function (tab, tabData, options = null) {
    let includePrivateData = options && options.includePrivateData;
    let window = tab.ownerDocument.defaultView;
    let browser = tab.linkedBrowser;
    
    if (!browser.currentURI
        || (browser.__SS_data && browser.__SS_tabStillLoading)) {
      return false;
    }

    let tabIndex = (tabData.index || tabData.entries.length) - 1;
    
    if (!tabData.entries[tabIndex]) {
      return false;
    }

    let selectedPageStyle = browser.markupDocumentViewer.authorStyleDisabled ? "_nostyle" :
                            this._getSelectedPageStyle(browser.contentWindow);
    if (selectedPageStyle)
      tabData.pageStyle = selectedPageStyle;
    else if (tabData.pageStyle)
      delete tabData.pageStyle;

    this._updateTextAndScrollDataForFrame(window, browser.contentWindow,
                                          tabData.entries[tabIndex],
                                          includePrivateData,
                                          !!tabData.pinned);
    if (browser.currentURI.spec == "about:config") {
      tabData.entries[tabIndex].formdata = {
        id: {
          "textbox": browser.contentDocument.getElementById("textbox").value
        },
        xpath: {}
      };
    }

    return true;
  },

  














  _updateTextAndScrollDataForFrame:
    function (window, content, data, includePrivateData, isPinned) {

    for (let i = 0; i < content.frames.length; i++) {
      if (data.children && data.children[i])
        this._updateTextAndScrollDataForFrame(window, content.frames[i],
                                              data.children[i],
                                              includePrivateData, isPinned);
    }
    let href = (content.parent || content).document.location.href;
    let isHttps = makeURI(href).schemeIs("https");
    let topURL = content.top.document.location.href;
    let isAboutSR = topURL == "about:sessionrestore" || topURL == "about:welcomeback";
    if (includePrivateData || isAboutSR ||
        PrivacyLevel.canSave({isHttps: isHttps, isPinned: isPinned})) {
      let formData = DocumentUtils.getFormData(content.document);

      
      
      
      if (formData && isAboutSR) {
        formData.id["sessionData"] = JSON.parse(formData.id["sessionData"]);
      }

      if (Object.keys(formData.id).length ||
          Object.keys(formData.xpath).length) {
        data.formdata = formData;
      } else if (data.formdata) {
        delete data.formdata;
      }

      
      if ((content.document.designMode || "") == "on" && content.document.body)
        data.innerHTML = content.document.body.innerHTML;
    }

    
    
    let domWindowUtils = content.QueryInterface(Ci.nsIInterfaceRequestor)
                                .getInterface(Ci.nsIDOMWindowUtils);
    let scrollX = {}, scrollY = {};
    domWindowUtils.getScrollXY(false, scrollX, scrollY);
    data.scroll = scrollX.value + "," + scrollY.value;
  },

  





  _getSelectedPageStyle: function (content) {
    const forScreen = /(?:^|,)\s*(?:all|screen)\s*(?:,|$)/i;
    for (let i = 0; i < content.document.styleSheets.length; i++) {
      let ss = content.document.styleSheets[i];
      let media = ss.media.mediaText;
      if (!ss.disabled && ss.title && (!media || forScreen.test(media)))
        return ss.title
    }
    for (let i = 0; i < content.frames.length; i++) {
      let selectedPageStyle = this._getSelectedPageStyle(content.frames[i]);
      if (selectedPageStyle)
        return selectedPageStyle;
    }
    return "";
  }
};

let TabStateCacheTelemetry = {
  
  _hits: 0,
  
  _misses: 0,
  
  _clears: 0,
  
  _initialized: false,

  





  recordAccess: function(isHit) {
    this._init();
    if (isHit) {
      ++this._hits;
    } else {
      ++this._misses;
    }
  },

  


  recordClear: function() {
    this._init();
    ++this._clears;
  },

  


  _init: function() {
    if (this._initialized) {
      
      return;
    }
    this._initialized = true;
    Services.obs.addObserver(this, "profile-before-change", false);
  },

  observe: function() {
    Services.obs.removeObserver(this, "profile-before-change");

    
    let accesses = this._hits + this._misses;
    if (accesses == 0) {
      return;
    }

    this._fillHistogram("HIT_RATE", this._hits, accesses);
    this._fillHistogram("CLEAR_RATIO", this._clears, accesses);
  },

  _fillHistogram: function(suffix, positive, total) {
    let PREFIX = "FX_SESSION_RESTORE_TABSTATECACHE_";
    let histo = Services.telemetry.getHistogramById(PREFIX + suffix);
    let rate = Math.floor( ( positive * 100 ) / total );
    histo.add(rate);
  }
};
