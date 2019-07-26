










"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/DOMRequestHelper.jsm");
Cu.import("resource://gre/modules/ObjectWrapper.jsm");

const DEBUG = false;
function debug(aMsg) {
  dump("-- InterAppMessagePort: " + Date.now() + ": " + aMsg + "\n");
}

XPCOMUtils.defineLazyServiceGetter(this, "cpmm",
                                   "@mozilla.org/childprocessmessagemanager;1",
                                   "nsIMessageSender");

XPCOMUtils.defineLazyServiceGetter(this, "appsService",
                                   "@mozilla.org/AppsService;1",
                                   "nsIAppsService");

const kMessages = ["InterAppMessagePort:OnMessage"];

function InterAppMessagePort() {
  if (DEBUG) debug("InterAppMessagePort()");
};

InterAppMessagePort.prototype = {
  __proto__: DOMRequestIpcHelper.prototype,

  classDescription: "MozInterAppMessagePort",

  classID: Components.ID("{c66e0f8c-e3cb-11e2-9e85-43ef6244b884}"),

  contractID: "@mozilla.org/dom/inter-app-message-port;1",

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIDOMGlobalPropertyInitializer,
                                         Ci.nsISupportsWeakReference]),

  
  init: function(aWindow) {
    if (DEBUG) debug("Calling init().");

    this.initDOMRequestHelper(aWindow, kMessages);

    let principal = aWindow.document.nodePrincipal;
    this._manifestURL = appsService.getManifestURLByLocalId(principal.appId);
    this._pageURL = principal.URI.spec;

    this._started = false;
    this._closed = false;
    this._messageQueue = [];
  },

  
  __init: function(aMessagePortID) {
    if (DEBUG) {
      debug("Calling __init(): aMessagePortID: " + aMessagePortID);
    }

    this._messagePortID = aMessagePortID;

    cpmm.sendAsyncMessage("InterAppMessagePort:Register",
                          { messagePortID: this._messagePortID,
                            manifestURL: this._manifestURL,
                            pageURL: this._pageURL });
  },

  
  uninit: function() {
    if (DEBUG) debug("Calling uninit().");

    
    
    if (this._closed) {
      if (DEBUG) debug("close() has been called. Don't need to close again.");
      return;
    }

    this.close();
  },

  postMessage: function(aMessage) {
    if (DEBUG) debug("Calling postMessage().");

    if (this._closed) {
      if (DEBUG) debug("close() has been called. Cannot post message.");
      return;
    }

    cpmm.sendAsyncMessage("InterAppMessagePort:PostMessage",
                          { messagePortID: this._messagePortID,
                            manifestURL: this._manifestURL,
                            message: aMessage });
  },

  start: function() {
    
    if (DEBUG) debug("Calling start().");

    if (this._closed) {
      if (DEBUG) debug("close() has been called. Cannot call start().");
      return;
    }

    if (this._started) {
      if (DEBUG) debug("start() has been called. Don't need to start again.");
      return;
    }

    
    
    this._started = true;
    while (this._messageQueue.length) {
      let message = this._messageQueue.shift();
      this._dispatchMessage(message);
    }
  },

  close: function() {
    
    if (DEBUG) debug("Calling close().");

    if (this._closed) {
      if (DEBUG) debug("close() has been called. Don't need to close again.");
      return;
    }

    this._closed = true;
    this._messageQueue.length = 0;

    
    
    cpmm.sendAsyncMessage("InterAppMessagePort:Unregister",
                          { messagePortID: this._messagePortID,
                            manifestURL: this._manifestURL });
  },

  get onmessage() {
    if (DEBUG) debug("Getting onmessage handler.");

    return this.__DOM_IMPL__.getEventHandler("onmessage");
  },

  set onmessage(aHandler) {
    if (DEBUG) debug("Setting onmessage handler.");

    this.__DOM_IMPL__.setEventHandler("onmessage", aHandler);

    
    
    
    if (this._started) {
      if (DEBUG) debug("start() has been called. Don't need to start again.");
      return;
    }

    this.start();
  },

  _dispatchMessage: function _dispatchMessage(aMessage) {
    let wrappedMessage = ObjectWrapper.wrap(aMessage, this._window);
    if (DEBUG) {
      debug("_dispatchMessage: wrappedMessage: " +
            JSON.stringify(wrappedMessage));
    }

    let event = new this._window
                    .MozInterAppMessageEvent("message",
                                             { data: wrappedMessage });
    this.__DOM_IMPL__.dispatchEvent(event);
  },

  receiveMessage: function(aMessage) {
    if (DEBUG) debug("receiveMessage: name: " + aMessage.name);

    let message = aMessage.json;
    if (message.manifestURL != this._manifestURL ||
        message.pageURL != this._pageURL ||
        message.messagePortID != this._messagePortID) {
      if (DEBUG) debug("The message doesn't belong to this page. Returning.");
      return;
    }

    switch (aMessage.name) {
      case "InterAppMessagePort:OnMessage":
        if (this._closed) {
          if (DEBUG) debug("close() has been called. Drop the message.");
          return;
        }

        if (!this._started) {
          if (DEBUG) debug("Not yet called start(). Queue up the message.");
          this._messageQueue.push(message.message);
          return;
        }

        this._dispatchMessage(message.message);
        break;

      default:
        if (DEBUG) debug("Error! Shouldn't fall into this case.");
        break;
    }
  }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([InterAppMessagePort]);

