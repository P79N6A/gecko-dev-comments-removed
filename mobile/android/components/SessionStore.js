



const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

#ifdef MOZ_CRASHREPORTER
XPCOMUtils.defineLazyServiceGetter(this, "CrashReporter",
  "@mozilla.org/xre/app-info;1", "nsICrashReporter");
#endif

XPCOMUtils.defineLazyGetter(this, "NetUtil", function() {
  Cu.import("resource://gre/modules/NetUtil.jsm");
  return NetUtil;
});

function dump(a) {
  Cc["@mozilla.org/consoleservice;1"].getService(Ci.nsIConsoleService).logStringMessage(a);
}





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
  _lastSaveTime: 0,
  _lastSessionTime: 0,
  _interval: 10000,
  _maxTabsUndo: 1,
  _shouldRestore: false,

  init: function ss_init() {
    
    this._sessionFile = Services.dirsvc.get("ProfD", Ci.nsILocalFile);
    this._sessionFileBackup = this._sessionFile.clone();
    this._sessionCache = this._sessionFile.clone();
    this._sessionFile.append("sessionstore.js");
    this._sessionFileBackup.append("sessionstore.bak");
    this._sessionCache.append("sessionstoreCache");

    this._loadState = STATE_STOPPED;

    try {
      if (this._sessionFile.exists()) {
        
        
        this._lastSessionTime = this._sessionFile.lastModifiedTime;
        let delta = Date.now() - this._lastSessionTime;
        let timeout = Services.prefs.getIntPref("browser.sessionstore.resume_from_crash_timeout");
        
        this._shouldRestore = (delta <= (timeout * 60000));
        this._sessionFile.clone().moveTo(null, this._sessionFileBackup.leafName);
      }

      if (!this._sessionCache.exists() || !this._sessionCache.isDirectory())
        this._sessionCache.create(Ci.nsIFile.DIRECTORY_TYPE, 0700);
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
      let tabs = aWindow.BrowserApp.tabs;
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
        break;
      case "final-ui-startup":
        observerService.removeObserver(this, "final-ui-startup");
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

        
        
        
        if (this._sessionFile.exists())
          this._sessionFile.moveTo(null, this._sessionFileBackup.leafName);

        observerService.removeObserver(this, "domwindowopened");
        observerService.removeObserver(this, "domwindowclosed");
        observerService.removeObserver(this, "browser-lastwindow-close-granted");
        observerService.removeObserver(this, "quit-application-requested");
        observerService.removeObserver(this, "quit-application-granted");
        observerService.removeObserver(this, "quit-application");

        
        if (this._saveTimer) {
          this._saveTimer.cancel();
          this._saveTimer = null;
          this.saveState();
        }
        break;
      case "browser:purge-session-history": 
        this._clearDisk();

        
        
        
        if (this._loadState == STATE_QUITTING)
          return;

        
        for (let [ssid, win] in Iterator(this._windows))
          win.closedTabs = [];

        if (this._loadState == STATE_RUNNING) {
          
          this.saveStateNow();
        }

        Services.obs.notifyObservers(null, "sessionstore-state-purge-complete", "");
        break;
      case "timer-callback":
        
        this._saveTimer = null;
        this.saveState();
        break;
    }
  },

  handleEvent: function ss_handleEvent(aEvent) {
    let window = aEvent.currentTarget.ownerDocument.defaultView;
    switch (aEvent.type) {
      case "TabOpen": {
        let browser = aEvent.target;
        this.onTabAdd(window, browser);
        break;
      }
      case "TabClose": {
        let browser = aEvent.target;
        this.onTabClose(window, browser);
        this.onTabRemove(window, browser);
        break;
      }
      case "TabSelect": {
        let browser = aEvent.target;
        this.onTabSelect(window, browser);
        break;
      }
      case "pageshow": {
        let browser = aEvent.currentTarget;
        this.onTabLoad(window, browser, aEvent.persisted);
        break;
      }
    }
  },

  onWindowOpen: function ss_onWindowOpen(aWindow) {
    
    if (aWindow && aWindow.__SSID && this._windows[aWindow.__SSID])
      return;

    
    if (aWindow.document.documentElement.getAttribute("windowtype") != "navigator:browser" || this._loadState == STATE_QUITTING)
      return;

    
    aWindow.__SSID = "window" + Date.now();
    this._windows[aWindow.__SSID] = { tabs: [], selected: 0, closedTabs: [] };

    
    if (this._loadState == STATE_STOPPED) {
      this._loadState = STATE_RUNNING;
      this._lastSaveTime = Date.now();

      
      if (!this._shouldRestore) {
        this._clearCache();
        Services.obs.notifyObservers(null, "sessionstore-windows-restored", "");
      }
    }

    
    let tabs = aWindow.BrowserApp.tabs;
    for (let i = 0; i < tabs.length; i++)
      this.onTabAdd(aWindow, tabs[i].browser, true);

    
    let browsers = aWindow.document.getElementById("browsers");
    browsers.addEventListener("TabOpen", this, true);
    browsers.addEventListener("TabClose", this, true);
    browsers.addEventListener("TabSelect", this, true);
  },

  onWindowClose: function ss_onWindowClose(aWindow) {
    
    if (!aWindow.__SSID || !this._windows[aWindow.__SSID])
      return;

    let browsers = aWindow.document.getElementById("browsers");
    browsers.removeEventListener("TabOpen", this, true);
    browsers.removeEventListener("TabClose", this, true);
    browsers.removeEventListener("TabSelect", this, true);

    if (this._loadState == STATE_RUNNING) {
      
      this._collectWindowData(aWindow);

      
      delete this._windows[aWindow.__SSID];

      
      this.saveStateDelayed();
    }

    let tabs = aWindow.BrowserApp.tabs;
    for (let i = 0; i < tabs.length; i++)
      this.onTabRemove(aWindow, tabs[i].browser, true);

    delete aWindow.__SSID;
  },

  onTabAdd: function ss_onTabAdd(aWindow, aBrowser, aNoNotification) {
    aBrowser.addEventListener("pageshow", this, true);
    if (!aNoNotification)
      this.saveStateDelayed();
    this._updateCrashReportURL(aWindow);
  },

  onTabRemove: function ss_onTabRemove(aWindow, aBrowser, aNoNotification) {
    aBrowser.removeEventListener("pageshow", this, true);

    
    if (aBrowser.__SS_restore)
      return;

    delete aBrowser.__SS_data;

    if (!aNoNotification)
      this.saveStateDelayed();
  },

  onTabClose: function ss_onTabClose(aWindow, aBrowser) {
    if (this._maxTabsUndo == 0)
      return;

    if (aWindow.BrowserApp.tabs.length > 0) {
      
      
      let data = aBrowser.__SS_data;
      data.extData = aBrowser.__SS_extdata;

      this._windows[aWindow.__SSID].closedTabs.unshift(data);
      let length = this._windows[aWindow.__SSID].closedTabs.length;
      if (length > this._maxTabsUndo)
        this._windows[aWindow.__SSID].closedTabs.splice(this._maxTabsUndo, length - this._maxTabsUndo);
    }
  },

  onTabLoad: function ss_onTabLoad(aWindow, aBrowser, aPersisted) {
    
    if (aBrowser.__SS_restore)
      return;

    
    if (!aBrowser.canGoBack && aBrowser.currentURI.spec == "about:blank")
      return;

    let history = aBrowser.sessionHistory;

    if (aPersisted && aBrowser.__SS_data) {
      
      aBrowser.__SS_data.index = history.index + 1;
      this.saveStateDelayed();
    } else {
      
      let entries = [];
      for (let i = 0; i < history.count; i++) {
        let entry = this._serializeHistoryEntry(history.getEntryAtIndex(i, false));
        entries.push(entry);
      }
      let index = history.index + 1;
      let data = { entries: entries, index: index };

      delete aBrowser.__SS_data;
      this._collectTabData(aWindow, aBrowser, data);
      this.saveStateNow();
    }

    this._updateCrashReportURL(aWindow);
  },

  onTabSelect: function ss_onTabSelect(aWindow, aBrowser) {
    if (this._loadState != STATE_RUNNING)
      return;

    let browsers = aWindow.document.getElementById("browsers");
    let index = browsers.selectedIndex;
    this._windows[aWindow.__SSID].selected = parseInt(index) + 1; 

    
    if (aBrowser.__SS_restore) {
      let data = aBrowser.__SS_data;
      if (data.entries.length > 0)
        this._restoreHistory(data, aBrowser.sessionHistory);

      delete aBrowser.__SS_restore;
    }

    this.saveStateDelayed();
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
    this._writeFile(this._sessionFile, JSON.stringify(data));

    this._lastSaveTime = Date.now();
  },

  _getCurrentState: function ss_getCurrentState() {
    let self = this;
    this._forEachBrowserWindow(function(aWindow) {
      self._collectWindowData(aWindow);
    });

    let data = { windows: [] };
    let index;
    for (index in this._windows)
      data.windows.push(this._windows[index]);
    return data;
  },

  _collectTabData: function ss__collectTabData(aWindow, aBrowser, aHistory) {
    
    if (aBrowser.__SS_restore)
      return;

    let aHistory = aHistory || { entries: [{ url: aBrowser.currentURI.spec, title: aBrowser.contentTitle }], index: 1 };

    let tabData = {};
    tabData.entries = aHistory.entries;
    tabData.index = aHistory.index;
    tabData.attributes = { image: aBrowser.mIconURL };
    tabData.desktopMode = aWindow.BrowserApp.getTabForBrowser(aBrowser).desktopMode;

    aBrowser.__SS_data = tabData;
  },

  _collectWindowData: function ss__collectWindowData(aWindow) {
    
    if (!aWindow.__SSID || !this._windows[aWindow.__SSID])
      return;

    let winData = this._windows[aWindow.__SSID];
    winData.tabs = [];

    let browsers = aWindow.document.getElementById("browsers");
    let index = browsers.selectedIndex;
    winData.selected = parseInt(index) + 1; 

    let tabs = aWindow.BrowserApp.tabs;
    for (let i = 0; i < tabs.length; i++) {
      let browser = tabs[i].browser;
      if (browser.__SS_data) {
        let tabData = browser.__SS_data;
        if (browser.__SS_extdata)
          tabData.extData = browser.__SS_extdata;
        winData.tabs.push(tabData);
      }
    }
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
        Services.obs.notifyObservers(null, "sessionstore-state-write-complete", "");
      }
    });
  },

  _updateCrashReportURL: function ss_updateCrashReportURL(aWindow) {
#ifdef MOZ_CRASHREPORTER
    if (!aWindow.BrowserApp.selectedBrowser)
      return;

    try {
      let currentURI = aWindow.BrowserApp.selectedBrowser.currentURI.clone();
      
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

  _serializeHistoryEntry: function _serializeHistoryEntry(aEntry) {
    let entry = { url: aEntry.URI.spec };

    if (aEntry.title && aEntry.title != entry.url)
      entry.title = aEntry.title;

    if (!(aEntry instanceof Ci.nsISHEntry))
      return entry;

    let cacheKey = aEntry.cacheKey;
    if (cacheKey && cacheKey instanceof Ci.nsISupportsPRUint32 && cacheKey.data != 0)
      entry.cacheKey = cacheKey.data;

    entry.ID = aEntry.ID;
    entry.docshellID = aEntry.docshellID;

    if (aEntry.referrerURI)
      entry.referrer = aEntry.referrerURI.spec;

    if (aEntry.contentType)
      entry.contentType = aEntry.contentType;

    let x = {}, y = {};
    aEntry.getScrollPosition(x, y);
    if (x.value != 0 || y.value != 0)
      entry.scroll = x.value + "," + y.value;

    if (aEntry.owner) {
      try {
        let binaryStream = Cc["@mozilla.org/binaryoutputstream;1"].createInstance(Ci.nsIObjectOutputStream);
        let pipe = Cc["@mozilla.org/pipe;1"].createInstance(Ci.nsIPipe);
        pipe.init(false, false, 0, 0xffffffff, null);
        binaryStream.setOutputStream(pipe.outputStream);
        binaryStream.writeCompoundObject(aEntry.owner, Ci.nsISupports, true);
        binaryStream.close();

        
        let scriptableStream = Cc["@mozilla.org/binaryinputstream;1"].createInstance(Ci.nsIBinaryInputStream);
        scriptableStream.setInputStream(pipe.inputStream);
        let ownerBytes = scriptableStream.readByteArray(scriptableStream.available());
        
        
        
        entry.owner_b64 = btoa(String.fromCharCode.apply(null, ownerBytes));
      } catch (e) { dump(e); }
    }

    entry.docIdentifier = aEntry.BFCacheEntry.ID;

    if (aEntry.stateData != null) {
      entry.structuredCloneState = aEntry.stateData.getDataAsBase64();
      entry.structuredCloneVersion = aEntry.stateData.formatVersion;
    }

    if (!(aEntry instanceof Ci.nsISHContainer))
      return entry;

    if (aEntry.childCount > 0) {
      entry.children = [];
      for (let i = 0; i < aEntry.childCount; i++) {
        let child = aEntry.GetChildAt(i);
        if (child)
          entry.children.push(this._serializeHistoryEntry(child));
        else 
          entry.children.push({ url: "about:blank" });

        
        if (/^wyciwyg:\/\//.test(entry.children[i].url)) {
          delete entry.children;
          break;
        }
      }
    }

    return entry;
  },

  _deserializeHistoryEntry: function _deserializeHistoryEntry(aEntry, aIdMap, aDocIdentMap) {
    let shEntry = Cc["@mozilla.org/browser/session-history-entry;1"].createInstance(Ci.nsISHEntry);

    shEntry.setURI(Services.io.newURI(aEntry.url, null, null));
    shEntry.setTitle(aEntry.title || aEntry.url);
    if (aEntry.subframe)
      shEntry.setIsSubFrame(aEntry.subframe || false);
    shEntry.loadType = Ci.nsIDocShellLoadInfo.loadHistory;
    if (aEntry.contentType)
      shEntry.contentType = aEntry.contentType;
    if (aEntry.referrer)
      shEntry.referrerURI = Services.io.newURI(aEntry.referrer, null, null);

    if (aEntry.cacheKey) {
      let cacheKey = Cc["@mozilla.org/supports-PRUint32;1"].createInstance(Ci.nsISupportsPRUint32);
      cacheKey.data = aEntry.cacheKey;
      shEntry.cacheKey = cacheKey;
    }

    if (aEntry.ID) {
      
      
      let id = aIdMap[aEntry.ID] || 0;
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

      shEntry.stateData.initFromBase64(aEntry.structuredCloneState, aEntry.structuredCloneVersion);
    }

    if (aEntry.scroll) {
      let scrollPos = aEntry.scroll.split(",");
      scrollPos = [parseInt(scrollPos[0]) || 0, parseInt(scrollPos[1]) || 0];
      shEntry.setScrollPosition(scrollPos[0], scrollPos[1]);
    }

    let childDocIdents = {};
    if (aEntry.docIdentifier) {
      
      
      
      
      let matchingEntry = aDocIdentMap[aEntry.docIdentifier];
      if (!matchingEntry) {
        matchingEntry = {shEntry: shEntry, childDocIdents: childDocIdents};
        aDocIdentMap[aEntry.docIdentifier] = matchingEntry;
      } else {
        shEntry.adoptBFCacheEntry(matchingEntry.shEntry);
        childDocIdents = matchingEntry.childDocIdents;
      }
    }

    if (aEntry.owner_b64) {
      let ownerInput = Cc["@mozilla.org/io/string-input-stream;1"].createInstance(Ci.nsIStringInputStream);
      let binaryData = atob(aEntry.owner_b64);
      ownerInput.setData(binaryData, binaryData.length);
      let binaryStream = Cc["@mozilla.org/binaryinputstream;1"].createInstance(Ci.nsIObjectInputStream);
      binaryStream.setInputStream(ownerInput);
      try { 
        shEntry.owner = binaryStream.readObject(true);
      } catch (ex) { dump(ex); }
    }

    if (aEntry.children && shEntry instanceof Ci.nsISHContainer) {
      for (let i = 0; i < aEntry.children.length; i++) {
        if (!aEntry.children[i].url)
          continue;

        
        
        
        
        
        
        
        
        
        
        
        
        
        

        shEntry.AddChild(this._deserializeHistoryEntry(aEntry.children[i], aIdMap, childDocIdents), i);
      }
    }

    return shEntry;
  },

  _restoreHistory: function _restoreHistory(aTabData, aHistory) {
    if (aHistory.count > 0)
      aHistory.PurgeHistory(aHistory.count);
    aHistory.QueryInterface(Ci.nsISHistoryInternal);

    
    
    let idMap = { used: {} };
    let docIdentMap = {};

    for (let i = 0; i < aTabData.entries.length; i++) {
      if (!aTabData.entries[i].url)
        continue;
      aHistory.addEntry(this._deserializeHistoryEntry(aTabData.entries[i], idMap, docIdentMap), true);
    }

    
    
    let activeIndex = (aTabData.index || aTabData.entries.length) - 1;
    aHistory.getEntryAtIndex(activeIndex, true);
    aHistory.QueryInterface(Ci.nsISHistory).reloadCurrentEntry();
  },

  getBrowserState: function ss_getBrowserState() {
    let data = this._getCurrentState();
    return JSON.stringify(data);
  },

  getClosedTabCount: function ss_getClosedTabCount(aWindow) {
    if (!aWindow || !aWindow.__SSID || !this._windows[aWindow.__SSID])
      return 0; 

    return this._windows[aWindow.__SSID].closedTabs.length;
  },

  getClosedTabData: function ss_getClosedTabData(aWindow) {
    if (!aWindow.__SSID)
      throw (Components.returnCode = Cr.NS_ERROR_INVALID_ARG);

    return JSON.stringify(this._windows[aWindow.__SSID].closedTabs);
  },

  undoCloseTab: function ss_undoCloseTab(aWindow, aIndex) {
    if (!aWindow.__SSID)
      throw (Components.returnCode = Cr.NS_ERROR_INVALID_ARG);

    let closedTabs = this._windows[aWindow.__SSID].closedTabs;
    if (!closedTabs)
      return null;

    
    aIndex = aIndex || 0;
    if (!(aIndex in closedTabs))
      throw (Components.returnCode = Cr.NS_ERROR_INVALID_ARG);

    
    let closedTab = closedTabs.splice(aIndex, 1).shift();

    
    let params = { selected: true };
    let tab = aWindow.BrowserApp.addTab(closedTab.entries[closedTab.index - 1].url, params);
    this._restoreHistory(closedTab, tab.browser.sessionHistory);

    
    tab.browser.__SS_extdata = closedTab.extData;

    return tab.browser;
  },

  forgetClosedTab: function ss_forgetClosedTab(aWindow, aIndex) {
    if (!aWindow.__SSID)
      throw (Components.returnCode = Cr.NS_ERROR_INVALID_ARG);

    let closedTabs = this._windows[aWindow.__SSID].closedTabs;

    
    aIndex = aIndex || 0;
    if (!(aIndex in closedTabs))
      throw (Components.returnCode = Cr.NS_ERROR_INVALID_ARG);

    
    closedTabs.splice(aIndex, 1);
  },

  getTabValue: function ss_getTabValue(aTab, aKey) {
    let browser = aTab.browser;
    let data = browser.__SS_extdata || {};
    return data[aKey] || "";
  },

  setTabValue: function ss_setTabValue(aTab, aKey, aStringValue) {
    let browser = aTab.browser;

    
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
    let browser = aTab.browser;
    if (browser.__SS_extdata && browser.__SS_extdata[aKey])
      delete browser.__SS_extdata[aKey];
    else
      throw (Components.returnCode = Cr.NS_ERROR_INVALID_ARG);
  },

  shouldRestore: function ss_shouldRestore() {
    return this._shouldRestore;
  },

  restoreLastSession: function ss_restoreLastSession(aBringToFront, aForceRestore) {
    let self = this;
    function notifyObservers(aMessage) {
      self._clearCache();
      Services.obs.notifyObservers(null, "sessionstore-windows-restored", aMessage || "");
    }

    if (!aForceRestore) {
      let maxCrashes = Services.prefs.getIntPref("browser.sessionstore.max_resumed_crashes");
      let recentCrashes = Services.prefs.getIntPref("browser.sessionstore.recent_crashes") + 1;
      Services.prefs.setIntPref("browser.sessionstore.recent_crashes", recentCrashes);
      Services.prefs.savePrefFile(null);

      if (recentCrashes > maxCrashes) {
        notifyObservers("fail");
        return;
      }
    }

    
    if (!this._sessionFileBackup.exists()) {
      notifyObservers("fail");
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

        
        if (!data || data.windows.length == 0 || !data.windows[0].tabs || data.windows[0].tabs.length == 0) {
          notifyObservers("fail");
          return;
        }

        let window = Services.wm.getMostRecentWindow("navigator:browser");

        let tabs = data.windows[0].tabs;
        let selected = data.windows[0].selected;
        if (selected > tabs.length) 
          selected = 1;

        for (let i=0; i<tabs.length; i++) {
          let tabData = tabs[i];
          let isSelected = (i + 1 == selected) && aBringToFront;
          let entry = tabData.entries[tabData.index - 1];

          
          let params = {
            selected: isSelected,
            delayLoad: true,
            title: entry.title,
            desktopMode: tabData.desktopMode == true
          };
          let tab = window.BrowserApp.addTab(entry.url, params);

          if (isSelected) {
            self._restoreHistory(tabData, tab.browser.sessionHistory);
          } else {
            
            tab.browser.__SS_data = tabData;
            tab.browser.__SS_restore = true;
          }

          tab.browser.__SS_extdata = tabData.extData;
        }

        notifyObservers();
      });
    } catch (ex) {
      Cu.reportError("SessionStore: Could not read from sessionstore.bak file: " + ex);
      notifyObservers("fail");
    }
  }
};

const NSGetFactory = XPCOMUtils.generateNSGetFactory([SessionStore]);
