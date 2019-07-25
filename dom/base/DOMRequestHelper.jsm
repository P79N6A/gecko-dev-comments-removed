







const Cu = Components.utils; 
const Cc = Components.classes;
const Ci = Components.interfaces;

let EXPORTED_SYMBOLS = ["DOMRequestIpcHelper"];

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "cpmm",
                                   "@mozilla.org/childprocessmessagemanager;1",
                                   "nsIMessageListenerManager");

function DOMRequestIpcHelper() {
}

DOMRequestIpcHelper.prototype = {
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

  observe: function(aSubject, aTopic, aData) {
    let wId = aSubject.QueryInterface(Ci.nsISupportsPRUint64).data;
    if (wId == this.innerWindowID) {
      Services.obs.removeObserver(this, "inner-window-destroyed");
      this._requests = [];
      this._window = null;
      this.removeMessageListener();
      if(this.uninit)
        this.uninit();
    }
  },

  initRequests: function initRequests() {
    this._requests = [];
  },

  initMessageListener: function initMessageListener(aMessages) {
    this._messages = aMessages;
    this._messages.forEach(function(msgName) {
      cpmm.addMessageListener(msgName, this);
    }, this);
  },
  
  initHelper: function(aWindow, aMessages) {
    this.initMessageListener(aMessages);
    this.initRequests();
    this._id = this._getRandomId();
    Services.obs.addObserver(this, "inner-window-destroyed", false);
    this._window = aWindow;
    let util = this._window.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindowUtils);
    this.innerWindowID = util.currentInnerWindowID;
  },

  removeMessageListener: function removeMessageListener() {
    this._messages.forEach(function(msgName) {
      cpmm.removeMessageListener(msgName, this);
    }, this);
    this._messages = null;
  },

  createRequest: function() {
    return Services.DOMRequest.createRequest(this._window);
  }
}
