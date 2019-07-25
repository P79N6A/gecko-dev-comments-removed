



































const EXPORTED_SYMBOLS = ['Engine', 'BookmarksEngine', 'HistoryEngine'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/constants.js");
Cu.import("resource://weave/util.js");
Cu.import("resource://weave/crypto.js");
Cu.import("resource://weave/stores.js");
Cu.import("resource://weave/syncCores.js");

Function.prototype.async = Utils.generatorAsync;
let Crypto = new WeaveCrypto();

function Engine(davCollection, cryptoId) {
  
}
Engine.prototype = {
  
  get name() { throw "name property must be overridden in subclasses"; },

  
  get logName() { throw "logName property must be overridden in subclasses"; },

  
  get serverPrefix() { throw "serverPrefix property must be overridden in subclasses"; },

  
  
  get statusFile() { return this.serverPrefix + "status.json"; },
  get snapshotFile() { return this.serverPrefix + "snapshot.json"; },
  get deltasFile() { return this.serverPrefix + "deltas.json"; },

  __os: null,
  get _os() {
    if (!this.__os)
      this.__os = Cc["@mozilla.org/observer-service;1"]
        .getService(Ci.nsIObserverService);
    return this.__os;
  },

  
  __core: null,
  get _core() {
    if (!this.__core)
      this.__core = new SyncCore();
    return this.__core;
  },

  __store: null,
  get _store() {
    if (!this.__store)
      this.__store = new Store();
    return this.__store;
  },

  __snapshot: null,
  get _snapshot() {
    if (!this.__snapshot)
      this.__snapshot = new SnapshotStore(this.name);
    return this.__snapshot;
  },
  set _snapshot(value) {
    this.__snapshot = value;
  },

  _init: function BmkEngine__init(davCollection, cryptoId) {
    this._dav = davCollection;
    this._cryptoId = cryptoId;
    this._log = Log4Moz.Service.getLogger("Service." + this.logName);
    this._osPrefix = "weave:" + this.name + ":";
    this._snapshot.load();
  },

  _checkStatus: function BmkEngine__checkStatus(code, msg, ok404) {
    if (code >= 200 && code < 300)
      return;
    if (ok404 && code == 404)
      return;
    this._log.error(msg + " Error code: " + code);
    throw 'checkStatus failed';
  },

  _resetServer: function Engine__resetServer(onComplete) {
    let [self, cont] = yield;
    let done = false;

    try {
      this._log.debug("Resetting server data");
      this._os.notifyObservers(null, this._osPrefix + "reset-server:start", "");

      this._dav.lock.async(this._dav, cont);
      let locked = yield;
      if (locked)
        this._log.debug("Lock acquired");
      else {
        this._log.warn("Could not acquire lock, aborting server reset");
        return;        
      }

      
      this._dav.DELETE(this.statusFile, cont);
      let statusResp = yield;
      this._dav.DELETE(this.snapshotFile, cont);
      let snapshotResp = yield;
      this._dav.DELETE(this.deltasFile, cont);
      let deltasResp = yield;

      this._dav.unlock.async(this._dav, cont);
      let unlocked = yield;

      this._checkStatus(statusResp.status,
                        "Could not delete status file.", true);
      this._checkStatus(snapshotResp.status,
                        "Could not delete snapshot file.", true);
      this._checkStatus(deltasResp.status,
                        "Could not delete deltas file.", true);

      this._log.debug("Server files deleted");
      done = true;
        
    } catch (e) {
      if (e != 'checkStatus failed')
	this._log.error("Exception caught: " + (e.message? e.message : e));

    } finally {
      if (done) {
        this._log.debug("Server reset completed successfully");
        this._os.notifyObservers(null, this._osPrefix + "reset-server:success", "");
      } else {
        this._log.debug("Server reset failed");
        this._os.notifyObservers(null, this._osPrefix + "reset-server:error", "");
      }
      Utils.generatorDone(this, self, onComplete, done)
      yield; 
    }
    this._log.warn("generator not properly closed");
  },

  _resetClient: function Engine__resetClient(onComplete) {
    let [self, cont] = yield;
    let done = false;

    try {
      this._log.debug("Resetting client state");
      this._os.notifyObservers(null, this._osPrefix + "reset-client:start", "");

      this._snapshot.wipe();
      this._store.wipe();
      done = true;

    } catch (e) {
      this._log.error("Exception caught: " + (e.message? e.message : e));

    } finally {
      if (done) {
        this._log.debug("Client reset completed successfully");
        this._os.notifyObservers(null, this._osPrefix + "reset-client:success", "");
      } else {
        this._log.debug("Client reset failed");
        this._os.notifyObservers(null, this._osPrefix + "reset-client:error", "");
      }
      Utils.generatorDone(this, self, onComplete, done);
      yield; 
    }
    this._log.warn("generator not properly closed");
  },

  
  
  
  
  
  
  
  
  

  
  

  
  
  

  
  
  
  
  
  
  
  
  
  

  _sync: function BmkEngine__sync(onComplete) {
    let [self, cont] = yield;
    let synced = false, locked = null;

    try {
      this._log.info("Beginning sync");
      this._os.notifyObservers(null, this._osPrefix + "sync:start", "");

      this._dav.lock.async(this._dav, cont);
      locked = yield;

      if (locked)
        this._log.info("Lock acquired");
      else {
        this._log.warn("Could not acquire lock, aborting sync");
        return;
      }

      
      this._dav.MKCOL(this.serverPrefix, cont);
      let ret = yield;
      this._checkStatus(ret.status, "Could not create remote folder.");

      
      this._getServerData.async(this, cont);
      let server = yield;

      this._log.info("Local snapshot version: " + this._snapshot.version);
      this._log.info("Server status: " + server.status);
      this._log.info("Server maxVersion: " + server.maxVersion);
      this._log.info("Server snapVersion: " + server.snapVersion);

      if (server.status != 0) {
        this._log.fatal("Sync error: could not get server status, " +
                        "or initial upload failed.  Aborting sync.");
        return;
      }

      

      let localJson = new SnapshotStore();
      localJson.data = this._store.wrap();
      this._core.detectUpdates(cont, this._snapshot.data, localJson.data);
      let localUpdates = yield;

      this._log.debug("local json:\n" + localJson.serialize());
      this._log.debug("Local updates: " + serializeCommands(localUpdates));
      this._log.debug("Server updates: " + serializeCommands(server.updates));

      if (server.updates.length == 0 && localUpdates.length == 0) {
        this._snapshot.version = server.maxVersion;
        this._log.info("Sync complete (1): no changes needed on client or server");
        synced = true;
        return;
      }
	  
      

      this._log.info("Reconciling client/server updates");
      this._core.reconcile(cont, localUpdates, server.updates);
      ret = yield;

      let clientChanges = ret.propagations[0];
      let serverChanges = ret.propagations[1];
      let clientConflicts = ret.conflicts[0];
      let serverConflicts = ret.conflicts[1];

      this._log.info("Changes for client: " + clientChanges.length);
      this._log.info("Predicted changes for server: " + serverChanges.length);
      this._log.info("Client conflicts: " + clientConflicts.length);
      this._log.info("Server conflicts: " + serverConflicts.length);
      this._log.debug("Changes for client: " + serializeCommands(clientChanges));
      this._log.debug("Predicted changes for server: " + serializeCommands(serverChanges));
      this._log.debug("Client conflicts: " + serializeConflicts(clientConflicts));
      this._log.debug("Server conflicts: " + serializeConflicts(serverConflicts));

      if (!(clientChanges.length || serverChanges.length ||
            clientConflicts.length || serverConflicts.length)) {
        this._log.info("Sync complete (2): no changes needed on client or server");
        this._snapshot.data = localJson.data;
        this._snapshot.version = server.maxVersion;
        this._snapshot.save();
        synced = true;
        return;
      }

      if (clientConflicts.length || serverConflicts.length) {
        this._log.warn("Conflicts found!  Discarding server changes");
      }

      let savedSnap = eval(uneval(this._snapshot.data));
      let savedVersion = this._snapshot.version;
      let newSnapshot;

      
      if (clientChanges.length) {
        this._log.info("Applying changes locally");
        
        

	localJson.applyCommands(clientChanges);
        this._snapshot.data = localJson.data;
        this._snapshot.version = server.maxVersion;
        this._store.applyCommands(clientChanges);
        newSnapshot = this._store.wrap();

        this._core.detectUpdates(cont, this._snapshot.data, newSnapshot);
        let diff = yield;
        if (diff.length != 0) {
          this._log.warn("Commands did not apply correctly");
          this._log.debug("Diff from snapshot+commands -> " +
                          "new snapshot after commands:\n" +
                          serializeCommands(diff));
          
          this._snapshot.data = eval(uneval(savedSnap));
          this._snapshot.version = savedVersion;
        }
        this._snapshot.save();
      }

      

      
      
      

      newSnapshot = this._store.wrap();
      this._core.detectUpdates(cont, server.snapshot, newSnapshot);
      let serverDelta = yield;

      
      if (!(serverConflicts.length ||
            Utils.deepEquals(serverChanges, serverDelta)))
        this._log.warn("Predicted server changes differ from " +
                       "actual server->client diff (can be ignored in many cases)");

      this._log.info("Actual changes for server: " + serverDelta.length);
      this._log.debug("Actual changes for server: " +
                      serializeCommands(serverDelta));

      if (serverDelta.length) {
        this._log.info("Uploading changes to server");

        this._snapshot.data = newSnapshot;
        this._snapshot.version = ++server.maxVersion;

        server.deltas.push(serverDelta);

        if (server.formatVersion != STORAGE_FORMAT_VERSION ||
            this._encryptionChanged) {
          this._fullUpload.async(this, cont);
          let status = yield;
          if (!status)
            this._log.error("Could not upload files to server"); 

        } else {
	  Crypto.PBEencrypt.async(Crypto, cont,
                                  serializeCommands(server.deltas),
				  this._cryptoId);
          let data = yield;
          this._dav.PUT(this.deltasFile, data, cont);
          let deltasPut = yield;

          let c = 0;
          for (GUID in this._snapshot.data)
            c++;

          this._dav.PUT(this.statusFile,
                        uneval({GUID: this._snapshot.GUID,
                                formatVersion: STORAGE_FORMAT_VERSION,
                                snapVersion: server.snapVersion,
                                maxVersion: this._snapshot.version,
                                snapEncryption: server.snapEncryption,
                                deltasEncryption: Crypto.defaultAlgorithm,
                                itemCount: c}), cont);
          let statusPut = yield;

          if (deltasPut.status >= 200 && deltasPut.status < 300 &&
              statusPut.status >= 200 && statusPut.status < 300) {
            this._log.info("Successfully updated deltas and status on server");
            this._snapshot.save();
          } else {
            
            
            this._log.error("Could not update deltas on server");
          }
        }
      }

      this._log.info("Sync complete");
      synced = true;

    } catch (e) {
      this._log.error("Exception caught: " + (e.message? e.message : e));

    } finally {
      let ok = false;
      if (locked) {
        this._dav.unlock.async(this._dav, cont);
        ok = yield;
      }
      if (ok && synced) {
        this._os.notifyObservers(null, this._osPrefix + "sync:success", "");
        Utils.generatorDone(this, self, onComplete, true);
      } else {
        this._os.notifyObservers(null, this._osPrefix + "sync:error", "");
        Utils.generatorDone(this, self, onComplete, false);
      }
      yield; 
    }
    this._log.warn("generator not properly closed");
  },

  























  _getServerData: function BmkEngine__getServerData(onComplete) {
    let [self, cont] = yield;
    let ret = {status: -1,
               formatVersion: null, maxVersion: null, snapVersion: null,
               snapEncryption: null, deltasEncryption: null,
               snapshot: null, deltas: null, updates: null};

    try {
      this._log.debug("Getting status file from server");
      this._dav.GET(this.statusFile, cont);
      let resp = yield;
      let status = resp.status;
  
      switch (status) {
      case 200: {
        this._log.info("Got status file from server");
  
        let status = eval(resp.responseText);
        let deltas, allDeltas;
	let snap = new SnapshotStore();
  
        
        if (status.formatVersion > STORAGE_FORMAT_VERSION) {
          this._log.error("Server uses storage format v" + status.formatVersion +
                    ", this client understands up to v" + STORAGE_FORMAT_VERSION);
          Utils.generatorDone(this, self, onComplete, ret)
          return;
        }

        if (status.formatVersion == 0) {
          ret.snapEncryption = status.snapEncryption = "none";
          ret.deltasEncryption = status.deltasEncryption = "none";
        }
  
        if (status.GUID != this._snapshot.GUID) {
          this._log.info("Remote/local sync GUIDs do not match.  " +
                      "Forcing initial sync.");
          this._log.debug("Remote: " + status.GUID);
          this._log.debug("Local: " + this._snapshot.GUID);
          this._store.resetGUIDs();
          this._snapshot.data = {};
          this._snapshot.version = -1;
          this._snapshot.GUID = status.GUID;
        }
  
        if (this._snapshot.version < status.snapVersion) {
          if (this._snapshot.version >= 0)
            this._log.info("Local snapshot is out of date");
  
          this._log.info("Downloading server snapshot");
          this._dav.GET(this.snapshotFile, cont);
          resp = yield;
          this._checkStatus(resp.status, "Could not download snapshot.");
          Crypto.PBEdecrypt.async(Crypto, cont,
                                  resp.responseText,
				  this._cryptoId,
				  status.snapEncryption);
          let data = yield;
          snap.data = eval(data);

          this._log.info("Downloading server deltas");
          this._dav.GET(this.deltasFile, cont);
          resp = yield;
          this._checkStatus(resp.status, "Could not download deltas.");
          Crypto.PBEdecrypt.async(Crypto, cont,
                                  resp.responseText,
				  this._cryptoId,
				  status.deltasEncryption);
          data = yield;
          allDeltas = eval(data);
          deltas = eval(data);
  
        } else if (this._snapshot.version >= status.snapVersion &&
                   this._snapshot.version < status.maxVersion) {
          snap.data = eval(uneval(this._snapshot.data));
  
          this._log.info("Downloading server deltas");
          this._dav.GET(this.deltasFile, cont);
          resp = yield;
          this._checkStatus(resp.status, "Could not download deltas.");
          Crypto.PBEdecrypt.async(Crypto, cont,
                                  resp.responseText,
				  this._cryptoId,
				  status.deltasEncryption);
          let data = yield;
          allDeltas = eval(data);
          deltas = allDeltas.slice(this._snapshot.version - status.snapVersion);
  
        } else if (this._snapshot.version == status.maxVersion) {
          snap.data = eval(uneval(this._snapshot.data));
  
          
          this._log.info("Downloading server deltas");
          this._dav.GET(this.deltasFile, cont);
          resp = yield;
          this._checkStatus(resp.status, "Could not download deltas.");
          Crypto.PBEdecrypt.async(Crypto, cont,
                                  resp.responseText,
				  this._cryptoId,
				  status.deltasEncryption);
          let data = yield;
          allDeltas = eval(data);
          deltas = [];
  
        } else { 
          this._log.error("Server snapshot is older than local snapshot");
          return;
        }
  
        for (var i = 0; i < deltas.length; i++) {
	  snap.applyCommands(deltas[i]);
        }
  
        ret.status = 0;
        ret.formatVersion = status.formatVersion;
        ret.maxVersion = status.maxVersion;
        ret.snapVersion = status.snapVersion;
        ret.snapEncryption = status.snapEncryption;
        ret.deltasEncryption = status.deltasEncryption;
        ret.snapshot = snap.data;
        ret.deltas = allDeltas;
        this._core.detectUpdates(cont, this._snapshot.data, snap.data);
        ret.updates = yield;
      } break;

      case 404: {
        this._log.info("Server has no status file, Initial upload to server");
  
        this._snapshot.data = this._store.wrap();
        this._snapshot.version = 0;
        this._snapshot.GUID = null; 

        this._fullUpload.async(this, cont);
        let uploadStatus = yield;
        if (!uploadStatus)
          return;
  
        this._log.info("Initial upload to server successful");
        this._snapshot.save();
  
        ret.status = 0;
        ret.formatVersion = STORAGE_FORMAT_VERSION;
        ret.maxVersion = this._snapshot.version;
        ret.snapVersion = this._snapshot.version;
        ret.snapEncryption = Crypto.defaultAlgorithm;
        ret.deltasEncryption = Crypto.defaultAlgorithm;
        ret.snapshot = eval(uneval(this._snapshot.data));
        ret.deltas = [];
        ret.updates = [];
      } break;

      default:
        this._log.error("Could not get status file: unknown HTTP status code " +
                        status);
        break;
      }

    } catch (e) {
      if (e != 'checkStatus failed' &&
          e != 'decrypt failed')
	this._log.error("Exception caught: " + (e.message? e.message : e));

    } finally {
      Utils.generatorDone(this, self, onComplete, ret)
      yield; 
    }
    this._log.warn("generator not properly closed");
  },

  _fullUpload: function Engine__fullUpload(onComplete) {
    let [self, cont] = yield;
    let ret = false;

    try {
      Crypto.PBEencrypt.async(Crypto, cont,
                              this._snapshot.serialize(),
			      this._cryptoId);
      let data = yield;
      this._dav.PUT(this.snapshotFile, data, cont);
      resp = yield;
      this._checkStatus(resp.status, "Could not upload snapshot.");

      this._dav.PUT(this.deltasFile, uneval([]), cont);
      resp = yield;
      this._checkStatus(resp.status, "Could not upload deltas.");

      let c = 0;
      for (GUID in this._snapshot.data)
        c++;

      this._dav.PUT(this.statusFile,
                    uneval({GUID: this._snapshot.GUID,
                            formatVersion: STORAGE_FORMAT_VERSION,
                            snapVersion: this._snapshot.version,
                            maxVersion: this._snapshot.version,
                            snapEncryption: Crypto.defaultAlgorithm,
                            deltasEncryption: "none",
                            itemCount: c}), cont);
      resp = yield;
      this._checkStatus(resp.status, "Could not upload status file.");

      this._log.info("Full upload to server successful");
      ret = true;

    } catch (e) {
      if (e != 'checkStatus failed')
	this._log.error("Exception caught: " + (e.message? e.message : e));

    } finally {
      Utils.generatorDone(this, self, onComplete, ret)
      yield; 
    }
    this._log.warn("generator not properly closed");
  },

  sync: function Engine_sync(onComplete) {
    return this._sync.async(this, onComplete);
  },

  resetServer: function Engine_resetServer(onComplete) {
    return this._resetServer.async(this, onComplete);
  },

  resetClient: function Engine_resetClient(onComplete) {
    return this._resetClient.async(this, onComplete);
  }
};

function BookmarksEngine(davCollection, cryptoId) {
  this._init(davCollection, cryptoId);
}
BookmarksEngine.prototype = {
  get name() { return "bookmarks-engine"; },
  get logName() { return "BmkEngine"; },
  get serverPrefix() { return "user-data/bookmarks/"; },

  __core: null,
  get _core() {
    if (!this.__core)
      this.__core = new BookmarksSyncCore();
    return this.__core;
  },

  __store: null,
  get _store() {
    if (!this.__store)
      this.__store = new BookmarksStore();
    return this.__store;
  }
};
BookmarksEngine.prototype.__proto__ = new Engine();

function HistoryEngine(davCollection, cryptoId) {
  this._init(davCollection, cryptoId);
}
HistoryEngine.prototype = {
  get name() { return "history-engine"; },
  get logName() { return "HistEngine"; },
  get serverPrefix() { return "user-data/history/"; },

  __core: null,
  get _core() {
    if (!this.__core)
      this.__core = new HistorySyncCore();
    return this.__core;
  },

  __store: null,
  get _store() {
    if (!this.__store)
      this.__store = new HistoryStore();
    return this.__store;
  }
};
HistoryEngine.prototype.__proto__ = new Engine();

serializeCommands: function serializeCommands(commands) {
  let json = uneval(commands);
  json = json.replace(/ {action/g, "\n {action");
  return json;
}

serializeConflicts: function serializeConflicts(conflicts) {
  let json = uneval(conflicts);
  json = json.replace(/ {action/g, "\n {action");
  return json;
}
