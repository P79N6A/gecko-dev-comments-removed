



"use strict";

this.EXPORTED_SYMBOLS = ["AlarmDB"];


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

this.AlarmDB = function AlarmDB(aGlobal) {
  debug("AlarmDB()");
  this._global = aGlobal;
}

AlarmDB.prototype = {
  __proto__: IndexedDBHelper.prototype,

  init: function init(aGlobal) {
    debug("init()");

    this.initDBHelper(ALARMDB_NAME, ALARMDB_VERSION, [ALARMSTORE_NAME], aGlobal);
  },

  upgradeSchema: function upgradeSchema(aTransaction, aDb, aOldVersion, aNewVersion) {
    debug("upgradeSchema()");

    let objectStore = aDb.createObjectStore(ALARMSTORE_NAME, { keyPath: "id", autoIncrement: true });

    objectStore.createIndex("date",           "date",           { unique: false });
    objectStore.createIndex("ignoreTimezone", "ignoreTimezone", { unique: false });
    objectStore.createIndex("timezoneOffset", "timezoneOffset", { unique: false });
    objectStore.createIndex("data",           "data",           { unique: false });
    objectStore.createIndex("pageURL",        "pageURL",        { unique: false });
    objectStore.createIndex("manifestURL",    "manifestURL",    { unique: false });

    debug("Created object stores and indexes");
  },

  







  add: function add(aAlarm, aSuccessCb, aErrorCb) {
    debug("add()");

    this.newTxn(
      "readwrite",
      ALARMSTORE_NAME,
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

  











  remove: function remove(aId, aManifestURL, aSuccessCb, aErrorCb) {
    debug("remove()");

    this.newTxn(
      "readwrite",
      ALARMSTORE_NAME,
      function txnCb(aTxn, aStore) {
        debug("Going to remove " + aId);

        
        
        aStore.get(aId).onsuccess = function doRemove(aEvent) {
          let alarm = aEvent.target.result;

          if (!alarm) {
            debug("Alarm doesn't exist. No need to remove it.");
            return;
          }

          if (aManifestURL && aManifestURL != alarm.manifestURL) {
            debug("Cannot remove the alarm added by other apps.");
            return;
          }

          aStore.delete(aId);
        };
      },
      aSuccessCb,
      aErrorCb
    );
  },

  









  getAll: function getAll(aManifestURL, aSuccessCb, aErrorCb) {
    debug("getAll()");

    this.newTxn(
      "readonly",
      ALARMSTORE_NAME,
      function txnCb(aTxn, aStore) {
        if (!aTxn.result)
          aTxn.result = [];

        aStore.mozGetAll().onsuccess = function setTxnResult(aEvent) {
          aEvent.target.result.forEach(function addAlarm(aAlarm) {
            if (!aManifestURL || aManifestURL == aAlarm.manifestURL)
              aTxn.result.push(aAlarm);
          });

          debug("Request successful. Record count: " + aTxn.result.length);
        };
      },
      aSuccessCb,
      aErrorCb
    );
  }
};
