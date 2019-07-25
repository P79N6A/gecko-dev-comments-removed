



































"use strict";

const EXPORTED_SYMBOLS = ['AppsEngine', 'AppRec'];

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://services-sync/util.js");
Cu.import("resource://services-sync/record.js");
Cu.import("resource://services-sync/engines.js");
Cu.import("resource://services-sync/constants.js");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Webapps.jsm");

function AppRec(collection, id) {
  CryptoWrapper.call(this, collection, id);
}

AppRec.prototype = {
  __proto__: CryptoWrapper.prototype,
  _logName: "Sync.Record.App"
}

Utils.deferGetSet(AppRec, "cleartext", ["value"]);

function AppStore(name) {
  Store.call(this, name);
}

AppStore.prototype = {
  __proto__: Store.prototype,

  getAllIDs: function getAllIDs() {
    let apps = DOMApplicationRegistry.getAllIDs();
    return apps;
  },

  changeItemID: function changeItemID(oldID, newID) {
    this._log.trace("AppsStore does not support changeItemID");
  },

  itemExists: function itemExists(guid) {
    return DOMApplicationRegistry.itemExists(guid);
  },

  createRecord: function createRecord(guid, collection) {
    let record = new AppRec(collection, guid);
    let app = DOMApplicationRegistry.getAppById(guid);
    
    if (app) {
      app.syncId = guid;
      let callback = Async.makeSyncCallback();
      DOMApplicationRegistry.getManifestFor(app.origin, function(aManifest) {
        app.manifest = aManifest;
        callback();
      });
      Async.waitForSyncCallback(callback);
      record.value = app;
    } else {
      record.deleted = true;
    }

    return record;
  },

  applyIncomingBatch: function applyIncomingBatch(aRecords) {
    let callback = Async.makeSyncCallback();
    DOMApplicationRegistry.updateApps(aRecords, callback);
    Async.waitForSyncCallback(callback);
    return [];
  },

  wipe: function wipe(record) {
    let callback = Async.makeSyncCallback();
    DOMApplicationRegistry.wipe(callback);
    Async.waitForSyncCallback(callback);
  }
}


function AppTracker(name) {
  Tracker.call(this, name);
  Svc.Obs.add("weave:engine:start-tracking", this);
  Svc.Obs.add("weave:engine:stop-tracking", this);
}

AppTracker.prototype = {
  __proto__: Tracker.prototype,
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver]),
  
  _enabled: false,

  observe: function(aSubject, aTopic, aData) {
    switch (aTopic) {
      case "webapps-sync-install":
      case "webapps-sync-uninstall":
        
        
        this.score += SCORE_INCREMENT_XLARGE;
        this.addChangedID(aData);
        break;
      case "weave:engine:start-tracking":
        this._enabled = true;
        Svc.Obs.add("webapps-sync-install", this);
        Svc.Obs.add("webapps-sync-uninstall", this);
        break;
      case "weave:engine:stop-tracking":
        this._enabled = false;
        Svc.Obs.remove("webapps-sync-install", this);
        Svc.Obs.remove("webapps-sync-uninstall", this);
        break;
    }
  }
}

function AppsEngine() {
  SyncEngine.call(this, "Apps");
}

AppsEngine.prototype = {
  __proto__: SyncEngine.prototype,
  _storeObj: AppStore,
  _trackerObj: AppTracker,
  _recordObj: AppRec,
  applyIncomingBatchSize: APPS_STORE_BATCH_SIZE
}
