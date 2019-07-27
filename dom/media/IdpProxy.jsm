



"use strict";

this.EXPORTED_SYMBOLS = ["IdpProxy"];

const {
  classes: Cc,
  interfaces: Ci,
  utils: Cu,
  results: Cr
} = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Sandbox",
                                  "resource://gre/modules/identity/Sandbox.jsm");








function IdpChannel(uri, messageCallback) {
  this.sandbox = null;
  this.messagechannel = null;
  this.source = uri;
  this.messageCallback = messageCallback;
}

IdpChannel.prototype = {
  







  open: function(callback) {
    if (this.sandbox) {
      return callback(new Error("IdP channel already open"));
    }

    let ready = this._sandboxReady.bind(this, callback);
    this.sandbox = new Sandbox(this.source, ready);
  },

  _sandboxReady: function(aCallback, aSandbox) {
    
    try {
      this.messagechannel = new aSandbox._frame.contentWindow.MessageChannel();
      Object.defineProperty(
        aSandbox._frame.contentWindow.wrappedJSObject,
        "rtcwebIdentityPort",
        {
          value: this.messagechannel.port2,
          configurable: true
        }
      );
    } catch (e) {
      this.close();
      aCallback(e); 
      return;
    }
    this.messagechannel.port1.onmessage = function(msg) {
      this.messageCallback(msg.data);
    }.bind(this);
    this.messagechannel.port1.start();
    aCallback();
  },

  send: function(msg) {
    this.messagechannel.port1.postMessage(msg);
  },

  close: function IdpChannel_close() {
    if (this.sandbox) {
      if (this.messagechannel) {
        this.messagechannel.port1.close();
      }
      this.sandbox.free();
    }
    this.messagechannel = null;
    this.sandbox = null;
  }
};







function IdpProxy(domain, protocol) {
  IdpProxy.validateDomain(domain);
  IdpProxy.validateProtocol(protocol);

  this.domain = domain;
  this.protocol = protocol || "default";

  this._reset();
}





IdpProxy.validateDomain = function(domain) {
  let message = "Invalid domain for identity provider; ";
  if (!domain || typeof domain !== "string") {
    throw new Error(message + "must be a non-zero length string");
  }

  message += "must only have a domain name and optionally a port";
  try {
    let ioService = Components.classes["@mozilla.org/network/io-service;1"]
                    .getService(Components.interfaces.nsIIOService);
    let uri = ioService.newURI('https://' + domain + '/', null, null);

    
    
    if (uri.hostPort !== domain) {
      throw new Error(message);
    }
  } catch (e if (e.result === Cr.NS_ERROR_MALFORMED_URI)) {
    throw new Error(message);
  }
};






IdpProxy.validateProtocol = function(protocol) {
  if (!protocol) {
    return;  
  }
  let message = "Invalid protocol for identity provider; ";
  if (typeof protocol !== "string") {
    throw new Error(message + "must be a string");
  }
  if (decodeURIComponent(protocol).match(/[\/\\]/)) {
    throw new Error(message + "must not include '/' or '\\'");
  }
};

IdpProxy.prototype = {
  _reset: function() {
    this.channel = null;
    this.ready = false;

    this.counter = 0;
    this.tracking = {};
    this.pending = [];
  },

  isSame: function(domain, protocol) {
    return this.domain === domain && ((protocol || "default") === this.protocol);
  },

  






  start: function(errorCallback) {
    if (this.channel) {
      return;
    }
    let well_known = "https://" + this.domain;
    well_known += "/.well-known/idp-proxy/" + this.protocol;
    this.channel = new IdpChannel(well_known, this._messageReceived.bind(this));
    this.channel.open(function(error) {
      if (error) {
        this.close();
        if (typeof errorCallback === "function") {
          errorCallback(error);
        }
      }
    }.bind(this));
  },

  











  send: function(message, callback) {
    this.start();
    if (this.ready) {
      message.id = "" + (++this.counter);
      this.tracking[message.id] = callback;
      this.channel.send(message);
    } else {
      this.pending.push({ message: message, callback: callback });
    }
  },

  



  _messageReceived: function(message) {
    if (!message) {
      return;
    }
    if (!this.ready && message.type === "READY") {
      this.ready = true;
      this.pending.forEach(function(p) {
        this.send(p.message, p.callback);
      }, this);
      this.pending = [];
    } else if (this.tracking[message.id]) {
      var callback = this.tracking[message.id];
      delete this.tracking[message.id];
      callback(message);
    } else {
      let console = Cc["@mozilla.org/consoleservice;1"].
        getService(Ci.nsIConsoleService);
      console.logStringMessage("Received bad message from IdP: " +
                               message.id + ":" + message.type);
    }
  },

  


  close: function() {
    if (!this.channel) {
      return;
    }

    
    let trackingCopy = this.tracking;
    let pendingCopy = this.pending;

    this.channel.close();
    this._reset();

    
    
    let error = { type: "ERROR", error: "IdP closed" };
    Object.keys(trackingCopy).forEach(function(k) {
      trackingCopy[k](error);
    });
    pendingCopy.forEach(function(p) {
      p.callback(error);
    });
  },

  toString: function() {
    return this.domain + '/.../' + this.protocol;
  }
};

this.IdpProxy = IdpProxy;
