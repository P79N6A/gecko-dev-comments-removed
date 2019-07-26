



"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

function debug(aMsg) {
  
}

function InterAppMessagePort() {
  debug("InterAppMessagePort()");
};

InterAppMessagePort.prototype = {
  classDescription: "MozInterAppMessagePort",

  classID: Components.ID("{c66e0f8c-e3cb-11e2-9e85-43ef6244b884}"),

  contractID: "@mozilla.org/dom/inter-app-message-port;1",

  QueryInterface: XPCOMUtils.generateQI([Ci.nsISupports]),

  
  __init: function(aKeyword, aMessagePortID, aIsPublisher) {
    debug("Calling __init(): aKeyword: " + aKeyword +
          " aMessagePortID: " + aMessagePortID +
          " aIsPublisher: " + aIsPublisher);
  },

  postMessage: function(aMessage) {
    
  },

  start: function() {
    
  },

  close: function() {
    
  },

  get onmessage() {
    return this.__DOM_IMPL__.getEventHandler("onmessage");
  },

  set onmessage(aHandler) {
    this.__DOM_IMPL__.setEventHandler("onmessage", aHandler);
  }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([InterAppMessagePort]);

