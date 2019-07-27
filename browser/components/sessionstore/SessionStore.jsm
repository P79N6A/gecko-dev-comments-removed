



"use strict";

this.EXPORTED_SYMBOLS = ["SessionStore"];

const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

const TAB_STATE_NEEDS_RESTORE = 1;
const TAB_STATE_RESTORING = 2;

const NOTIFY_WINDOWS_RESTORED = "sessionstore-windows-restored";
const NOTIFY_BROWSER_STATE_RESTORED = "sessionstore-browser-state-restored";
const NOTIFY_LAST_SESSION_CLEARED = "sessionstore-last-session-cleared";

const NOTIFY_TAB_RESTORED = "sessionstore-debug-tab-restored"; 



const MAX_CONCURRENT_TAB_RESTORES = 3;


const OBSERVING = [
  "browser-window-before-show", "domwindowclosed",
  "quit-application-requested", "browser-lastwindow-close-granted",
  "quit-application", "browser:purge-session-history",
  "browser:purge-domain-data",
  "gather-telemetry",
  "idle-daily",
];



const WINDOW_ATTRIBUTES = ["width", "height", "screenX", "screenY", "sizemode"];



const WINDOW_HIDEABLE_FEATURES = [
  "menubar", "toolbar", "locationbar", "personalbar", "statusbar", "scrollbars"
];


const MESSAGES = [
  
  
  "SessionStore:setupSyncHandler",

  
  
  "SessionStore:update",

  
  "SessionStore:restoreHistoryComplete",

  
  
  "SessionStore:restoreTabContentStarted",

  
  
  
  
  "SessionStore:restoreTabContentComplete",

  
  
  "SessionStore:reloadPendingTab",

  
  
  "SessionStore:crashedTabRevived",
];



const NOTAB_MESSAGES = new Set([
  
  "SessionStore:setupSyncHandler",

  
  "SessionStore:update",
]);




const CLOSED_MESSAGES = new Set([
  
  "SessionStore:crashedTabRevived",
]);


const TAB_EVENTS = [
  "TabOpen", "TabClose", "TabSelect", "TabShow", "TabHide", "TabPinned",
  "TabUnpinned"
];


const MS_PER_DAY = 1000.0 * 60.0 * 60.0 * 24.0;

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
XPCOMUtils.defineLazyServiceGetter(this, "Telemetry",
  "@mozilla.org/base/telemetry;1", "nsITelemetry");
XPCOMUtils.defineLazyServiceGetter(this, "ppmm",
  "@mozilla.org/parentprocessmessagemanager;1",
  "nsIMessageListenerManager");
XPCOMUtils.defineLazyModuleGetter(this, "console",
  "resource://gre/modules/devtools/Console.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "RecentWindow",
  "resource:///modules/RecentWindow.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "AppConstants",
  "resource://gre/modules/AppConstants.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "GlobalState",
  "resource:///modules/sessionstore/GlobalState.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PrivacyFilter",
  "resource:///modules/sessionstore/PrivacyFilter.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "RunState",
  "resource:///modules/sessionstore/RunState.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "ScratchpadManager",
  "resource:///modules/devtools/scratchpad-manager.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "SessionSaver",
  "resource:///modules/sessionstore/SessionSaver.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "SessionCookies",
  "resource:///modules/sessionstore/SessionCookies.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "SessionFile",
  "resource:///modules/sessionstore/SessionFile.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "TabAttributes",
  "resource:///modules/sessionstore/TabAttributes.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "TabState",
  "resource:///modules/sessionstore/TabState.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "TabStateCache",
  "resource:///modules/sessionstore/TabStateCache.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Utils",
  "resource:///modules/sessionstore/Utils.jsm");





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

  init: function ss_init() {
    SessionStoreInternal.init();
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

  
  
  
  _restoreTabAndLoad: function ss_restoreTabAndLoad(aTab, aState, aLoadArguments) {
    SessionStoreInternal.setTabState(aTab, aState, {
      restoreImmediately: true,
      loadArguments: aLoadArguments
    });
  },

  duplicateTab: function ss_duplicateTab(aWindow, aTab, aDelta = 0) {
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

  getGlobalValue: function ss_getGlobalValue(aKey) {
    return SessionStoreInternal.getGlobalValue(aKey);
  },

  setGlobalValue: function ss_setGlobalValue(aKey, aStringValue) {
    SessionStoreInternal.setGlobalValue(aKey, aStringValue);
  },

  deleteGlobalValue: function ss_deleteGlobalValue(aKey) {
    SessionStoreInternal.deleteGlobalValue(aKey);
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

  reviveCrashedTab(aTab) {
    return SessionStoreInternal.reviveCrashedTab(aTab);
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

  _globalState: new GlobalState(),

  
  
  _restoreCount: -1,

  
  _nextRestoreEpoch: 1,

  
  _browserEpochs: new WeakMap(),

  
  
  
  _crashedBrowsers: new WeakSet(),

  
  _browserSetState: false,

  
  
  _sessionStartTime: Date.now(),

  
  _windows: {},

  
  _nextWindowID: 0,

  
  _closedWindows: [],

  
  _statesToRestore: {},

  
  _recentCrashes: 0,

  
  _restoreLastWindow: false,

  
  _tabsRestoringCount: 0,

  
  
  
  _deferredInitialState: null,

  
  _deferredInitialized: (function () {
    let deferred = {};

    deferred.promise = new Promise((resolve, reject) => {
      deferred.resolve = resolve;
      deferred.reject = reject;
    });

    return deferred;
  })(),

  
  _sessionInitialized: false,

  
  
  _promiseReadyForInitialization: null,

  
  _windowBusyStates: new WeakMap(),

  


  get promiseInitialized() {
    return this._deferredInitialized.promise;
  },

  get canRestoreLastSession() {
    return LastSession.canRestore;
  },

  set canRestoreLastSession(val) {
    
    if (!val) {
      LastSession.clear();
    }
  },

  


  init: function () {
    if (this._initialized) {
      throw new Error("SessionStore.init() must only be called once!");
    }

    TelemetryTimestamps.add("sessionRestoreInitialized");
    OBSERVING.forEach(function(aTopic) {
      Services.obs.addObserver(this, aTopic, true);
    }, this);

    this._initPrefs();
    this._initialized = true;
  },

  


  initSession: function () {
    TelemetryStopwatch.start("FX_SESSION_RESTORE_STARTUP_INIT_SESSION_MS");
    let state;
    let ss = gSessionStartup;

    if (ss.doRestore() ||
        ss.sessionType == Ci.nsISessionStartup.DEFER_SESSION) {
      state = ss.state;
    }

    if (state) {
      try {
        
        
        if (ss.sessionType == Ci.nsISessionStartup.DEFER_SESSION) {
          let [iniState, remainingState] = this._prepDataForDeferredRestore(state);
          
          
          if (iniState.windows.length)
            state = iniState;
          else
            state = null;

          if (remainingState.windows.length) {
            LastSession.setState(remainingState);
          }
        }
        else {
          
          
          LastSession.setState(state.lastSessionState);

          if (ss.previousSessionCrashed) {
            this._recentCrashes = (state.session &&
                                   state.session.recentCrashes || 0) + 1;

            if (this._needsRestorePage(state, this._recentCrashes)) {
              
              let url = "about:sessionrestore";
              let formdata = {id: {sessionData: state}, url};
              state = { windows: [{ tabs: [{ entries: [{url}], formdata }] }] };
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

    
    
    if (!RunState.isQuitting &&
        this._prefBranch.getBoolPref("sessionstore.resume_session_once"))
      this._prefBranch.setBoolPref("sessionstore.resume_session_once", false);

    TelemetryStopwatch.finish("FX_SESSION_RESTORE_STARTUP_INIT_SESSION_MS");
    return state;
  },

  _initPrefs : function() {
    this._prefBranch = Services.prefs.getBranch("browser.");

    gDebuggingEnabled = this._prefBranch.getBoolPref("sessionstore.debug");

    Services.prefs.addObserver("browser.sessionstore.debug", () => {
      gDebuggingEnabled = this._prefBranch.getBoolPref("sessionstore.debug");
    }, false);

    this._max_tabs_undo = this._prefBranch.getIntPref("sessionstore.max_tabs_undo");
    this._prefBranch.addObserver("sessionstore.max_tabs_undo", this, true);

    this._max_windows_undo = this._prefBranch.getIntPref("sessionstore.max_windows_undo");
    this._prefBranch.addObserver("sessionstore.max_windows_undo", this, true);
  },

  



  _uninit: function ssi_uninit() {
    if (!this._initialized) {
      throw new Error("SessionStore is not initialized.");
    }

    
    RunState.setClosing();

    
    if (this._sessionInitialized) {
      SessionSaver.run();
    }

    
    TabRestoreQueue.reset();

    
    SessionSaver.cancel();
  },

  


  observe: function ssi_observe(aSubject, aTopic, aData) {
    switch (aTopic) {
      case "browser-window-before-show": 
        this.onBeforeBrowserWindowShown(aSubject);
        break;
      case "domwindowclosed": 
        this.onClose(aSubject);
        break;
      case "quit-application-requested":
        this.onQuitApplicationRequested();
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
      case "gather-telemetry":
        this.onGatherTelemetry();
        break;
      case "idle-daily":
        this.onIdleDaily();
        break;
    }
  },

  




  receiveMessage(aMessage) {
    
    
    var browser = aMessage.target;
    var win = browser.ownerDocument.defaultView;
    let tab = win.gBrowser.getTabForBrowser(browser);

    
    
    if (!tab && !NOTAB_MESSAGES.has(aMessage.name)) {
      throw new Error(`received unexpected message '${aMessage.name}' ` +
                      `from a browser that has no tab`);
    }

    switch (aMessage.name) {
      case "SessionStore:setupSyncHandler":
        TabState.setSyncHandler(browser, aMessage.objects.handler);
        break;
      case "SessionStore:update":
        if (this._crashedBrowsers.has(browser.permanentKey)) {
          
          
          return;
        }
        this.recordTelemetry(aMessage.data.telemetry);
        TabState.update(browser, aMessage.data);
        this.saveStateDelayed(win);
        break;
      case "SessionStore:restoreHistoryComplete":
        if (this.isCurrentEpoch(browser, aMessage.data.epoch)) {
          
          let tabData = browser.__SS_data;

          
          
          let activePageData = tabData.entries[tabData.index - 1] || null;
          let uri = activePageData ? activePageData.url || null : null;
          browser.userTypedValue = uri;

          
          if (activePageData) {
            if (activePageData.title) {
              tab.label = activePageData.title;
              tab.crop = "end";
            } else if (activePageData.url != "about:blank") {
              tab.label = activePageData.url;
              tab.crop = "center";
            }
          }

          
          if ("image" in tabData) {
            win.gBrowser.setIcon(tab, tabData.image);
          }

          let event = win.document.createEvent("Events");
          event.initEvent("SSTabRestoring", true, false);
          tab.dispatchEvent(event);
        }
        break;
      case "SessionStore:restoreTabContentStarted":
        if (this.isCurrentEpoch(browser, aMessage.data.epoch)) {
          if (browser.__SS_restoreState == TAB_STATE_NEEDS_RESTORE) {
            
            
            this.markTabAsRestoring(tab);
          } else {
            
            
            
            
            let tabData = browser.__SS_data;
            if (tabData.userTypedValue && !tabData.userTypedClear) {
              browser.userTypedValue = tabData.userTypedValue;
              win.URLBarSetURI();
            }
          }
        }
        break;
      case "SessionStore:restoreTabContentComplete":
        if (this.isCurrentEpoch(browser, aMessage.data.epoch)) {
          
          
          if (gDebuggingEnabled) {
            Services.obs.notifyObservers(browser, NOTIFY_TAB_RESTORED, null);
          }

          delete browser.__SS_data;

          SessionStoreInternal._resetLocalTabRestoringState(tab);
          SessionStoreInternal.restoreNextTab();

          this._sendTabRestoredNotification(tab);
        }
        break;
      case "SessionStore:reloadPendingTab":
        if (this.isCurrentEpoch(browser, aMessage.data.epoch)) {
          if (browser.__SS_restoreState == TAB_STATE_NEEDS_RESTORE) {
            this.restoreTabContent(tab);
          }
        }
        break;
      case "SessionStore:crashedTabRevived":
        this._crashedBrowsers.delete(browser.permanentKey);
        break;
      default:
        throw new Error(`received unknown message '${aMessage.name}'`);
        break;
    }
  },

  





  recordTelemetry: function (telemetry) {
    for (let histogramId in telemetry){
      Telemetry.getHistogramById(histogramId).add(telemetry[histogramId]);
    }
  },

  

  


  handleEvent: function ssi_handleEvent(aEvent) {
    var win = aEvent.currentTarget.ownerDocument.defaultView;
    switch (aEvent.type) {
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
      case "SwapDocShells":
        this.saveStateDelayed(win);
        break;
      case "oop-browser-crashed":
        this.onBrowserCrashed(win, aEvent.originalTarget);
        break;
    }
    this._clearRestoringWindows();
  },

  




  _generateWindowID: function ssi_generateWindowID() {
    return "window" + (this._nextWindowID++);
  },

  










  onLoad: function ssi_onLoad(aWindow, aInitialState = null) {
    
    if (aWindow && aWindow.__SSi && this._windows[aWindow.__SSi])
      return;

    
    if (RunState.isQuitting)
      return;

    
    
    aWindow.__SSi = this._generateWindowID();

    let mm = aWindow.getGroupMessageManager("browsers");
    MESSAGES.forEach(msg => {
      let listenWhenClosed = CLOSED_MESSAGES.has(msg);
      mm.addMessageListener(msg, this, listenWhenClosed);
    });

    
    mm.loadFrameScript("chrome://browser/content/content-sessionStore.js", true);

    
    this._windows[aWindow.__SSi] = { tabs: [], selected: 0, _closedTabs: [], busy: false };

    let isPrivateWindow = false;
    if (PrivateBrowsingUtils.isWindowPrivate(aWindow))
      this._windows[aWindow.__SSi].isPrivate = isPrivateWindow = true;
    if (!this._isWindowLoaded(aWindow))
      this._windows[aWindow.__SSi]._restoring = true;
    if (!aWindow.toolbar.visible)
      this._windows[aWindow.__SSi].isPopup = true;

    
    if (RunState.isStopped) {
      RunState.setRunning();
      SessionSaver.updateLastSaveTime();

      
      if (aInitialState) {
        if (isPrivateWindow) {
          
          
          
          this._deferredInitialState = gSessionStartup.state;

          
          Services.obs.notifyObservers(null, NOTIFY_WINDOWS_RESTORED, "");
        } else {
          TelemetryTimestamps.add("sessionRestoreRestoring");
          this._restoreCount = aInitialState.windows ? aInitialState.windows.length : 0;

          
          
          this._globalState.setFromState(aInitialState);

          let overwrite = this._isCmdLineEmpty(aWindow, aInitialState);
          let options = {firstWindow: true, overwriteTabs: overwrite};
          this.restoreWindows(aWindow, aInitialState, options);
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
      this.restoreWindow(aWindow, state.windows[0], options);
    }
    
    
    
    else if (this._deferredInitialState && !isPrivateWindow &&
             aWindow.toolbar.visible) {

      
      
      this._globalState.setFromState(this._deferredInitialState);

      this._restoreCount = this._deferredInitialState.windows ?
        this._deferredInitialState.windows.length : 0;
      this.restoreWindows(aWindow, this._deferredInitialState, {firstWindow: true});
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
        if (AppConstants.platform == "macosx" || !this._doResumeSession()) {
          
          
          
          
          
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
        }
        else {
          
          
          this._closedWindows.splice(closedWindowIndex, 1);
          newWindowState = closedWindowState;
          delete newWindowState.hidden;
        }

        if (newWindowState) {
          
          this._restoreCount = 1;
          let state = { windows: [newWindowState] };
          let options = {overwriteTabs: this._isCmdLineEmpty(aWindow, state)};
          this.restoreWindow(aWindow, newWindowState, options);
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

  




  onBeforeBrowserWindowShown: function (aWindow) {
    
    if (this._sessionInitialized) {
      this.onLoad(aWindow);
      return;
    }

    
    
    
    if (!this._promiseReadyForInitialization) {
      
      let promise = new Promise(resolve => {
        Services.obs.addObserver(function obs(subject, topic) {
          if (aWindow == subject) {
            Services.obs.removeObserver(obs, topic);
            resolve();
          }
        }, "browser-delayed-startup-finished", false);
      });

      
      
      this._promiseReadyForInitialization =
        Promise.all([promise, gSessionStartup.onceInitialized]);
    }

    
    
    
    
    
    
    this._promiseReadyForInitialization.then(() => {
      if (aWindow.closed) {
        return;
      }

      if (this._sessionInitialized) {
        this.onLoad(aWindow);
      } else {
        let initialState = this.initSession();
        this._sessionInitialized = true;

        TelemetryStopwatch.start("FX_SESSION_RESTORE_STARTUP_ONLOAD_INITIAL_WINDOW_MS");
        this.onLoad(aWindow, initialState);
        TelemetryStopwatch.finish("FX_SESSION_RESTORE_STARTUP_ONLOAD_INITIAL_WINDOW_MS");

        
        this._deferredInitialized.resolve();
      }
    }, console.error);
  },

  






  onClose: function ssi_onClose(aWindow) {
    
    let isFullyLoaded = this._isWindowLoaded(aWindow);
    if (!isFullyLoaded) {
      if (!aWindow.__SSi) {
        aWindow.__SSi = this._generateWindowID();
      }

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

    let winData = this._windows[aWindow.__SSi];

    
    if (RunState.isRunning) {
      
      TabState.flushWindow(aWindow);

      
      this._collectWindowData(aWindow);

      if (isFullyLoaded) {
        winData.title = tabbrowser.selectedBrowser.contentTitle || tabbrowser.selectedTab.label;
        winData.title = this._replaceLoadingTitle(winData.title, tabbrowser,
                                                  tabbrowser.selectedTab);
        SessionCookies.update([winData]);
      }

      if (AppConstants.platform != "macosx") {
        
        
        winData._shouldRestore = true;
      }

      
      
      
      winData.closedAt = Date.now();

      
      
      if (!winData.isPrivate) {
        
        PrivacyFilter.filterPrivateTabs(winData);

        
        let hasSaveableTabs = winData.tabs.some(this._shouldSaveTabState);

        
        
        
        
        
        
        let isLastWindow =
          Object.keys(this._windows).length == 1 &&
          !this._closedWindows.some(win => win._shouldRestore || false);

        if (hasSaveableTabs || isLastWindow) {
          
          delete winData.busy;

          this._closedWindows.unshift(winData);
          this._capClosedWindows();
        }
      }

      
      delete this._windows[aWindow.__SSi];

      
      this.saveStateDelayed();
    }

    for (let i = 0; i < tabbrowser.tabs.length; i++) {
      this.onTabRemove(aWindow, tabbrowser.tabs[i], true);
    }

    
    DyingWindowCache.set(aWindow, winData);

    let mm = aWindow.getGroupMessageManager("browsers");
    MESSAGES.forEach(msg => mm.removeMessageListener(msg, this));

    delete aWindow.__SSi;
  },

  


  onQuitApplicationRequested: function ssi_onQuitApplicationRequested() {
    
    this._forEachBrowserWindow(function(aWindow) {
      
      
      TabState.flushWindow(aWindow);
      this._collectWindowData(aWindow);
    });
    
    
    var activeWindow = this._getMostRecentBrowserWindow();
    if (activeWindow)
      this.activeWindowSSiCache = activeWindow.__SSi || "";
    DirtyWindows.clear();
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
      
      LastSession.clear();
    }

    this._uninit();
  },

  


  onPurgeSessionHistory: function ssi_onPurgeSessionHistory() {
    SessionFile.wipe();
    
    
    
    if (RunState.isQuitting)
      return;
    LastSession.clear();
    let openWindows = {};
    this._forEachBrowserWindow(function(aWindow) {
      Array.forEach(aWindow.gBrowser.tabs, function(aTab) {
        delete aTab.linkedBrowser.__SS_data;
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
    } else if (RunState.isRunning) {
      SessionSaver.run();
    }

    this._clearRestoringWindows();
  },

  




  onPurgeDomainData: function ssi_onPurgeDomainData(aData) {
    
    function containsDomain(aEntry) {
      if (Utils.hasRootDomain(aEntry.url, aData)) {
        return true;
      }
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

    if (RunState.isRunning) {
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
    browser.addEventListener("SwapDocShells", this);
    browser.addEventListener("oop-browser-crashed", this);
    if (!aNoNotification) {
      this.saveStateDelayed(aWindow);
    }
  },

  








  onTabRemove: function ssi_onTabRemove(aWindow, aTab, aNoNotification) {
    let browser = aTab.linkedBrowser;
    delete browser.__SS_data;
    browser.removeEventListener("SwapDocShells", this);
    browser.removeEventListener("oop-browser-crashed", this);

    
    
    
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

    
    TabState.flush(aTab.linkedBrowser);

    
    let tabState = TabState.collect(aTab);

    
    let isPrivateWindow = PrivateBrowsingUtils.isWindowPrivate(aWindow);
    if (!isPrivateWindow && tabState.isPrivate) {
      return;
    }

    
    if (this._shouldSaveTabState(tabState)) {
      let tabTitle = aTab.label;
      let tabbrowser = aWindow.gBrowser;
      tabTitle = this._replaceLoadingTitle(tabTitle, tabbrowser, aTab);

      this._windows[aWindow.__SSi]._closedTabs.unshift({
        state: tabState,
        title: tabTitle,
        image: tabbrowser.getIcon(aTab),
        pos: aTab._tPos,
        closedAt: Date.now()
      });
      var length = this._windows[aWindow.__SSi]._closedTabs.length;
      if (length > this._max_tabs_undo)
        this._windows[aWindow.__SSi]._closedTabs.splice(this._max_tabs_undo, length - this._max_tabs_undo);
    }
  },

  




  onTabSelect: function ssi_onTabSelect(aWindow) {
    if (RunState.isRunning) {
      this._windows[aWindow.__SSi].selected = aWindow.gBrowser.tabContainer.selectedIndex;

      let tab = aWindow.gBrowser.selectedTab;
      
      
      
      if (tab.linkedBrowser.__SS_restoreState &&
          tab.linkedBrowser.__SS_restoreState == TAB_STATE_NEEDS_RESTORE)
        this.restoreTabContent(tab);
    }
  },

  onTabShow: function ssi_onTabShow(aWindow, aTab) {
    
    if (aTab.linkedBrowser.__SS_restoreState &&
        aTab.linkedBrowser.__SS_restoreState == TAB_STATE_NEEDS_RESTORE) {
      TabRestoreQueue.hiddenToVisible(aTab);

      
      
      this.restoreNextTab();
    }

    
    
    this.saveStateDelayed(aWindow);
  },

  onTabHide: function ssi_onTabHide(aWindow, aTab) {
    
    if (aTab.linkedBrowser.__SS_restoreState &&
        aTab.linkedBrowser.__SS_restoreState == TAB_STATE_NEEDS_RESTORE) {
      TabRestoreQueue.visibleToHidden(aTab);
    }

    
    
    this.saveStateDelayed(aWindow);
  },

  







  onBrowserCrashed: function(aWindow, aBrowser) {
    this._crashedBrowsers.add(aBrowser.permanentKey);
    
    
    
    
    
    
    
    
    
    let tab = aWindow.gBrowser.getTabForBrowser(aBrowser);
    this._resetLocalTabRestoringState(tab);
  },

  onGatherTelemetry: function() {
    
    
    Services.obs.removeObserver(this, "gather-telemetry");
    let stateString = SessionStore.getBrowserState();
    return SessionFile.gatherTelemetry(stateString);
  },

  
  
  
  onIdleDaily: function() {
    
    this._cleanupOldData([this._closedWindows]);

    
    this._cleanupOldData([winData._closedTabs for (winData of this._closedWindows)]);

    
    this._cleanupOldData([this._windows[key]._closedTabs for (key of Object.keys(this._windows))]);
  },

  
  _cleanupOldData: function(targets) {
    const TIME_TO_LIVE = this._prefBranch.getIntPref("sessionstore.cleanup.forget_closed_after");
    const now = Date.now();

    for (let array of targets) {
      for (let i = array.length - 1; i >= 0; --i)  {
        let data = array[i];
        
        
        
        data.closedAt = data.closedAt || now;
        if (now - data.closedAt > TIME_TO_LIVE) {
          array.splice(i, 1);
        }
      }
    }
  },

  

  getBrowserState: function ssi_getBrowserState() {
    let state = this.getCurrentState();

    
    delete state.lastSessionState;

    
    delete state.deferredInitialState;

    return JSON.stringify(state);
  },

  setBrowserState: function ssi_setBrowserState(aState) {
    this._handleClosedWindows();

    try {
      var state = JSON.parse(aState);
    }
    catch (ex) {  }
    if (!state) {
      throw Components.Exception("Invalid state string: not JSON", Cr.NS_ERROR_INVALID_ARG);
    }
    if (!state.windows) {
      throw Components.Exception("No windows", Cr.NS_ERROR_INVALID_ARG);
    }

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

    
    
    this._globalState.setFromState(state);

    
    this.restoreWindows(window, state, {overwriteTabs: true});
  },

  getWindowState: function ssi_getWindowState(aWindow) {
    if ("__SSi" in aWindow) {
      return JSON.stringify(this._getWindowState(aWindow));
    }

    if (DyingWindowCache.has(aWindow)) {
      let data = DyingWindowCache.get(aWindow);
      return JSON.stringify({ windows: [data] });
    }

    throw Components.Exception("Window is not tracked", Cr.NS_ERROR_INVALID_ARG);
  },

  setWindowState: function ssi_setWindowState(aWindow, aState, aOverwrite) {
    if (!aWindow.__SSi) {
      throw Components.Exception("Window is not tracked", Cr.NS_ERROR_INVALID_ARG);
    }

    this.restoreWindows(aWindow, aState, {overwriteTabs: aOverwrite});
  },

  getTabState: function ssi_getTabState(aTab) {
    if (!aTab.ownerDocument.defaultView.__SSi) {
      throw Components.Exception("Default view is not tracked", Cr.NS_ERROR_INVALID_ARG);
    }

    let tabState = TabState.collect(aTab);

    return JSON.stringify(tabState);
  },

  setTabState: function ssi_setTabState(aTab, aState, aOptions) {
    
    
    
    
    let tabState = JSON.parse(aState);
    if (!tabState) {
      throw Components.Exception("Invalid state string: not JSON", Cr.NS_ERROR_INVALID_ARG);
    }
    if (typeof tabState != "object") {
      throw Components.Exception("Not an object", Cr.NS_ERROR_INVALID_ARG);
    }
    if (!("entries" in tabState)) {
      throw Components.Exception("Invalid state object: no entries", Cr.NS_ERROR_INVALID_ARG);
    }

    let window = aTab.ownerDocument.defaultView;
    if (!("__SSi" in window)) {
      throw Components.Exception("Window is not tracked", Cr.NS_ERROR_INVALID_ARG);
    }

    if (aTab.linkedBrowser.__SS_restoreState) {
      this._resetTabRestoringState(aTab);
    }

    this.restoreTab(aTab, tabState, aOptions);
  },

  duplicateTab: function ssi_duplicateTab(aWindow, aTab, aDelta = 0) {
    if (!aTab.ownerDocument.defaultView.__SSi) {
      throw Components.Exception("Default view is not tracked", Cr.NS_ERROR_INVALID_ARG);
    }
    if (!aWindow.getBrowser) {
      throw Components.Exception("Invalid window object: no getBrowser", Cr.NS_ERROR_INVALID_ARG);
    }

    
    
    TabState.flush(aTab.linkedBrowser);

    
    let tabState = TabState.clone(aTab);

    tabState.index += aDelta;
    tabState.index = Math.max(1, Math.min(tabState.index, tabState.entries.length));
    tabState.pinned = false;

    let newTab = aTab == aWindow.gBrowser.selectedTab ?
      aWindow.gBrowser.addTab(null, {relatedToCurrent: true, ownerTab: aTab}) :
      aWindow.gBrowser.addTab();

    this.restoreTab(newTab, tabState, {
      restoreImmediately: true 
    });
    return newTab;
  },

  getClosedTabCount: function ssi_getClosedTabCount(aWindow) {
    if ("__SSi" in aWindow) {
      return this._windows[aWindow.__SSi]._closedTabs.length;
    }

    if (!DyingWindowCache.has(aWindow)) {
      throw Components.Exception("Window is not tracked", Cr.NS_ERROR_INVALID_ARG);
    }

    return DyingWindowCache.get(aWindow)._closedTabs.length;
  },

  getClosedTabData: function ssi_getClosedTabDataAt(aWindow) {
    if ("__SSi" in aWindow) {
      return JSON.stringify(this._windows[aWindow.__SSi]._closedTabs);
    }

    if (!DyingWindowCache.has(aWindow)) {
      throw Components.Exception("Window is not tracked", Cr.NS_ERROR_INVALID_ARG);
    }

    let data = DyingWindowCache.get(aWindow);
    return JSON.stringify(data._closedTabs);
  },

  undoCloseTab: function ssi_undoCloseTab(aWindow, aIndex) {
    if (!aWindow.__SSi) {
      throw Components.Exception("Window is not tracked", Cr.NS_ERROR_INVALID_ARG);
    }

    var closedTabs = this._windows[aWindow.__SSi]._closedTabs;

    
    aIndex = aIndex || 0;
    if (!(aIndex in closedTabs)) {
      throw Components.Exception("Invalid index: not in the closed tabs", Cr.NS_ERROR_INVALID_ARG);
    }

    
    let closedTab = closedTabs.splice(aIndex, 1).shift();
    let closedTabState = closedTab.state;

    
    let tabbrowser = aWindow.gBrowser;
    let tab = tabbrowser.selectedTab = tabbrowser.addTab();

    
    this.restoreTab(tab, closedTabState);

    
    tabbrowser.moveTabTo(tab, closedTab.pos);

    
    tab.linkedBrowser.focus();

    return tab;
  },

  forgetClosedTab: function ssi_forgetClosedTab(aWindow, aIndex) {
    if (!aWindow.__SSi) {
      throw Components.Exception("Window is not tracked", Cr.NS_ERROR_INVALID_ARG);
    }

    var closedTabs = this._windows[aWindow.__SSi]._closedTabs;

    
    aIndex = aIndex || 0;
    if (!(aIndex in closedTabs)) {
      throw Components.Exception("Invalid index: not in the closed tabs", Cr.NS_ERROR_INVALID_ARG);
    }

    
    closedTabs.splice(aIndex, 1);
  },

  getClosedWindowCount: function ssi_getClosedWindowCount() {
    return this._closedWindows.length;
  },

  getClosedWindowData: function ssi_getClosedWindowData() {
    return JSON.stringify(this._closedWindows);
  },

  undoCloseWindow: function ssi_undoCloseWindow(aIndex) {
    if (!(aIndex in this._closedWindows)) {
      throw Components.Exception("Invalid index: not in the closed windows", Cr.NS_ERROR_INVALID_ARG);
    }

    
    let state = { windows: this._closedWindows.splice(aIndex, 1) };
    delete state.windows[0].closedAt; 

    let window = this._openWindowWithState(state);
    this.windowToFocus = window;
    return window;
  },

  forgetClosedWindow: function ssi_forgetClosedWindow(aIndex) {
    
    aIndex = aIndex || 0;
    if (!(aIndex in this._closedWindows)) {
      throw Components.Exception("Invalid index: not in the closed windows", Cr.NS_ERROR_INVALID_ARG);
    }

    
    this._closedWindows.splice(aIndex, 1);
  },

  getWindowValue: function ssi_getWindowValue(aWindow, aKey) {
    if ("__SSi" in aWindow) {
      var data = this._windows[aWindow.__SSi].extData || {};
      return data[aKey] || "";
    }

    if (DyingWindowCache.has(aWindow)) {
      let data = DyingWindowCache.get(aWindow).extData || {};
      return data[aKey] || "";
    }

    throw Components.Exception("Window is not tracked", Cr.NS_ERROR_INVALID_ARG);
  },

  setWindowValue: function ssi_setWindowValue(aWindow, aKey, aStringValue) {
    if (typeof aStringValue != "string") {
      throw new TypeError("setWindowValue only accepts string values");
    }

    if (!("__SSi" in aWindow)) {
      throw Components.Exception("Window is not tracked", Cr.NS_ERROR_INVALID_ARG);
    }
    if (!this._windows[aWindow.__SSi].extData) {
      this._windows[aWindow.__SSi].extData = {};
    }
    this._windows[aWindow.__SSi].extData[aKey] = aStringValue;
    this.saveStateDelayed(aWindow);
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
    if (typeof aStringValue != "string") {
      throw new TypeError("setTabValue only accepts string values");
    }

    
    
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

    if (deleteFrom && aKey in deleteFrom) {
      delete deleteFrom[aKey];
      this.saveStateDelayed(aTab.ownerDocument.defaultView);
    }
  },

  getGlobalValue: function ssi_getGlobalValue(aKey) {
    return this._globalState.get(aKey);
  },

  setGlobalValue: function ssi_setGlobalValue(aKey, aStringValue) {
    if (typeof aStringValue != "string") {
      throw new TypeError("setGlobalValue only accepts string values");
    }

    this._globalState.set(aKey, aStringValue);
    this.saveStateDelayed();
  },

  deleteGlobalValue: function ssi_deleteGlobalValue(aKey) {
    this._globalState.delete(aKey);
    this.saveStateDelayed();
  },

  persistTabAttribute: function ssi_persistTabAttribute(aName) {
    if (TabAttributes.persist(aName)) {
      this.saveStateDelayed();
    }
  },

  






  restoreLastSession: function ssi_restoreLastSession() {
    
    if (!this.canRestoreLastSession) {
      throw Components.Exception("Last session can not be restored");
    }

    
    let windows = {};
    this._forEachBrowserWindow(function(aWindow) {
      if (aWindow.__SS_lastSessionWindowID)
        windows[aWindow.__SS_lastSessionWindowID] = aWindow;
    });

    let lastSessionState = LastSession.getState();

    
    if (!lastSessionState.windows.length) {
      throw Components.Exception("lastSessionState has no windows", Cr.NS_ERROR_UNEXPECTED);
    }

    
    
    this._restoreCount = lastSessionState.windows.length;
    this._browserSetState = true;

    
    
    
    
    let lastWindow = this._getMostRecentBrowserWindow();
    let canUseLastWindow = lastWindow &&
                           !lastWindow.__SS_lastSessionWindowID;

    
    
    this._globalState.setFromState(lastSessionState);

    
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
          curWinState._closedTabs.splice(this._max_tabs_undo, curWinState._closedTabs.length);
        }

        
        
        
        
        
        
        
        let options = {overwriteTabs: canOverwriteTabs, isFollowUp: true};
        this.restoreWindow(windowToUse, winState, options);
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

    LastSession.clear();
  },

  







  reviveCrashedTab(aTab) {
    if (!aTab) {
      throw new Error("SessionStore.reviveCrashedTab expected a tab, but got null.");
    }

    let browser = aTab.linkedBrowser;
    if (!this._crashedBrowsers.has(browser.permanentKey)) {
      return;
    }

    
    
    if (browser.isRemoteBrowser) {
      throw new Error("SessionStore.reviveCrashedTab: " +
                      "Somehow a crashed browser is still remote.")
    }

    let data = TabState.collect(aTab);
    this.restoreTab(aTab, data);
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

    TelemetryStopwatch.start("FX_SESSION_RESTORE_COLLECT_ALL_WINDOWS_DATA_MS");
    if (RunState.isRunning) {
      
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
    TelemetryStopwatch.finish("FX_SESSION_RESTORE_COLLECT_ALL_WINDOWS_DATA_MS");

    
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

    TelemetryStopwatch.start("FX_SESSION_RESTORE_COLLECT_COOKIES_MS");
    SessionCookies.update(total);
    TelemetryStopwatch.finish("FX_SESSION_RESTORE_COLLECT_COOKIES_MS");

    
    for (ix in this._statesToRestore) {
      for each (let winData in this._statesToRestore[ix].windows) {
        total.push(winData);
        if (!winData.isPopup)
          nonPopupCount++;
      }
    }

    
    let lastClosedWindowsCopy = this._closedWindows.slice();

    if (AppConstants.platform != "macosx") {
      
      
      
      
      
      if (nonPopupCount == 0 && lastClosedWindowsCopy.length > 0 &&
          RunState.isQuitting) {
        
        
        do {
          total.unshift(lastClosedWindowsCopy.shift())
        } while (total[0].isPopup && lastClosedWindowsCopy.length > 0)
      }
    }

    if (activeWindow) {
      this.activeWindowSSiCache = activeWindow.__SSi || "";
    }
    ix = ids.indexOf(this.activeWindowSSiCache);
    
    
    if (ix != -1 && total[ix] && total[ix].sizemode == "minimized")
      ix = -1;

    let session = {
      lastUpdate: Date.now(),
      startTime: this._sessionStartTime,
      recentCrashes: this._recentCrashes
    };

    let state = {
      windows: total,
      selectedWindow: ix + 1,
      _closedWindows: lastClosedWindowsCopy,
      session: session,
      global: this._globalState.getState()
    };

    if (Cu.isModuleLoaded("resource:///modules/devtools/scratchpad-manager.jsm")) {
      
      let scratchpads = ScratchpadManager.getSessionState();
      if (scratchpads && scratchpads.length) {
        state.scratchpads = scratchpads;
      }
    }

    
    if (LastSession.canRestore) {
      state.lastSessionState = LastSession.getState();
    }

    
    
    
    if (this._deferredInitialState) {
      state.deferredInitialState = this._deferredInitialState;
    }

    return state;
  },

  





  _getWindowState: function ssi_getWindowState(aWindow) {
    if (!this._isWindowLoaded(aWindow))
      return this._statesToRestore[aWindow.__SS_restoreID];

    if (RunState.isRunning) {
      this._collectWindowData(aWindow);
    }

    let windows = [this._windows[aWindow.__SSi]];
    SessionCookies.update(windows);

    return { windows: windows };
  },

  _collectWindowData: function ssi_collectWindowData(aWindow) {
    if (!this._isWindowLoaded(aWindow))
      return;
    TelemetryStopwatch.start("FX_SESSION_RESTORE_COLLECT_SINGLE_WINDOW_DATA_MS");

    let tabbrowser = aWindow.gBrowser;
    let tabs = tabbrowser.tabs;
    let winData = this._windows[aWindow.__SSi];
    let tabsData = winData.tabs = [];

    
    for (let tab of tabs) {
      tabsData.push(TabState.collect(tab));
    }
    winData.selected = tabbrowser.mTabBox.selectedIndex + 1;

    this._updateWindowFeatures(aWindow);

    
    
    if (aWindow.__SS_lastSessionWindowID)
      this._windows[aWindow.__SSi].__lastSessionWindowID =
        aWindow.__SS_lastSessionWindowID;

    DirtyWindows.remove(aWindow);
    TelemetryStopwatch.finish("FX_SESSION_RESTORE_COLLECT_SINGLE_WINDOW_DATA_MS");
  },

  

  












  restoreWindow: function ssi_restoreWindow(aWindow, winData, aOptions = {}) {
    let overwriteTabs = aOptions && aOptions.overwriteTabs;
    let isFollowUp = aOptions && aOptions.isFollowUp;
    let firstWindow = aOptions && aOptions.firstWindow;

    if (isFollowUp) {
      this.windowToFocus = aWindow;
    }

    
    if (aWindow && (!aWindow.__SSi || !this._windows[aWindow.__SSi]))
      this.onLoad(aWindow);

    TelemetryStopwatch.start("FX_SESSION_RESTORE_RESTORE_WINDOW_MS");

    
    
    this._setWindowStateBusy(aWindow);

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

    
    
    let initialTabs = [];
    if (!overwriteTabs && firstWindow) {
      initialTabs = Array.slice(tabbrowser.tabs);
    }

    
    
    if (overwriteTabs && tabbrowser.selectedTab._tPos >= newTabCount)
      tabbrowser.moveTabTo(tabbrowser.selectedTab, newTabCount - 1);

    let numVisibleTabs = 0;

    for (var t = 0; t < newTabCount; t++) {
      tabs.push(t < openTabCount ?
                tabbrowser.tabs[t] :
                tabbrowser.addTab("about:blank", {skipAnimation: true}));

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

    if (!overwriteTabs && firstWindow) {
      
      let endPosition = tabbrowser.tabs.length - 1;
      for (let i = 0; i < initialTabs.length; i++) {
        tabbrowser.moveTabTo(initialTabs[i], endPosition);
      }
    }

    
    if (!numVisibleTabs && winData.tabs.length) {
      winData.tabs[0].hidden = false;
      tabbrowser.showTab(tabs[0]);
    }

    
    
    
    
    if (overwriteTabs) {
      for (let i = 0; i < tabbrowser.tabs.length; i++) {
        let tab = tabbrowser.tabs[i];
        if (tabbrowser.browsers[i].__SS_restoreState)
          this._resetTabRestoringState(tab);
      }
    }

    
    
    
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
      SessionCookies.restore(winData.cookies);
    }
    if (winData.extData) {
      if (!this._windows[aWindow.__SSi].extData) {
        this._windows[aWindow.__SSi].extData = {};
      }
      for (var key in winData.extData) {
        this._windows[aWindow.__SSi].extData[key] = winData.extData[key];
      }
    }

    let newClosedTabsData = winData._closedTabs || [];

    if (overwriteTabs || firstWindow) {
      
      
      this._windows[aWindow.__SSi]._closedTabs = newClosedTabsData;
    } else if (this._max_tabs_undo > 0) {
      
      
      
      newClosedTabsData =
        newClosedTabsData.concat(this._windows[aWindow.__SSi]._closedTabs);

      
      
      this._windows[aWindow.__SSi]._closedTabs =
        newClosedTabsData.slice(0, this._max_tabs_undo);
    }

    
    if (winData.tabs.length) {
      this.restoreTabs(aWindow, tabs, winData.tabs,
        (overwriteTabs ? (parseInt(winData.selected || "1")) : 0));
    }

    
    tabstrip.smoothScroll = smoothScroll;

    TelemetryStopwatch.finish("FX_SESSION_RESTORE_RESTORE_WINDOW_MS");

    this._setWindowStateReady(aWindow);
    this._sendRestoreCompletedNotifications();
  },

  













  restoreWindows: function ssi_restoreWindows(aWindow, aState, aOptions = {}) {
    let isFollowUp = aOptions && aOptions.isFollowUp;

    if (isFollowUp) {
      this.windowToFocus = aWindow;
    }

    
    if (aWindow && (!aWindow.__SSi || !this._windows[aWindow.__SSi]))
      this.onLoad(aWindow);

    let root;
    try {
      root = (typeof aState == "string") ? JSON.parse(aState) : aState;
    }
    catch (ex) { 
      debug(ex);
      this._sendRestoreCompletedNotifications();
      return;
    }

    
    if (root._closedWindows) {
      this._closedWindows = root._closedWindows;
    }

    
    if (!root.windows || !root.windows.length) {
      this._sendRestoreCompletedNotifications();
      return;
    }

    if (!root.selectedWindow || root.selectedWindow > root.windows.length) {
      root.selectedWindow = 0;
    }

    
    
    let winData;
    for (var w = 1; w < root.windows.length; w++) {
      winData = root.windows[w];
      if (winData && winData.tabs && winData.tabs[0]) {
        var window = this._openWindowWithState({ windows: [winData] });
        if (w == root.selectedWindow - 1) {
          this.windowToFocus = window;
        }
      }
    }

    this.restoreWindow(aWindow, root.windows[0], aOptions);

    if (aState.scratchpads) {
      ScratchpadManager.restoreSession(aState.scratchpads);
    }
  },

  












  restoreTabs(aWindow, aTabs, aTabData, aSelectTab) {
    var tabbrowser = aWindow.gBrowser;

    if (!this._isWindowLoaded(aWindow)) {
      
      delete this._statesToRestore[aWindow.__SS_restoreID];
      delete aWindow.__SS_restoreID;
      delete this._windows[aWindow.__SSi]._restoring;
    }

    let numTabsToRestore = aTabs.length;
    let numTabsInWindow = tabbrowser.tabs.length;
    let tabsDataArray = this._windows[aWindow.__SSi].tabs;

    
    
    if (numTabsInWindow == numTabsToRestore) {
      
      tabsDataArray.length = 0;
    } else {
      
      tabsDataArray.splice(numTabsInWindow - numTabsToRestore);
    }

    
    tabsDataArray.length = numTabsInWindow;

    
    if (aSelectTab > 0 && aSelectTab <= aTabs.length) {
      tabbrowser.selectedTab = aTabs[aSelectTab - 1];

      
      this._windows[aWindow.__SSi].selected = aSelectTab;
    }

    
    for (let t = 0; t < aTabs.length; t++) {
      this.restoreTab(aTabs[t], aTabData[t]);
    }
  },

  
  restoreTab(tab, tabData, options = {}) {
    let restoreImmediately = options.restoreImmediately;
    let loadArguments = options.loadArguments;
    let browser = tab.linkedBrowser;
    let window = tab.ownerDocument.defaultView;
    let tabbrowser = window.gBrowser;

    
    this._setWindowStateBusy(window);

    
    
    DirtyWindows.add(window);

    
    
    
    for (let tab of Array.slice(tabbrowser.tabs, 0, tab._tPos)) {
      this._windows[window.__SSi].tabs.push(TabState.collect(tab));
    }

    
    this._windows[window.__SSi].tabs[tab._tPos] = tabData;

    
    
    
    if (tabData.pinned) {
      tabbrowser.pinTab(tab);
    } else {
      tabbrowser.unpinTab(tab);
    }

    if (tabData.hidden) {
      tabbrowser.hideTab(tab);
    } else {
      tabbrowser.showTab(tab);
    }

    if (tabData.lastAccessed) {
      tab.lastAccessed = tabData.lastAccessed;
    }

    if ("attributes" in tabData) {
      
      Object.keys(tabData.attributes).forEach(a => TabAttributes.persist(a));
    }

    if (!tabData.entries) {
      tabData.entries = [];
    }
    if (tabData.extData) {
      tab.__SS_extdata = Cu.cloneInto(tabData.extData, {});
    } else {
      delete tab.__SS_extdata;
    }

    
    delete tabData.closedAt;

    
    
    
    TabState.flush(browser);

    
    let activeIndex = (tabData.index || tabData.entries.length) - 1;
    activeIndex = Math.min(activeIndex, tabData.entries.length - 1);
    activeIndex = Math.max(activeIndex, 0);

    
    tabData.index = activeIndex + 1;

    
    
    let activePageData = tabData.entries[activeIndex] || null;
    let uri = activePageData ? activePageData.url || null : null;
    if (loadArguments) {
      uri = loadArguments.uri;
    }
    tabbrowser.updateBrowserRemotenessByURL(browser, uri);

    
    
    
    let epoch = this._nextRestoreEpoch++;
    this._browserEpochs.set(browser.permanentKey, epoch);

    
    
    browser.__SS_data = tabData;
    browser.__SS_restoreState = TAB_STATE_NEEDS_RESTORE;
    browser.setAttribute("pending", "true");
    tab.setAttribute("pending", "true");

    
    TabStateCache.update(browser, {
      history: {entries: tabData.entries, index: tabData.index},
      scroll: tabData.scroll || null,
      storage: tabData.storage || null,
      formdata: tabData.formdata || null,
      disallow: tabData.disallow || null,
      pageStyle: tabData.pageStyle || null
    });

    browser.messageManager.sendAsyncMessage("SessionStore:restoreHistory",
                                            {tabData: tabData, epoch: epoch, loadArguments});

    
    if ("attributes" in tabData) {
      TabAttributes.set(tab, tabData.attributes);
    }

    
    
    if (restoreImmediately || tabbrowser.selectedBrowser == browser || loadArguments) {
      this.restoreTabContent(tab, loadArguments);
    } else {
      TabRestoreQueue.add(tab);
      this.restoreNextTab();
    }

    
    this._setWindowStateReady(window);
  },

  







  restoreTabContent: function (aTab, aLoadArguments = null) {
    this.markTabAsRestoring(aTab);

    let browser = aTab.linkedBrowser;
    browser.messageManager.sendAsyncMessage("SessionStore:restoreTabContent",
      {loadArguments: aLoadArguments});
  },

  





  markTabAsRestoring(aTab) {
    let browser = aTab.linkedBrowser;
    if (browser.__SS_restoreState != TAB_STATE_NEEDS_RESTORE) {
      throw new Error("Given tab is not pending.");
    }

    
    TabRestoreQueue.remove(aTab);

    
    this._tabsRestoringCount++;

    
    browser.__SS_restoreState = TAB_STATE_RESTORING;
    browser.removeAttribute("pending");
    aTab.removeAttribute("pending");
  },

  







  restoreNextTab: function ssi_restoreNextTab() {
    
    if (RunState.isQuitting)
      return;

    
    if (this._tabsRestoringCount >= MAX_CONCURRENT_TAB_RESTORES)
      return;

    let tab = TabRestoreQueue.shift();
    if (tab) {
      this.restoreTabContent(tab);
    }
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
        +(aWinData.width || 0),
        +(aWinData.height || 0),
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
      
      
      if (aSizeMode != "maximized" || win_("sizemode") != "maximized") {
        aWindow.resizeTo(aWidth, aHeight);
      }
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
      aWindow.SidebarUI.show(aSidebar);
    }
    
    
    if (this.windowToFocus) {
      this.windowToFocus.focus();
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
    return RecentWindow.getMostRecentBrowserWindow({ allowPopups: true });
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
                 aTabState.entries[0].url == "about:newtab" ||
                 aTabState.entries[0].url == "about:privatebrowsing") &&
                 !aTabState.userTypedValue);
  },

  
















  _prepDataForDeferredRestore: function ssi_prepDataForDeferredRestore(state) {
    
    
    state = Cu.cloneInto(state, {});

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

    
    
    let hosts = Object.keys(cookieHosts).join("|").replace(/\./g, "\\.");
    
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
    let newCount = (this._windowBusyStates.get(aWindow) || 0) - 1;
    if (newCount < 0) {
      throw new Error("Invalid window busy state (less than zero).");
    }
    this._windowBusyStates.set(aWindow, newCount);

    if (newCount == 0) {
      this._setWindowStateBusyValue(aWindow, false);
      this._sendWindowStateEvent(aWindow, "Ready");
    }
  },

  



  _setWindowStateBusy: function ssi_setWindowStateBusy(aWindow) {
    let newCount = (this._windowBusyStates.get(aWindow) || 0) + 1;
    this._windowBusyStates.set(aWindow, newCount);

    if (newCount == 1) {
      this._setWindowStateBusyValue(aWindow, true);
      this._sendWindowStateEvent(aWindow, "Busy");
    }
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
    if (AppConstants.platform != "macosx") {
      let normalWindowIndex = 0;
      
      while (normalWindowIndex < this._closedWindows.length &&
             !!this._closedWindows[normalWindowIndex].isPopup)
        normalWindowIndex++;
      if (normalWindowIndex >= this._max_windows_undo)
        spliceTo = normalWindowIndex + 1;
    }
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

  






  _resetLocalTabRestoringState: function (aTab) {
    let window = aTab.ownerDocument.defaultView;
    let browser = aTab.linkedBrowser;

    
    let previousState = browser.__SS_restoreState;

    
    delete browser.__SS_restoreState;
    this._browserEpochs.delete(browser.permanentKey);

    aTab.removeAttribute("pending");
    browser.removeAttribute("pending");

    if (previousState == TAB_STATE_RESTORING) {
      if (this._tabsRestoringCount)
        this._tabsRestoringCount--;
    } else if (previousState == TAB_STATE_NEEDS_RESTORE) {
      
      
      
      TabRestoreQueue.remove(aTab);
    }
  },

  _resetTabRestoringState: function (tab) {
    let browser = tab.linkedBrowser;
    if (browser.__SS_restoreState) {
      browser.messageManager.sendAsyncMessage("SessionStore:resetRestore", {});
    }
    this._resetLocalTabRestoringState(tab);
  },

  






  isCurrentEpoch: function (browser, epoch) {
    return (this._browserEpochs.get(browser.permanentKey) || 0) == epoch;
  },

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





let LastSession = {
  _state: null,

  get canRestore() {
    return !!this._state;
  },

  getState: function () {
    return this._state;
  },

  setState: function (state) {
    this._state = state;
  },

  clear: function () {
    if (this._state) {
      this._state = null;
      Services.obs.notifyObservers(null, NOTIFY_LAST_SESSION_CLEARED, null);
    }
  }
};
