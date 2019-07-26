



"use strict";

function debug(msg) {
  Services.console.logStringMessage("SessionStoreContent: " + msg);
}

let Cu = Components.utils;
let Cc = Components.classes;
let Ci = Components.interfaces;
let Cr = Components.results;

Cu.import("resource://gre/modules/XPCOMUtils.jsm", this);
Cu.import("resource://gre/modules/Timer.jsm", this);

XPCOMUtils.defineLazyModuleGetter(this, "DocShellCapabilities",
  "resource:///modules/sessionstore/DocShellCapabilities.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "FormData",
  "resource:///modules/sessionstore/FormData.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PageStyle",
  "resource:///modules/sessionstore/PageStyle.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "ScrollPosition",
  "resource:///modules/sessionstore/ScrollPosition.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "SessionHistory",
  "resource:///modules/sessionstore/SessionHistory.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "SessionStorage",
  "resource:///modules/sessionstore/SessionStorage.jsm");

Cu.import("resource:///modules/sessionstore/FrameTree.jsm", this);
let gFrameTree = new FrameTree(this);

Cu.import("resource:///modules/sessionstore/ContentRestore.jsm", this);
XPCOMUtils.defineLazyGetter(this, 'gContentRestore',
                            () => { return new ContentRestore(this) });





function createLazy(fn) {
  let cached = false;
  let cachedValue = null;

  return function lazy() {
    if (!cached) {
      cachedValue = fn();
      cached = true;
    }

    return cachedValue;
  };
}





function isSessionStorageEvent(event) {
  try {
    return event.storageArea == content.sessionStorage;
  } catch (ex if ex instanceof Ci.nsIException && ex.result == Cr.NS_ERROR_NOT_AVAILABLE) {
    
    
    return false;
  }
}





let EventListener = {

  init: function () {
    addEventListener("load", this, true);
    addEventListener("pageshow", this, true);
  },

  handleEvent: function (event) {
    switch (event.type) {
      case "load":
        
        if (event.target == content.document) {
          
          
          let epoch = gContentRestore.getRestoreEpoch();
          if (epoch) {
            
            gContentRestore.restoreDocument();

            
            sendAsyncMessage("SessionStore:restoreDocumentComplete", {epoch: epoch});
          }

          
          sendAsyncMessage("SessionStore:load");
        }
        break;
      case "pageshow":
        if (event.persisted && event.target == content.document)
          sendAsyncMessage("SessionStore:pageshow");
        break;
      default:
        debug("received unknown event '" + event.type + "'");
        break;
    }
  }
};




let MessageListener = {

  MESSAGES: [
    "SessionStore:collectSessionHistory",

    "SessionStore:restoreHistory",
    "SessionStore:restoreTabContent",
    "SessionStore:resetRestore",
  ],

  init: function () {
    this.MESSAGES.forEach(m => addMessageListener(m, this));
  },

  receiveMessage: function ({name, data}) {
    let id = data ? data.id : 0;
    switch (name) {
      case "SessionStore:collectSessionHistory":
        let history = SessionHistory.collect(docShell);
        sendAsyncMessage(name, {id: id, data: history});
        break;
      case "SessionStore:restoreHistory":
        let reloadCallback = () => {
          
          
          sendAsyncMessage("SessionStore:reloadPendingTab", {epoch: data.epoch});
        };
        gContentRestore.restoreHistory(data.epoch, data.tabData, reloadCallback);

        
        
        
        
        
        
        sendSyncMessage("SessionStore:restoreHistoryComplete", {epoch: data.epoch});
        break;
      case "SessionStore:restoreTabContent":
        let epoch = gContentRestore.getRestoreEpoch();
        let finishCallback = () => {
          
          
          sendAsyncMessage("SessionStore:restoreTabContentComplete", {epoch: epoch});
        };

        
        let didStartLoad = gContentRestore.restoreTabContent(finishCallback);

        sendAsyncMessage("SessionStore:restoreTabContentStarted", {epoch: epoch});

        if (!didStartLoad) {
          
          sendAsyncMessage("SessionStore:restoreTabContentComplete", {epoch: epoch});
          sendAsyncMessage("SessionStore:restoreDocumentComplete", {epoch: epoch});
        }
        break;
      case "SessionStore:resetRestore":
        gContentRestore.resetRestore();
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
    return SessionHistory.collect(docShell);
  },

  









  flush: function (id) {
    MessageQueue.flush(id);
  },

  





  flushAsync: function () {
    MessageQueue.flushAsync();
  }
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













let ScrollPositionListener = {
  init: function () {
    addEventListener("scroll", this);
    gFrameTree.addObserver(this);
  },

  handleEvent: function (event) {
    let frame = event.target && event.target.defaultView;

    
    
    if (frame && gFrameTree.contains(frame)) {
      MessageQueue.push("scroll", () => this.collect());
    }
  },

  onFrameTreeCollected: function () {
    MessageQueue.push("scroll", () => this.collect());
  },

  onFrameTreeReset: function () {
    MessageQueue.push("scroll", () => null);
  },

  collect: function () {
    return gFrameTree.map(ScrollPosition.collect);
  }
};


















let FormDataListener = {
  init: function () {
    addEventListener("input", this, true);
    addEventListener("change", this, true);
    gFrameTree.addObserver(this);
  },

  handleEvent: function (event) {
    let frame = event.target &&
                event.target.ownerDocument &&
                event.target.ownerDocument.defaultView;

    
    
    if (frame && gFrameTree.contains(frame)) {
      MessageQueue.push("formdata", () => this.collect());
    }
  },

  onFrameTreeReset: function () {
    MessageQueue.push("formdata", () => null);
  },

  collect: function () {
    return gFrameTree.map(FormData.collect);
  }
};












let PageStyleListener = {
  init: function () {
    Services.obs.addObserver(this, "author-style-disabled-changed", true);
    Services.obs.addObserver(this, "style-sheet-applicable-state-changed", true);
    gFrameTree.addObserver(this);
  },

  observe: function (subject, topic) {
    let frame = subject.defaultView;

    if (frame && gFrameTree.contains(frame)) {
      MessageQueue.push("pageStyle", () => this.collect());
    }
  },

  collect: function () {
    return PageStyle.collect(docShell, gFrameTree);
  },

  onFrameTreeCollected: function () {
    MessageQueue.push("pageStyle", () => this.collect());
  },

  onFrameTreeReset: function () {
    MessageQueue.push("pageStyle", () => null);
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference])
};










let DocShellCapabilitiesListener = {
  



  _latestCapabilities: "",

  init: function () {
    gFrameTree.addObserver(this);
  },

  


  onFrameTreeReset: function() {
    
    
    let caps = DocShellCapabilities.collect(docShell).join(",");

    
    if (caps != this._latestCapabilities) {
      this._latestCapabilities = caps;
      MessageQueue.push("disallow", () => caps || null);
    }
  }
};










let SessionStorageListener = {
  init: function () {
    addEventListener("MozStorageChanged", this);
    Services.obs.addObserver(this, "browser:purge-domain-data", true);
    gFrameTree.addObserver(this);
  },

  handleEvent: function (event) {
    
    if (gFrameTree.contains(event.target) && isSessionStorageEvent(event)) {
      this.collect();
    }
  },

  observe: function () {
    
    
    setTimeout(() => this.collect(), 0);
  },

  collect: function () {
    if (docShell) {
      MessageQueue.push("storage", () => SessionStorage.collect(docShell, gFrameTree));
    }
  },

  onFrameTreeCollected: function () {
    this.collect();
  },

  onFrameTreeReset: function () {
    this.collect();
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference])
};











let PrivacyListener = {
  init: function() {
    docShell.addWeakPrivacyTransitionObserver(this);

    
    
    if (docShell.QueryInterface(Ci.nsILoadContext).usePrivateBrowsing) {
      MessageQueue.push("isPrivate", () => true);
    }
  },

  
  privateModeChanged: function(enabled) {
    MessageQueue.push("isPrivate", () => enabled || null);
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIPrivacyTransitionObserver,
                                         Ci.nsISupportsWeakReference])
};








let MessageQueue = {
  




  _id: 1,

  




  _data: new Map(),

  




  _lastUpdated: new Map(),

  



  BATCH_DELAY_MS: 1000,

  



  _timeout: null,

  










  push: function (key, fn) {
    this._data.set(key, createLazy(fn));
    this._lastUpdated.set(key, this._id);

    if (!this._timeout) {
      
      this._timeout = setTimeout(() => this.send(), this.BATCH_DELAY_MS);
    }
  },

  






  send: function (options = {}) {
    
    
    
    if (!docShell) {
      return;
    }

    if (this._timeout) {
      clearTimeout(this._timeout);
      this._timeout = null;
    }

    let sync = options && options.sync;
    let startID = (options && options.id) || this._id;

    
    
    
    
    let sendMessage = sync ? sendRpcMessage : sendAsyncMessage;

    let durationMs = Date.now();

    let data = {};
    for (let [key, id] of this._lastUpdated) {
      
      
      if (!this._data.has(key)) {
        continue;
      }

      if (startID > id) {
        
        
        
        this._data.delete(key);
        continue;
      }

      data[key] = this._data.get(key)();
    }

    durationMs = Date.now() - durationMs;
    let telemetry = {
      FX_SESSION_RESTORE_CONTENT_COLLECT_DATA_LONGEST_OP_MS: durationMs
    }

    
    sendMessage("SessionStore:update", {
      id: this._id,
      data: data,
      telemetry: telemetry
    });

    
    this._id++;
  },

  








  flush: function (id) {
    
    
    
    this.send({id: id + 1, sync: true});

    this._data.clear();
    this._lastUpdated.clear();
  },

  





  flushAsync: function () {
    if (!Services.prefs.getBoolPref("browser.sessionstore.debug")) {
      throw new Error("flushAsync() must be used for testing, only.");
    }

    this.send();
  }
};

EventListener.init();
MessageListener.init();
FormDataListener.init();
SyncHandler.init();
ProgressListener.init();
PageStyleListener.init();
SessionStorageListener.init();
ScrollPositionListener.init();
DocShellCapabilitiesListener.init();
PrivacyListener.init();
