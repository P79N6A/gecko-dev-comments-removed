



































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

#ifdef MOZ_CRASH_REPORTER
XPCOMUtils.defineLazyServiceGetter(this, "CrashReporter",
  "@mozilla.org/xre/app-info;1", "nsICrashReporter");
#endif

XPCOMUtils.defineLazyGetter(this, "NetUtil", function() {
  Cu.import("resource://gre/modules/NetUtil.jsm");
  return NetUtil;
});





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
  _maxTabsUndo: 5,
  _shouldRestore: false,

  init: function ss_init() {
    
    this._sessionFile = Services.dirsvc.get("ProfD", Ci.nsILocalFile);
    this._sessionFileBackup = this._sessionFile.clone();
    this._sessionFile.append("sessionstore.js");
    this._sessionFileBackup.append("sessionstore.bak");

    this._loadState = STATE_STOPPED;

    try {
      if (this._sessionFileBackup.exists()) {
        this._shouldRestore = true;
        this._sessionFileBackup.remove(false);
      }
      if (this._sessionFile.exists()) {
        
        this._lastSessionTime = this._sessionFile.lastModifiedTime;
        let delta = Date.now() - this._lastSessionTime;
        let timeout = Services.prefs.getIntPref("browser.sessionstore.resume_from_crash_timeout");
        if (delta > (timeout * 60000))
          this._shouldRestore = false;

        this._sessionFile.copyTo(null, this._sessionFileBackup.leafName);
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

        
        if (this._sessionFileBackup.exists())
          this._sessionFileBackup.remove(false);

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
      case "TabOpen":
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
      case "TabSelect": {
        let browser = aEvent.originalTarget.linkedBrowser;
        this.onTabSelect(window, browser);
        break;
      }
    }
  },

  receiveMessage: function ss_receiveMessage(aMessage) {
    let window = aMessage.target.ownerDocument.defaultView;
    this.onTabLoad(window, aMessage.target, aMessage);
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

      
      if (!this._shouldRestore)
        Services.obs.notifyObservers(null, "sessionstore-windows-restored", "");
    }

    
    let tabs = aWindow.Browser.tabs;
    for (let i = 0; i < tabs.length; i++)
      this.onTabAdd(aWindow, tabs[i].browser, true);

    
    let tabContainer = aWindow.document.getElementById("tabs");
    tabContainer.addEventListener("TabOpen", this, true);
    tabContainer.addEventListener("TabClose", this, true);
    tabContainer.addEventListener("TabSelect", this, true);
  },

  onWindowClose: function ss_onWindowClose(aWindow) {
    
    if (!aWindow.__SSID || !this._windows[aWindow.__SSID])
      return;

    let tabContainer = aWindow.document.getElementById("tabs");
    tabContainer.removeEventListener("TabOpen", this, true);
    tabContainer.removeEventListener("TabClose", this, true);
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
    aBrowser.messageManager.addMessageListener("DOMContentLoaded", this);
    aBrowser.messageManager.addMessageListener("pageshow", this);

    if (!aNoNotification)
      this.saveStateDelayed();
    this._updateCrashReportURL(aWindow);
  },

  onTabRemove: function ss_onTabRemove(aWindow, aBrowser, aNoNotification) {
    aBrowser.messageManager.removeMessageListener("DOMContentLoaded", this);
    aBrowser.messageManager.removeMessageListener("pageshow", this);

    
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
      data.extData = aBrowser.__SS_extdata;

      this._windows[aWindow.__SSID].closedTabs.unshift(data);
      let length = this._windows[aWindow.__SSID].closedTabs.length;
      if (length > this._maxTabsUndo)
        this._windows[aWindow.__SSID].closedTabs.splice(this._maxTabsUndo, length - this._maxTabsUndo);
    }
  },

  onTabLoad: function ss_onTabLoad(aWindow, aBrowser, aMessage) {
    
    if (aBrowser.__SS_restore)
      return;

    
    if (!aBrowser.canGoBack && aBrowser.currentURI.spec == "about:blank")
      return;

    delete aBrowser.__SS_data;
    this._collectTabData(aBrowser);

    
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
      if (data.entries.length > 0)
        aBrowser.loadURI(data.entries[0].url, null, null);

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

  _collectTabData: function ss__collectTabData(aBrowser) {
    
    if (aBrowser.__SS_restore)
      return;

    let tabData = { entries: [{}] };
    tabData.entries[0] = { url: aBrowser.currentURI.spec, title: aBrowser.contentTitle };
    tabData.index = 1;
    tabData.attributes = { image: aBrowser.mIconURL };

    aBrowser.__SS_data = tabData;
  },

  _collectWindowData: function ss__collectWindowData(aWindow) {
    
    if (!aWindow.__SSID || !this._windows[aWindow.__SSID])
      return;

    let winData = this._windows[aWindow.__SSID];
    winData.tabs = [];

    let index = aWindow.Elements.browsers.selectedIndex;
    winData.selected = parseInt(index) + 1; 

    let tabs = aWindow.Browser.tabs;
    for (let i = 0; i < tabs.length; i++) {
      if (tabs[i].browser.__SS_data) {
        let browser = tabs[i].browser;
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
#ifdef MOZ_CRASH_REPORTER
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

    
    let tab = aWindow.Browser.addTab(closedTab.entries[0].url, true);

    
    tab.browser.__SS_extdata = closedTab.extData;

    

    return tab.chromeTab;
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
    let browser = aTab.linkedBrowser;
    let data = browser.__SS_extdata || {};
    return data[aKey] || "";
  },

  setTabValue: function ss_setTabValue(aTab, aKey, aStringValue) {
    let browser = aTab.linkedBrowser;
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
    return this._shouldRestore;
  },

  restoreLastSession: function ss_restoreLastSession(aBringToFront) {
    
    let dirService = Cc["@mozilla.org/file/directory_service;1"].getService(Ci.nsIProperties);
    let session = dirService.get("ProfD", Ci.nsILocalFile);
    session.append("sessionstore.bak");
    if (!session.exists())
      return;

    function notifyObservers() {
      Services.obs.notifyObservers(null, "sessionstore-windows-restored", "");
    }

    try {
      let channel = NetUtil.newChannel(session);
      channel.contentType = "application/json";
      NetUtil.asyncFetch(channel, function(aStream, aResult) {
        if (!Components.isSuccessCode(aResult)) {
          Cu.reportError("SessionStore: Could not read from sessionstore.bak file");
          notifyObservers();
          return;
        }

        
        let state = Cc["@mozilla.org/supports-string;1"].createInstance(Ci.nsISupportsString);
        state.data = NetUtil.readInputStreamToString(aStream, aStream.available()) || "";
        aStream.close();

        Services.obs.notifyObservers(state, "sessionstore-state-read", "");

        let data = null;
        try {
          data = JSON.parse(state.data);
        } catch (ex) {
          Cu.reportError("SessionStore: Could not parse JSON: " + ex);
        }

        if (!data || data.windows.length == 0) {
          notifyObservers();
          return;
        }

        let window = Services.wm.getMostRecentWindow("navigator:browser");
    
        let selected = data.windows[0].selected;
        let tabs = data.windows[0].tabs;
        for (let i=0; i<tabs.length; i++) {
          let tabData = tabs[i];
    
          
          let params = { getAttention: false, delayLoad: true };
          if (i + 1 == selected)
            params.delayLoad = false;
    
          
          
          
          let bringToFront = (i + 1 <= selected) && aBringToFront;
          let tab = window.Browser.addTab(tabData.entries[0].url, bringToFront, null, params);
    
          
          if (tabData.extData && params.delayLoad) {
              let canvas = tab.chromeTab.thumbnail;
              canvas.setAttribute("restored", "true");
    
              let image = new window.Image();
              image.onload = function() {
                if (canvas)
                  canvas.getContext("2d").drawImage(image, 0, 0);
              };
              image.src = tabData.extData.thumbnail;
          }
    
          tab.browser.__SS_data = tabData;
          tab.browser.__SS_extdata = tabData.extData;
          tab.browser.__SS_restore = params.delayLoad;
        }
    
        notifyObservers();
      });
    } catch (ex) {
      Cu.reportError("SessionStore: Could not read from sessionstore.bak file: " + ex);
      notifyObservers();
    }
  }
};

const NSGetFactory = XPCOMUtils.generateNSGetFactory([SessionStore]);
