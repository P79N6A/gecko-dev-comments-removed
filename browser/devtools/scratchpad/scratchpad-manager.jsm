





































"use strict";

var EXPORTED_SYMBOLS = ["ScratchpadManager"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

const SCRATCHPAD_WINDOW_URL = "chrome://browser/content/scratchpad.xul";
const SCRATCHPAD_WINDOW_FEATURES = "chrome,titlebar,toolbar,centerscreen,resizable,dialog=no";

Cu.import("resource://gre/modules/Services.jsm");






var ScratchpadManager = {

  _scratchpads: [],

  






  getSessionState: function SPM_getSessionState()
  {
    return this._scratchpads;
  },

  









  restoreSession: function SPM_restoreSession(aSession)
  {
    if (!Array.isArray(aSession)) {
      return [];
    }

    let wins = [];
    aSession.forEach(function(state) {
      let win = this.openScratchpad(state);
      wins.push(win);
    }, this);

    return wins;
  },

  


  saveOpenWindows: function SPM_saveOpenWindows() {
    this._scratchpads = [];

    let enumerator = Services.wm.getEnumerator("devtools:scratchpad");
    while (enumerator.hasMoreElements()) {
      let win = enumerator.getNext();
      if (!win.closed) {
        this._scratchpads.push(win.Scratchpad.getState());
      }
    }
  },

  









  openScratchpad: function SPM_openScratchpad(aState)
  {
    let params = null;
    if (aState) {
      if (typeof aState != 'object') {
        return;
      }
      params = Cc["@mozilla.org/embedcomp/dialogparam;1"]
               .createInstance(Ci.nsIDialogParamBlock);
      params.SetNumberStrings(1);
      params.SetString(0, JSON.stringify(aState));
    }
    let win = Services.ww.openWindow(null, SCRATCHPAD_WINDOW_URL, "_blank",
                                     SCRATCHPAD_WINDOW_FEATURES, params);
    
    ShutdownObserver.init();

    return win;
  }
};






var ShutdownObserver = {
  _initialized: false,

  init: function SDO_init()
  {
    if (this._initialized) {
      return;
    }

    Services.obs.addObserver(this, "quit-application-granted", false);
    this._initialized = true;
  },

  observe: function SDO_observe(aMessage, aTopic, aData)
  {
    if (aTopic == "quit-application-granted") {
      ScratchpadManager.saveOpenWindows();
      this.uninit();
    }
  },

  uninit: function SDO_uninit()
  {
    Services.obs.removeObserver(this, "quit-application-granted");
  }
};
