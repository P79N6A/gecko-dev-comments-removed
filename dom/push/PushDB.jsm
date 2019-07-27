




"use strict";


let gDebuggingEnabled = false;

function debug(s) {
  if (gDebuggingEnabled) {
    dump("-*- PushDB.jsm: " + s + "\n");
  }
}

const Cu = Components.utils;
Cu.import("resource://gre/modules/IndexedDBHelper.jsm");
Cu.import("resource://gre/modules/Preferences.jsm");
Cu.importGlobalProperties(["indexedDB"]);

const prefs = new Preferences("dom.push.");

this.EXPORTED_SYMBOLS = ["PushDB"];

this.PushDB = function PushDB(dbName, dbVersion, dbStoreName, schemaFunction) {
  debug("PushDB()");
  this._dbStoreName = dbStoreName;
  this._schemaFunction = schemaFunction;

  
  this.initDBHelper(dbName, dbVersion,
                    [dbStoreName]);
  gDebuggingEnabled = prefs.get("debug");
  prefs.observe("debug", this);
};

this.PushDB.prototype = {
  __proto__: IndexedDBHelper.prototype,

  upgradeSchema: function(aTransaction, aDb, aOldVersion, aNewVersion) {
    if (this._schemaFunction) {
      this._schemaFunction(aTransaction, aDb, aOldVersion, aNewVersion, this);
    }
  },

  




  put: function(aRecord) {
    debug("put()" + JSON.stringify(aRecord));

    return new Promise((resolve, reject) =>
      this.newTxn(
        "readwrite",
        this._dbStoreName,
        function txnCb(aTxn, aStore) {
          aStore.put(aRecord).onsuccess = function setTxnResult(aEvent) {
            debug("Request successful. Updated record ID: " +
                  aEvent.target.result);
          };
        },
        resolve,
        reject
      )
    );
  },

  



  delete: function(aKeyID) {
    debug("delete()");

    return new Promise((resolve, reject) =>
      this.newTxn(
        "readwrite",
        this._dbStoreName,
        function txnCb(aTxn, aStore) {
          debug("Going to delete " + aKeyID);
          aStore.delete(aKeyID);
        },
        resolve,
        reject
      )
    );
  },

  clearAll: function clear() {
    return new Promise((resolve, reject) =>
      this.newTxn(
        "readwrite",
        this._dbStoreName,
        function (aTxn, aStore) {
          debug("Going to clear all!");
          aStore.clear();
        },
        resolve,
        reject
      )
    );
  },

  getByPushEndpoint: function(aPushEndpoint) {
    debug("getByPushEndpoint()");

    return new Promise((resolve, reject) =>
      this.newTxn(
        "readonly",
        this._dbStoreName,
        function txnCb(aTxn, aStore) {
          aTxn.result = undefined;

          let index = aStore.index("pushEndpoint");
          index.get(aPushEndpoint).onsuccess = function setTxnResult(aEvent) {
            aTxn.result = aEvent.target.result;
            debug("Fetch successful " + aEvent.target.result);
          };
        },
        resolve,
        reject
      )
    );
  },

  getByKeyID: function(aKeyID) {
    debug("getByKeyID()");

    return new Promise((resolve, reject) =>
      this.newTxn(
        "readonly",
        this._dbStoreName,
        function txnCb(aTxn, aStore) {
          aTxn.result = undefined;

          aStore.get(aKeyID).onsuccess = function setTxnResult(aEvent) {
            aTxn.result = aEvent.target.result;
            debug("Fetch successful " + aEvent.target.result);
          };
        },
        resolve,
        reject
      )
    );
  },

  
  getByIdentifiers: function(aPageRecord) {
    debug("getByIdentifiers() { " + aPageRecord.scope + ", " +
          JSON.stringify(aPageRecord.originAttributes) + " }");
    if (!aPageRecord.scope || aPageRecord.originAttributes == undefined) {
      return Promise.reject(
               new TypeError("Scope and originAttributes are required! " +
                             JSON.stringify(aPageRecord)));
    }

    return new Promise((resolve, reject) =>
      this.newTxn(
        "readonly",
        this._dbStoreName,
        function txnCb(aTxn, aStore) {
          aTxn.result = undefined;

          let index = aStore.index("identifiers");
          let request = index.get(IDBKeyRange.only([aPageRecord.scope, aPageRecord.originAttributes]));
          request.onsuccess = function setTxnResult(aEvent) {
            aTxn.result = aEvent.target.result;
          }
        },
        resolve,
        reject
      )
    );
  },

  _getAllByKey: function(aKeyName, aKeyValue) {
    return new Promise((resolve, reject) =>
      this.newTxn(
        "readonly",
        this._dbStoreName,
        function txnCb(aTxn, aStore) {
          aTxn.result = undefined;

          let index = aStore.index(aKeyName);
          
          
          
          let getAllReq = index.mozGetAll(aKeyValue);
          getAllReq.onsuccess = function setTxnResult(aEvent) {
            aTxn.result = aEvent.target.result;
          }
        },
        resolve,
        reject
      )
    );
  },

  
  getAllByOriginAttributes: function(aOriginAttributes) {
    if (typeof aOriginAttributes !== "string") {
      return Promise.reject("Expected string!");
    }
    return this._getAllByKey("originAttributes", aOriginAttributes);
  },

  getAllKeyIDs: function() {
    debug("getAllKeyIDs()");

    return new Promise((resolve, reject) =>
      this.newTxn(
        "readonly",
        this._dbStoreName,
        function txnCb(aTxn, aStore) {
          aTxn.result = undefined;
          aStore.mozGetAll().onsuccess = function(event) {
            aTxn.result = event.target.result;
          };
        },
        resolve,
        reject
      )
    );
  },

  drop: function() {
    debug("drop()");

    return new Promise((resolve, reject) =>
      this.newTxn(
        "readwrite",
        this._dbStoreName,
        function txnCb(aTxn, aStore) {
          aStore.clear();
        },
        resolve,
        reject
      )
    );
  },

  observe: function observe(aSubject, aTopic, aData) {
    if ((aTopic == "nsPref:changed") && (aData == "dom.push.debug"))
      gDebuggingEnabled = prefs.get("debug");
  }
};
