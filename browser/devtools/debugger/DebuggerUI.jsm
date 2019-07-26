




"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

const DBG_XUL = "chrome://browser/content/devtools/debugger.xul";
const DBG_STRINGS_URI = "chrome://browser/locale/devtools/debugger.properties";
const CHROME_DEBUGGER_PROFILE_NAME = "-chrome-debugger";

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this,
  "DebuggerServer", "resource://gre/modules/devtools/dbg-server.jsm");

XPCOMUtils.defineLazyModuleGetter(this,
  "Services", "resource://gre/modules/Services.jsm");

this.EXPORTED_SYMBOLS = ["DebuggerUI"];







this.DebuggerUI = function DebuggerUI(aWindow) {
  this.chromeWindow = aWindow;
  this.listenToTabs();
};

DebuggerUI.prototype = {
  



  listenToTabs: function() {
    let win = this.chromeWindow;
    let tabs = win.gBrowser.tabContainer;

    let bound_refreshCommand = this.refreshCommand.bind(this);
    tabs.addEventListener("TabSelect", bound_refreshCommand, true);

    win.addEventListener("unload", function onClose(aEvent) {
      win.removeEventListener("unload", onClose, false);
      tabs.removeEventListener("TabSelect", bound_refreshCommand, true);
    }, false);
  },

  



  refreshCommand: function() {
    let scriptDebugger = this.getDebugger();
    let command = this.chromeWindow.document.getElementById("Tools:Debugger");
    let selectedTab = this.chromeWindow.gBrowser.selectedTab;

    if (scriptDebugger && scriptDebugger.ownerTab === selectedTab) {
      command.setAttribute("checked", "true");
    } else {
      command.setAttribute("checked", "false");
    }
  },

  





  toggleDebugger: function() {
    let scriptDebugger = this.findDebugger();
    let selectedTab = this.chromeWindow.gBrowser.selectedTab;

    if (scriptDebugger) {
      scriptDebugger.close();
      return null;
    }
    return new DebuggerPane(this, selectedTab);
  },

  





  toggleRemoteDebugger: function() {
    let remoteDebugger = this.getRemoteDebugger();

    if (remoteDebugger) {
      remoteDebugger.close();
      return null;
    }
    return new RemoteDebuggerWindow(this);
  },

  





  toggleChromeDebugger: function(aOnClose, aOnRun) {
    let chromeDebugger = this.getChromeDebugger();

    if (chromeDebugger) {
      chromeDebugger.close();
      return null;
    }
    return new ChromeDebuggerProcess(this, aOnClose, aOnRun);
  },

  





  findDebugger: function() {
    let enumerator = Services.wm.getEnumerator("navigator:browser");
    while (enumerator.hasMoreElements()) {
      let chromeWindow = enumerator.getNext().QueryInterface(Ci.nsIDOMWindow);
      let scriptDebugger = chromeWindow.DebuggerUI.getDebugger();
      if (scriptDebugger) {
        return scriptDebugger;
      }
    }
    return null;
  },

  





  getDebugger: function() {
    return '_scriptDebugger' in this ? this._scriptDebugger : null;
  },

  





  getRemoteDebugger: function() {
    return '_remoteDebugger' in this ? this._remoteDebugger : null;
  },

  





  getChromeDebugger: function() {
    return '_chromeDebugger' in this ? this._chromeDebugger : null;
  }
};









this.DebuggerPane = function DebuggerPane(aDebuggerUI, aTab) {
  this.globalUI = aDebuggerUI;
  this._win = aDebuggerUI.chromeWindow;
  this._tab = aTab;

  this.close = this.close.bind(this);
  this._initServer();
  this._create();
}

DebuggerPane.prototype = {
  


  _initServer: function() {
    if (!DebuggerServer.initialized) {
      DebuggerServer.init();
      DebuggerServer.addBrowserActors();
    }
  },

  


  _create: function() {
    this.globalUI._scriptDebugger = this;

    let gBrowser = this._win.gBrowser;
    let ownerDocument = gBrowser.parentNode.ownerDocument;

    this._splitter = ownerDocument.createElement("splitter");
    this._splitter.setAttribute("class", "devtools-horizontal-splitter");

    this._frame = ownerDocument.createElement("iframe");

    this._nbox = gBrowser.getNotificationBox(this._tab.linkedBrowser);
    this._nbox.appendChild(this._splitter);
    this._nbox.appendChild(this._frame);

    let self = this;

    this._frame.addEventListener("Debugger:Loaded", function dbgLoaded() {
      self._frame.removeEventListener("Debugger:Loaded", dbgLoaded, true);
      self._frame.addEventListener("Debugger:Unloaded", self.close, true);

      
      let bkp = self.contentWindow.DebuggerController.Breakpoints;
      self.addBreakpoint = bkp.addBreakpoint;
      self.removeBreakpoint = bkp.removeBreakpoint;
      self.getBreakpoint = bkp.getBreakpoint;
      self.breakpoints = bkp.store;
    }, true);

    this._frame.setAttribute("src", DBG_XUL);
    this.globalUI.refreshCommand();
  },

  






  close: function(aCloseCallback) {
    if (!this.globalUI) {
      return;
    }
    delete this.globalUI._scriptDebugger;

    
    
    if (typeof aCloseCallback == "function") {
      let frame = this._frame;
      frame.addEventListener("unload", function onUnload() {
        frame.removeEventListener("unload", onUnload, true);
        aCloseCallback();
      }, true)
    }

    this._frame.removeEventListener("Debugger:Unloaded", this.close, true);
    this._nbox.removeChild(this._splitter);
    this._nbox.removeChild(this._frame);

    this._splitter = null;
    this._frame = null;
    this._nbox = null;
    this._win = null;
    this._tab = null;

    
    delete this.addBreakpoint;
    delete this.removeBreakpoint;
    delete this.getBreakpoint;
    delete this.breakpoints;

    this.globalUI.refreshCommand();
    this.globalUI = null;
  },

  



  get ownerWindow() {
    return this._win;
  },

  



  get ownerTab() {
    return this._tab;
  },

  



  get contentWindow() {
    return this._frame ? this._frame.contentWindow : null;
  }
};







this.RemoteDebuggerWindow = function RemoteDebuggerWindow(aDebuggerUI) {
  this.globalUI = aDebuggerUI;
  this._win = aDebuggerUI.chromeWindow;

  this.close = this.close.bind(this);
  this._create();
}

RemoteDebuggerWindow.prototype = {
  


  _create: function() {
    this.globalUI._remoteDebugger = this;

    this._dbgwin = this.globalUI.chromeWindow.open(DBG_XUL,
      L10N.getStr("remoteDebuggerWindowTitle"), "chrome,dependent,resizable");

    let self = this;

    this._dbgwin.addEventListener("Debugger:Loaded", function dbgLoaded() {
      self._dbgwin.removeEventListener("Debugger:Loaded", dbgLoaded, true);
      self._dbgwin.addEventListener("Debugger:Unloaded", self.close, true);

      
      let bkp = self.contentWindow.DebuggerController.Breakpoints;
      self.addBreakpoint = bkp.addBreakpoint;
      self.removeBreakpoint = bkp.removeBreakpoint;
      self.getBreakpoint = bkp.getBreakpoint;
      self.breakpoints = bkp.store;
    }, true);

    this._dbgwin._remoteFlag = true;
  },

  


  close: function() {
    if (!this.globalUI) {
      return;
    }
    delete this.globalUI._remoteDebugger;

    this._dbgwin.removeEventListener("Debugger:Unloaded", this.close, true);
    this._dbgwin.close();
    this._dbgwin = null;
    this._win = null;

    
    delete this.addBreakpoint;
    delete this.removeBreakpoint;
    delete this.getBreakpoint;
    delete this.breakpoints;

    this.globalUI = null;
  },

  



  get ownerWindow() {
    return this._win;
  },

  



  get contentWindow() {
    return this._dbgwin;
  }
};











this.ChromeDebuggerProcess = function ChromeDebuggerProcess(aDebuggerUI, aOnClose, aOnRun) {
  this.globalUI = aDebuggerUI;
  this._win = aDebuggerUI.chromeWindow;
  this._closeCallback = aOnClose;
  this._runCallback = aOnRun;

  this._initServer();
  this._initProfile();
  this._create();
}

ChromeDebuggerProcess.prototype = {
  


  _initServer: function() {
    if (!DebuggerServer.initialized) {
      DebuggerServer.init();
      DebuggerServer.addBrowserActors();
    }
    DebuggerServer.openListener(Prefs.chromeDebuggingPort);
  },

  


  _initProfile: function() {
    let profileService = Cc["@mozilla.org/toolkit/profile-service;1"]
      .createInstance(Ci.nsIToolkitProfileService);

    let profileName;
    try {
      
      profileName = profileService.selectedProfile.name + CHROME_DEBUGGER_PROFILE_NAME;
    } catch (e) {
      
      profileName = CHROME_DEBUGGER_PROFILE_NAME;
      Cu.reportError(e);
    }

    let profileObject;
    try {
      
      profileObject = profileService.getProfileByName(profileName);

      
      var enumerator = Services.dirsvc.get("ProfD", Ci.nsIFile).parent.directoryEntries;
      while (enumerator.hasMoreElements()) {
        let profileDir = enumerator.getNext().QueryInterface(Ci.nsIFile);
        if (profileDir.leafName.contains(profileName)) {
          
          this._dbgProfile = profileObject;
          return;
        }
      }
      
      profileObject.remove(true);
    } catch (e) {
      
      Cu.reportError(e);
    }

    
    this._dbgProfile = profileService.createProfile(null, null, profileName);
    profileService.flush();
  },

  


  _create: function() {
    this.globalUI._chromeDebugger = this;

    let file = Services.dirsvc.get("XREExeF", Ci.nsIFile);

    dumpn("Initializing chrome debugging process");
    let process = Cc["@mozilla.org/process/util;1"].createInstance(Ci.nsIProcess);
    process.init(file);

    let args = [
      "-no-remote", "-P", this._dbgProfile.name,
      "-chrome", DBG_XUL];

    dumpn("Running chrome debugging process");
    process.runwAsync(args, args.length, { observe: this.close.bind(this) });
    this._dbgProcess = process;

    if (typeof this._runCallback == "function") {
      this._runCallback.call({}, this);
    }
  },

  


  close: function() {
    dumpn("Closing chrome debugging process");
    if (!this.globalUI) {
      dumpn("globalUI is missing");
      return;
    }
    delete this.globalUI._chromeDebugger;

    if (this._dbgProcess.isRunning) {
      dumpn("Killing chrome debugging process...");
      this._dbgProcess.kill();
    }
    dumpn("...done.");
    if (typeof this._closeCallback == "function") {
      this._closeCallback.call({}, this);
    }

    this._dbgProcess = null;
    this._dbgProfile = null;
    this._win = null;

    this.globalUI = null;
  }
};




let L10N = {
  





  getStr: function(aName) {
    return this.stringBundle.GetStringFromName(aName);
  }
};

XPCOMUtils.defineLazyGetter(L10N, "stringBundle", function() {
  return Services.strings.createBundle(DBG_STRINGS_URI);
});




let Prefs = {};





XPCOMUtils.defineLazyGetter(Prefs, "chromeDebuggingPort", function() {
  return Services.prefs.getIntPref("devtools.debugger.chrome-debugging-port");
});





function dumpn(str) {
  if (wantLogging) {
    dump("DBG-FRONTEND: " + str + "\n");
  }
}

let wantLogging = Services.prefs.getBoolPref("devtools.debugger.log");
