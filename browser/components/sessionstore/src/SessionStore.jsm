



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

const PRIVACY_NONE = 0;
const PRIVACY_ENCRYPTED = 1;
const PRIVACY_FULL = 2;

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









const CAPABILITIES = [
  "Subframes", "Plugins", "Javascript", "MetaRedirects", "Images",
  "DNSPrefetch", "Auth", "WindowControl"
];


const TAB_EVENTS = [
  "TabOpen", "TabClose", "TabSelect", "TabShow", "TabHide", "TabPinned",
  "TabUnpinned"
];

#ifndef XP_WIN
#define BROKEN_WM_Z_ORDER
#endif

Cu.import("resource://gre/modules/Services.jsm", this);
Cu.import("resource://gre/modules/XPCOMUtils.jsm", this);

Cu.import("resource://gre/modules/debug.js", this);
Cu.import("resource://gre/modules/TelemetryTimestamps.jsm", this);
Cu.import("resource://gre/modules/TelemetryStopwatch.jsm", this);
Cu.import("resource://gre/modules/osfile.jsm", this);
Cu.import("resource://gre/modules/PrivateBrowsingUtils.jsm", this);
Cu.import("resource://gre/modules/commonjs/sdk/core/promise.js", this);

XPCOMUtils.defineLazyServiceGetter(this, "gSessionStartup",
  "@mozilla.org/browser/sessionstartup;1", "nsISessionStartup");

XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
  "resource://gre/modules/NetUtil.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "ScratchpadManager",
  "resource:///modules/devtools/scratchpad-manager.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "DocumentUtils",
  "resource:///modules/sessionstore/DocumentUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "SessionStorage",
  "resource:///modules/sessionstore/SessionStorage.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "_SessionFile",
  "resource:///modules/sessionstore/_SessionFile.jsm");

#ifdef MOZ_CRASHREPORTER
XPCOMUtils.defineLazyServiceGetter(this, "CrashReporter",
  "@mozilla.org/xre/app-info;1", "nsICrashReporter");
#endif

function debug(aMsg) {
  aMsg = ("SessionStore: " + aMsg).replace(/\S{80}/g, "$&\n");
  Services.console.logStringMessage(aMsg);
}

this.SessionStore = {
  get promiseInitialized() {
    return SessionStoreInternal.promiseInitialized.promise;
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

  checkPrivacyLevel: function ss_checkPrivacyLevel(aIsHTTPS, aUseDefaultPref) {
    return SessionStoreInternal.checkPrivacyLevel(aIsHTTPS, aUseDefaultPref);
  }
};


Object.freeze(SessionStore);

let SessionStoreInternal = {
  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsIDOMEventListener,
    Ci.nsIObserver,
    Ci.nsISupportsWeakReference
  ]),

  
  
  xulAttributes: {"image": true},

  
  _loadState: STATE_STOPPED,

  
  
  _restoreCount: -1,

  
  _browserSetState: false,

  
  _lastSaveTime: 0,

  
  
  _sessionStartTime: Date.now(),

  
  _windows: {},

  
  
  _internalWindows: {},

  
  _closedWindows: [],

  
  _dirtyWindows: {},

  
  _statesToRestore: {},

  
  _recentCrashes: 0,

  
  _restoreLastWindow: false,

  
  _tabsToRestore: { priority: [], visible: [], hidden: [] },
  _tabsRestoringCount: 0,

  
  _restoreOnDemand: false,

  
  _restoreHiddenTabs: null,

  
  _restorePinnedTabsOnDemand: null,

  
  
  
  
  _lastSessionState: null,

  
  
  
  _deferredInitialState: null,

  
  _promiseInitialization: Promise.defer(),

  
  _sessionInitialized: false,

  
  
  
  
  
  
  
  
  _resume_session_once_on_shutdown: null,

  


  get promiseInitialized() {
    return this._promiseInitialization;
  },

  
  get canRestoreLastSession() {
    return this._lastSessionState;
  },

  set canRestoreLastSession(val) {
    
    if (val)
      return;
    this._lastSessionState = null;
  },

  

  


  initService: function ssi_initService() {
    if (this._sessionInitialized) {
      return;
    }
    TelemetryTimestamps.add("sessionRestoreInitialized");
    OBSERVING.forEach(function(aTopic) {
      Services.obs.addObserver(this, aTopic, true);
    }, this);

    this._initPrefs();

    
    this._migratePrefs();

    
    this._sessionhistory_max_entries =
      this._prefBranch.getIntPref("sessionhistory.max_entries");

    this._restoreOnDemand =
      this._prefBranch.getBoolPref("sessionstore.restore_on_demand");
    this._prefBranch.addObserver("sessionstore.restore_on_demand", this, true);

    this._restoreHiddenTabs =
      this._prefBranch.getBoolPref("sessionstore.restore_hidden_tabs");
    this._prefBranch.addObserver("sessionstore.restore_hidden_tabs", this, true);

    this._restorePinnedTabsOnDemand =
      this._prefBranch.getBoolPref("sessionstore.restore_pinned_tabs_on_demand");
    this._prefBranch.addObserver("sessionstore.restore_pinned_tabs_on_demand", this, true);

    gSessionStartup.onceInitialized.then(
      this.initSession.bind(this)
    );
  },

  initSession: function ssi_initSession() {
    let ss = gSessionStartup;
    try {
      if (ss.doRestore() ||
          ss.sessionType == Ci.nsISessionStartup.DEFER_SESSION)
        this._initialState = ss.state;
    }
    catch(ex) { dump(ex + "\n"); } 

    if (this._initialState) {
      try {
        
        
        if (ss.sessionType == Ci.nsISessionStartup.DEFER_SESSION) {
          let [iniState, remainingState] = this._prepDataForDeferredRestore(this._initialState);
          
          
          if (iniState.windows.length)
            this._initialState = iniState;
          else
            this._initialState = null;
          if (remainingState.windows.length)
            this._lastSessionState = remainingState;
        }
        else {
          
          
          this._lastSessionState = this._initialState.lastSessionState;

          let lastSessionCrashed =
            this._initialState.session && this._initialState.session.state &&
            this._initialState.session.state == STATE_RUNNING_STR;
          if (lastSessionCrashed) {
            this._recentCrashes = (this._initialState.session &&
                                   this._initialState.session.recentCrashes || 0) + 1;

            if (this._needsRestorePage(this._initialState, this._recentCrashes)) {
              
              let pageData = {
                url: "about:sessionrestore",
                formdata: {
                  id: { "sessionData": this._initialState },
                  xpath: {}
                }
              };
              this._initialState = { windows: [{ tabs: [{ entries: [pageData] }] }] };
            }
          }

          
          this._sessionStartTime = this._initialState.session &&
                                   this._initialState.session.startTime ||
                                   this._sessionStartTime;

          
          delete this._initialState.windows[0].hidden;
          
          delete this._initialState.windows[0].isPopup;
          
          if (this._initialState.windows[0].sizemode == "minimized")
            this._initialState.windows[0].sizemode = "normal";
          
          
          this._initialState.windows.forEach(function(aWindow) {
            delete aWindow.__lastSessionWindowID;
          });
        }
      }
      catch (ex) { debug("The session file is invalid: " + ex); }
    }

    if (this._resume_from_crash) {
      
      
      
      
      _SessionFile.createBackupCopy();
    }

    
    
    if (this._loadState != STATE_QUITTING &&
        this._prefBranch.getBoolPref("sessionstore.resume_session_once"))
      this._prefBranch.setBoolPref("sessionstore.resume_session_once", false);

    this._initEncoding();

    
    this._sessionInitialized = true;
    this._promiseInitialization.resolve();
  },

  _initEncoding : function ssi_initEncoding() {
    
    XPCOMUtils.defineLazyGetter(this, "_writeFileEncoder", function () {
      return new TextEncoder();
    });
  },

  _initPrefs : function() {
    XPCOMUtils.defineLazyGetter(this, "_prefBranch", function () {
      return Services.prefs.getBranch("browser.");
    });

    
    XPCOMUtils.defineLazyGetter(this, "_interval", function () {
      
      this._prefBranch.addObserver("sessionstore.interval", this, true);
      return this._prefBranch.getIntPref("sessionstore.interval");
    });

    
    XPCOMUtils.defineLazyGetter(this, "_resume_from_crash", function () {
      
      this._prefBranch.addObserver("sessionstore.resume_from_crash", this, true);
      return this._prefBranch.getBoolPref("sessionstore.resume_from_crash");
    });

    XPCOMUtils.defineLazyGetter(this, "_max_tabs_undo", function () {
      this._prefBranch.addObserver("sessionstore.max_tabs_undo", this, true);
      return this._prefBranch.getIntPref("sessionstore.max_tabs_undo");
    });

    XPCOMUtils.defineLazyGetter(this, "_max_windows_undo", function () {
      this._prefBranch.addObserver("sessionstore.max_windows_undo", this, true);
      return this._prefBranch.getIntPref("sessionstore.max_windows_undo");
    });
  },

  _initWindow: function ssi_initWindow(aWindow) {
    if (!aWindow || this._loadState == STATE_RUNNING) {
      
      
      if (aWindow && (!aWindow.__SSi || !this._windows[aWindow.__SSi]))
        this.onLoad(aWindow);
      
      
      
      
      
      if (!aWindow && this._loadState == STATE_STOPPED)
        this._loadState = STATE_RUNNING;
      return;
    }

    
    this.onLoad(aWindow);
  },

  





  init: function ssi_init(aWindow) {
    let self = this;
    this.initService();
    this._promiseInitialization.promise.then(
      function onSuccess() {
        self._initWindow(aWindow);
      }
    );
  },

  



  _uninit: function ssi_uninit() {
    
    if (this._sessionInitialized)
      this.saveState(true);

    
    this._tabsToRestore.priority = null;
    this._tabsToRestore.visible = null;
    this._tabsToRestore.hidden = null;

    
    if (this._saveTimer) {
      this._saveTimer.cancel();
      this._saveTimer = null;
    }
  },

  _migratePrefs: function ssi_migratePrefs() {
    
    
    
    if (this._prefBranch.prefHasUserValue("sessionstore.max_concurrent_tabs") &&
        !this._prefBranch.prefHasUserValue("sessionstore.restore_on_demand")) {
      let maxConcurrentTabs =
        this._prefBranch.getIntPref("sessionstore.max_concurrent_tabs");
      this._prefBranch.setBoolPref("sessionstore.restore_on_demand", maxConcurrentTabs == 0);
      this._prefBranch.clearUserPref("sessionstore.max_concurrent_tabs");
    }
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

  

  


  handleEvent: function ssi_handleEvent(aEvent) {
    var win = aEvent.currentTarget.ownerDocument.defaultView;
    switch (aEvent.type) {
      case "load":
        
        
        
        if (aEvent.currentTarget.__SS_restore_data)
          this.restoreDocument(win, aEvent.currentTarget, aEvent);
        
      case "pageshow":
        this.onTabLoad(win, aEvent.currentTarget, aEvent);
        break;
      case "change":
      case "input":
      case "DOMAutoComplete":
        this.onTabInput(win, aEvent.currentTarget);
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
      case "TabUnpinned":
        this.saveStateDelayed(win);
        break;
    }

    this._clearRestoringWindows();
  },

  








  onLoad: function ssi_onLoad(aWindow) {
    
    if (aWindow && aWindow.__SSi && this._windows[aWindow.__SSi])
      return;

    
    if (aWindow.document.documentElement.getAttribute("windowtype") != "navigator:browser" ||
        this._loadState == STATE_QUITTING)
      return;

    
    aWindow.__SSi = "window" + Date.now();

    
    this._windows[aWindow.__SSi] = { tabs: [], selected: 0, _closedTabs: [], busy: false };

    
    this._internalWindows[aWindow.__SSi] = { hosts: {} }

    let isPrivateWindow = false;
    if (PrivateBrowsingUtils.isWindowPrivate(aWindow))
      this._windows[aWindow.__SSi].isPrivate = isPrivateWindow = true;
    if (!this._isWindowLoaded(aWindow))
      this._windows[aWindow.__SSi]._restoring = true;
    if (!aWindow.toolbar.visible)
      this._windows[aWindow.__SSi].isPopup = true;

    
    if (this._loadState == STATE_STOPPED) {
      this._loadState = STATE_RUNNING;
      this._lastSaveTime = Date.now();

      
      if (this._initialState) {
        if (isPrivateWindow) {
          
          
          
          this._deferredInitialState = this._initialState;
          delete this._initialState;

          
          Services.obs.notifyObservers(null, NOTIFY_WINDOWS_RESTORED, "");
        } else {
          TelemetryTimestamps.add("sessionRestoreRestoring");
          
          this._initialState._firstTabs = true;
          this._restoreCount = this._initialState.windows ? this._initialState.windows.length : 0;
          this.restoreWindow(aWindow, this._initialState,
                             this._isCmdLineEmpty(aWindow, this._initialState));
          delete this._initialState;

          
          
          this.saveState(true);
        }
      }
      else {
        
        Services.obs.notifyObservers(null, NOTIFY_WINDOWS_RESTORED, "");

        
        this._lastSaveTime -= this._interval;
      }
    }
    
    else if (!this._isWindowLoaded(aWindow)) {
      let followUp = this._statesToRestore[aWindow.__SS_restoreID].windows.length == 1;
      this.restoreWindow(aWindow, this._statesToRestore[aWindow.__SS_restoreID], true, followUp);
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
          this.restoreWindow(aWindow, state, this._isCmdLineEmpty(aWindow, state));
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
    var _this = this;
    aWindow.addEventListener("load", function(aEvent) {
      aEvent.currentTarget.removeEventListener("load", arguments.callee, false);
      _this.onLoad(aEvent.currentTarget);
    }, false);
    return;
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
        let windows = {};
        windows[aWindow.__SSi] = winData;
        this._updateCookies(windows);
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
      delete this._internalWindows[aWindow.__SSi];

      
      this.saveStateDelayed();
    }

    for (let i = 0; i < tabbrowser.tabs.length; i++) {
      this.onTabRemove(aWindow, tabbrowser.tabs[i], true);
    }

    
    aWindow.__SS_dyingCache = winData;

    delete aWindow.__SSi;
  },

  


  onQuitApplicationRequested: function ssi_onQuitApplicationRequested() {
    
    this._forEachBrowserWindow(function(aWindow) {
      this._collectWindowData(aWindow);
    });
    
    
    var activeWindow = this._getMostRecentBrowserWindow();
    if (activeWindow)
      this.activeWindowSSiCache = activeWindow.__SSi || "";
    this._dirtyWindows = [];
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
    else if (this._resume_session_once_on_shutdown != null) {
      
      
      
      
      
      this._prefBranch.setBoolPref("sessionstore.resume_session_once",
                                   this._resume_session_once_on_shutdown);
    }

    if (aData != "restart") {
      
      this._lastSessionState = null;
    }

    this._loadState = STATE_QUITTING; 
    this._uninit();
  },

  


  onPurgeSessionHistory: function ssi_onPurgeSessionHistory() {
    var _this = this;
    _SessionFile.wipe();
    
    
    
    if (this._loadState == STATE_QUITTING)
      return;
    this._lastSessionState = null;
    let openWindows = {};
    this._forEachBrowserWindow(function(aWindow) {
      Array.forEach(aWindow.gBrowser.tabs, function(aTab) {
        delete aTab.linkedBrowser.__SS_data;
        delete aTab.linkedBrowser.__SS_tabStillLoading;
        delete aTab.linkedBrowser.__SS_formDataSaved;
        delete aTab.linkedBrowser.__SS_hostSchemeData;
        if (aTab.linkedBrowser.__SS_restoreState)
          this._resetTabRestoringState(aTab);
      });
      openWindows[aWindow.__SSi] = true;
    });
    
    for (let ix in this._windows) {
      if (ix in openWindows) {
        this._windows[ix]._closedTabs = [];
      }
      else {
        delete this._windows[ix];
        delete this._internalWindows[ix];
      }
    }
    
    this._closedWindows = [];
    
    var win = this._getMostRecentBrowserWindow();
    if (win)
      win.setTimeout(function() { _this.saveState(true); }, 0);
    else if (this._loadState == STATE_RUNNING)
      this.saveState(true);
    
    if ("_stateBackup" in this)
      delete this._stateBackup;

    this._clearRestoringWindows();
  },

  




  onPurgeDomainData: function ssi_onPurgeDomainData(aData) {
    
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
        
        if (this._resume_session_once_on_shutdown != null) {
          this._prefBranch.setBoolPref("sessionstore.resume_session_once",
                                       this._resume_session_once_on_shutdown);
          this._resume_session_once_on_shutdown = null;
        }
        
        
        if (!this._resume_from_crash)
          _SessionFile.wipe();
        this.saveState(true);
        break;
      case "sessionstore.restore_on_demand":
        this._restoreOnDemand =
          this._prefBranch.getBoolPref("sessionstore.restore_on_demand");
        break;
      case "sessionstore.restore_hidden_tabs":
        this._restoreHiddenTabs =
          this._prefBranch.getBoolPref("sessionstore.restore_hidden_tabs");
        break;
      case "sessionstore.restore_pinned_tabs_on_demand":
        this._restorePinnedTabsOnDemand =
          this._prefBranch.getBoolPref("sessionstore.restore_pinned_tabs_on_demand");
        break;
    }
  },

  


  onTimerCallback: function ssi_onTimerCallback() {
    this._saveTimer = null;
    this.saveState();
  },

  








  onTabAdd: function ssi_onTabAdd(aWindow, aTab, aNoNotification) {
    let browser = aTab.linkedBrowser;
    browser.addEventListener("load", this, true);
    browser.addEventListener("pageshow", this, true);
    browser.addEventListener("change", this, true);
    browser.addEventListener("input", this, true);
    browser.addEventListener("DOMAutoComplete", this, true);

    if (!aNoNotification) {
      this.saveStateDelayed(aWindow);
    }

    this._updateCrashReportURL(aWindow);
  },

  








  onTabRemove: function ssi_onTabRemove(aWindow, aTab, aNoNotification) {
    let browser = aTab.linkedBrowser;
    browser.removeEventListener("load", this, true);
    browser.removeEventListener("pageshow", this, true);
    browser.removeEventListener("change", this, true);
    browser.removeEventListener("input", this, true);
    browser.removeEventListener("DOMAutoComplete", this, true);

    delete browser.__SS_data;
    delete browser.__SS_tabStillLoading;
    delete browser.__SS_formDataSaved;
    delete browser.__SS_hostSchemeData;

    
    
    
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

    
    var tabState = this._collectTabData(aTab);
    this._updateTextAndScrollDataForTab(aWindow, aTab.linkedBrowser, tabState);

    
    if (this._shouldSaveTabState(tabState)) {
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
      if (length > this._max_tabs_undo)
        this._windows[aWindow.__SSi]._closedTabs.splice(this._max_tabs_undo, length - this._max_tabs_undo);
    }
  },

  








  onTabLoad: function ssi_onTabLoad(aWindow, aBrowser, aEvent) {
    
    
    
    
    if ((aEvent.type != "load" && !aEvent.persisted) ||
        (aBrowser.__SS_restoreState &&
         aBrowser.__SS_restoreState == TAB_STATE_NEEDS_RESTORE)) {
      return;
    }

    delete aBrowser.__SS_data;
    delete aBrowser.__SS_tabStillLoading;
    delete aBrowser.__SS_formDataSaved;
    this.saveStateDelayed(aWindow);

    
    this._updateCrashReportURL(aWindow);
  },

  






  onTabInput: function ssi_onTabInput(aWindow, aBrowser) {
    
    delete aBrowser.__SS_formDataSaved;

    this.saveStateDelayed(aWindow, 3000);
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
      this._tabsToRestore.hidden.splice(this._tabsToRestore.hidden.indexOf(aTab), this._tabsToRestore.hidden.length);
      
      this._tabsToRestore.visible.push(aTab);

      
      
      this.restoreNextTab();
    }

    
    
    this.saveStateDelayed(aWindow);
  },

  onTabHide: function ssi_onTabHide(aWindow, aTab) {
    
    if (aTab.linkedBrowser.__SS_restoreState &&
        aTab.linkedBrowser.__SS_restoreState == TAB_STATE_NEEDS_RESTORE) {
      this._tabsToRestore.visible.splice(this._tabsToRestore.visible.indexOf(aTab), this._tabsToRestore.visible.length);
      
      this._tabsToRestore.hidden.push(aTab);
    }

    
    
    this.saveStateDelayed(aWindow);
  },

  

  getBrowserState: function ssi_getBrowserState() {
    return this._toJSONString(this._getCurrentState());
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

    
    this.restoreWindow(window, state, true);
  },

  getWindowState: function ssi_getWindowState(aWindow) {
    if (!aWindow.__SSi && !aWindow.__SS_dyingCache)
      throw (Components.returnCode = Cr.NS_ERROR_INVALID_ARG);

    if (!aWindow.__SSi)
      return this._toJSONString({ windows: [aWindow.__SS_dyingCache] });
    return this._toJSONString(this._getWindowState(aWindow));
  },

  setWindowState: function ssi_setWindowState(aWindow, aState, aOverwrite) {
    if (!aWindow.__SSi)
      throw (Components.returnCode = Cr.NS_ERROR_INVALID_ARG);

    this.restoreWindow(aWindow, aState, aOverwrite);
  },

  getTabState: function ssi_getTabState(aTab) {
    if (!aTab.ownerDocument || !aTab.ownerDocument.defaultView.__SSi)
      throw (Components.returnCode = Cr.NS_ERROR_INVALID_ARG);

    var tabState = this._collectTabData(aTab);

    var window = aTab.ownerDocument.defaultView;
    this._updateTextAndScrollDataForTab(window, aTab.linkedBrowser, tabState);

    return this._toJSONString(tabState);
  },

  setTabState: function ssi_setTabState(aTab, aState) {
    var tabState = JSON.parse(aState);
    if (!tabState.entries || !aTab.ownerDocument || !aTab.ownerDocument.defaultView.__SSi)
      throw (Components.returnCode = Cr.NS_ERROR_INVALID_ARG);

    var window = aTab.ownerDocument.defaultView;
    this._setWindowStateBusy(window);
    this.restoreHistoryPrecursor(window, [aTab], [tabState], 0, 0, 0);
  },

  duplicateTab: function ssi_duplicateTab(aWindow, aTab, aDelta) {
    if (!aTab.ownerDocument || !aTab.ownerDocument.defaultView.__SSi ||
        !aWindow.getBrowser)
      throw (Components.returnCode = Cr.NS_ERROR_INVALID_ARG);

    var tabState = this._collectTabData(aTab, true);
    var sourceWindow = aTab.ownerDocument.defaultView;
    this._updateTextAndScrollDataForTab(sourceWindow, aTab.linkedBrowser, tabState, true);
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

  getClosedTabCount: function ssi_getClosedTabCount(aWindow) {
    if (!aWindow.__SSi && aWindow.__SS_dyingCache)
      return aWindow.__SS_dyingCache._closedTabs.length;
    if (!aWindow.__SSi)
      
      return 0; 

    return this._windows[aWindow.__SSi]._closedTabs.length;
  },

  getClosedTabData: function ssi_getClosedTabDataAt(aWindow) {
    if (!aWindow.__SSi && !aWindow.__SS_dyingCache)
      throw (Components.returnCode = Cr.NS_ERROR_INVALID_ARG);

    if (!aWindow.__SSi)
      return this._toJSONString(aWindow.__SS_dyingCache._closedTabs);
    return this._toJSONString(this._windows[aWindow.__SSi]._closedTabs);
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

    if (deleteFrom && deleteFrom[aKey])
      delete deleteFrom[aKey];
  },

  persistTabAttribute: function ssi_persistTabAttribute(aName) {
    if (aName in this.xulAttributes)
      return; 

    this.xulAttributes[aName] = true;
    this.saveStateDelayed();
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

        
        
        
        
        
        
        
        this.restoreWindow(windowToUse, { windows: [winState] }, canOverwriteTabs, true);
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
    this._sessionStartTime = lastSessionState.session &&
                             lastSessionState.session.startTime ||
                             this._sessionStartTime;

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

  

  




  _saveWindowHistory: function ssi_saveWindowHistory(aWindow) {
    var tabbrowser = aWindow.gBrowser;
    var tabs = tabbrowser.tabs;
    var tabsData = this._windows[aWindow.__SSi].tabs = [];

    for (var i = 0; i < tabs.length; i++)
      tabsData.push(this._collectTabData(tabs[i]));

    this._windows[aWindow.__SSi].selected = tabbrowser.mTabBox.selectedIndex + 1;
  },

  







  _collectTabData: function ssi_collectTabData(aTab, aFullData) {
    var tabData = { entries: [] };
    var browser = aTab.linkedBrowser;

    if (!browser || !browser.currentURI)
      
      return tabData;
    else if (browser.__SS_data && browser.__SS_tabStillLoading) {
      
      tabData = browser.__SS_data;
      if (aTab.pinned)
        tabData.pinned = true;
      else
        delete tabData.pinned;
      tabData.hidden = aTab.hidden;

      
      if (aTab.__SS_extdata)
        tabData.extData = aTab.__SS_extdata;
      
      
      if (tabData.extData && !Object.keys(tabData.extData).length)
        delete tabData.extData;
      return tabData;
    }

    var history = null;
    try {
      history = browser.sessionHistory;
    }
    catch (ex) { } 

    
    
    if (history && browser.__SS_data &&
        browser.__SS_data.entries[history.index] &&
        browser.__SS_data.entries[history.index].url == browser.currentURI.spec &&
        history.index < this._sessionhistory_max_entries - 1 && !aFullData) {
      tabData = browser.__SS_data;
      tabData.index = history.index + 1;
    }
    else if (history && history.count > 0) {
      browser.__SS_hostSchemeData = [];
      try {
        for (var j = 0; j < history.count; j++) {
          let entry = this._serializeHistoryEntry(history.getEntryAtIndex(j, false),
                                                  aFullData, aTab.pinned, browser.__SS_hostSchemeData);
          tabData.entries.push(entry);
        }
        
        
        delete aTab.__SS_broken_history;
      }
      catch (ex) {
        
        
        
        
        
        
        if (!aTab.__SS_broken_history) {
          
          aTab.ownerDocument.defaultView.focus();
          aTab.ownerDocument.defaultView.gBrowser.selectedTab = aTab;
          NS_ASSERT(false, "SessionStore failed gathering complete history " +
                           "for the focused window/tab. See bug 669196.");
          aTab.__SS_broken_history = true;
        }
      }
      tabData.index = history.index + 1;

      
      if (!aFullData)
        browser.__SS_data = tabData;
    }
    else if (browser.currentURI.spec != "about:blank" ||
             browser.contentDocument.body.hasChildNodes()) {
      tabData.entries[0] = { url: browser.currentURI.spec };
      tabData.index = 1;
    }

    
    
    
    
    if (browser.userTypedValue) {
      tabData.userTypedValue = browser.userTypedValue;
      tabData.userTypedClear = browser.userTypedClear;
    } else {
      delete tabData.userTypedValue;
      delete tabData.userTypedClear;
    }

    if (aTab.pinned)
      tabData.pinned = true;
    else
      delete tabData.pinned;
    tabData.hidden = aTab.hidden;

    var disallow = [];
    for (var i = 0; i < CAPABILITIES.length; i++)
      if (!browser.docShell["allow" + CAPABILITIES[i]])
        disallow.push(CAPABILITIES[i]);
    if (disallow.length > 0)
      tabData.disallow = disallow.join(",");
    else if (tabData.disallow)
      delete tabData.disallow;

    tabData.attributes = {};
    for (let name in this.xulAttributes) {
      if (aTab.hasAttribute(name))
        tabData.attributes[name] = aTab.getAttribute(name);
    }

    if (aTab.__SS_extdata)
      tabData.extData = aTab.__SS_extdata;
    else if (tabData.extData)
      delete tabData.extData;

    if (history && browser.docShell instanceof Ci.nsIDocShell) {
      let storageData = SessionStorage.serialize(browser.docShell, aFullData)
      if (Object.keys(storageData).length)
        tabData.storage = storageData;
    }

    return tabData;
  },

  












  _serializeHistoryEntry:
    function ssi_serializeHistoryEntry(aEntry, aFullData, aIsPinned, aHostSchemeData) {
    var entry = { url: aEntry.URI.spec };

    try {
      
      if (entry.url.indexOf("about:") != 0)
        aHostSchemeData.push({ host: aEntry.URI.host, scheme: aEntry.URI.scheme });
    }
    catch (ex) {
      
    }

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
    entry.docshellID = aEntry.docshellID;

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
      if (aEntry.postData && (aFullData || prefPostdata &&
            this.checkPrivacyLevel(aEntry.URI.schemeIs("https"), aIsPinned))) {
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

    entry.docIdentifier = aEntry.BFCacheEntry.ID;

    if (aEntry.stateData != null) {
      entry.structuredCloneState = aEntry.stateData.getDataAsBase64();
      entry.structuredCloneVersion = aEntry.stateData.formatVersion;
    }

    if (!(aEntry instanceof Ci.nsISHContainer)) {
      return entry;
    }

    if (aEntry.childCount > 0) {
      let children = [];
      for (var i = 0; i < aEntry.childCount; i++) {
        var child = aEntry.GetChildAt(i);

        if (child) {
          
          if (child.URI.schemeIs("wyciwyg")) {
            children = [];
            break;
          }

          children.push(this._serializeHistoryEntry(child, aFullData,
                                                    aIsPinned, aHostSchemeData));
        }
      }

      if (children.length)
        entry.children = children;
    }

    return entry;
  },

  





  _updateTextAndScrollData: function ssi_updateTextAndScrollData(aWindow) {
    var browsers = aWindow.gBrowser.browsers;
    this._windows[aWindow.__SSi].tabs.forEach(function (tabData, i) {
      try {
        this._updateTextAndScrollDataForTab(aWindow, browsers[i], tabData);
      }
      catch (ex) { debug(ex); } 
    }, this);
  },

  











  _updateTextAndScrollDataForTab:
    function ssi_updateTextAndScrollDataForTab(aWindow, aBrowser, aTabData, aFullData) {
    
    if (aBrowser.__SS_data && aBrowser.__SS_tabStillLoading)
      return;

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
                                          !aBrowser.__SS_formDataSaved, aFullData,
                                          !!aTabData.pinned);
    aBrowser.__SS_formDataSaved = true;
    if (aBrowser.currentURI.spec == "about:config")
      aTabData.entries[tabIndex].formdata = {
        id: {
          "textbox": aBrowser.contentDocument.getElementById("textbox").value
        },
        xpath: {}
      };
  },

  















  _updateTextAndScrollDataForFrame:
    function ssi_updateTextAndScrollDataForFrame(aWindow, aContent, aData,
                                                 aUpdateFormData, aFullData, aIsPinned) {
    for (var i = 0; i < aContent.frames.length; i++) {
      if (aData.children && aData.children[i])
        this._updateTextAndScrollDataForFrame(aWindow, aContent.frames[i],
                                              aData.children[i], aUpdateFormData,
                                              aFullData, aIsPinned);
    }
    var isHTTPS = this._getURIFromString((aContent.parent || aContent).
                                         document.location.href).schemeIs("https");
    let isAboutSR = aContent.top.document.location.href == "about:sessionrestore";
    if (aFullData || this.checkPrivacyLevel(isHTTPS, aIsPinned) || isAboutSR) {
      if (aFullData || aUpdateFormData) {
        let formData = DocumentUtils.getFormData(aContent.document);

        
        
        
        if (formData && isAboutSR) {
          formData.id["sessionData"] = JSON.parse(formData.id["sessionData"]);
        }

        if (Object.keys(formData.id).length ||
            Object.keys(formData.xpath).length) {
          aData.formdata = formData;
        } else if (aData.formdata) {
          delete aData.formdata;
        }
      }

      
      if ((aContent.document.designMode || "") == "on" && aContent.document.body) {
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

  





  _getSelectedPageStyle: function ssi_getSelectedPageStyle(aContent) {
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

  











  _extractHostsForCookiesFromEntry:
    function ssi_extractHostsForCookiesFromEntry(aEntry, aHosts, aCheckPrivacy, aIsPinned) {

    let host = aEntry._host,
        scheme = aEntry._scheme;

    
    
    
    
    if (!host && !scheme) {
      try {
        let uri = this._getURIFromString(aEntry.url);
        host = uri.host;
        scheme = uri.scheme;
        this._extractHostsForCookiesFromHostScheme(host, scheme, aHosts, aCheckPrivacy, aIsPinned);
      }
      catch(ex) { }
    }

    if (aEntry.children) {
      aEntry.children.forEach(function(entry) {
        this._extractHostsForCookiesFromEntry(entry, aHosts, aCheckPrivacy, aIsPinned);
      }, this);
    }
  },

  













  _extractHostsForCookiesFromHostScheme:
    function ssi_extractHostsForCookiesFromHostScheme(aHost, aScheme, aHosts, aCheckPrivacy, aIsPinned) {
    
    
    if (/https?/.test(aScheme) && !aHosts[aHost] &&
        (!aCheckPrivacy ||
         this.checkPrivacyLevel(aScheme == "https", aIsPinned))) {
      
      
      aHosts[aHost] = aIsPinned;
    }
    else if (aScheme == "file") {
      aHosts[aHost] = true;
    }
  },

  




  _updateCookieHosts: function ssi_updateCookieHosts(aWindow) {
    var hosts = this._internalWindows[aWindow.__SSi].hosts = {};

    
    
    
    for (let i = 0; i < aWindow.gBrowser.tabs.length; i++) {
      let tab = aWindow.gBrowser.tabs[i];
      let hostSchemeData = tab.linkedBrowser.__SS_hostSchemeData || [];
      for (let j = 0; j < hostSchemeData.length; j++) {
        this._extractHostsForCookiesFromHostScheme(hostSchemeData[j].host,
                                                   hostSchemeData[j].scheme,
                                                   hosts, true, tab.pinned);
      }
    }
  },

  





  _updateCookies: function ssi_updateCookies(aWindows) {
    function addCookieToHash(aHash, aHost, aPath, aName, aCookie) {
      
      
      if (!aHash[aHost])
        aHash[aHost] = {};
      if (!aHash[aHost][aPath])
        aHash[aHost][aPath] = {};
      aHash[aHost][aPath][aName] = aCookie;
    }

    var jscookies = {};
    var _this = this;
    
    var MAX_EXPIRY = Math.pow(2, 62);

    for (let [id, window] in Iterator(aWindows)) {
      window.cookies = [];
      let internalWindow = this._internalWindows[id];
      if (!internalWindow.hosts)
        return;
      for (var [host, isPinned] in Iterator(internalWindow.hosts)) {
        let list;
        try {
          list = Services.cookies.getCookiesFromHost(host);
        }
        catch (ex) {
          debug("getCookiesFromHost failed. Host: " + host);
        }
        while (list && list.hasMoreElements()) {
          var cookie = list.getNext().QueryInterface(Ci.nsICookie2);
          
          
          
          if (cookie.isSession && _this.checkPrivacyLevel(cookie.isSecure, isPinned)) {
            
            
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
            window.cookies.push(jscookies[cookie.host][cookie.path][cookie.name]);
          }
        }
      }

      
      if (!window.cookies.length)
        delete window.cookies;
    }
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

  







  _getCurrentState: function ssi_getCurrentState(aUpdateAll, aPinnedOnly) {
    this._handleClosedWindows();

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
      });
      this._dirtyWindows = [];
    }

    
    var total = [], windows = {}, ids = [];
    var nonPopupCount = 0;
    var ix;
    for (ix in this._windows) {
      if (this._windows[ix]._restoring) 
        continue;
      total.push(this._windows[ix]);
      ids.push(ix);
      windows[ix] = this._windows[ix];
      if (!this._windows[ix].isPopup)
        nonPopupCount++;
    }
    this._updateCookies(windows);

    
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

    if (aPinnedOnly) {
      
      total = JSON.parse(this._toJSONString(total));
      total = total.filter(function (win) {
        win.tabs = win.tabs.filter(function (tab) tab.pinned);
        
        win._closedTabs = [];
        
        if (win.selected > win.tabs.length)
          win.selected = 1;
        return win.tabs.length > 0;
      });
      if (total.length == 0)
        return null;

      lastClosedWindowsCopy = [];
    }

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

    return {
      windows: total,
      selectedWindow: ix + 1,
      _closedWindows: lastClosedWindowsCopy,
      session: session,
      scratchpads: scratchpads
    };
  },

  





  _getWindowState: function ssi_getWindowState(aWindow) {
    if (!this._isWindowLoaded(aWindow))
      return this._statesToRestore[aWindow.__SS_restoreID];

    if (this._loadState == STATE_RUNNING) {
      this._collectWindowData(aWindow);
    }

    var winData = this._windows[aWindow.__SSi];
    let windows = {};
    windows[aWindow.__SSi] = winData;
    this._updateCookies(windows);

    return { windows: [winData] };
  },

  _collectWindowData: function ssi_collectWindowData(aWindow) {
    if (!this._isWindowLoaded(aWindow))
      return;

    
    this._saveWindowHistory(aWindow);
    this._updateTextAndScrollData(aWindow);
    this._updateCookieHosts(aWindow);
    this._updateWindowFeatures(aWindow);

    
    
    if (aWindow.__SS_lastSessionWindowID)
      this._windows[aWindow.__SSi].__lastSessionWindowID =
        aWindow.__SS_lastSessionWindowID;

    this._dirtyWindows[aWindow.__SSi] = false;
  },

  

  










  restoreWindow: function ssi_restoreWindow(aWindow, aState, aOverwriteTabs, aFollowUp) {
    if (!aFollowUp) {
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

    
    if (aOverwriteTabs) {
      for (let t = tabbrowser._numPinnedTabs - 1; t > -1; t--)
        tabbrowser.unpinTab(tabbrowser.tabs[t]);
    }

    
    
    if (aOverwriteTabs && tabbrowser.selectedTab._tPos >= newTabCount)
      tabbrowser.moveTabTo(tabbrowser.selectedTab, newTabCount - 1);

    let numVisibleTabs = 0;

    for (var t = 0; t < newTabCount; t++) {
      tabs.push(t < openTabCount ?
                tabbrowser.tabs[t] :
                tabbrowser.addTab("about:blank", {skipAnimation: true}));
      
      if (!aOverwriteTabs && root._firstTabs) {
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

    
    
    
    
    if (aOverwriteTabs) {
      for (let i = 0; i < tabbrowser.tabs.length; i++) {
        if (tabbrowser.browsers[i].__SS_restoreState)
          this._resetTabRestoringState(tabbrowser.tabs[i]);
      }
    }

    
    
    
    
    
    if (!aWindow.__SS_tabsToRestore)
      aWindow.__SS_tabsToRestore = 0;
    if (aOverwriteTabs)
      aWindow.__SS_tabsToRestore = newTabCount;
    else
      aWindow.__SS_tabsToRestore += newTabCount;

    
    
    
    delete aWindow.__SS_lastSessionWindowID;
    if (winData.__lastSessionWindowID)
      aWindow.__SS_lastSessionWindowID = winData.__lastSessionWindowID;

    
    if (aOverwriteTabs && newTabCount < openTabCount) {
      Array.slice(tabbrowser.tabs, newTabCount, openTabCount)
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

    if (aState.scratchpads) {
      ScratchpadManager.restoreSession(aState.scratchpads);
    }

    
    tabstrip.smoothScroll = smoothScroll;

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
          }
          aWindow.setTimeout(restoreHistoryFunc, 100, this);
          return;
        }
      }
    }

    if (!this._isWindowLoaded(aWindow)) {
      
      delete this._statesToRestore[aWindow.__SS_restoreID];
      delete aWindow.__SS_restoreID;
      delete this._windows[aWindow.__SSi]._restoring;
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

      for (let name in tabData.attributes)
        this.xulAttributes[name] = true;

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

      
      
      
      if (uri)
        browser.docShell.setCurrentURI(this._getURIFromString(uri));

      
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

    
    var disallow = (tabData.disallow)?tabData.disallow.split(","):[];
    CAPABILITIES.forEach(function(aCapability) {
      browser.docShell["allow" + aCapability] = disallow.indexOf(aCapability) == -1;
    });
    for (let name in this.xulAttributes)
      tab.removeAttribute(name);
    for (let name in tabData.attributes)
      tab.setAttribute(name, tabData.attributes[name]);

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
      
      if (tabData.pinned)
        this._tabsToRestore.priority.push(tab);
      else if (tabData.hidden)
        this._tabsToRestore.hidden.push(tab);
      else
        this._tabsToRestore.visible.push(tab);
      this.restoreNextTab();
    }
  },

  
















  restoreTab: function ssi_restoreTab(aTab) {
    let window = aTab.ownerDocument.defaultView;
    let browser = aTab.linkedBrowser;
    let tabData = browser.__SS_data;

    
    
    
    let didStartLoad = false;

    
    this._ensureTabsProgressListener(window);

    
    this._removeTabFromTabsToRestore(aTab);

    
    this._tabsRestoringCount++;

    
    browser.__SS_restoreState = TAB_STATE_RESTORING;
    browser.removeAttribute("pending");
    aTab.removeAttribute("pending");

    
    this._removeSHistoryListener(aTab);

    let activeIndex = (tabData.index || tabData.entries.length) - 1;
    if (activeIndex >= tabData.entries.length)
      activeIndex = tabData.entries.length - 1;
    
    
    
    browser.webNavigation.setCurrentURI(this._getURIFromString("about:blank"));
    
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

    
    if ((this._restoreOnDemand &&
        (this._restorePinnedTabsOnDemand || !this._tabsToRestore.priority.length)) ||
        this._tabsRestoringCount >= MAX_CONCURRENT_TAB_RESTORES)
      return;

    
    let nextTabArray;
    if (this._tabsToRestore.priority.length) {
      nextTabArray = this._tabsToRestore.priority
    }
    else if (this._tabsToRestore.visible.length) {
      nextTabArray = this._tabsToRestore.visible;
    }
    else if (this._restoreHiddenTabs && this._tabsToRestore.hidden.length) {
      nextTabArray = this._tabsToRestore.hidden;
    }

    if (nextTabArray) {
      let tab = nextTabArray.shift();
      let didStartLoad = this.restoreTab(tab);
      
      
      if (!didStartLoad)
        this.restoreNextTab();
    }
  },

  







  _deserializeHistoryEntry:
    function ssi_deserializeHistoryEntry(aEntry, aIdMap, aDocIdentMap) {

    var shEntry = Cc["@mozilla.org/browser/session-history-entry;1"].
                  createInstance(Ci.nsISHEntry);

    shEntry.setURI(this._getURIFromString(aEntry.url));
    shEntry.setTitle(aEntry.title || aEntry.url);
    if (aEntry.subframe)
      shEntry.setIsSubFrame(aEntry.subframe || false);
    shEntry.loadType = Ci.nsIDocShellLoadInfo.loadHistory;
    if (aEntry.contentType)
      shEntry.contentType = aEntry.contentType;
    if (aEntry.referrer)
      shEntry.referrerURI = this._getURIFromString(aEntry.referrer);

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

        
        
        
        if (aData.url == "about:sessionrestore" &&
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
      _this.restoreDimensions.apply(_this, [aWindow, aWinData.width || 0,
        aWinData.height || 0, "screenX" in aWinData ? aWinData.screenX : NaN,
        "screenY" in aWinData ? aWinData.screenY : NaN,
        aWinData.sizemode || "", aWinData.sidebar || ""]);
    }, 0);
  },

  














  restoreDimensions: function ssi_restoreDimensions(aWindow, aWidth, aHeight, aLeft, aTop, aSizeMode, aSidebar) {
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

  

  







  saveStateDelayed: function ssi_saveStateDelayed(aWindow, aDelay) {
    if (aWindow) {
      this._dirtyWindows[aWindow.__SSi] = true;
    }

    if (!this._saveTimer) {
      
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

  




  saveState: function ssi_saveState(aUpdateAll) {
    
    
    let pinnedOnly = this._loadState == STATE_RUNNING && !this._resume_from_crash;

    TelemetryStopwatch.start("FX_SESSION_RESTORE_COLLECT_DATA_MS");
    TelemetryStopwatch.start("FX_SESSION_RESTORE_COLLECT_DATA_LONGEST_OP_MS");

    var oState = this._getCurrentState(aUpdateAll, pinnedOnly);
    if (!oState) {
      TelemetryStopwatch.cancel("FX_SESSION_RESTORE_COLLECT_DATA_MS");
      TelemetryStopwatch.cancel("FX_SESSION_RESTORE_COLLECT_DATA_LONGEST_OP_MS");
      return;
    }

    
    for (let i = oState.windows.length - 1; i >= 0; i--) {
      if (oState.windows[i].isPrivate) {
        oState.windows.splice(i, 1);
        if (oState.selectedWindow >= i) {
          oState.selectedWindow--;
        }
      }
    }

    
    
    if (oState.windows.length == 0) {
      TelemetryStopwatch.cancel("FX_SESSION_RESTORE_COLLECT_DATA_MS");
      TelemetryStopwatch.cancel("FX_SESSION_RESTORE_COLLECT_DATA_LONGEST_OP_MS");
      return;
    }

    for (let i = oState._closedWindows.length - 1; i >= 0; i--) {
      if (oState._closedWindows[i].isPrivate) {
        oState._closedWindows.splice(i, 1);
      }
    }

#ifndef XP_MACOSX
    
    
    
    while (oState._closedWindows.length) {
      let i = oState._closedWindows.length - 1;
      if (oState._closedWindows[i]._shouldRestore) {
        delete oState._closedWindows[i]._shouldRestore;
        oState.windows.unshift(oState._closedWindows.pop());
      }
      else {
        
        break;
      }
    }
#endif

    if (pinnedOnly) {
      
      
      
      if (this._resume_session_once_on_shutdown == null) {
        this._resume_session_once_on_shutdown =
          this._prefBranch.getBoolPref("sessionstore.resume_session_once");
        this._prefBranch.setBoolPref("sessionstore.resume_session_once", true);
        
        Services.prefs.savePrefFile(null);
      }
    }

    
    if (this._lastSessionState)
      oState.lastSessionState = this._lastSessionState;

    TelemetryStopwatch.finish("FX_SESSION_RESTORE_COLLECT_DATA_MS");
    TelemetryStopwatch.finish("FX_SESSION_RESTORE_COLLECT_DATA_LONGEST_OP_MS");

    this._saveStateObject(oState);
  },

  


  _saveStateObject: function ssi_saveStateObject(aStateObj) {
    TelemetryStopwatch.start("FX_SESSION_RESTORE_SERIALIZE_DATA_MS");
    TelemetryStopwatch.start("FX_SESSION_RESTORE_SERIALIZE_DATA_LONGEST_OP_MS");
    let data = this._toJSONString(aStateObj);
    TelemetryStopwatch.finish("FX_SESSION_RESTORE_SERIALIZE_DATA_MS");
    TelemetryStopwatch.finish("FX_SESSION_RESTORE_SERIALIZE_DATA_LONGEST_OP_MS");

    let stateString = this._createSupportsString(data);
    Services.obs.notifyObservers(stateString, "sessionstore-state-write", "");
    data = stateString.data;

    
    if (!data) {
      return;
    }

    let self = this;
    _SessionFile.write(data).then(
      function onSuccess() {
        self._lastSaveTime = Date.now();
        Services.obs.notifyObservers(null, "sessionstore-state-write-complete", "");
      }
    );
  },

  

  
  _createSupportsString: function ssi_createSupportsString(aData) {
    let string = Cc["@mozilla.org/supports-string;1"]
                   .createInstance(Ci.nsISupportsString);
    string.data = aData;
    return string;
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

  








  checkPrivacyLevel: function ssi_checkPrivacyLevel(aIsHTTPS, aUseDefaultPref) {
    let pref = "sessionstore.privacy_level";
    
    
    
    if (!aUseDefaultPref && this._loadState == STATE_QUITTING && !this._doResumeSession())
      pref = "sessionstore.privacy_level_deferred";
    return this._prefBranch.getIntPref(pref) < (aIsHTTPS ? PRIVACY_ENCRYPTED : PRIVACY_FULL);
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

    
    if (winData.length == 1 && winData[0].tabs &&
        winData[0].tabs.length == 1 && winData[0].tabs[0].entries &&
        winData[0].tabs[0].entries.length == 1 &&
        winData[0].tabs[0].entries[0].url == "about:sessionrestore")
      return false;

    
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

  







  _shouldSaveTabState: function ssi_shouldSaveTabState(aTabState) {
    
    
    
    return aTabState.entries.length &&
           !(aTabState.entries.length == 1 &&
             aTabState.entries[0].url == "about:blank" &&
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

    
    let cookieHosts = {};
    aTargetWinState.tabs.forEach(function(tab) {
      tab.entries.forEach(function(entry) {
        this._extractHostsForCookiesFromEntry(entry, cookieHosts, false);
      }, this);
    }, this);

    
    
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
    this._tabsToRestore = { priority: [], visible: [], hidden: [] };
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

      
      
      
      this._removeTabFromTabsToRestore(aTab);
    }
  },

  




  _removeTabFromTabsToRestore: function ssi_removeTabFromTabsToRestore(aTab) {
    
    
    let arr = this._tabsToRestore.priority;
    let index = arr.indexOf(aTab);
    if (index == -1) {
      arr = this._tabsToRestore[aTab.hidden ? "hidden" : "visible"];
      index = arr.indexOf(aTab);
    }
    if (index > -1)
      arr.splice(index, 1);
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
  OnHistoryNewEntry: function(aNewURI) { },
  OnHistoryGoBack: function(aBackURI) { return true; },
  OnHistoryGoForward: function(aForwardURI) { return true; },
  OnHistoryGotoIndex: function(aIndex, aGotoURI) { return true; },
  OnHistoryPurge: function(aNumEntries) { return true; },
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
}
