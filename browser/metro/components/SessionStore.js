



const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/WindowsPrefSync.jsm");

#ifdef MOZ_CRASHREPORTER
XPCOMUtils.defineLazyServiceGetter(this, "CrashReporter",
  "@mozilla.org/xre/app-info;1", "nsICrashReporter");
#endif

XPCOMUtils.defineLazyModuleGetter(this, "CrashMonitor",
  "resource://gre/modules/CrashMonitor.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "gUUIDGenerator",
  "@mozilla.org/uuid-generator;1", "nsIUUIDGenerator");

XPCOMUtils.defineLazyGetter(this, "NetUtil", function() {
  Cu.import("resource://gre/modules/NetUtil.jsm");
  return NetUtil;
});

XPCOMUtils.defineLazyModuleGetter(this, "UITelemetry",
  "resource://gre/modules/UITelemetry.jsm");





const STATE_STOPPED = 0;
const STATE_RUNNING = 1;
const STATE_QUITTING = -1;

function SessionStore() { }

SessionStore.prototype = {
  classID: Components.ID("{8c1f07d6-cba3-4226-a315-8bd43d67d032}"),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsISessionStore,
                                         Ci.nsIDOMEventListener,
                                         Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference]),

  _windows: {},
  _tabsFromOtherGroups: [],
  _selectedWindow: 1,
  _orderedWindows: [],
  _lastSaveTime: 0,
  _lastSessionTime: 0,
  _interval: 10000,
  _maxTabsUndo: 1,
  _shouldRestore: false,

  
  _maxTabsOpen: 1,

  init: function ss_init() {
    
    this._sessionFile = Services.dirsvc.get("ProfD", Ci.nsILocalFile);
    this._sessionFileBackup = this._sessionFile.clone();
    this._sessionCache = this._sessionFile.clone();
    this._sessionFile.append("sessionstore.js");
    this._sessionFileBackup.append("sessionstore.bak");
    this._sessionCache.append("sessionstoreCache");

    this._loadState = STATE_STOPPED;

    try {
      UITelemetry.addSimpleMeasureFunction("metro-tabs",
                                          this._getTabStats.bind(this));
    } catch (ex) {
      
    }

    CrashMonitor.previousCheckpoints.then(checkpoints => {
      let previousSessionCrashed = false;

      if (checkpoints) {
        
        
        previousSessionCrashed = !checkpoints["sessionstore-final-state-write-complete"];
      } else {
        
        
        
        
        previousSessionCrashed = Services.metro.previousExecutionState == 1 ||
          Services.metro.previousExecutionState == 2;
      }

      Services.telemetry.getHistogramById("SHUTDOWN_OK").add(!previousSessionCrashed);
    });

    try {
      let shutdownWasUnclean = false;

      if (this._sessionFileBackup.exists()) {
        this._sessionFileBackup.remove(false);
        shutdownWasUnclean = true;
      }

      if (this._sessionFile.exists()) {
        this._sessionFile.copyTo(null, this._sessionFileBackup.leafName);

        switch(Services.metro.previousExecutionState) {
          
          case 0:
            
            this._lastSessionTime = this._sessionFile.lastModifiedTime;
            let delta = Date.now() - this._lastSessionTime;
            let timeout =
              Services.prefs.getIntPref(
                  "browser.sessionstore.resume_from_crash_timeout");
            this._shouldRestore = shutdownWasUnclean
                                && (delta < (timeout * 60000));
            break;
          
          case 1:
            
            Components.utils.reportError("SessionRestore.init called with "
                                       + "previous execution state 'Running'");
            this._shouldRestore = true;
            break;
          
          case 2:
            
            Components.utils.reportError("SessionRestore.init called with "
                                       + "previous execution state 'Suspended'");
            this._shouldRestore = true;
            break;
          
          case 3:
            
            
            
            
            this._shouldRestore = true;
            break;
          
          case 4:
            
            
            
            
            this._shouldRestore = false;
            break;
        }
      }

      if (!this._sessionCache.exists() || !this._sessionCache.isDirectory()) {
        this._sessionCache.create(Ci.nsIFile.DIRECTORY_TYPE, 0700);
      }
    } catch (ex) {
      Cu.reportError(ex); 
    }

    this._interval = Services.prefs.getIntPref("browser.sessionstore.interval");
    this._maxTabsUndo = Services.prefs.getIntPref("browser.sessionstore.max_tabs_undo");

    
    if (!Services.prefs.getBoolPref("browser.sessionstore.resume_from_crash"))
      this._shouldRestore = false;

    
    if (Services.prefs.getBoolPref("browser.sessionstore.resume_session_once")) {
      Services.prefs.setBoolPref("browser.sessionstore.resume_session_once", false);
      this._shouldRestore = true;
    }
  },

  _clearDisk: function ss_clearDisk() {
    if (this._sessionFile.exists()) {
      try {
        this._sessionFile.remove(false);
      } catch (ex) { dump(ex + '\n'); } 
    }
    if (this._sessionFileBackup.exists()) {
      try {
        this._sessionFileBackup.remove(false);
      } catch (ex) { dump(ex + '\n'); } 
    }

    this._clearCache();
  },

  _clearCache: function ss_clearCache() {
    
    let activeFiles = [];
    this._forEachBrowserWindow(function(aWindow) {
      let tabs = aWindow.Browser.tabs;
      for (let i = 0; i < tabs.length; i++) {
        let browser = tabs[i].browser;
        if (browser.__SS_extdata && "thumbnail" in browser.__SS_extdata)
          activeFiles.push(browser.__SS_extdata.thumbnail);
      }
    });

    
    let staleFiles = [];
    let cacheFiles = this._sessionCache.directoryEntries;
    while (cacheFiles.hasMoreElements()) {
      let file = cacheFiles.getNext().QueryInterface(Ci.nsILocalFile);
      let fileURI = Services.io.newFileURI(file);
      if (activeFiles.indexOf(fileURI) == -1)
        staleFiles.push(file);
    }

    
    
    staleFiles.forEach(function(aFile) {
      aFile.remove(false);
    })
  },

  _getTabStats: function() {
    return {
      currTabCount: this._currTabCount,
      maxTabCount: this._maxTabsOpen
    };
  },

  observe: function ss_observe(aSubject, aTopic, aData) {
    let self = this;
    let observerService = Services.obs;
    switch (aTopic) {
      case "app-startup":
        observerService.addObserver(this, "final-ui-startup", true);
        observerService.addObserver(this, "domwindowopened", true);
        observerService.addObserver(this, "domwindowclosed", true);
        observerService.addObserver(this, "browser-lastwindow-close-granted", true);
        observerService.addObserver(this, "browser:purge-session-history", true);
        observerService.addObserver(this, "quit-application-requested", true);
        observerService.addObserver(this, "quit-application-granted", true);
        observerService.addObserver(this, "quit-application", true);
        observerService.addObserver(this, "reset-telemetry-vars", true);
        break;
      case "final-ui-startup":
        observerService.removeObserver(this, "final-ui-startup");
        if (WindowsPrefSync) {
          
          WindowsPrefSync.init();
        }
        this.init();
        break;
      case "domwindowopened":
        let window = aSubject;
        window.addEventListener("load", function() {
          self.onWindowOpen(window);
          window.removeEventListener("load", arguments.callee, false);
        }, false);
        break;
      case "domwindowclosed": 
        this.onWindowClose(aSubject);
        break;
      case "browser-lastwindow-close-granted":
        
        if (this._saveTimer) {
          this._saveTimer.cancel();
          this._saveTimer = null;
          this.saveState();
        }

        
        this._loadState = STATE_QUITTING;
        break;
      case "quit-application-requested":
        
        this._forEachBrowserWindow(function(aWindow) {
          self._collectWindowData(aWindow);
        });
        break;
      case "quit-application-granted":
        
        this._forEachBrowserWindow(function(aWindow) {
          self._collectWindowData(aWindow);
        });

        
        this._loadState = STATE_QUITTING;
        break;
      case "quit-application":
        
        if (aData == "restart") {
          Services.prefs.setBoolPref("browser.sessionstore.resume_session_once", true);

          
          Services.obs.removeObserver(this, "browser:purge-session-history");
        }

        
        this._loadState = STATE_QUITTING;

        
        if (this._sessionFileBackup.exists())
          this._sessionFileBackup.remove(false);

        observerService.removeObserver(this, "domwindowopened");
        observerService.removeObserver(this, "domwindowclosed");
        observerService.removeObserver(this, "browser-lastwindow-close-granted");
        observerService.removeObserver(this, "quit-application-requested");
        observerService.removeObserver(this, "quit-application-granted");
        observerService.removeObserver(this, "quit-application");
        observerService.removeObserver(this, "reset-telemetry-vars");

        
        if (this._saveTimer) {
          this._saveTimer.cancel();
          this._saveTimer = null;
        }
        this.saveState();
        break;
      case "browser:purge-session-history": 
        this._clearDisk();

        
        
        
        if (this._loadState == STATE_QUITTING)
          return;

        
        for (let [ssid, win] in Iterator(this._windows))
          win._closedTabs = [];

        if (this._loadState == STATE_RUNNING) {
          
          this.saveStateNow();
        }
        break;
      case "timer-callback":
        
        this._saveTimer = null;
        this.saveState();
        break;
      case "reset-telemetry-vars":
        
        this._maxTabsOpen = 1;
    }
  },

  updateTabTelemetryVars: function(window) {
    this._currTabCount = window.Browser.tabs.length;
      if (this._currTabCount > this._maxTabsOpen) {
        this._maxTabsOpen = this._currTabCount;
      }
  },

  handleEvent: function ss_handleEvent(aEvent) {
    let window = aEvent.currentTarget.ownerDocument.defaultView;
    switch (aEvent.type) {
      case "load":
        browser = aEvent.currentTarget;
        if (aEvent.target == browser.contentDocument && browser.__SS_tabFormData) {
          browser.messageManager.sendAsyncMessage("SessionStore:restoreSessionTabData", {
            formdata: browser.__SS_tabFormData.formdata,
            scroll: browser.__SS_tabFormData.scroll
          });
        }
        break;
      case "TabOpen":
        this.updateTabTelemetryVars(window);
        let browser = aEvent.originalTarget.linkedBrowser;
        browser.addEventListener("load", this, true);
      case "TabClose": {
        let browser = aEvent.originalTarget.linkedBrowser;
        if (aEvent.type == "TabOpen") {
          this.onTabAdd(window, browser);
        }
        else {
          this.onTabClose(window, browser);
          this.onTabRemove(window, browser);
        }
        break;
    }
      case "TabRemove":
        this.updateTabTelemetryVars(window);
        break;
      case "TabSelect": {
        let browser = aEvent.originalTarget.linkedBrowser;
        this.onTabSelect(window, browser);
        break;
      }
    }
  },

  receiveMessage: function ss_receiveMessage(aMessage) {
    let browser = aMessage.target;
    switch (aMessage.name) {
      case "SessionStore:collectFormdata":
        browser.__SS_data.formdata = aMessage.json.data;
        break;
      case "SessionStore:collectScrollPosition":
        browser.__SS_data.scroll = aMessage.json.data;
        break;
      default:
        let window = aMessage.target.ownerDocument.defaultView;
        this.onTabLoad(window, aMessage.target, aMessage);
        break;
    }
  },

  onWindowOpen: function ss_onWindowOpen(aWindow) {
    
    if (aWindow && aWindow.__SSID && this._windows[aWindow.__SSID])
      return;

    
    if (aWindow.document.documentElement.getAttribute("windowtype") != "navigator:browser" || this._loadState == STATE_QUITTING)
      return;

    
    aWindow.__SSID = "window" + gUUIDGenerator.generateUUID().toString();
    this._windows[aWindow.__SSID] = { tabs: [], selected: 0, _closedTabs: [] };
    this._orderedWindows.push(aWindow.__SSID);

    
    if (this._loadState == STATE_STOPPED) {
      this._loadState = STATE_RUNNING;
      this._lastSaveTime = Date.now();

      
      if (!this.shouldRestore()) {
        this._clearCache();
        Services.obs.notifyObservers(null, "sessionstore-windows-restored", "");
      }
    }

    
    let tabs = aWindow.Browser.tabs;
    for (let i = 0; i < tabs.length; i++)
      this.onTabAdd(aWindow, tabs[i].browser, true);

    
    let tabContainer = aWindow.document.getElementById("tabs");
    tabContainer.addEventListener("TabOpen", this, true);
    tabContainer.addEventListener("TabClose", this, true);
    tabContainer.addEventListener("TabRemove", this, true);
    tabContainer.addEventListener("TabSelect", this, true);
  },

  onWindowClose: function ss_onWindowClose(aWindow) {
    
    if (!aWindow.__SSID || !this._windows[aWindow.__SSID])
      return;

    let tabContainer = aWindow.document.getElementById("tabs");
    tabContainer.removeEventListener("TabOpen", this, true);
    tabContainer.removeEventListener("TabClose", this, true);
    tabContainer.removeEventListener("TabRemove", this, true);
    tabContainer.removeEventListener("TabSelect", this, true);

    if (this._loadState == STATE_RUNNING) {
      
      this._collectWindowData(aWindow);

      
      delete this._windows[aWindow.__SSID];

      
      this.saveStateDelayed();
    }

    let tabs = aWindow.Browser.tabs;
    for (let i = 0; i < tabs.length; i++)
      this.onTabRemove(aWindow, tabs[i].browser, true);

    delete aWindow.__SSID;
  },

  onTabAdd: function ss_onTabAdd(aWindow, aBrowser, aNoNotification) {
    aBrowser.messageManager.addMessageListener("pageshow", this);
    aBrowser.messageManager.addMessageListener("Content:SessionHistory", this);
    aBrowser.messageManager.addMessageListener("SessionStore:collectFormdata", this);
    aBrowser.messageManager.addMessageListener("SessionStore:collectScrollPosition", this);

    if (!aNoNotification)
      this.saveStateDelayed();
    this._updateCrashReportURL(aWindow);
  },

  onTabRemove: function ss_onTabRemove(aWindow, aBrowser, aNoNotification) {
    aBrowser.messageManager.removeMessageListener("pageshow", this);
    aBrowser.messageManager.removeMessageListener("Content:SessionHistory", this);
    aBrowser.messageManager.removeMessageListener("SessionStore:collectFormdata", this);
    aBrowser.messageManager.removeMessageListener("SessionStore:collectScrollPosition", this);

    
    if (aBrowser.__SS_restore)
      return;

    delete aBrowser.__SS_data;

    if (!aNoNotification)
      this.saveStateDelayed();
  },

  onTabClose: function ss_onTabClose(aWindow, aBrowser) {
    if (this._maxTabsUndo == 0)
      return;

    if (aWindow.Browser.tabs.length > 0) {
      
      
      
      
      
      let data = aBrowser.__SS_data;
      if (!data) {
        return; 
      }
      try { data.extData = aBrowser.__SS_extdata; } catch (e) { }

      this._windows[aWindow.__SSID]._closedTabs.unshift({ state: data });
      let length = this._windows[aWindow.__SSID]._closedTabs.length;
      if (length > this._maxTabsUndo)
        this._windows[aWindow.__SSID]._closedTabs.splice(this._maxTabsUndo, length - this._maxTabsUndo);
    }
  },

  onTabLoad: function ss_onTabLoad(aWindow, aBrowser, aMessage) {
    
    if (aBrowser.__SS_restore)
      return;

    
    if (!aBrowser.canGoBack && aBrowser.currentURI.spec == "about:blank")
      return;

    if (aMessage.name == "Content:SessionHistory") {
      delete aBrowser.__SS_data;
      this._collectTabData(aBrowser, aMessage.json);
    }

    
    if (aMessage.name == "pageshow")
      this.saveStateNow();

    this._updateCrashReportURL(aWindow);
  },

  onTabSelect: function ss_onTabSelect(aWindow, aBrowser) {
    if (this._loadState != STATE_RUNNING)
      return;

    let index = aWindow.Elements.browsers.selectedIndex;
    this._windows[aWindow.__SSID].selected = parseInt(index) + 1; 

    
    if (aBrowser.__SS_restore) {
      let data = aBrowser.__SS_data;
      if (data.entries.length > 0) {
        let json = {
          uri: data.entries[data.index - 1].url,
          flags: null,
          entries: data.entries,
          index: data.index
        };
        aBrowser.messageManager.sendAsyncMessage("WebNavigation:LoadURI", json);
      }

      delete aBrowser.__SS_restore;
    }

    this._updateCrashReportURL(aWindow);
  },

  saveStateDelayed: function ss_saveStateDelayed() {
    if (!this._saveTimer) {
      
      let minimalDelay = this._lastSaveTime + this._interval - Date.now();

      
      let delay = Math.max(minimalDelay, 2000);
      if (delay > 0) {
        this._saveTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
        this._saveTimer.init(this, delay, Ci.nsITimer.TYPE_ONE_SHOT);
      } else {
        this.saveState();
      }
    }
  },

  saveStateNow: function ss_saveStateNow() {
    
    if (this._saveTimer) {
      this._saveTimer.cancel();
      this._saveTimer = null;
    }
    this.saveState();
  },

  saveState: function ss_saveState() {
    let data = this._getCurrentState();
    
    if (data.windows && data.windows.length && data.selectedWindow) {
      this._writeFile(this._sessionFile, JSON.stringify(data));

      this._lastSaveTime = Date.now();
    } else {
      dump("SessionStore: Not saving state with invalid data: " + JSON.stringify(data) + "\n");
    }
  },

  _getCurrentState: function ss_getCurrentState() {
    let self = this;
    this._forEachBrowserWindow(function(aWindow) {
      self._collectWindowData(aWindow);
    });

    let data = { windows: [] };
    for (let i = 0; i < this._orderedWindows.length; i++)
      data.windows.push(this._windows[this._orderedWindows[i]]);
    data.selectedWindow = this._selectedWindow;
    return data;
  },

  _collectTabData: function ss__collectTabData(aBrowser, aHistory) {
    
    if (aBrowser.__SS_restore)
      return;

    aHistory = aHistory || { entries: [{ url: aBrowser.currentURI.spec, title: aBrowser.contentTitle }], index: 1 };

    let tabData = {};
    tabData.entries = aHistory.entries;
    tabData.index = aHistory.index;
    tabData.attributes = { image: aBrowser.mIconURL };

    aBrowser.__SS_data = tabData;
  },

  _getTabData: function(aWindow) {
    return aWindow.Browser.tabs
      .filter(tab => !tab.isPrivate && tab.browser.__SS_data)
      .map(tab => {
        let browser = tab.browser;
        let tabData = browser.__SS_data;
        if (browser.__SS_extdata)
          tabData.extData = browser.__SS_extdata;
        return tabData;
      });
  },

  _collectWindowData: function ss__collectWindowData(aWindow) {
    
    if (!aWindow.__SSID || !this._windows[aWindow.__SSID])
      return;

    let winData = this._windows[aWindow.__SSID];

    let index = aWindow.Elements.browsers.selectedIndex;
    winData.selected = parseInt(index) + 1; 

    let tabData = this._getTabData(aWindow);
    winData.tabs = tabData.concat(this._tabsFromOtherGroups);
  },

  _forEachBrowserWindow: function ss_forEachBrowserWindow(aFunc) {
    let windowsEnum = Services.wm.getEnumerator("navigator:browser");
    while (windowsEnum.hasMoreElements()) {
      let window = windowsEnum.getNext();
      if (window.__SSID && !window.closed)
        aFunc.call(this, window);
    }
  },

  _writeFile: function ss_writeFile(aFile, aData) {
    let stateString = Cc["@mozilla.org/supports-string;1"].createInstance(Ci.nsISupportsString);
    stateString.data = aData;
    Services.obs.notifyObservers(stateString, "sessionstore-state-write", "");

    
    if (!stateString.data)
      return;

    
    let ostream = Cc["@mozilla.org/network/safe-file-output-stream;1"].createInstance(Ci.nsIFileOutputStream);
    ostream.init(aFile, 0x02 | 0x08 | 0x20, 0600, ostream.DEFER_OPEN);

    
    let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"].createInstance(Ci.nsIScriptableUnicodeConverter);
    converter.charset = "UTF-8";

    
    let istream = converter.convertToInputStream(aData);
    NetUtil.asyncCopy(istream, ostream, function(rc) {
      if (Components.isSuccessCode(rc)) {
        if (Services.startup.shuttingDown) {
          Services.obs.notifyObservers(null, "sessionstore-final-state-write-complete", "");
        }
        Services.obs.notifyObservers(null, "sessionstore-state-write-complete", "");
      }
    });
  },

  _updateCrashReportURL: function ss_updateCrashReportURL(aWindow) {
#ifdef MOZ_CRASHREPORTER
    try {
      let currentURI = aWindow.Browser.selectedBrowser.currentURI.clone();
      
      try {
        currentURI.userPass = "";
      }
      catch (ex) { } 

      CrashReporter.annotateCrashReport("URL", currentURI.spec);
    }
    catch (ex) {
      
      if (ex.result != Components.results.NS_ERROR_NOT_INITIALIZED)
        Components.utils.reportError("SessionStore:" + ex);
    }
#endif
  },

  getBrowserState: function ss_getBrowserState() {
    let data = this._getCurrentState();
    return JSON.stringify(data);
  },

  getClosedTabCount: function ss_getClosedTabCount(aWindow) {
    if (!aWindow || !aWindow.__SSID)
      return 0; 

    return this._windows[aWindow.__SSID]._closedTabs.length;
  },

  getClosedTabData: function ss_getClosedTabData(aWindow) {
    if (!aWindow.__SSID)
      throw (Components.returnCode = Cr.NS_ERROR_INVALID_ARG);

    return JSON.stringify(this._windows[aWindow.__SSID]._closedTabs);
  },

  undoCloseTab: function ss_undoCloseTab(aWindow, aIndex) {
    if (!aWindow.__SSID)
      throw (Components.returnCode = Cr.NS_ERROR_INVALID_ARG);

    let closedTabs = this._windows[aWindow.__SSID]._closedTabs;
    if (!closedTabs)
      return null;

    
    aIndex = aIndex || 0;
    if (!(aIndex in closedTabs))
      throw (Components.returnCode = Cr.NS_ERROR_INVALID_ARG);

    
    let closedTab = closedTabs.splice(aIndex, 1).shift();

    
    let tab = aWindow.Browser.addTab(closedTab.state.entries[closedTab.state.index - 1].url, true);

    tab.browser.messageManager.sendAsyncMessage("WebNavigation:LoadURI", {
      uri: closedTab.state.entries[closedTab.state.index - 1].url,
      flags: null,
      entries: closedTab.state.entries,
      index: closedTab.state.index
    });

    
    tab.browser.__SS_extdata = closedTab.extData;

    return tab.chromeTab;
  },

  forgetClosedTab: function ss_forgetClosedTab(aWindow, aIndex) {
    if (!aWindow.__SSID)
      throw (Components.returnCode = Cr.NS_ERROR_INVALID_ARG);

    let closedTabs = this._windows[aWindow.__SSID]._closedTabs;

    
    aIndex = aIndex || 0;
    if (!(aIndex in closedTabs))
      throw (Components.returnCode = Cr.NS_ERROR_INVALID_ARG);

    
    closedTabs.splice(aIndex, 1);
  },

  getTabValue: function ss_getTabValue(aTab, aKey) {
    let browser = aTab.linkedBrowser;
    let data = browser.__SS_extdata || {};
    return data[aKey] || "";
  },

  setTabValue: function ss_setTabValue(aTab, aKey, aStringValue) {
    let browser = aTab.linkedBrowser;

    
    if (aKey == "thumbnail") {
      let file = this._sessionCache.clone();
      file.append("thumbnail-" + browser.contentWindowId);
      file.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, 0600);

      let source = Services.io.newURI(aStringValue, "UTF8", null);
      let target = Services.io.newFileURI(file)

      let persist = Cc["@mozilla.org/embedding/browser/nsWebBrowserPersist;1"].createInstance(Ci.nsIWebBrowserPersist);
      persist.persistFlags = Ci.nsIWebBrowserPersist.PERSIST_FLAGS_REPLACE_EXISTING_FILES | Ci.nsIWebBrowserPersist.PERSIST_FLAGS_AUTODETECT_APPLY_CONVERSION;
      persist.saveURI(source, null, null, null, null, file);

      aStringValue = target.spec;
    }

    if (!browser.__SS_extdata)
      browser.__SS_extdata = {};
    browser.__SS_extdata[aKey] = aStringValue;
    this.saveStateDelayed();
  },

  deleteTabValue: function ss_deleteTabValue(aTab, aKey) {
    let browser = aTab.linkedBrowser;
    if (browser.__SS_extdata && browser.__SS_extdata[aKey])
      delete browser.__SS_extdata[aKey];
    else
      throw (Components.returnCode = Cr.NS_ERROR_INVALID_ARG);
  },

  shouldRestore: function ss_shouldRestore() {
    return this._shouldRestore || (3 == Services.prefs.getIntPref("browser.startup.page"));
  },

  restoreLastSession: function ss_restoreLastSession(aBringToFront) {
    let self = this;
    function notifyObservers(aMessage) {
      self._clearCache();
      Services.obs.notifyObservers(null, "sessionstore-windows-restored", aMessage || "");
    }

    
    if (!this._sessionFileBackup.exists()) {
      notifyObservers("fail")
      return;
    }

    try {
      let channel = NetUtil.newChannel(this._sessionFileBackup);
      channel.contentType = "application/json";
      NetUtil.asyncFetch(channel, function(aStream, aResult) {
        if (!Components.isSuccessCode(aResult)) {
          Cu.reportError("SessionStore: Could not read from sessionstore.bak file");
          notifyObservers("fail");
          return;
        }

        
        let state = Cc["@mozilla.org/supports-string;1"].createInstance(Ci.nsISupportsString);
        state.data = NetUtil.readInputStreamToString(aStream, aStream.available(), { charset : "UTF-8" }) || "";
        aStream.close();

        Services.obs.notifyObservers(state, "sessionstore-state-read", "");

        let data = null;
        try {
          data = JSON.parse(state.data);
        } catch (ex) {
          Cu.reportError("SessionStore: Could not parse JSON: " + ex);
        }

        if (!data || data.windows.length == 0) {
          notifyObservers("fail");
          return;
        }

        let window = Services.wm.getMostRecentWindow("navigator:browser");

        if (typeof data.selectedWindow == "number") {
          this._selectedWindow = data.selectedWindow;
        }
        let windowIndex = this._selectedWindow - 1;
        let tabs = data.windows[windowIndex].tabs;
        let selected = data.windows[windowIndex].selected;

        let currentGroupId;
        try {
          currentGroupId = JSON.parse(data.windows[windowIndex].extData["tabview-groups"]).activeGroupId;
        } catch (ex) {  }

        
        this._orderedWindows = [];
        for (let i = 0; i < data.windows.length; i++) {
          let SSID;
          if (i != windowIndex) {
            SSID = "window" + gUUIDGenerator.generateUUID().toString();
            this._windows[SSID] = data.windows[i];
          } else {
            SSID = window.__SSID;
            this._windows[SSID].extData = data.windows[i].extData;
            this._windows[SSID]._closedTabs =
              this._windows[SSID]._closedTabs.concat(data.windows[i]._closedTabs);
          }
          this._orderedWindows.push(SSID);
        }

        if (selected > tabs.length) 
          selected = 1;

        for (let i=0; i<tabs.length; i++) {
          let tabData = tabs[i];
          let tabGroupId = (typeof currentGroupId == "number") ?
            JSON.parse(tabData.extData["tabview-tab"]).groupID : null;

          if (tabGroupId && tabGroupId != currentGroupId) {
            this._tabsFromOtherGroups.push(tabData);
            continue;
          }

          
          
          
          let bringToFront = (i + 1 <= selected) && aBringToFront;
          let tab = window.Browser.addTab(tabData.entries[tabData.index - 1].url, bringToFront);

          
          if (i + 1 == selected) {
            let json = {
              uri: tabData.entries[tabData.index - 1].url,
              flags: null,
              entries: tabData.entries,
              index: tabData.index
            };
            tab.browser.messageManager.sendAsyncMessage("WebNavigation:LoadURI", json);
          } else {
            
            tab.browser.__SS_data = tabData;
            tab.browser.__SS_restore = true;

            
            tab.chromeTab.updateTitle(tabData.entries[tabData.index - 1].title);
          }

          tab.browser.__SS_tabFormData = tabData
          tab.browser.__SS_extdata = tabData.extData;
        }

        notifyObservers();
      }.bind(this));
    } catch (ex) {
      Cu.reportError("SessionStore: Could not read from sessionstore.bak file: " + ex);
      notifyObservers("fail");
    }
  }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([SessionStore]);
