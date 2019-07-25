








































"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

const DBG_XUL = "chrome://browser/content/debugger.xul";
const REMOTE_PROFILE_NAME = "_remote-debug";

Cu.import("resource://gre/modules/devtools/dbg-server.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/FileUtils.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

let EXPORTED_SYMBOLS = ["DebuggerUI"];







function DebuggerUI(aWindow) {
  this.chromeWindow = aWindow;
}

DebuggerUI.prototype = {
  



  refreshCommand: function DUI_refreshCommand() {
    let selectedTab = this.chromeWindow.getBrowser().selectedTab;
    let command = this.chromeWindow.document.getElementById("Tools:Debugger");

    if (this.getDebugger(selectedTab) != null) {
      command.setAttribute("checked", "true");
    } else {
      command.removeAttribute("checked");
    }
  },

  



  toggleDebugger: function DUI_toggleDebugger() {
    let tab = this.chromeWindow.gBrowser.selectedTab;

    if (tab._scriptDebugger) {
      tab._scriptDebugger.close();
      return null;
    }
    return new DebuggerPane(this, tab);
  },

  




  toggleRemoteDebugger: function DUI_toggleRemoteDebugger(aOnClose, aOnRun) {
    let win = this.chromeWindow;

    if (win._remoteDebugger) {
      win._remoteDebugger.close();
      return null;
    }
    return new DebuggerProcess(win, aOnClose, aOnRun);
  },

  




  toggleChromeDebugger: function DUI_toggleChromeDebugger(aOnClose, aOnRun) {
    let win = this.chromeWindow;

    if (win._chromeDebugger) {
      win._chromeDebugger.close();
      return null;
    }
    return new DebuggerProcess(win, aOnClose, aOnRun, true);
  },

  



  getDebugger: function DUI_getDebugger(aTab) {
    return '_scriptDebugger' in aTab ? aTab._scriptDebugger : null;
  },

  



  get preferences() {
    return DebuggerPreferences;
  }
};







function DebuggerPane(aDebuggerUI, aTab) {
  this._globalUI = aDebuggerUI;
  this._tab = aTab;
  
  this._initServer();
  this._create();
}

DebuggerPane.prototype = {

  


  _initServer: function DP__initServer() {
    if (!DebuggerServer.initialized) {
      DebuggerServer.init();
      DebuggerServer.addBrowserActors();
    }
  },

  


  _create: function DP__create() {
    this._tab._scriptDebugger = this;

    let gBrowser = this._tab.linkedBrowser.getTabBrowser();
    let ownerDocument = gBrowser.parentNode.ownerDocument;

    this._splitter = ownerDocument.createElement("splitter");
    this._splitter.setAttribute("class", "hud-splitter");

    this._frame = ownerDocument.createElement("iframe");
    this._frame.height = DebuggerPreferences.height;

    this._nbox = gBrowser.getNotificationBox(this._tab.linkedBrowser);
    this._nbox.appendChild(this._splitter);
    this._nbox.appendChild(this._frame);

    this.close = this.close.bind(this);
    let self = this;

    this._frame.addEventListener("Debugger:Loaded", function dbgLoaded() {
      self._frame.removeEventListener("Debugger:Loaded", dbgLoaded, true);
      self._frame.addEventListener("Debugger:Close", self.close, true);
      self._frame.addEventListener("unload", self.close, true);

      
      let bkp = self.debuggerWindow.DebuggerController.Breakpoints;
      self.addBreakpoint = bkp.addBreakpoint;
      self.removeBreakpoint = bkp.removeBreakpoint;
      self.getBreakpoint = bkp.getBreakpoint;
    }, true);

    this._frame.setAttribute("src", DBG_XUL);

    this._globalUI.refreshCommand();
  },

  


  close: function DP_close() {
    if (!this._tab) {
      return;
    }
    delete this._tab._scriptDebugger;
    this._tab = null;

    DebuggerPreferences.height = this._frame.height;
    this._frame.removeEventListener("Debugger:Close", this.close, true);
    this._frame.removeEventListener("unload", this.close, true);

    this._nbox.removeChild(this._splitter);
    this._nbox.removeChild(this._frame);

    this._splitter = null;
    this._frame = null;
    this._nbox = null;

    this._globalUI.refreshCommand();
  },

  



  get debuggerWindow() {
    return this._frame ? this._frame.contentWindow : null;
  },

  



  get breakpoints() {
    let debuggerWindow = this.debuggerWindow;
    if (debuggerWindow) {
      return debuggerWindow.DebuggerController.Breakpoints.store;
    }
    return null;
  }
};














function DebuggerProcess(aWindow, aOnClose, aOnRun, aInitServerFlag) {
  this._win = aWindow;
  this._closeCallback = aOnClose;
  this._runCallback = aOnRun;

  aInitServerFlag && this._initServer();
  this._initProfile();
  this._create();
}

DebuggerProcess.prototype = {

  


  _initServer: function RDP__initServer() {
    if (!DebuggerServer.initialized) {
      DebuggerServer.init();
      DebuggerServer.addBrowserActors();
    }
    DebuggerServer.closeListener();
    DebuggerServer.openListener(DebuggerPreferences.remotePort, false);
  },

  


  _initProfile: function RDP__initProfile() {
    let profileService = Cc["@mozilla.org/toolkit/profile-service;1"]
      .createInstance(Ci.nsIToolkitProfileService);

    let dbgProfileName;
    try {
      dbgProfileName = profileService.selectedProfile.name + REMOTE_PROFILE_NAME;
    } catch(e) {
      dbgProfileName = REMOTE_PROFILE_NAME;
      Cu.reportError(e);
    }

    this._dbgProfile = profileService.createProfile(null, null, dbgProfileName);
    profileService.flush();
  },

  


  _create: function RDP__create() {
    this._win._remoteDebugger = this;

    let file = FileUtils.getFile("CurProcD",
      [Services.appinfo.OS == "WINNT" ? "firefox.exe"
                                      : "firefox-bin"]);

    let process = Cc["@mozilla.org/process/util;1"].createInstance(Ci.nsIProcess);
    process.init(file);

    let args = [
      "-no-remote", "-P", this._dbgProfile.name,
      "-chrome", DBG_XUL,
      "-width", DebuggerPreferences.remoteWinWidth,
      "-height", DebuggerPreferences.remoteWinHeight];

    process.runwAsync(args, args.length, { observe: this.close.bind(this) });
    this._dbgProcess = process;

    if (typeof this._runCallback === "function") {
      this._runCallback.call({}, this);
    }
  },

  


  close: function RDP_close() {
    if (!this._win) {
      return;
    }
    delete this._win._remoteDebugger;
    this._win = null;

    if (this._dbgProcess.isRunning) {
      this._dbgProcess.kill();
    }
    if (this._dbgProfile) {
      this._dbgProfile.remove(false);
    }
    if (typeof this._closeCallback === "function") {
      this._closeCallback.call({}, this);
    }

    this._dbgProcess = null;
    this._dbgProfile = null;
  }
};




let DebuggerPreferences = {

  



  get height() {
    if (this._height === undefined) {
      this._height = Services.prefs.getIntPref("devtools.debugger.ui.height");
    }
    return this._height;
  },

  



  set height(value) {
    Services.prefs.setIntPref("devtools.debugger.ui.height", value);
    this._height = value;
  }
};





XPCOMUtils.defineLazyGetter(DebuggerPreferences, "remoteWinWidth", function() {
  return Services.prefs.getIntPref("devtools.debugger.ui.remote-win.width");
});





XPCOMUtils.defineLazyGetter(DebuggerPreferences, "remoteWinHeight", function() {
  return Services.prefs.getIntPref("devtools.debugger.ui.remote-win.height");
});





XPCOMUtils.defineLazyGetter(DebuggerPreferences, "remoteHost", function() {
  return Services.prefs.getCharPref("devtools.debugger.remote-host");
});





XPCOMUtils.defineLazyGetter(DebuggerPreferences, "remotePort", function() {
  return Services.prefs.getIntPref("devtools.debugger.remote-port");
});
