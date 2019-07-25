



































const EXPORTED_SYMBOLS = ["Clients"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://services-sync/constants.js");
Cu.import("resource://services-sync/engines.js");
Cu.import("resource://services-sync/ext/StringBundle.js");
Cu.import("resource://services-sync/stores.js");
Cu.import("resource://services-sync/type_records/clients.js");
Cu.import("resource://services-sync/util.js");

Utils.lazy(this, "Clients", ClientEngine);

function ClientEngine() {
  SyncEngine.call(this, "Clients");

  
  this._resetClient();
}
ClientEngine.prototype = {
  __proto__: SyncEngine.prototype,
  _storeObj: ClientStore,
  _recordObj: ClientsRec,

  
  get enabled() true,

  
  get stats() {
    let stats = {
      hasMobile: this.localType == "mobile",
      names: [this.localName],
      numClients: 1,
    };

    for each (let {name, type} in this._store._remoteClients) {
      stats.hasMobile = stats.hasMobile || type == "mobile";
      stats.names.push(name);
      stats.numClients++;
    }

    return stats;
  },

  
  clearCommands: function clearCommands() {
    delete this.localCommands;
    this._tracker.addChangedID(this.localID);
  },

  
  sendCommand: function sendCommand(command, args) {
    
    let notDupe = function(other) other.command != command ||
      JSON.stringify(other.args) != JSON.stringify(args);

    
    let action = {
      command: command,
      args: args,
    };

    
    for (let [id, client] in Iterator(this._store._remoteClients)) {
      
      if (client.commands == null)
        client.commands = [action];
      
      else if (client.commands.every(notDupe))
        client.commands.push(action);
      
      else
        continue;

      this._log.trace("Client " + id + " got a new action: " + [command, args]);
      this._tracker.addChangedID(id);
    }
  },

  get localID() {
    
    let localID = Svc.Prefs.get("client.GUID", "");
    return localID == "" ? this.localID = Utils.makeGUID() : localID;
  },
  set localID(value) Svc.Prefs.set("client.GUID", value),

  get localName() {
    let localName = Svc.Prefs.get("client.name", "");
    if (localName != "")
      return localName;

    
    let user = Svc.Env.get("USER") || Svc.Env.get("USERNAME") ||
               Svc.Prefs.get("username");
    let brand = new StringBundle("chrome://branding/locale/brand.properties");
    let app = brand.get("brandShortName");

    let system = Svc.SysInfo.get("device") ||
                 Cc["@mozilla.org/network/protocol;1?name=http"]
                   .getService(Ci.nsIHttpProtocolHandler).oscpu;

    return this.localName = Str.sync.get("client.name2", [user, app, system]);
  },
  set localName(value) Svc.Prefs.set("client.name", value),

  get localType() Svc.Prefs.get("client.type", "desktop"),
  set localType(value) Svc.Prefs.set("client.type", value),

  isMobile: function isMobile(id) {
    if (this._store._remoteClients[id])
      return this._store._remoteClients[id].type == "mobile";
    return false;
  },

  
  _reconcile: function _reconcile() {
    return true;
  },

  
  _resetClient: function _resetClient() this._wipeClient(),

  _wipeClient: function _wipeClient() {
    SyncEngine.prototype._resetClient.call(this);
    this._store.wipe();
  }
};

function ClientStore(name) {
  Store.call(this, name);
}
ClientStore.prototype = {
  __proto__: Store.prototype,

  create: function create(record) this.update(record),

  update: function update(record) {
    
    if (record.id == Clients.localID)
      Clients.localCommands = record.commands;
    else
      this._remoteClients[record.id] = record.cleartext;
  },

  createRecord: function createRecord(guid, uri) {
    let record = new ClientsRec(uri);

    
    if (guid == Clients.localID) {
      record.name = Clients.localName;
      record.type = Clients.localType;
      record.commands = Clients.localCommands;
    }
    else
      record.cleartext = this._remoteClients[guid];

    return record;
  },

  itemExists: function itemExists(id) id in this.getAllIDs(),

  getAllIDs: function getAllIDs() {
    let ids = {};
    ids[Clients.localID] = true;
    for (let id in this._remoteClients)
      ids[id] = true;
    return ids;
  },

  wipe: function wipe() {
    this._remoteClients = {};
  },
};
