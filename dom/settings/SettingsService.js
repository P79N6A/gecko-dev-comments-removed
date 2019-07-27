



"use strict";

const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import('resource://gre/modules/SettingsRequestManager.jsm');


let DEBUG = false;
let VERBOSE = false;

try {
  DEBUG   =
    Services.prefs.getBoolPref("dom.mozSettings.SettingsService.debug.enabled");
  VERBOSE =
    Services.prefs.getBoolPref("dom.mozSettings.SettingsService.verbose.enabled");
} catch (ex) { }

function debug(s) {
  dump("-*- SettingsService: " + s + "\n");
}

XPCOMUtils.defineLazyServiceGetter(this, "uuidgen",
                                   "@mozilla.org/uuid-generator;1",
                                   "nsIUUIDGenerator");
XPCOMUtils.defineLazyServiceGetter(this, "cpmm",
                                   "@mozilla.org/childprocessmessagemanager;1",
                                   "nsIMessageSender");

const nsIClassInfo            = Ci.nsIClassInfo;

const SETTINGSSERVICELOCK_CONTRACTID = "@mozilla.org/settingsServiceLock;1";
const SETTINGSSERVICELOCK_CID        = Components.ID("{d7a395a0-e292-11e1-834e-1761d57f5f99}");
const nsISettingsServiceLock         = Ci.nsISettingsServiceLock;

function makeSettingsServiceRequest(aCallback, aName, aValue) {
  return {
    callback: aCallback,
    name: aName,
    value: aValue
  };
};

function SettingsServiceLock(aSettingsService, aTransactionCallback) {
  if (VERBOSE) debug("settingsServiceLock constr!");
  this._open = true;
  this._settingsService = aSettingsService;
  this._id = uuidgen.generateUUID().toString();
  this._transactionCallback = aTransactionCallback;
  this._requests = {};
  let closeHelper = function() {
    if (VERBOSE) debug("closing lock " + this._id);
    this._open = false;
    this.runOrFinalizeQueries();
  }.bind(this);

  let msgs =   ["Settings:Get:OK", "Settings:Get:KO",
                "Settings:Clear:OK", "Settings:Clear:KO",
                "Settings:Set:OK", "Settings:Set:KO",
                "Settings:Finalize:OK", "Settings:Finalize:KO"];

  for (let msg in msgs) {
    cpmm.addMessageListener(msgs[msg], this);
  }

  cpmm.sendAsyncMessage("Settings:CreateLock",
                        { lockID: this._id,
                          isServiceLock: true,
                          windowID: undefined },
                        undefined,
                        Services.scriptSecurityManager.getSystemPrincipal());
  Services.tm.currentThread.dispatch(closeHelper, Ci.nsIThread.DISPATCH_NORMAL);
}

SettingsServiceLock.prototype = {
  get closed() {
    return !this._open;
  },

  runOrFinalizeQueries: function() {
    if (!this._requests || Object.keys(this._requests).length == 0) {
      cpmm.sendAsyncMessage("Settings:Finalize", {lockID: this._id}, undefined, Services.scriptSecurityManager.getSystemPrincipal());
    } else {
      cpmm.sendAsyncMessage("Settings:Run", {lockID: this._id}, undefined, Services.scriptSecurityManager.getSystemPrincipal());
    }
  },

  receiveMessage: function(aMessage) {

    let msg = aMessage.data;
    
    
    if(msg.lockID != this._id) {
      return;
    }
    if (VERBOSE) debug("receiveMessage (" + this._id + "): " + aMessage.name);
    
    
    if (!msg.requestID) {
      switch (aMessage.name) {
        case "Settings:Finalize:OK":
          if (VERBOSE) debug("Lock finalize ok!");
          this.callTransactionHandle();
          break;
        case "Settings:Finalize:KO":
          if (DEBUG) debug("Lock finalize failed!");
          this.callAbort();
          break;
        default:
          if (DEBUG) debug("Message type " + aMessage.name + " is missing a requestID");
      }
      return;
    }

    let req = this._requests[msg.requestID];
    if (!req) {
      if (DEBUG) debug("Matching request not found.");
      return;
    }
    delete this._requests[msg.requestID];
    switch (aMessage.name) {
      case "Settings:Get:OK":
        this._open = true;
        let settings_names = Object.keys(msg.settings);
        if (settings_names.length > 0) {
          let name = settings_names[0];        
          if (DEBUG && settings_names.length > 1) {
            debug("Warning: overloaded setting:" + name);
          }
          let result = msg.settings[name];
          this.callHandle(req.callback, name, result);
        } else {
          this.callHandle(req.callback, req.name, null);
        }
        this._open = false;
        break;
      case "Settings:Set:OK":
        this._open = true;
        
        this.callHandle(req.callback, req.name, req.value);
        this._open = false;
        break;
      case "Settings:Get:KO":
      case "Settings:Set:KO":
        if (DEBUG) debug("error:" + msg.errorMsg);
        this.callError(req.callback, msg.error);
        break;
      default:
        if (DEBUG) debug("Wrong message: " + aMessage.name);
    }
    this.runOrFinalizeQueries();
  },

  get: function get(aName, aCallback) {
    if (VERBOSE) debug("get (" + this._id + "): " + aName);
    if (!this._open) {
      dump("Settings lock not open!\n");
      throw Components.results.NS_ERROR_ABORT;
    }
    let reqID = uuidgen.generateUUID().toString();
    this._requests[reqID] = makeSettingsServiceRequest(aCallback, aName);
    cpmm.sendAsyncMessage("Settings:Get", {requestID: reqID,
                                           lockID: this._id,
                                           name: aName},
                                           undefined,
                                           Services.scriptSecurityManager.getSystemPrincipal());
  },

  set: function set(aName, aValue, aCallback) {
    if (VERBOSE) debug("set: " + aName + " " + aValue);
    if (!this._open) {
      throw "Settings lock not open";
    }
    let reqID = uuidgen.generateUUID().toString();
    this._requests[reqID] = makeSettingsServiceRequest(aCallback, aName, aValue);
    let settings = {};
    settings[aName] = aValue;
    cpmm.sendAsyncMessage("Settings:Set", {requestID: reqID,
                                           lockID: this._id,
                                           settings: settings},
                                           undefined,
                                           Services.scriptSecurityManager.getSystemPrincipal());
  },

  callHandle: function callHandle(aCallback, aName, aValue) {
    try {
      aCallback ? aCallback.handle(aName, aValue) : null;
    } catch (e) {
      dump("settings 'handle' callback threw an exception, dropping: " + e + "\n");
    }
  },

  callAbort: function callAbort(aCallback, aMessage) {
    try {
      aCallback ? aCallback.handleAbort(aMessage) : null;
    } catch (e) {
      dump("settings 'abort' callback threw an exception, dropping: " + e + "\n");
    }
  },

  callError: function callError(aCallback, aMessage) {
    try {
      aCallback ? aCallback.handleError(aMessage) : null;
    } catch (e) {
      dump("settings 'error' callback threw an exception, dropping: " + e + "\n");
    }
  },

  callTransactionHandle: function callTransactionHandle() {
    try {
      this._transactionCallback ? this._transactionCallback.handle() : null;
    } catch (e) {
      dump("settings 'Transaction handle' callback threw an exception, dropping: " + e + "\n");
    }
  },

  classID : SETTINGSSERVICELOCK_CID,
  QueryInterface : XPCOMUtils.generateQI([nsISettingsServiceLock])
};

const SETTINGSSERVICE_CID        = Components.ID("{f656f0c0-f776-11e1-a21f-0800200c9a66}");

function SettingsService()
{
  if (VERBOSE) debug("settingsService Constructor");
}

SettingsService.prototype = {

  createLock: function createLock(aCallback) {
    var lock = new SettingsServiceLock(this, aCallback);
    return lock;
  },

  classID : SETTINGSSERVICE_CID,
  QueryInterface : XPCOMUtils.generateQI([Ci.nsISettingsService])
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([SettingsService, SettingsServiceLock]);
