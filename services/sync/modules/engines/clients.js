



































const EXPORTED_SYMBOLS = ["Clients"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://weave/constants.js");
Cu.import("resource://weave/util.js");
Cu.import("resource://weave/engines.js");
Cu.import("resource://weave/stores.js");
Cu.import("resource://weave/type_records/clients.js");

Utils.lazy(this, "Clients", ClientEngine);

function ClientEngine() {
  SyncEngine.call(this, "Clients");

  
  this._resetClient();
}
ClientEngine.prototype = {
  __proto__: SyncEngine.prototype,
  _storeObj: ClientStore,
  _recordObj: ClientsRec,

  
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

    return this.localName = Str.sync.get("client.name", [user, app, host]);
  },
  set localName(value) Svc.Prefs.set("client.name", value),

  get localType() {
    
    let localType = Svc.Prefs.get("client.type", "");
    if (localType == "") {
      
      localType = "desktop";
      switch (Svc.AppInfo.ID) {
        case FENNEC_ID:
          localType = "mobile";
          break;
      }
      this.localType = localType;
    }
    return localType;
  },
  set localType(value) Svc.Prefs.set("client.type", value),

  
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
    
    if (record.id == Clients.localID) {
      Clients.localName = record.name;
      Clients.localType = record.type;
      Clients.localCommands = record.commands;
    }
    else
      this._remoteClients[record.id] = record.cleartext;
  },

  createRecord: function createRecord(guid) {
    let record = new ClientsRec();

    
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
