



"use strict";

this.EXPORTED_SYMBOLS = ["ClientID"];

const {classes: Cc, interfaces: Ci, results: Cr, utils: Cu} = Components;

Cu.import("resource://gre/modules/osfile.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "CommonUtils",
                                  "resource://services-common/utils.js");

XPCOMUtils.defineLazyGetter(this, "gDatareportingPath", () => {
  return OS.Path.join(OS.Constants.Path.profileDir, "datareporting");
});

XPCOMUtils.defineLazyGetter(this, "gStateFilePath", () => {
  return OS.Path.join(gDatareportingPath, "state.json");
});

this.ClientID = Object.freeze({
  






  getClientID: function() {
    return ClientIDImpl.getClientID();
  },

  



  resetClientID: function() {
    return ClientIDImpl.resetClientID();
  },

  



  _reset: function() {
    return ClientIDImpl._reset();
  },
});

let ClientIDImpl = {
  _clientID: null,
  _loadClientIdTask: null,
  _saveClientIdTask: null,

  _loadClientID: function () {
    if (this._loadClientIdTask) {
      return this._loadClientIdTask;
    }

    this._loadClientIdTask = this._doLoadClientID();
    let clear = () => this._loadClientIdTask = null;
    this._loadClientIdTask.then(clear, clear);
    return this._loadClientIdTask;
  },

  _doLoadClientID: Task.async(function* () {
    
    
    
    

    
    try {
      let state = yield CommonUtils.readJSON(gStateFilePath);
      if (state && 'clientID' in state && typeof(state.clientID) == 'string') {
        this._clientID = state.clientID;
        return this._clientID;
      }
    } catch (e) {
      
    }

    
    try {
      let fhrStatePath = OS.Path.join(OS.Constants.Path.profileDir, "healthreport", "state.json");
      let state = yield CommonUtils.readJSON(fhrStatePath);
      if (state && 'clientID' in state && typeof(state.clientID) == 'string') {
        this._clientID = state.clientID;
        this._saveClientID();
        return this._clientID;
      }
    } catch (e) {
      
    }

    
    this._clientID = CommonUtils.generateUUID();
    this._saveClientIdTask = this._saveClientID();

    
    
    
    
    yield this._saveClientIdTask;

    return this._clientID;
  }),

  




  _saveClientID: Task.async(function* () {
    let obj = { clientID: this._clientID };
    yield OS.File.makeDir(gDatareportingPath);
    yield CommonUtils.writeJSON(obj, gStateFilePath);
    this._saveClientIdTask = null;
  }),

  






  getClientID: function() {
    if (!this._clientID) {
      return this._loadClientID();
    }

    return Promise.resolve(this._clientID);
  },

  




  resetClientID: Task.async(function* () {
    yield this._loadClientIdTask;
    yield this._saveClientIdTask;

    this._clientID = CommonUtils.generateUUID();
    this._saveClientIdTask = this._saveClientID();
    yield this._saveClientIdTask;

    return this._clientID;
  }),

  


  _reset: Task.async(function* () {
    yield this._loadClientIdTask;
    yield this._saveClientIdTask;
    this._clientID = null;
  }),
};
