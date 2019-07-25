




"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

const DBG_XUL = "chrome://browser/content/debugger.xul";
const DBG_STRINGS_URI = "chrome://browser/locale/devtools/debugger.properties";
const REMOTE_PROFILE_NAME = "_remote-debug";
const TAB_SWITCH_NOTIFICATION = "debugger-tab-switch";

Cu.import("resource://gre/modules/devtools/dbg-server.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/FileUtils.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

let EXPORTED_SYMBOLS = ["DebuggerUI"];







function DebuggerUI(aWindow) {
  this.chromeWindow = aWindow;
  this.listenToTabs();
}

DebuggerUI.prototype = {

  



  listenToTabs: function DUI_listenToTabs() {
    let win = this.chromeWindow;
    let tabs = win.gBrowser.tabContainer;

    let bound_refreshCommand = this.refreshCommand.bind(this);
    tabs.addEventListener("TabSelect", bound_refreshCommand, true);

    win.addEventListener("unload", function onClose(aEvent) {
      tabs.removeEventListener("TabSelect", bound_refreshCommand, true);
      win.removeEventListener("unload", onClose, false);
    }, false);
  },

  



  refreshCommand: function DUI_refreshCommand() {
    let scriptDebugger = this.getDebugger();
    let command = this.chromeWindow.document.getElementById("Tools:Debugger");
    let selectedTab = this.chromeWindow.gBrowser.selectedTab;

    if (scriptDebugger && scriptDebugger.ownerTab === selectedTab) {
      command.setAttribute("checked", "true");
    } else {
      command.setAttribute("checked", "false");
    }
  },

  



  toggleDebugger: function DUI_toggleDebugger() {
    let scriptDebugger = this.findDebugger();
    let selectedTab = this.chromeWindow.gBrowser.selectedTab;

    if (scriptDebugger) {
      if (scriptDebugger.ownerTab !== selectedTab) {
        this.showTabSwitchNotification();
        return scriptDebugger;
      }
      scriptDebugger.close();
      return null;
    }
    return new DebuggerPane(this, selectedTab);
  },

  



  toggleRemoteDebugger: function DUI_toggleRemoteDebugger() {
    let remoteDebugger = this.getRemoteDebugger();

    if (remoteDebugger) {
      remoteDebugger.close();
      return null;
    }
    return new RemoteDebuggerWindow(this);
  },

  



  toggleChromeDebugger: function DUI_toggleChromeDebugger(aOnClose, aOnRun) {
    let chromeDebugger = this.getChromeDebugger();

    if (chromeDebugger) {
      chromeDebugger.close();
      return null;
    }
    return new ChromeDebuggerProcess(this, aOnClose, aOnRun);
  },

  





  findDebugger: function DUI_findDebugger() {
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

  





  getDebugger: function DUI_getDebugger() {
    return '_scriptDebugger' in this ? this._scriptDebugger : null;
  },

  





  getRemoteDebugger: function DUI_getRemoteDebugger() {
    return '_remoteDebugger' in this ? this._remoteDebugger : null;
  },

  





  getChromeDebugger: function DUI_getChromeDebugger() {
    return '_chromeDebugger' in this ? this._chromeDebugger : null;
  },

  



  get preferences() {
    return DebuggerPreferences;
  },

  




  showTabSwitchNotification: function DUI_showTabSwitchNotification()
  {
    let gBrowser = this.chromeWindow.gBrowser;
    let selectedBrowser = gBrowser.selectedBrowser;

    let nbox = gBrowser.getNotificationBox(selectedBrowser);
    let notification = nbox.getNotificationWithValue(TAB_SWITCH_NOTIFICATION);
    if (notification) {
      nbox.removeNotification(notification);
      return;
    }

    let buttons = [{
      id: "debugger.confirmTabSwitch.buttonSwitch",
      label: L10N.getStr("confirmTabSwitch.buttonSwitch"),
      accessKey: L10N.getStr("confirmTabSwitch.buttonSwitch.accessKey"),
      callback: function DUI_notificationButtonSwitch() {
        let scriptDebugger = this.findDebugger();
        let targetWindow = scriptDebugger.globalUI.chromeWindow;
        targetWindow.gBrowser.selectedTab = scriptDebugger.ownerTab;
        targetWindow.focus();
      }.bind(this)
    }, {
      id: "debugger.confirmTabSwitch.buttonOpen",
      label: L10N.getStr("confirmTabSwitch.buttonOpen"),
      accessKey: L10N.getStr("confirmTabSwitch.buttonOpen.accessKey"),
      callback: function DUI_notificationButtonOpen() {
        this.findDebugger().close();
        this.toggleDebugger();
      }.bind(this)
    }];

    let message = L10N.getStr("confirmTabSwitch.message");
    let imageURL = "chrome://browser/skin/Info.png";

    notification = nbox.appendNotification(
      message, TAB_SWITCH_NOTIFICATION,
      imageURL, nbox.PRIORITY_WARNING_HIGH, buttons, null);

    
    
    notification.persistence = -1;
  }
};









function DebuggerPane(aDebuggerUI, aTab) {
  this.globalUI = aDebuggerUI;
  this._win = aDebuggerUI.chromeWindow;
  this._tab = aTab;

  this._initServer();
  this._create();
}

DebuggerPane.prototype = {

  


  _initServer: function DP__initServer() {
    if (!DebuggerServer.initialized) {
      
      DebuggerServer.init(function () { return true; });
      DebuggerServer.addBrowserActors();
    }
  },

  


  _create: function DP__create() {
    this.globalUI._scriptDebugger = this;

    let gBrowser = this._win.gBrowser;
    let ownerDocument = gBrowser.parentNode.ownerDocument;

    this._splitter = ownerDocument.createElement("splitter");
    this._splitter.setAttribute("class", "devtools-horizontal-splitter");

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

      
      let bkp = self.contentWindow.DebuggerController.Breakpoints;
      self.addBreakpoint = bkp.addBreakpoint;
      self.removeBreakpoint = bkp.removeBreakpoint;
      self.getBreakpoint = bkp.getBreakpoint;
    }, true);

    this._frame.setAttribute("src", DBG_XUL);
    this.globalUI.refreshCommand();
  },

  






  close: function DP_close(aCloseCallback) {
    if (!this.globalUI) {
      return;
    }
    delete this.globalUI._scriptDebugger;
    this._win = null;
    this._tab = null;

    DebuggerPreferences.height = this._frame.height;
    this._frame.removeEventListener("Debugger:Close", this.close, true);
    this._frame.removeEventListener("unload", this.close, true);

    
    
    if (typeof(aCloseCallback) == "function") {
      let frame = this._frame;
      frame.addEventListener("unload", function onUnload() {
        frame.removeEventListener("unload", onUnload, true);
        aCloseCallback();
      }, true)
    }

    this._nbox.removeChild(this._splitter);
    this._nbox.removeChild(this._frame);

    this._splitter = null;
    this._frame = null;
    this._nbox = null;

    this.globalUI.refreshCommand();
    this.globalUI = null;
  },

  



  get ownerTab() {
    return this._tab;
  },

  



  get contentWindow() {
    return this._frame ? this._frame.contentWindow : null;
  },

  



  get breakpoints() {
    let contentWindow = this.contentWindow;
    if (contentWindow) {
      return contentWindow.DebuggerController.Breakpoints.store;
    }
    return null;
  }
};







function RemoteDebuggerWindow(aDebuggerUI) {
  this.globalUI = aDebuggerUI;
  this._win = aDebuggerUI.chromeWindow;

  this._create();
}

RemoteDebuggerWindow.prototype = {

  


  _create: function DP__create() {
    this.globalUI._remoteDebugger = this;

    this._dbgwin = this.globalUI.chromeWindow.open(DBG_XUL,
      L10N.getStr("remoteDebuggerWindowTitle"),
      "width=" + DebuggerPreferences.remoteWinWidth + "," +
      "height=" + DebuggerPreferences.remoteWinHeight + "," +
      "chrome,dependent,resizable,centerscreen");

    this._dbgwin._remoteFlag = true;

    this.close = this.close.bind(this);
    let self = this;

    this._dbgwin.addEventListener("Debugger:Loaded", function dbgLoaded() {
      self._dbgwin.removeEventListener("Debugger:Loaded", dbgLoaded, true);
      self._dbgwin.addEventListener("Debugger:Close", self.close, true);
      self._dbgwin.addEventListener("unload", self.close, true);

      
      let bkp = self.contentWindow.DebuggerController.Breakpoints;
      self.addBreakpoint = bkp.addBreakpoint;
      self.removeBreakpoint = bkp.removeBreakpoint;
      self.getBreakpoint = bkp.getBreakpoint;
    }, true);
  },

  


  close: function DP_close() {
    if (!this.globalUI) {
      return;
    }
    delete this.globalUI._remoteDebugger;
    this.globalUI = null;
    this._win = null;

    this._dbgwin.close();
    this._dbgwin = null;
  },

  



  get contentWindow() {
    return this._dbgwin;
  },

  



  get breakpoints() {
    let contentWindow = this.contentWindow;
    if (contentWindow) {
      return contentWindow.DebuggerController.Breakpoints.store;
    }
    return null;
  }
};











function ChromeDebuggerProcess(aDebuggerUI, aOnClose, aOnRun) {
  this.globalUI = aDebuggerUI;
  this._win = aDebuggerUI.chromeWindow;
  this._closeCallback = aOnClose;
  this._runCallback = aOnRun;

  this._initServer();
  this._initProfile();
  this._create();
}

ChromeDebuggerProcess.prototype = {

  


  _initServer: function RDP__initServer() {
    if (!DebuggerServer.initialized) {
      DebuggerServer.init(this._allowConnection);
      DebuggerServer.addBrowserActors();
    }
    DebuggerServer.closeListener();
    DebuggerServer.openListener(DebuggerPreferences.remotePort);
  },

  




  _allowConnection: function RDP__allowConnection() {
    let title = L10N.getStr("remoteIncomingPromptTitle");
    let msg = L10N.getStr("remoteIncomingPromptMessage");
    let disableButton = L10N.getStr("remoteIncomingPromptDisable");
    let prompt = Services.prompt;
    let flags = prompt.BUTTON_POS_0 * prompt.BUTTON_TITLE_OK +
                prompt.BUTTON_POS_1 * prompt.BUTTON_TITLE_CANCEL +
                prompt.BUTTON_POS_2 * prompt.BUTTON_TITLE_IS_STRING +
                prompt.BUTTON_POS_1_DEFAULT;
    let result = prompt.confirmEx(null, title, msg, flags, null, null,
                                  disableButton, null, { value: false });
    if (result == 0) {
      return true;
    }
    if (result == 2) {
      DebuggerServer.closeListener();
      Services.prefs.setBoolPref("devtools.debugger.remote-enabled", false);
    }
    return false;
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
    this.globalUI._chromeDebugger = this;

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
    if (!this.globalUI) {
      return;
    }
    delete this.globalUI._chromeDebugger;
    this.globalUI = null;
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




let L10N = {

  





  getStr: function L10N_getStr(aName) {
    return this.stringBundle.GetStringFromName(aName);
  }
};

XPCOMUtils.defineLazyGetter(L10N, "stringBundle", function() {
  return Services.strings.createBundle(DBG_STRINGS_URI);
});




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
