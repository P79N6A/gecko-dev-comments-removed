














'use strict';

var EXPORTED_SYMBOLS = ['PdfjsContentUtils'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import('resource://gre/modules/XPCOMUtils.jsm');
Cu.import('resource://gre/modules/Services.jsm');

let PdfjsContentUtils = {
  _mm: null,

  



  get isRemote() {
    return Services.appinfo.processType == Services.appinfo.PROCESS_TYPE_CONTENT;
  },

  init: function () {
    
    
    if (!this._mm) {
      this._mm = Cc["@mozilla.org/childprocessmessagemanager;1"].getService(Ci.nsISyncMessageSender);
      this._mm.addMessageListener("PDFJS:Child:refreshSettings", this);
      Services.obs.addObserver(this, "quit-application", false);
    }
  },

  uninit: function () {
    if (this._mm) {
      this._mm.removeMessageListener("PDFJS:Child:refreshSettings", this);
      Services.obs.removeObserver(this, "quit-application");
    }
    this._mm = null;
  },

  





  clearUserPref: function (aPrefName) {
    this._mm.sendSyncMessage("PDFJS:Parent:clearUserPref", {
      name: aPrefName
    });
  },

  setIntPref: function (aPrefName, aPrefValue) {
    this._mm.sendSyncMessage("PDFJS:Parent:setIntPref", {
      name: aPrefName,
      value: aPrefValue
    });
  },

  setBoolPref: function (aPrefName, aPrefValue) {
    this._mm.sendSyncMessage("PDFJS:Parent:setBoolPref", {
      name: aPrefName,
      value: aPrefValue
    });
  },

  setCharPref: function (aPrefName, aPrefValue) {
    this._mm.sendSyncMessage("PDFJS:Parent:setCharPref", {
      name: aPrefName,
      value: aPrefValue
    });
  },

  setStringPref: function (aPrefName, aPrefValue) {
    this._mm.sendSyncMessage("PDFJS:Parent:setStringPref", {
      name: aPrefName,
      value: aPrefValue
    });
  },

  



  isDefaultHandlerApp: function () {
    return this._mm.sendSyncMessage("PDFJS:Parent:isDefaultHandlerApp")[0];
  },

  



  displayWarning: function (aWindow, aMessage, aCallback, aLabel, accessKey) {
    
    let winmm = aWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                       .getInterface(Ci.nsIDocShell)
                       .QueryInterface(Ci.nsIInterfaceRequestor)
                       .getInterface(Ci.nsIContentFrameMessageManager);
    winmm.sendAsyncMessage("PDFJS:Parent:displayWarning", {
      message: aMessage,
      label: aLabel,
      accessKey: accessKey
    }, {
      callback: aCallback
    });
  },

  



  observe: function(aSubject, aTopic, aData) {
    if (aTopic == "quit-application") {
      this.uninit();
    }
  },

  receiveMessage: function (aMsg) {
    switch (aMsg.name) {
      case "PDFJS:Child:refreshSettings":
        
        if (Services.appinfo.processType == Services.appinfo.PROCESS_TYPE_CONTENT) {
          let jsm = "resource://pdf.js/PdfJs.jsm";
          let pdfjs = Components.utils.import(jsm, {}).PdfJs;
          pdfjs.updateRegistration();
        }
        break;
    }
  },

  



  getChromeWindow: function (aWindow) {
    let winmm = aWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                        .getInterface(Ci.nsIDocShell)
                        .sameTypeRootTreeItem
                        .QueryInterface(Ci.nsIDocShell)
                        .QueryInterface(Ci.nsIInterfaceRequestor)
                        .getInterface(Ci.nsIContentFrameMessageManager);
    
    
    let suitcase = {
      _window: null,
      setChromeWindow: function (aObj) { this._window = aObj; }
    }
    if (!winmm.sendSyncMessage("PDFJS:Parent:getChromeWindow", {},
                               { suitcase: suitcase })[0]) {
      Cu.reportError("A request for a CPOW wrapped chrome window " +
                     "failed for unknown reasons.");
      return null;
    }
    return suitcase._window;
  },

  getFindBar: function (aWindow) {
    let winmm = aWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                        .getInterface(Ci.nsIDocShell)
                        .sameTypeRootTreeItem
                        .QueryInterface(Ci.nsIDocShell)
                        .QueryInterface(Ci.nsIInterfaceRequestor)
                        .getInterface(Ci.nsIContentFrameMessageManager);
    let suitcase = {
      _findbar: null,
      setFindBar: function (aObj) { this._findbar = aObj; }
    }
    if (!winmm.sendSyncMessage("PDFJS:Parent:getFindBar", {},
                               { suitcase: suitcase })[0]) {
      Cu.reportError("A request for a CPOW wrapped findbar " +
                     "failed for unknown reasons.");
      return null;
    }
    return suitcase._findbar;
  }
};

