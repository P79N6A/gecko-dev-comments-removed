








































"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");

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

  



  getDebugger: function DUI_getDebugger(aTab) {
    return '_scriptDebugger' in aTab ? aTab._scriptDebugger : null;
  },

  



  get preferences() {
    return DebuggerUIPreferences;
  }
};







function DebuggerPane(aDebuggerUI, aTab) {
  this._globalUI = aDebuggerUI;
  this._tab = aTab;
  this._create();
}

DebuggerPane.prototype = {

  


  _create: function DP__create() {
    this._tab._scriptDebugger = this;

    let gBrowser = this._tab.linkedBrowser.getTabBrowser();
    let ownerDocument = gBrowser.parentNode.ownerDocument;

    this._splitter = ownerDocument.createElement("splitter");
    this._splitter.setAttribute("class", "hud-splitter");

    this._frame = ownerDocument.createElement("iframe");
    this._frame.height = DebuggerUIPreferences.height;

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

    this._frame.setAttribute("src", "chrome://browser/content/debugger.xul");

    this._globalUI.refreshCommand();
  },

  


  close: function DP_close() {
    if (!this._tab) {
      return;
    }
    this._tab._scriptDebugger = null;
    this._tab = null;

    DebuggerUIPreferences.height = this._frame.height;
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




let DebuggerUIPreferences = {

  



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
