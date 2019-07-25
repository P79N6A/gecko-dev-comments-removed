



































const EXPORTED_SYMBOLS = ['Clients'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://weave/util.js");
Cu.import("resource://weave/engines.js");
Cu.import("resource://weave/stores.js");
Cu.import("resource://weave/trackers.js");
Cu.import("resource://weave/type_records/clientData.js");

Utils.lazy(this, 'Clients', ClientEngine);

function ClientEngine() {
  SyncEngine.call(this, "Clients");

  
  this._resetClient();
  Utils.prefs.addObserver("", this, false);
}
ClientEngine.prototype = {
  __proto__: SyncEngine.prototype,
  _storeObj: ClientStore,
  _trackerObj: ClientTracker,
  _recordObj: ClientRecord,

  

  
  

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

  get clientName() {
    if (Svc.Prefs.isSet("client.name"))
      return Svc.Prefs.get("client.name");

    
    let user = Svc.Env.get("USER") || Svc.Env.get("USERNAME");
    let app = Svc.AppInfo.name;
    let host = Svc.SysInfo.get("host");

    
    let prof = Svc.Directory.get("ProfD", Components.interfaces.nsIFile).path;
    let profiles = Svc.Profiles.profiles;
    while (profiles.hasMoreElements()) {
      let profile = profiles.getNext().QueryInterface(Ci.nsIToolkitProfile);
      if (prof == profile.rootDir.path) {
        
        if (profile.name != "default")
          host = profile.name + "-" + host;
        break;
      }
    }

    return this.clientName = Str.sync.get("client.name", [user, app, host]);
  },
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

  
  _reconcile: function _reconcile() {
    return true;
  },

  _resetClient: function ClientEngine__resetClient() {
    SyncEngine.prototype._resetClient.call(this);
    this._store.wipe();

    
    this.setInfo(this.clientID, this.updateLocalInfo({}));
  }
};

function ClientStore(name) {
  Store.call(this, name);
}
ClientStore.prototype = {
  
  

  clients: {},

  __proto__: Store.prototype,

  
  

  


  getInfo: function ClientStore_getInfo(id) this.clients[id],

  


  setInfo: function ClientStore_setInfo(id, info) {
    this._log.debug("Setting client " + id + ": " + JSON.stringify(info));
    this.clients[id] = info;
  },

  
  

  changeItemID: function ClientStore_changeItemID(oldID, newID) {
    this._log.debug("Changing id from " + oldId + " to " + newID);
    this.clients[newID] = this.clients[oldID];
    delete this.clients[oldID];
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
    this._log.debug("Removing client " + record.id);
    delete this.clients[record.id];
  },

  update: function ClientStore_update(record) {
    this._log.debug("Updating client " + record.id);
    this.clients[record.id] = record.payload;
  },

  wipe: function ClientStore_wipe() {
    this._log.debug("Wiping local clients store")
    this.clients = {};
  },
};

function ClientTracker(name) {
  Tracker.call(this, name);
}
ClientTracker.prototype = {
  __proto__: Tracker.prototype,
  get score() 100 
};
