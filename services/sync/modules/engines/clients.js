



this.EXPORTED_SYMBOLS = [
  "ClientEngine",
  "ClientsRec"
];

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://services-common/stringbundle.js");
Cu.import("resource://services-sync/constants.js");
Cu.import("resource://services-sync/engines.js");
Cu.import("resource://services-sync/record.js");
Cu.import("resource://services-sync/util.js");

const CLIENTS_TTL = 1814400; 
const CLIENTS_TTL_REFRESH = 604800; 

const SUPPORTED_PROTOCOL_VERSIONS = ["1.1", "1.5"];

this.ClientsRec = function ClientsRec(collection, id) {
  CryptoWrapper.call(this, collection, id);
}
ClientsRec.prototype = {
  __proto__: CryptoWrapper.prototype,
  _logName: "Sync.Record.Clients",
  ttl: CLIENTS_TTL
};

Utils.deferGetSet(ClientsRec,
                  "cleartext",
                  ["name", "type", "commands",
                   "version", "protocols",
                   "formfactor", "os", "appPackage", "application", "device"]);


this.ClientEngine = function ClientEngine(service) {
  SyncEngine.call(this, "Clients", service);

  
  this._resetClient();
}
ClientEngine.prototype = {
  __proto__: SyncEngine.prototype,
  _storeObj: ClientStore,
  _recordObj: ClientsRec,
  _trackerObj: ClientsTracker,

  
  get enabled() true,

  get lastRecordUpload() {
    return Svc.Prefs.get(this.name + ".lastRecordUpload", 0);
  },
  set lastRecordUpload(value) {
    Svc.Prefs.set(this.name + ".lastRecordUpload", Math.floor(value));
  },

  
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

  




  get deviceTypes() {
    let counts = new Map();

    counts.set(this.localType, 1);

    for each (let record in this._store._remoteClients) {
      let type = record.type;
      if (!counts.has(type)) {
        counts.set(type, 0);
      }

      counts.set(type, counts.get(type) + 1);
    }

    return counts;
  },

  get localID() {
    
    let localID = Svc.Prefs.get("client.GUID", "");
    return localID == "" ? this.localID = Utils.makeGUID() : localID;
  },
  set localID(value) Svc.Prefs.set("client.GUID", value),

  get brandName() {
    let brand = new StringBundle("chrome://branding/locale/brand.properties");
    return brand.get("brandShortName");
  },

  get localName() {
    let localName = Svc.Prefs.get("client.name", "");
    if (localName != "")
      return localName;

    
    let env = Cc["@mozilla.org/process/environment;1"]
                .getService(Ci.nsIEnvironment);
    let user = env.get("USER") || env.get("USERNAME") ||
               Svc.Prefs.get("account") || Svc.Prefs.get("username");

    let brandName = this.brandName;
    let appName;
    try {
      let syncStrings = new StringBundle("chrome://browser/locale/sync.properties");
      appName = syncStrings.getFormattedString("sync.defaultAccountApplication", [brandName]);
    } catch (ex) {}
    appName = appName || brandName;

    let system =
      
      Cc["@mozilla.org/system-info;1"].getService(Ci.nsIPropertyBag2).get("device") ||
      
      Cc["@mozilla.org/system-info;1"].getService(Ci.nsIPropertyBag2).get("host") ||
      
      Cc["@mozilla.org/network/protocol;1?name=http"].getService(Ci.nsIHttpProtocolHandler).oscpu;

    return this.localName = Str.sync.get("client.name2", [user, appName, system]);
  },
  set localName(value) Svc.Prefs.set("client.name", value),

  get localType() Svc.Prefs.get("client.type", "desktop"),
  set localType(value) Svc.Prefs.set("client.type", value),

  isMobile: function isMobile(id) {
    if (this._store._remoteClients[id])
      return this._store._remoteClients[id].type == "mobile";
    return false;
  },

  _syncStartup: function _syncStartup() {
    
    if (Date.now() / 1000 - this.lastRecordUpload > CLIENTS_TTL_REFRESH) {
      this._tracker.addChangedID(this.localID);
      this.lastRecordUpload = Date.now() / 1000;
    }
    SyncEngine.prototype._syncStartup.call(this);
  },

  
  _reconcile: function _reconcile() {
    return true;
  },

  
  _resetClient() {
    this._wipeClient();
  },

  _wipeClient: function _wipeClient() {
    SyncEngine.prototype._resetClient.call(this);
    this._store.wipe();
  },

  removeClientData: function removeClientData() {
    let res = this.service.resource(this.engineURL + "/" + this.localID);
    res.delete();
  },

  
  handleHMACMismatch: function handleHMACMismatch(item, mayRetry) {
    this._log.debug("Handling HMAC mismatch for " + item.id);

    let base = SyncEngine.prototype.handleHMACMismatch.call(this, item, mayRetry);
    if (base != SyncEngine.kRecoveryStrategy.error)
      return base;

    
    this._log.debug("Bad client record detected. Scheduling for deletion.");
    this._deleteId(item.id);

    
    return SyncEngine.kRecoveryStrategy.ignore;
  },

  




  _commands: {
    resetAll:    { args: 0, desc: "Clear temporary local data for all engines" },
    resetEngine: { args: 1, desc: "Clear temporary local data for engine" },
    wipeAll:     { args: 0, desc: "Delete all client data for all engines" },
    wipeEngine:  { args: 1, desc: "Delete all client data for engine" },
    logout:      { args: 0, desc: "Log out client" },
    displayURI:  { args: 3, desc: "Instruct a client to display a URI" },
  },

  


  clearCommands: function clearCommands() {
    delete this.localCommands;
    this._tracker.addChangedID(this.localID);
  },

  






  _sendCommandToClient: function sendCommandToClient(command, args, clientId) {
    this._log.trace("Sending " + command + " to " + clientId);

    let client = this._store._remoteClients[clientId];
    if (!client) {
      throw new Error("Unknown remote client ID: '" + clientId + "'.");
    }

    
    let notDupe = function(other) {
      return other.command != command || !Utils.deepEquals(other.args, args);
    };

    let action = {
      command: command,
      args: args,
    };

    if (!client.commands) {
      client.commands = [action];
    }
    
    else if (client.commands.every(notDupe)) {
      client.commands.push(action);
    }
    
    else {
      return;
    }

    this._log.trace("Client " + clientId + " got a new action: " + [command, args]);
    this._tracker.addChangedID(clientId);
  },

  




  processIncomingCommands: function processIncomingCommands() {
    return this._notify("clients:process-commands", "", function() {
      let commands = this.localCommands;

      
      this.clearCommands();

      
      for each (let {command, args} in commands) {
        this._log.debug("Processing command: " + command + "(" + args + ")");

        let engines = [args[0]];
        switch (command) {
          case "resetAll":
            engines = null;
            
          case "resetEngine":
            this.service.resetClient(engines);
            break;
          case "wipeAll":
            engines = null;
            
          case "wipeEngine":
            this.service.wipeClient(engines);
            break;
          case "logout":
            this.service.logout();
            return false;
          case "displayURI":
            this._handleDisplayURI.apply(this, args);
            break;
          default:
            this._log.debug("Received an unknown command: " + command);
            break;
        }
      }

      return true;
    })();
  },

  














  sendCommand: function sendCommand(command, args, clientId) {
    let commandData = this._commands[command];
    
    if (!commandData) {
      this._log.error("Unknown command to send: " + command);
      return;
    }
    
    else if (!args || args.length != commandData.args) {
      this._log.error("Expected " + commandData.args + " args for '" +
                      command + "', but got " + args);
      return;
    }

    if (clientId) {
      this._sendCommandToClient(command, args, clientId);
    } else {
      for (let id in this._store._remoteClients) {
        this._sendCommandToClient(command, args, id);
      }
    }
  },

  
















  sendURIToClientForDisplay: function sendURIToClientForDisplay(uri, clientId, title) {
    this._log.info("Sending URI to client: " + uri + " -> " +
                   clientId + " (" + title + ")");
    this.sendCommand("displayURI", [uri, this.localID, title], clientId);

    this._tracker.score += SCORE_INCREMENT_XLARGE;
  },

  




















  _handleDisplayURI: function _handleDisplayURI(uri, clientId, title) {
    this._log.info("Received a URI for display: " + uri + " (" + title +
                   ") from " + clientId);

    let subject = {uri: uri, client: clientId, title: title};
    Svc.Obs.notify("weave:engine:clients:display-uri", subject);
  }
};

function ClientStore(name, engine) {
  Store.call(this, name, engine);
}
ClientStore.prototype = {
  __proto__: Store.prototype,

  create(record) {
    this.update(record)
  },

  update: function update(record) {
    
    if (record.id == this.engine.localID)
      this.engine.localCommands = record.commands;
    else
      this._remoteClients[record.id] = record.cleartext;
  },

  createRecord: function createRecord(id, collection) {
    let record = new ClientsRec(collection, id);

    
    if (id == this.engine.localID) {
      record.name = this.engine.localName;
      record.type = this.engine.localType;
      record.commands = this.engine.localCommands;
      record.version = Services.appinfo.version;
      record.protocols = SUPPORTED_PROTOCOL_VERSIONS;

      
      record.os = Services.appinfo.OS;             
      record.appPackage = Services.appinfo.ID;
      record.application = this.engine.brandName   

      
      
      
    } else {
      record.cleartext = this._remoteClients[id];
    }

    return record;
  },

  itemExists(id) {
    return id in this.getAllIDs();
  },

  getAllIDs: function getAllIDs() {
    let ids = {};
    ids[this.engine.localID] = true;
    for (let id in this._remoteClients)
      ids[id] = true;
    return ids;
  },

  wipe: function wipe() {
    this._remoteClients = {};
  },
};

function ClientsTracker(name, engine) {
  Tracker.call(this, name, engine);
  Svc.Obs.add("weave:engine:start-tracking", this);
  Svc.Obs.add("weave:engine:stop-tracking", this);
}
ClientsTracker.prototype = {
  __proto__: Tracker.prototype,

  _enabled: false,

  observe: function observe(subject, topic, data) {
    switch (topic) {
      case "weave:engine:start-tracking":
        if (!this._enabled) {
          Svc.Prefs.observe("client.name", this);
          this._enabled = true;
        }
        break;
      case "weave:engine:stop-tracking":
        if (this._enabled) {
          Svc.Prefs.ignore("clients.name", this);
          this._enabled = false;
        }
        break;
      case "nsPref:changed":
        this._log.debug("client.name preference changed");
        this.addChangedID(Svc.Prefs.get("client.GUID"));
        this.score += SCORE_INCREMENT_XLARGE;
        break;
    }
  }
};
