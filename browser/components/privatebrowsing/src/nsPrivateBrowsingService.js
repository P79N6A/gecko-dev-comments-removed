# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http:

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");

#ifndef XP_WIN
#define BROKEN_WM_Z_ORDER
#endif




const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

const STATE_IDLE = 0;
const STATE_TRANSITION_STARTED = 1;
const STATE_WAITING_FOR_RESTORE = 2;
const STATE_RESTORE_FINISHED = 3;




function PrivateBrowsingService() {
  this._obs = Cc["@mozilla.org/observer-service;1"].
              getService(Ci.nsIObserverService);
  this._obs.addObserver(this, "profile-after-change", true);
  this._obs.addObserver(this, "quit-application-granted", true);
  this._obs.addObserver(this, "private-browsing", true);
  this._obs.addObserver(this, "command-line-startup", true);
  this._obs.addObserver(this, "sessionstore-browser-state-restored", true);

  
  this._windowsToClose = [];
}

PrivateBrowsingService.prototype = {
  
  get _prefs() {
    let prefs = Cc["@mozilla.org/preferences-service;1"].
                getService(Ci.nsIPrefBranch);
    this.__defineGetter__("_prefs", function() prefs);
    return this._prefs;
  },

  
  _inPrivateBrowsing: false,

  
  _savedBrowserState: null,

  
  _quitting: false,

  
  _saveSession: true,

  
  _currentStatus: STATE_IDLE,

  
  _autoStarted: false,

  
  _viewSrcURLs: [],

  
  _lastChangedByCommandLine: false,

  
  _enterTimestamps: {},
  _exitTimestamps: {},

  
  classID: Components.ID("{c31f4883-839b-45f6-82ad-a6a9bc5ad599}"),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIPrivateBrowsingService,
                                         Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference,
                                         Ci.nsICommandLineHandler]),

  _unload: function PBS__destroy() {
    
    this._quitting = true;
    if (this._inPrivateBrowsing)
      this.privateBrowsingEnabled = false;
  },

  _setPerWindowPBFlag: function PBS__setPerWindowPBFlag(aWindow, aFlag) {
    aWindow.QueryInterface(Ci.nsIInterfaceRequestor)
           .getInterface(Ci.nsIWebNavigation)
           .QueryInterface(Ci.nsIDocShellTreeItem)
           .treeOwner
           .QueryInterface(Ci.nsIInterfaceRequestor)
           .getInterface(Ci.nsIXULWindow)
           .docShell.QueryInterface(Ci.nsILoadContext)
           .usePrivateBrowsing = aFlag;
  },

  _adjustPBFlagOnExistingWindows: function PBS__adjustPBFlagOnExistingWindows() {
    var windowsEnum = Services.wm.getEnumerator(null);
    while (windowsEnum.hasMoreElements()) {
      var window = windowsEnum.getNext();
      this._setPerWindowPBFlag(window, this._inPrivateBrowsing);
    }
  },

  _onBeforePrivateBrowsingModeChange: function PBS__onBeforePrivateBrowsingModeChange() {
    
    if (this._inPrivateBrowsing) {
      this._adjustPBFlagOnExistingWindows();
    }

    
    if (!this._autoStarted) {
      let ss = Cc["@mozilla.org/browser/sessionstore;1"].
               getService(Ci.nsISessionStore);
      let blankState = JSON.stringify({
        "windows": [{
          "tabs": [{
            "entries": [{
              "url": "about:blank"
            }]
          }],
          "_closedTabs": []
        }]
      });

      if (this._inPrivateBrowsing) {
        
        if (this._saveSession && !this._savedBrowserState) {
          if (this._getBrowserWindow())
            this._savedBrowserState = ss.getBrowserState();
          else 
            this._savedBrowserState = blankState;
        }
      }

      this._closePageInfoWindows();

      
      let viewSrcWindowsEnum = Services.wm.getEnumerator("navigator:view-source");
      while (viewSrcWindowsEnum.hasMoreElements()) {
        let win = viewSrcWindowsEnum.getNext();
        if (this._inPrivateBrowsing) {
          let plainURL = win.gBrowser.currentURI.spec;
          if (plainURL.indexOf("view-source:") == 0) {
            plainURL = plainURL.substr(12);
            this._viewSrcURLs.push(plainURL);
          }
        }
        win.close();
      }

      if (!this._quitting && this._saveSession) {
        let browserWindow = this._getBrowserWindow();

        
        
        if (browserWindow) {
          
          ss.setBrowserState(blankState);

          
          
          
          browserWindow = this._getBrowserWindow();
          let browser = browserWindow.gBrowser;

          
          
          browser.addTab();
          browser.getBrowserForTab(browser.tabContainer.firstChild).stop();
          browser.removeTab(browser.tabContainer.firstChild);
          browserWindow.getInterface(Ci.nsIWebNavigation)
                       .QueryInterface(Ci.nsIDocShellTreeItem)
                       .treeOwner
                       .QueryInterface(Ci.nsIInterfaceRequestor)
                       .getInterface(Ci.nsIXULWindow)
                       .docShell.contentViewer.resetCloseWindow();
        }
      }
    }
    else
      this._saveSession = false;

    
    if (!this._inPrivateBrowsing) {
      this._adjustPBFlagOnExistingWindows();
    }
  },

  _onAfterPrivateBrowsingModeChange: function PBS__onAfterPrivateBrowsingModeChange() {
    
    
    if (!this._autoStarted && this._saveSession) {
      let ss = Cc["@mozilla.org/browser/sessionstore;1"].
               getService(Ci.nsISessionStore);
      
      
      if (!this._inPrivateBrowsing) {
        this._currentStatus = STATE_WAITING_FOR_RESTORE;
        if (!this._getBrowserWindow()) {
          ss.init(null);
        }
        ss.setBrowserState(this._savedBrowserState);
        this._savedBrowserState = null;

        this._closePageInfoWindows();

        
        let windowWatcher = Cc["@mozilla.org/embedcomp/window-watcher;1"].
                            getService(Ci.nsIWindowWatcher);
        this._viewSrcURLs.forEach(function(uri) {
          let args = Cc["@mozilla.org/supports-array;1"].
                     createInstance(Ci.nsISupportsArray);
          let str = Cc["@mozilla.org/supports-string;1"].
                    createInstance(Ci.nsISupportsString);
          str.data = uri;
          args.AppendElement(str);
          args.AppendElement(null); 
          args.AppendElement(null); 
          args.AppendElement(null); 
          let forcedCharset = Cc["@mozilla.org/supports-PRBool;1"].
                              createInstance(Ci.nsISupportsPRBool);
          forcedCharset.data = false;
          args.AppendElement(forcedCharset);
          windowWatcher.openWindow(null, "chrome://global/content/viewSource.xul",
            "_blank", "all,dialog=no", args);
        });
        this._viewSrcURLs = [];
      }
      else {
        
        
        let privateBrowsingState = {
          "windows": [{
            "tabs": [{
              "entries": [{
                "url": "about:privatebrowsing"
              }]
            }],
            "_closedTabs": []
          }]
        };
        
        this._currentStatus = STATE_WAITING_FOR_RESTORE;
        if (!this._getBrowserWindow()) {
          ss.init(null);
        }
        ss.setBrowserState(JSON.stringify(privateBrowsingState));
      }
    }
  },

  _notifyIfTransitionComplete: function PBS__notifyIfTransitionComplete() {
    switch (this._currentStatus) {
      case STATE_TRANSITION_STARTED:
        
      case STATE_RESTORE_FINISHED:
        
        this._currentStatus = STATE_IDLE;
        this._obs.notifyObservers(null, "private-browsing-transition-complete", "");
        this._recordTransitionTime("completed");
        break;
      case STATE_WAITING_FOR_RESTORE:
        
        break;
      case STATE_IDLE:
        
        break;
      default:
        
        Cu.reportError("Unexpected private browsing status reached: " +
                       this._currentStatus);
        break;
    }
  },

  _recordTransitionTime: function PBS__recordTransitionTime(aPhase) {
    
    
    
    
    if (this._inPrivateBrowsing) {
      this._enterTimestamps[aPhase] = Date.now();
    } else {
      if (this._quitting) {
        
        
        return;
      }
      this._exitTimestamps[aPhase] = Date.now();
      if (aPhase == "completed") {
        
        
        this._reportTelemetry();
      }
    }
  },

  _reportTelemetry: function PBS__reportTelemetry() {
    function reportTelemetryEntry(aHistogramId, aValue) {
      try {
        Services.telemetry.getHistogramById(aHistogramId).add(aValue);
      } catch (ex) {
        Cu.reportError(ex);
      }
    }

    reportTelemetryEntry(
          "PRIVATE_BROWSING_TRANSITION_ENTER_PREPARATION_MS",
          this._enterTimestamps.prepared - this._enterTimestamps.started);
    reportTelemetryEntry(
          "PRIVATE_BROWSING_TRANSITION_ENTER_TOTAL_MS",
          this._enterTimestamps.completed - this._enterTimestamps.started);
    reportTelemetryEntry(
          "PRIVATE_BROWSING_TRANSITION_EXIT_PREPARATION_MS",
          this._exitTimestamps.prepared - this._exitTimestamps.started);
    reportTelemetryEntry(
          "PRIVATE_BROWSING_TRANSITION_EXIT_TOTAL_MS",
          this._exitTimestamps.completed - this._exitTimestamps.started);
  },

  _canEnterPrivateBrowsingMode: function PBS__canEnterPrivateBrowsingMode() {
    let cancelEnter = Cc["@mozilla.org/supports-PRBool;1"].
                      createInstance(Ci.nsISupportsPRBool);
    cancelEnter.data = false;
    this._obs.notifyObservers(cancelEnter, "private-browsing-cancel-vote", "enter");
    return !cancelEnter.data;
  },

  _canLeavePrivateBrowsingMode: function PBS__canLeavePrivateBrowsingMode() {
    let cancelLeave = Cc["@mozilla.org/supports-PRBool;1"].
                      createInstance(Ci.nsISupportsPRBool);
    cancelLeave.data = false;
    this._obs.notifyObservers(cancelLeave, "private-browsing-cancel-vote", "exit");
    return !cancelLeave.data;
  },

  _getBrowserWindow: function PBS__getBrowserWindow() {
    var wm = Cc["@mozilla.org/appshell/window-mediator;1"].
             getService(Ci.nsIWindowMediator);

    var win = wm.getMostRecentWindow("navigator:browser");

    

    if (!win)
      return null;
    if (!win.closed)
      return win;

#ifdef BROKEN_WM_Z_ORDER
    win = null;
    var windowsEnum = wm.getEnumerator("navigator:browser");
    
    while (windowsEnum.hasMoreElements()) {
      let nextWin = windowsEnum.getNext();
      if (!nextWin.closed)
        win = nextWin;
    }
    return win;
#else
    var windowsEnum = wm.getZOrderDOMWindowEnumerator("navigator:browser", true);
    while (windowsEnum.hasMoreElements()) {
      win = windowsEnum.getNext();
      if (!win.closed)
        return win;
    }
    return null;
#endif
  },

  _ensureCanCloseWindows: function PBS__ensureCanCloseWindows() {
    
    this._saveSession = true;
    try {
      if (this._prefs.getBoolPref("browser.privatebrowsing.keep_current_session")) {
        this._saveSession = false;
        return;
      }
    } catch (ex) {}

    let windowMediator = Cc["@mozilla.org/appshell/window-mediator;1"].
                         getService(Ci.nsIWindowMediator);
    let windowsEnum = windowMediator.getEnumerator("navigator:browser");

    while (windowsEnum.hasMoreElements()) {
      let win = windowsEnum.getNext();
      if (win.closed)
        continue;
      let xulWin = win.QueryInterface(Ci.nsIInterfaceRequestor).
                   getInterface(Ci.nsIWebNavigation).
                   QueryInterface(Ci.nsIDocShellTreeItem).
                   treeOwner.QueryInterface(Ci.nsIInterfaceRequestor).
                   getInterface(Ci.nsIXULWindow);
      if (xulWin.docShell.contentViewer.permitUnload(true))
        this._windowsToClose.push(xulWin);
      else
        throw Cr.NS_ERROR_ABORT;
    }
  },

  _closePageInfoWindows: function PBS__closePageInfoWindows() {
    let pageInfoEnum = Cc["@mozilla.org/appshell/window-mediator;1"].
                       getService(Ci.nsIWindowMediator).
                       getEnumerator("Browser:page-info");
    while (pageInfoEnum.hasMoreElements()) {
      let win = pageInfoEnum.getNext();
      win.close();
    }
  },

  

  observe: function PBS_observe(aSubject, aTopic, aData) {
    switch (aTopic) {
      case "profile-after-change":
        
        
        
        
        if (!this._autoStarted) {
          this._autoStarted = this._prefs.getBoolPref("browser.privatebrowsing.autostart");
          if (this._autoStarted)
            this.privateBrowsingEnabled = true;
        }
        this._obs.removeObserver(this, "profile-after-change");
        break;
      case "quit-application-granted":
        this._unload();
        break;
      case "private-browsing":
        
        let sdr = Cc["@mozilla.org/security/sdr;1"].
                  getService(Ci.nsISecretDecoderRing);
        sdr.logoutAndTeardown();
    
        try {
          this._prefs.deleteBranch("geo.wifi.access_token.");
        } catch (ex) {}

        if (!this._inPrivateBrowsing) {
          
          let consoleService = Cc["@mozilla.org/consoleservice;1"].
                               getService(Ci.nsIConsoleService);
          consoleService.logStringMessage(null); 
          consoleService.reset();
        }
        break;
      case "command-line-startup":
        this._obs.removeObserver(this, "command-line-startup");
        aSubject.QueryInterface(Ci.nsICommandLine);
        if (aSubject.findFlag("private", false) >= 0) {
          
          if (this._autoStarted)
            aSubject.handleFlag("private", false);

          Services.prefs.setBoolPref("browser.privatebrowsing.autostart", true);
          this.privateBrowsingEnabled = true;
          this._autoStarted = true;
          this._lastChangedByCommandLine = true;
        }
        else if (aSubject.findFlag("private-toggle", false) >= 0) {
          this._lastChangedByCommandLine = true;
        }
        break;
      case "sessionstore-browser-state-restored":
        if (this._currentStatus == STATE_WAITING_FOR_RESTORE) {
          this._currentStatus = STATE_RESTORE_FINISHED;
          this._notifyIfTransitionComplete();
        }
        break;
    }
  },

  

  handle: function PBS_handle(aCmdLine) {
    if (aCmdLine.handleFlag("private", false))
      aCmdLine.preventDefault = true; 
    else if (aCmdLine.handleFlag("private-toggle", false)) {
      if (this._autoStarted) {
        throw Cr.NS_ERROR_ABORT;
      }
      this.privateBrowsingEnabled = !this.privateBrowsingEnabled;
      this._lastChangedByCommandLine = true;
    }
  },

  get helpInfo() {
    return "  -private           Enable private browsing mode.\n" +
           "  -private-toggle    Toggle private browsing mode.\n";
  },

  

  


  get privateBrowsingEnabled() {
    return this._inPrivateBrowsing;
  },

  


  set privateBrowsingEnabled(val) {
    
    
    
    
    
    if (this._currentStatus != STATE_IDLE)
      throw Cr.NS_ERROR_FAILURE;

    if (val == this._inPrivateBrowsing)
      return;

    try {
      if (val) {
        if (!this._canEnterPrivateBrowsingMode())
          return;
      }
      else {
        if (!this._canLeavePrivateBrowsingMode())
          return;
      }

      this._ensureCanCloseWindows();

      
      this._currentStatus = STATE_TRANSITION_STARTED;

      this._autoStarted = this._prefs.getBoolPref("browser.privatebrowsing.autostart");
      this._inPrivateBrowsing = val != false;

      this._recordTransitionTime("started");

      let data = val ? "enter" : "exit";

      let quitting = Cc["@mozilla.org/supports-PRBool;1"].
                     createInstance(Ci.nsISupportsPRBool);
      quitting.data = this._quitting;

      
      this._obs.notifyObservers(quitting, "private-browsing-change-granted", data);

      
      this._onBeforePrivateBrowsingModeChange();

      this._obs.notifyObservers(quitting, "private-browsing", data);

      this._recordTransitionTime("prepared");

      
      this._onAfterPrivateBrowsingModeChange();
    } catch (ex) {
      
      
      for (let i = 0; i < this._windowsToClose.length; i++)
        this._windowsToClose[i].docShell.contentViewer.resetCloseWindow();
      
      if (ex != Cr.NS_ERROR_ABORT)
        Cu.reportError("Exception thrown while processing the " +
          "private browsing mode change request: " + ex.toString());
    } finally {
      this._windowsToClose = [];
      this._notifyIfTransitionComplete();
      this._lastChangedByCommandLine = false;
    }
  },

  


  get autoStarted() {
    return this._inPrivateBrowsing && this._autoStarted;
  },

  


  get lastChangedByCommandLine() {
    return this._lastChangedByCommandLine;
  }
};

var NSGetFactory = XPCOMUtils.generateNSGetFactory([PrivateBrowsingService]);
