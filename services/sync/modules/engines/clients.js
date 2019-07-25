



































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

  updateLocalInfo: function ClientEngine_updateLocalInfo(info) {
    
    if (!info)
      info = this.getInfo(this.clientID);

    
    info.name = this.clientName;
    info.type = this.clientType;

    return info;
  },

  observe: function ClientEngine_observe(subject, topic, data) {
    switch (topic) {
    case "nsPref:changed":
      switch (data) {
      case "client.name":
      case "client.type":
        
        this.setInfo(this.clientID, this.updateLocalInfo());
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
  this.init();
}
ClientStore.prototype = {
  
  

  clients: {},

  __proto__: Store.prototype,

  _snapshot: "meta/clients",

  
  

  


  getInfo: function ClientStore_getInfo(id) this.clients[id],

  


  init: function ClientStore_init() {
    this._init.call(this);
    this.loadSnapshot();

    
    let id = Clients.clientID;
    this.setInfo(id, Clients.updateLocalInfo(this.clients[id] || {}));
  },

  


  loadSnapshot: function ClientStore_loadSnapshot() {
    Utils.jsonLoad(this._snapshot, this, function(json) this.clients = json);
  },

  


  modify: function ClientStore_modify(message, action) {
    this._log.debug(message);
    action.call(this);
    this.saveSnapshot();
  },

  


  saveSnapshot: function ClientStore_saveSnapshot() {
    Utils.jsonSave(this._snapshot, this, this.clients);
  },

  


  setInfo: function ClientStore_setInfo(id, info) {
    this.modify("Setting client " + id + ": " + JSON.stringify(info),
      function() this.clients[id] = info);
  },

  
  

  _logName: "Clients.Store",

  
  

  changeItemID: function ClientStore_changeItemID(oldID, newID) {
    this.modify("Changing id from " + oldId + " to " + newID, function() {
      this.clients[newID] = this.clients[oldID];
      delete this.clients[oldID];
    });
  },

  create: function ClientStore_create(record) {
    this.update(record);
  },

  createRecord: function ClientStore_createRecord(id) {
    let record = new ClientRecord();
    record.id = id;
    record.payload = this.clients[id];

    return record;
  },

  getAllIDs: function ClientStore_getAllIDs() this.clients,

  itemExists: function ClientStore_itemExists(id) id in this.clients,

  remove: function ClientStore_remove(record) {
    this.modify("Removing client " + record.id, function()
      delete this.clients[record.id]);
  },

  update: function ClientStore_update(record) {
    this.modify("Updating client " + record.id, function()
      this.clients[record.id] = record.payload);
  },

  wipe: function ClientStore_wipe() {
    this.modify("Wiping local clients store", function() this.clients = {});
  },
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
