



































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "gObserverService",
  "@mozilla.org/observer-service;1", "nsIObserverService");

XPCOMUtils.defineLazyServiceGetter(this, "gWindowMediator",
  "@mozilla.org/appshell/window-mediator;1", "nsIWindowMediator");

XPCOMUtils.defineLazyServiceGetter(this, "gPrefService",
  "@mozilla.org/preferences-service;1", "nsIPrefBranch2");

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
  classID: Components.ID("{90c3dfaf-4245-46d3-9bc1-1d8251ff8c01}"),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIDOMEventListener, Ci.nsIObserver, Ci.nsISupportsWeakReference]),

  _windows: {},
  _lastSaveTime: 0,
  _interval: 15000,
  
  init: function ss_init() {
    
    let dirService = Cc["@mozilla.org/file/directory_service;1"].getService(Ci.nsIProperties);
    this._sessionFile = dirService.get("ProfD", Ci.nsILocalFile);
    this._sessionFileBackup = this._sessionFile.clone();
    this._sessionFile.append("sessionstore.js");
    this._sessionFileBackup.append("sessionstore.bak");

    this._loadState = STATE_STOPPED;

    try {
      if (this._sessionFileBackup.exists())
        this._sessionFileBackup.remove(false);
      if (this._sessionFile.exists())
        this._sessionFile.copyTo(null, this._sessionFileBackup.leafName);
    } catch (ex) {
      Cu.reportError(ex);  
    }

    try {
      this._interval = gPrefService.getIntPref("sessionstore.interval");
    } catch (e) {}
  },
  
  observe: function ss_observe(aSubject, aTopic, aData) {
    let self = this;
    switch (aTopic) {
      case "app-startup": 
        gObserverService.addObserver(this, "final-ui-startup", true);
        gObserverService.addObserver(this, "domwindowopened", true);
        gObserverService.addObserver(this, "domwindowclosed", true);
        gObserverService.addObserver(this, "browser-lastwindow-close-granted", true);
        gObserverService.addObserver(this, "quit-application-requested", true);
        gObserverService.addObserver(this, "quit-application-granted", true);
        gObserverService.addObserver(this, "quit-application", true);
        break;
      case "final-ui-startup": 
        gObserverService.removeObserver(this, "final-ui-startup");
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
      case "quit-application":
        
        this._loadState = STATE_QUITTING;

        gObserverService.removeObserver(this, "domwindowopened");
        gObserverService.removeObserver(this, "domwindowclosed");
        gObserverService.removeObserver(this, "browser-lastwindow-close-granted");
        gObserverService.removeObserver(this, "quit-application-requested");
        gObserverService.removeObserver(this, "quit-application-granted");
        gObserverService.removeObserver(this, "quit-application");

        
        if (this._saveTimer) {
          this._saveTimer.cancel();
          this._saveTimer = null;
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
      case "load":
      case "pageshow":
        this.onTabLoad(window, aEvent.currentTarget, aEvent);
        break;
      case "TabOpen":
      case "TabClose":
        let browser = window.Browser.getTabFromChrome(aEvent.originalTarget).browser;
        if (aEvent.type == "TabOpen") {
          this.onTabAdd(window, browser);
        }
        else {
          this.onTabClose(window, aEvent.originalTarget);
          this.onTabRemove(window, browser);
        }
        break;
      case "TabSelect":
        this.onTabSelect(window);
        break;
    }
  },

  onWindowOpen: function ss_onWindowOpen(aWindow) {
    
    if (aWindow && aWindow.__SSID && this._windows[aWindow.__SSID])
      return;

    
    if (aWindow.document.documentElement.getAttribute("windowtype") != "navigator:browser" || this._loadState == STATE_QUITTING)
      return;

    
    aWindow.__SSID = "window" + Date.now();
    this._windows[aWindow.__SSID] = { tabs: [], selected: 0 };

    
    if (this._loadState == STATE_STOPPED) {
      this._loadState = STATE_RUNNING;
      this._lastSaveTime = Date.now();
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
    aBrowser.addEventListener("load", this, true);
    aBrowser.addEventListener("pageshow", this, true);
    
    if (!aNoNotification) {
      this.saveStateDelayed(aWindow);
    }

    this._updateCrashReportURL(aWindow);
  },

  onTabRemove: function ss_onTabRemove(aWindow, aBrowser, aNoNotification) {
    aBrowser.removeEventListener("load", this, true);
    aBrowser.removeEventListener("pageshow", this, true);
    
    delete aBrowser.__SS_data;
    
    if (!aNoNotification) {
      this.saveStateDelayed(aWindow);
    }
  },

  onTabClose: function ss_onTabClose(aWindow, aTab) {
  },

  onTabLoad: function ss_onTabLoad(aWindow, aBrowser, aEvent) { 
    
    
    if (aEvent.type != "load" && !aEvent.persisted) {
      return;
    }
    
    delete aBrowser.__SS_data;
    this._collectTabData(aBrowser);

    this.saveStateDelayed(aWindow);

    this._updateCrashReportURL(aWindow);
  },

  onTabSelect: function ss_onTabSelect(aWindow) {
    this._updateCrashReportURL(aWindow);
  },

  saveStateDelayed: function ss_saveStateDelayed() {
    if (!this._saveTimer) {
      
      let minimalDelay = this._lastSaveTime + this._interval - Date.now();
      
      
      let delay = Math.max(minimalDelay, 2000);
      if (delay > 0) {
        this._saveTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
        this._saveTimer.init(this, delay, Ci.nsITimer.TYPE_ONE_SHOT);
      }
      else {
        this.saveState();
      }
    }
  },

  saveState: function ss_saveState() {
    let self = this;
    this._forEachBrowserWindow(function(aWindow) {
      self._collectWindowData(aWindow);
    });

    let data = { windows: [] };
    let index;
    for (index in this._windows)
      data.windows.push(this._windows[index]);
    
    this._writeFile(this._sessionFile, JSON.stringify(data));

    this._lastSaveTime = Date.now();
  },
  
  _collectTabData: function ss__collectTabData(aBrowser) {
    let tabData = { url: aBrowser.currentURI.spec, title: aBrowser.contentTitle };
    aBrowser.__SS_data = tabData;
  },
  
  _collectWindowData: function ss__collectWindowData(aWindow) {
    
    if (!aWindow.__SSID || !this._windows[aWindow.__SSID])
      return;

    let winData = this._windows[aWindow.__SSID];
    winData.tabs = [];

    let tabs = aWindow.Browser.tabs;
    for (let i = 0; i < tabs.length; i++) {
      if (tabs[i].browser.__SS_data)
        winData.tabs.push(tabs[i].browser.__SS_data);
    }
  },

  _forEachBrowserWindow: function ss_forEachBrowserWindow(aFunc) {
    let windowsEnum = gWindowMediator.getEnumerator("navigator:browser");
    while (windowsEnum.hasMoreElements()) {
      let window = windowsEnum.getNext();
      if (window.__SSID && !window.closed)
        aFunc.call(this, window);
    }
  },

  _writeFile: function ss_writeFile(aFile, aData) {
    let stateString = Cc["@mozilla.org/supports-string;1"].createInstance(Ci.nsISupportsString);
    stateString.data = aData;
    gObserverService.notifyObservers(stateString, "sessionstore-state-write", "");

    
    if (!stateString.data)
      return;

    
    let ostream = Cc["@mozilla.org/network/safe-file-output-stream;1"].createInstance(Ci.nsIFileOutputStream);
    ostream.init(aFile, 0x02 | 0x08 | 0x20, 0600, 0);

    
    let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"].createInstance(Ci.nsIScriptableUnicodeConverter);
    converter.charset = "UTF-8";

    
    let istream = converter.convertToInputStream(aData);
    NetUtil.asyncCopy(istream, ostream, function(rc) {
      if (Components.isSuccessCode(rc)) {
        gObserverService.notifyObservers(null, "sessionstore-state-write-complete", "");
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
  }
};

const NSGetFactory = XPCOMUtils.generateNSGetFactory([SessionStore]);
