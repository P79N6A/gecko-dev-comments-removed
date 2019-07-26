



"use strict";

function debug(msg) {
  Services.console.logStringMessage("SessionStoreContent: " + msg);
}

let Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm", this);

XPCOMUtils.defineLazyModuleGetter(this, "DocShellCapabilities",
  "resource:///modules/sessionstore/DocShellCapabilities.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PageStyle",
  "resource:///modules/sessionstore/PageStyle.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "SessionHistory",
  "resource:///modules/sessionstore/SessionHistory.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "SessionStorage",
  "resource:///modules/sessionstore/SessionStorage.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "TextAndScrollData",
  "resource:///modules/sessionstore/TextAndScrollData.jsm");





let EventListener = {

  DOM_EVENTS: [
    "pageshow", "change", "input", "MozStorageChanged"
  ],

  init: function () {
    this.DOM_EVENTS.forEach(e => addEventListener(e, this, true));
  },

  handleEvent: function (event) {
    switch (event.type) {
      case "pageshow":
        if (event.persisted)
          sendAsyncMessage("SessionStore:pageshow");
        break;
      case "input":
      case "change":
        sendAsyncMessage("SessionStore:input");
        break;
      case "MozStorageChanged": {
        let isSessionStorage = true;
        
        try {
          if (event.storageArea != content.sessionStorage) {
            isSessionStorage = false;
          }
        } catch (ex) {
          
          
          isSessionStorage = false;
        }
        if (isSessionStorage) {
          sendAsyncMessage("SessionStore:MozStorageChanged");
        }
        break;
      }
      default:
        debug("received unknown event '" + event.type + "'");
        break;
    }
  }
};




let MessageListener = {

  MESSAGES: [
    "SessionStore:collectSessionHistory",
    "SessionStore:collectSessionStorage",
    "SessionStore:collectDocShellCapabilities",
    "SessionStore:collectPageStyle"
  ],

  init: function () {
    this.MESSAGES.forEach(m => addMessageListener(m, this));
  },

  receiveMessage: function ({name, data: {id}}) {
    switch (name) {
      case "SessionStore:collectSessionHistory":
        let history = SessionHistory.read(docShell);
        if ("index" in history) {
          let tabIndex = history.index - 1;
          
          
          TextAndScrollData.updateFrame(history.entries[tabIndex],
                                        content,
                                        docShell.isAppTab);
        }
        sendAsyncMessage(name, {id: id, data: history});
        break;
      case "SessionStore:collectSessionStorage":
        let storage = SessionStorage.serialize(docShell);
        sendAsyncMessage(name, {id: id, data: storage});
        break;
      case "SessionStore:collectDocShellCapabilities":
        let disallow = DocShellCapabilities.collect(docShell);
        sendAsyncMessage(name, {id: id, data: disallow});
        break;
      case "SessionStore:collectPageStyle":
        let pageStyle = PageStyle.collect(docShell);
        sendAsyncMessage(name, {id: id, data: pageStyle});
        break;
      default:
        debug("received unknown message '" + name + "'");
        break;
    }
  }
};









let SyncHandler = {
  init: function () {
    
    
    
    
    sendSyncMessage("SessionStore:setupSyncHandler", {}, {handler: this});
  },

  collectSessionHistory: function (includePrivateData) {
    let history = SessionHistory.read(docShell);
    if ("index" in history) {
      let tabIndex = history.index - 1;
      TextAndScrollData.updateFrame(history.entries[tabIndex],
                                    content,
                                    docShell.isAppTab,
                                    {includePrivateData: includePrivateData});
    }
    return history;
  },

  collectSessionStorage: function () {
    return SessionStorage.serialize(docShell);
  },

  collectDocShellCapabilities: function () {
    return DocShellCapabilities.collect(docShell);
  },

  collectPageStyle: function () {
    return PageStyle.collect(docShell);
  },
};

let ProgressListener = {
  init: function() {
    let webProgress = docShell.QueryInterface(Ci.nsIInterfaceRequestor)
                              .getInterface(Ci.nsIWebProgress);
    webProgress.addProgressListener(this, Ci.nsIWebProgress.NOTIFY_LOCATION);
  },
  onLocationChange: function(aWebProgress, aRequest, aLocation, aFlags) {
    
    sendAsyncMessage("SessionStore:loadStart");
  },
  onStateChange: function(aWebProgress, aRequest, aStateFlags, aStatus) {},
  onProgressChange: function() {},
  onStatusChange: function() {},
  onSecurityChange: function() {},
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIWebProgressListener,
                                         Ci.nsISupportsWeakReference])
};

EventListener.init();
MessageListener.init();
SyncHandler.init();
ProgressListener.init();
