







const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;

this.EXPORTED_SYMBOLS = ["DOMRequestIpcHelper"];

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "cpmm",
                                   "@mozilla.org/childprocessmessagemanager;1",
                                   "nsIMessageListenerManager");

















this.DOMRequestIpcHelperMessageListener = function(aHelper, aWindow, aMessages) {
  this._weakHelper = Cu.getWeakReference(aHelper);

  this._messages = aMessages;
  this._messages.forEach(function(msgName) {
    cpmm.addMessageListener(msgName, this);
  }, this);

  Services.obs.addObserver(this, "inner-window-destroyed",  true);

  
  
  if (aWindow) {
    let util = aWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                      .getInterface(Ci.nsIDOMWindowUtils);
    this._innerWindowID = util.currentInnerWindowID;
  }
}

DOMRequestIpcHelperMessageListener.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIMessageListener,
                                         Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference]),

  observe: function(aSubject, aTopic, aData) {
    if (aTopic !== "inner-window-destroyed") {
      return;
    }

    let wId = aSubject.QueryInterface(Ci.nsISupportsPRUint64).data;
    if (wId != this._innerWindowID) {
      return;
    }

    this.destroy();
  },

  receiveMessage: function(aMsg) {
    let helper = this._weakHelper.get();
    if (helper) {
      helper.receiveMessage(aMsg);
    } else {
      this.destroy();
    }
  },

  destroy: function() {
    
    if (this._destroyed) {
      return;
    }
    this._destroyed = true;

    Services.obs.removeObserver(this, "inner-window-destroyed");

    this._messages.forEach(function(msgName) {
      cpmm.removeMessageListener(msgName, this);
    }, this);
    this._messages = null;

    let helper = this._weakHelper.get();
    if (helper) {
      helper.destroyDOMRequestHelper();
    }
  }
}

this.DOMRequestIpcHelper = function DOMRequestIpcHelper() {
}

DOMRequestIpcHelper.prototype = {
  



  QueryInterface: XPCOMUtils.generateQI([Ci.nsISupportsWeakReference]),

  initDOMRequestHelper: function(aWindow, aMessages) {
    this._DOMRequestIpcHelperMessageListener =
      new DOMRequestIpcHelperMessageListener(this, aWindow, aMessages);

    this._window = aWindow;
    this._requests = [];
    this._id = this._getRandomId();

    if (this._window) {
      
      let util = this._window.QueryInterface(Ci.nsIInterfaceRequestor)
                             .getInterface(Ci.nsIDOMWindowUtils);
      this.innerWindowID = util.currentInnerWindowID;
    }
  },

  getRequestId: function(aRequest) {
    let id = "id" + this._getRandomId();
    this._requests[id] = aRequest;
    return id;
  },

  getRequest: function(aId) {
    if (this._requests[aId])
      return this._requests[aId];
  },

  removeRequest: function(aId) {
    if (this._requests[aId])
      delete this._requests[aId];
  },

  takeRequest: function(aId) {
    if (!this._requests[aId])
      return null;
    let request = this._requests[aId];
    delete this._requests[aId];
    return request;
  },

  _getRandomId: function() {
    return Cc["@mozilla.org/uuid-generator;1"].getService(Ci.nsIUUIDGenerator).generateUUID().toString();
  },

  destroyDOMRequestHelper: function() {
    
    
    
    if (this._destroyed) {
      return;
    }
    this._destroyed = true;

    this._DOMRequestIpcHelperMessageListener.destroy();
    this._requests = [];
    this._window = null;

    if(this.uninit) {
      this.uninit();
    }
  },

  createRequest: function() {
    return Services.DOMRequest.createRequest(this._window);
  }
}
