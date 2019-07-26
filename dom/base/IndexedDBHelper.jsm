



"use strict"

let DEBUG = 0;
let debug;
if (DEBUG) {
  debug = function (s) { dump("-*- IndexedDBHelper: " + s + "\n"); }
} else {
  debug = function (s) {}
}

const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;

this.EXPORTED_SYMBOLS = ["IndexedDBHelper"];

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

this.IndexedDBHelper = function IndexedDBHelper() {}

IndexedDBHelper.prototype = {

  
  _db: null,

  
  close: function close() {
    if (this._db) {
      this._db.close();
    }
  },

  








  open: function open(aSuccessCb, aFailureCb) {
    let self = this;
    if (DEBUG) debug("Try to open database:" + self.dbName + " " + self.dbVersion);
    let req = this.dbGlobal.indexedDB.open(this.dbName, this.dbVersion);
    req.onsuccess = function (event) {
      if (DEBUG) debug("Opened database:" + self.dbName + " " + self.dbName);
      self._db = event.target.result;
      self._db.onversionchange = function(event) {
        if (DEBUG) debug("WARNING: DB modified from a different window.");
      }
      aSuccessCb();
    };

    req.onupgradeneeded = function (aEvent) {
      if (DEBUG) {
        debug("Database needs upgrade:" + self.dbName + aEvent.oldVersion + aEvent.newVersion);
        debug("Correct new database version:" + aEvent.newVersion == this.dbVersion);
      }

      let _db = aEvent.target.result;
      self.upgradeSchema(req.transaction, _db, aEvent.oldVersion, aEvent.newVersion);
    };
    req.onerror = function (aEvent) {
      if (DEBUG) debug("Failed to open database:" + self.dbName);
      aFailureCb(aEvent.target.errorMessage);
    };
    req.onblocked = function (aEvent) {
      if (DEBUG) debug("Opening database request is blocked.");
    };
  },

  







  ensureDB: function ensureDB(aSuccessCb, aFailureCb) {
    if (this._db) {
      if (DEBUG) debug("ensureDB: already have a database, returning early.");
      aSuccessCb();
      return;
    }
    this.open(aSuccessCb, aFailureCb);
  },

  















  newTxn: function newTxn(txn_type, store_name, callback, successCb, failureCb) {
    this.ensureDB(function () {
      if (DEBUG) debug("Starting new transaction" + txn_type);
      let txn = this._db.transaction(this.dbStoreNames, txn_type);
      if (DEBUG) debug("Retrieving object store", this.dbName);
      let store = txn.objectStore(store_name);

      txn.oncomplete = function (event) {
        if (DEBUG) debug("Transaction complete. Returning to callback.");
        successCb(txn.result);
      };

      txn.onabort = function (event) {
        if (DEBUG) debug("Caught error on transaction");
        



        if (event.target.error)
            failureCb(event.target.error.name);
        else
            failureCb("UnknownError");
      };
      callback(txn, store);
    }.bind(this), failureCb);
  },

  











  initDBHelper: function initDBHelper(aDBName, aDBVersion, aDBStoreNames, aGlobal) {
    this.dbName = aDBName;
    this.dbVersion = aDBVersion;
    this.dbStoreNames = aDBStoreNames;
    this.dbGlobal = aGlobal;
  }
}
