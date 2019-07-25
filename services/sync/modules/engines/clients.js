



































const EXPORTED_SYMBOLS = ['Clients'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

Cu.import("resource://weave/ext/Preferences.js");
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
    if (!this.getInfo(this.clientID))
      this.setInfo(this.clientID, {name: "Firefox", type: "desktop"});
  },

  
  _createRecord: function SyncEngine__createRecord(id) {
    let record = this._store.createRecord(id);
    record.uri = this.engineURL + id;
    return record;
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
    if (!Preferences.get(PREFS_BRANCH + "client.GUID"))
      Preferences.set(PREFS_BRANCH + "client.GUID", Utils.makeGUID());
    return Preferences.get(PREFS_BRANCH + "client.GUID");
  },

  get clientName() { return this.getInfo(this.clientID).name; },
  set clientName(value) {
    let info = this.getInfo(this.clientID);
    info.name = value;
    this.setInfo(this.clientID, info);
  },

  get clientType() { return this.getInfo(this.clientID).type; },
  set clientType(value) {
    let info = this.getInfo(this.clientID);
    info.type = value;
    this.setInfo(this.clientID, info);
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
    this.clients = {};
    this.loadSnapshot();
  },

  
  

  getInfo: function ClientStore_getInfo(id) {
    return this.clients[id];
  },

  setInfo: function ClientStore_setInfo(id, info) {
    this.clients[id] = info;
    this.saveSnapshot();
  },

  

  saveSnapshot: function ClientStore_saveSnapshot() {
    this._log.debug("Saving client list to disk");
    let file = Utils.getProfileFile(
      {path: "weave/meta/clients.json", autoCreate: true});
    let out = Svc.Json.encode(this.clients);
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
      this.clients = Svc.Json.decode(json);
    } catch (e) {
      this._log.debug("Failed to load saved client list" + e);
    }
  },

  

  create: function ClientStore_create(record) {
    this.update(record);
  },

  update: function ClientStore_update(record) {
    this.clients[record.id] = record.payload;
    this.saveSnapshot();
  },

  remove: function ClientStore_remove(record) {
    delete this.clients[record.id];
    this.saveSnapshot();
  },

  changeItemID: function ClientStore_changeItemID(oldID, newID) {
    this.clients[newID] = this.clients[oldID];
    delete this.clients[oldID];
    this.saveSnapshot();
  },

  wipe: function ClientStore_wipe() {
    this.clients = {};
    this.saveSnapshot();
  },

  

  getAllIDs: function ClientStore_getAllIDs() {
    return this.clients;
  },

  itemExists: function ClientStore_itemExists(id) {
    return id in this.clients;
  },

  createRecord: function ClientStore_createRecord(id) {
    let record = new ClientRecord();
    record.payload = this.clients[id];
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
  get score() "75" 
};
