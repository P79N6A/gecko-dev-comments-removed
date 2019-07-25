



"use strict";

const EXPORTED_SYMBOLS = ["AlarmDB"];


const DEBUG = false;

function debug(aStr) {
  if (DEBUG)
    dump("AlarmDB: " + aStr + "\n");
}

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/IndexedDBHelper.jsm");

const ALARMDB_NAME    = "alarms";
const ALARMDB_VERSION = 1;
const ALARMSTORE_NAME = "alarms";

function AlarmDB(aGlobal) {
  debug("AlarmDB()");
  this._global = aGlobal;
}

AlarmDB.prototype = {
  __proto__: IndexedDBHelper.prototype,

  init: function init(aGlobal) {
    debug("init()");

    this.initDBHelper(ALARMDB_NAME, ALARMDB_VERSION, ALARMSTORE_NAME, aGlobal);
  },

  upgradeSchema: function upgradeSchema(aTransaction, aDb, aOldVersion, aNewVersion) {
    debug("upgradeSchema()");

    let objectStore = aDb.createObjectStore(ALARMSTORE_NAME, { keyPath: "id", autoIncrement: true });

    objectStore.createIndex("date",           "date",           { unique: false });
    objectStore.createIndex("ignoreTimezone", "ignoreTimezone", { unique: false });
    objectStore.createIndex("timezoneOffset", "timezoneOffset", { unique: false });
    objectStore.createIndex("data",           "data",           { unique: false });

    debug("Created object stores and indexes");
  },

  







  add: function add(aAlarm, aSuccessCb, aErrorCb) {
    debug("add()");

    this.newTxn(
      "readwrite", 
      function txnCb(aTxn, aStore) {
        debug("Going to add " + JSON.stringify(aAlarm));
        aStore.put(aAlarm).onsuccess = function setTxnResult(aEvent) {
          aTxn.result = aEvent.target.result;
          debug("Request successful. New record ID: " + aTxn.result);
        };
      },
      aSuccessCb, 
      aErrorCb
    );
  },

  







  remove: function remove(aId, aSuccessCb, aErrorCb) {
    debug("remove()");
    
    this.newTxn(
      "readwrite", 
      function txnCb(aTxn, aStore) {
        debug("Going to remove " + aId);
        aStore.delete(aId);
      }, 
      aSuccessCb, 
      aErrorCb
    );
  },

  





  getAll: function getAll(aSuccessCb, aErrorCb) {
    debug("getAll()");

    this.newTxn(
      "readonly", 
      function txnCb(aTxn, aStore) {
        if (!aTxn.result)
          aTxn.result = {};
        
        aStore.getAll().onsuccess = function setTxnResult(aEvent) {
          aTxn.result = aEvent.target.result;
          debug("Request successful. Record count: " + aTxn.result.length);
        };
      }, 
      aSuccessCb, 
      aErrorCb
    );
  }
};
