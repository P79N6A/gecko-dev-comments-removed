



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
XPCOMUtils.defineLazyModuleGetter(this, "PageStyle",
  "resource:///modules/sessionstore/PageStyle.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "ScrollPosition",
  "resource:///modules/sessionstore/ScrollPosition.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "SessionHistory",
  "resource:///modules/sessionstore/SessionHistory.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "SessionStorage",
  "resource:///modules/sessionstore/SessionStorage.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "TextAndScrollData",
  "resource:///modules/sessionstore/TextAndScrollData.jsm");

Cu.import("resource:///modules/sessionstore/FrameTree.jsm", this);
let gFrameTree = new FrameTree(this);





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

  DOM_EVENTS: [
    "pageshow", "change", "input"
  ],

  init: function () {
    this.DOM_EVENTS.forEach(e => addEventListener(e, this, true));
  },

  handleEvent: function (event) {
    switch (event.type) {
      case "pageshow":
        if (event.persisted && event.target == content.document)
          sendAsyncMessage("SessionStore:pageshow");
        break;
      case "input":
      case "change":
        sendAsyncMessage("SessionStore:input");
        break;
      default:
        debug("received unknown event '" + event.type + "'");
        break;
    }
  }
};




let MessageListener = {

  MESSAGES: [
    "SessionStore:collectSessionHistory"
  ],

  init: function () {
    this.MESSAGES.forEach(m => addMessageListener(m, this));
  },

  receiveMessage: function ({name, data: {id}}) {
    switch (name) {
      case "SessionStore:collectSessionHistory":
        let history = SessionHistory.collect(docShell);
        if ("index" in history) {
          let tabIndex = history.index - 1;
          
          
          TextAndScrollData.updateFrame(history.entries[tabIndex],
                                        content,
                                        docShell.isAppTab);
        }
        sendAsyncMessage(name, {id: id, data: history});
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
    let history = SessionHistory.collect(docShell);
    if ("index" in history) {
      let tabIndex = history.index - 1;
      TextAndScrollData.updateFrame(history.entries[tabIndex],
                                    content,
                                    docShell.isAppTab,
                                    {includePrivateData: includePrivateData});
    }
    return history;
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

  onFrameTreeReset: function () {
    MessageQueue.push("scroll", () => null);
  },

  collect: function () {
    return gFrameTree.map(ScrollPosition.collect);
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

  onFrameTreeReset: function () {
    MessageQueue.push("pageStyle", () => null);
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference])
};










let DocShellCapabilitiesListener = {
  



  _latestCapabilities: "",

  init: function () {
    let webProgress = docShell.QueryInterface(Ci.nsIInterfaceRequestor)
                              .getInterface(Ci.nsIWebProgress);

    webProgress.addProgressListener(this, Ci.nsIWebProgress.NOTIFY_LOCATION);
  },

  




  onLocationChange: function() {
    
    
    let caps = DocShellCapabilities.collect(docShell).join(",");

    
    if (caps != this._latestCapabilities) {
      this._latestCapabilities = caps;
      MessageQueue.push("disallow", () => caps || null);
    }
  },

  onStateChange: function () {},
  onProgressChange: function () {},
  onStatusChange: function () {},
  onSecurityChange: function () {},

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIWebProgressListener,
                                         Ci.nsISupportsWeakReference])
};










let SessionStorageListener = {
  init: function () {
    addEventListener("MozStorageChanged", this);
    Services.obs.addObserver(this, "browser:purge-domain-data", true);
    Services.obs.addObserver(this, "browser:purge-session-history", true);
  },

  handleEvent: function (event) {
    
    if (isSessionStorageEvent(event)) {
      this.collect();
    }
  },

  observe: function () {
    
    
    setTimeout(() => this.collect(), 0);
  },

  collect: function () {
    MessageQueue.push("storage", () => SessionStorage.collect(docShell));
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference])
};











let PrivacyListener = {
  init: function() {
    docShell.addWeakPrivacyTransitionObserver(this);
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
SyncHandler.init();
ProgressListener.init();
PageStyleListener.init();
SessionStorageListener.init();
ScrollPositionListener.init();
DocShellCapabilitiesListener.init();
PrivacyListener.init();
