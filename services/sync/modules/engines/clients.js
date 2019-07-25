



































const EXPORTED_SYMBOLS = ['Clients'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/constants.js");
Cu.import("resource://weave/util.js");
Cu.import("resource://weave/engines.js");
Cu.import("resource://weave/stores.js");
Cu.import("resource://weave/trackers.js");
Cu.import("resource://weave/async.js");
Cu.import("resource://weave/type_records/clientData.js");

Function.prototype.async = Async.sugar;

Utils.lazy(this, 'Clients', ClientEngine);

function ClientEngine() {
  this._ClientEngine_init();
}
ClientEngine.prototype = {
  __proto__: SyncEngine.prototype,
  name: "clients",
  displayName: "Clients",
  logName: "Clients",
  _storeObj: ClientStore,
  _trackerObj: ClientTracker,
  _recordObj: ClientRecord,

  _ClientEngine_init: function ClientEngine__init() {
    this._init();
    Utils.prefs.addObserver("", this, false);
  },

  
  
  _recordLike: function SyncEngine__recordLike(a, b) {
    return false;
  },

  

  
  

  getClients: function ClientEngine_getClients() {
    return this._store.clients;
  },

  getInfo: function ClientEngine_getInfo(id) {
    return this._store.getInfo(id);
  },

  setInfo: function ClientEngine_setInfo(id, info) {
    this._store.setInfo(id, info);
    this._tracker.addChangedID(id);
  },

  

  get clientID() {
    if (!Svc.Prefs.get("client.GUID"))
      Svc.Prefs.set("client.GUID", Utils.makeGUID());
    return Svc.Prefs.get("client.GUID");
  },

  get syncID() {
    if (!Svc.Prefs.get("client.syncID"))
      Svc.Prefs.set("client.syncID", Utils.makeGUID());
    return Svc.Prefs.get("client.syncID");
  },
  set syncID(value) {
    Svc.Prefs.set("client.syncID", value);
  },
  resetSyncID: function ClientEngine_resetSyncID() {
    Svc.Prefs.reset("client.syncID");
  },

  get clientName() { return Svc.Prefs.get("client.name", "Firefox"); },
  set clientName(value) { Svc.Prefs.set("client.name", value); },

  get clientType() { return Svc.Prefs.get("client.type", "desktop"); },
  set clientType(value) { Svc.Prefs.set("client.type", value); },

  observe: function ClientEngine_observe(subject, topic, data) {
    switch (topic) {
    case "nsPref:changed":
      switch (data) {
      case "client.name":
      case "client.type":
        this._tracker.addChangedID(this.clientID);
        break;
      }
      break;
    }
  },

  _resetClient: function ClientEngine__resetClient() {
    let self = yield;
    this.resetLastSync();
    this._store.wipe();
  }
};

function ClientStore() {
  this._ClientStore_init();
}
ClientStore.prototype = {
  __proto__: Store.prototype,
  _logName: "Clients.Store",

  _ClientStore_init: function ClientStore__init() {
    this._init.call(this);
    this._clients = {};
    this.loadSnapshot();
  },

  get clients() {
    this._clients[Clients.clientID] = this.createRecord(Clients.clientID).payload;
    return this._clients;
  },

  
  

  getInfo: function ClientStore_getInfo(id) {
    return this._clients[id];
  },

  setInfo: function ClientStore_setInfo(id, info) {
    this._clients[id] = info;
    this.saveSnapshot();
  },

  

  saveSnapshot: function ClientStore_saveSnapshot() {
    this._log.debug("Saving client list to disk");
    let file = Utils.getProfileFile(
      {path: "weave/meta/clients.json", autoCreate: true});
    let out = Svc.Json.encode(this._clients);
    let [fos] = Utils.open(file, ">");
    fos.writeString(out);
    fos.close();
  },

  loadSnapshot: function ClientStore_loadSnapshot() {
    let file = Utils.getProfileFile("weave/meta/clients.json");
    if (!file.exists())
      return;
    this._log.debug("Loading client list from disk");
    try {
      let [is] = Utils.open(file, "<");
      let json = Utils.readStream(is);
      is.close();
      this._clients = Svc.Json.decode(json);
    } catch (e) {
      this._log.debug("Failed to load saved client list" + e);
    }
  },

  

  applyIncoming: function ClientStore_applyIncoming(onComplete, record) {
    let fn = function(rec) {
      let self = yield;
      if (!rec.payload)
        this.remove(rec);
      else if (!this.itemExists(rec.id))
        this.create(rec);
      else
        this.update(rec);
    };
    fn.async(this, onComplete, record);
  },

  create: function ClientStore_create(record) {
    this.update(record);
  },

  update: function ClientStore_update(record) {
    this._log.debug("Updating client " + record.id);
    this._clients[record.id] = record.payload;
    this.saveSnapshot();
  },

  remove: function ClientStore_remove(record) {
    this._log.debug("Removing client " + record.id);
    delete this._clients[record.id];
    this.saveSnapshot();
  },

  changeItemID: function ClientStore_changeItemID(oldID, newID) {
    this._clients[newID] = this._clients[oldID];
    delete this._clients[oldID];
    this.saveSnapshot();
  },

  wipe: function ClientStore_wipe() {
    this._log.debug("Wiping local clients store");
    this._clients = {};
    this.saveSnapshot();
  },

  

  getAllIDs: function ClientStore_getAllIDs() {
    return this.clients;
  },

  itemExists: function ClientStore_itemExists(id) {
    return id in this._clients;
  },

  createRecord: function ClientStore_createRecord(id) {
    let record = new ClientRecord();
    record.id = id;
    record.payload = this._clients[id] || {};

    
    if (id == Clients.clientID) {
      record.payload.name = Clients.clientName;
      record.payload.type = Clients.clientType;
    }

    return record;
  }
};

function ClientTracker() {
  this._init();
}
ClientTracker.prototype = {
  __proto__: Tracker.prototype,
  _logName: "ClientTracker",
  file: "clients",
  get score() 100 
};
