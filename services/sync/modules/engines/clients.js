



































const EXPORTED_SYMBOLS = ['Clients'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/util.js");
Cu.import("resource://weave/engines.js");
Cu.import("resource://weave/stores.js");
Cu.import("resource://weave/trackers.js");
Cu.import("resource://weave/async.js");
Cu.import("resource://weave/type_records/clientData.js");

Function.prototype.async = Async.sugar;

Utils.lazy(this, 'Clients', ClientEngine);

function ClientEngine() {
  this._init();
}
ClientEngine.prototype = {
  __proto__: SyncEngine.prototype,
  name: "clients",
  displayName: "Clients",
  logName: "Clients",
  _storeObj: ClientStore,
  _trackerObj: ClientTracker,
  _recordObj: ClientRecord
};

function ClientStore() {
  this._ClientStore_init();
}
ClientStore.prototype = {
  __proto__: Store.prototype,
  _logName: "Clients.Store",

  get GUID() this._getCharPref("client.GUID", function() Utils.makeGUID()),
  set GUID(value) Utils.prefs.setCharPref("client.GUID", value),

  get name() this._getCharPref("client.name", function() "Firefox"),
  set name(value) Utils.prefs.setCharPref("client.name", value),

  get type() this._getCharPref("client.type", function() "desktop"),
  set type(value) Utils.prefs.setCharPref("client.type", value),

  
  _getCharPref: function ClientData__getCharPref(pref, defaultCb) {
    let value;
    try {
      value = Utils.prefs.getCharPref(pref);
    } catch (e) {
      value = defaultCb();
      Utils.prefs.setCharPref(pref, value);
    }
    return value;
  },

  _ClientStore_init: function ClientStore__init() {
    this._init.call(this);
    this.clients = {};
    this.loadSnapshot();
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

  remove: function ClientStore_remove(record) {
    delete this.clients[record.id];
    this.saveSnapshot();
  },

  update: function ClientStore_update(record) {
    this.clients[record.id] = record.payload;
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
    
    
    this.clients[this.GUID] = true;
    return this.clients;
  },

  itemExists: function ClientStore_itemExists(id) {
    return id in this.clients;
  },

  createRecord: function ClientStore_createRecord(guid) {
    let record = new ClientRecord();
    if (guid == this.GUID)
      record.payload = {name: this.name, type: this.type};
    else
      record.payload = this.clients[guid];
    return record;
  }
};

function ClientTracker() {
  this._init();
}
ClientTracker.prototype = {
  __proto__: Tracker.prototype,
  _logName: "ClientTracker",

  
  get score() "75", 

  
  _init: function ClientTracker__init() {
    this._log = Log4Moz.repository.getLogger(this._logName);
    this._store = new ClientStore(); 
    this.enable();
  },

  
  
  get changedIDs() {
    let items = {};
    items[this._store.GUID] = true;
    return items;
  },

  
  addChangedID: function(id) {},
  removeChangedID: function(id) {},
  clearChangedIDs: function() {}
};
