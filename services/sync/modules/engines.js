



































const EXPORTED_SYMBOLS = ['BookmarksEngine'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/constants.js");
Cu.import("resource://weave/util.js");
Cu.import("resource://weave/crypto.js");
Cu.import("resource://weave/stores.js");
Cu.import("resource://weave/syncCores.js");

Function.prototype.async = generatorAsync;
let Crypto = new WeaveCrypto();

function BookmarksEngine(davCollection, cryptoId) {
  this._init(davCollection, cryptoId);
}
BookmarksEngine.prototype = {
  _logName: "BmkEngine",

  __os: null,
  get _os() {
    if (!this.__os)
      this.__os = Cc["@mozilla.org/observer-service;1"]
        .getService(Ci.nsIObserverService);
    return this.__os;
  },

  __store: null,
  get _store() {
    if (!this.__store)
      this.__store = new BookmarksStore();
    return this.__store;
  },

  __core: null,
  get _core() {
    if (!this.__core)
      this.__core = new BookmarksSyncCore();
    return this.__core;
  },

  __snapshot: null,
  get _snapshot() {
    if (!this.__snapshot)
      this.__snapshot = new SnapshotStore();
    return this.__snapshot;
  },
  set _snapshot(value) {
    this.__snapshot = value;
  },

  _init: function BmkEngine__init(davCollection, cryptoId) {
    this._dav = davCollection;
    this._cryptoId = cryptoId;
    this._log = Log4Moz.Service.getLogger("Service." + this._logName);
    this._snapshot.load();
  },

  _checkStatus: function BmkEngine__checkStatus(code, msg) {
    if (code >= 200 && code < 300)
      return;
    this._log.error(msg + " Error code: " + code);
    throw 'checkStatus failed';
  },

  























  _getServerData: function BmkEngine__getServerData(onComplete) {
    let [self, cont] = yield;
    let ret = {status: -1,
               formatVersion: null, maxVersion: null, snapVersion: null,
               snapEncryption: null, deltasEncryption: null,
               snapshot: null, deltas: null, updates: null};

    try {
      this._log.info("Getting bookmarks status from server");
      this._dav.GET("bookmarks-status.json", cont);
      let resp = yield;
      let status = resp.status;
  
      switch (status) {
      case 200:
        this._log.info("Got bookmarks status from server");
  
        let status = eval(resp.responseText);
        let deltas, allDeltas;
	let snap = new SnapshotStore();
  
        
        if (status.formatVersion > STORAGE_FORMAT_VERSION) {
          this._log.error("Server uses storage format v" + status.formatVersion +
                    ", this client understands up to v" + STORAGE_FORMAT_VERSION);
          generatorDone(this, self, onComplete, ret)
          return;
        }

        if (status.formatVersion == 0) {
          ret.snapEncryption = status.snapEncryption = "none";
          ret.deltasEncryption = status.deltasEncryption = "none";
        }
  
        if (status.GUID != this._snapshot.GUID) {
          this._log.info("Remote/local sync GUIDs do not match.  " +
                      "Forcing initial sync.");
          this._store.resetGUIDs();
          this._snapshot.data = {};
          this._snapshot.version = -1;
          this._snapshot.GUID = status.GUID;
        }
  
        if (this._snapshot.version < status.snapVersion) {
          if (this._snapshot.version >= 0)
            this._log.info("Local snapshot is out of date");
  
          this._log.info("Downloading server snapshot");
          this._dav.GET("bookmarks-snapshot.json", cont);
          resp = yield;
          this._checkStatus(resp.status, "Could not download snapshot.");
          snap.data = Crypto.PBEdecrypt(resp.responseText,
					this._cryptoId,
					status.snapEncryption);

          this._log.info("Downloading server deltas");
          this._dav.GET("bookmarks-deltas.json", cont);
          resp = yield;
          this._checkStatus(resp.status, "Could not download deltas.");
          allDeltas = Crypto.PBEdecrypt(resp.responseText,
					this._cryptoId,
					status.deltasEncryption);
          deltas = eval(uneval(allDeltas));
  
        } else if (this._snapshot.version >= status.snapVersion &&
                   this._snapshot.version < status.maxVersion) {
          snap.data = eval(uneval(this._snapshot.data));
  
          this._log.info("Downloading server deltas");
          this._dav.GET("bookmarks-deltas.json", cont);
          resp = yield;
          this._checkStatus(resp.status, "Could not download deltas.");
          allDeltas = Crypto.PBEdecrypt(resp.responseText,
					this._cryptoId,
					status.deltasEncryption);
          deltas = allDeltas.slice(this._snapshot.version - status.snapVersion);
  
        } else if (this._snapshot.version == status.maxVersion) {
          snap.data = eval(uneval(this._snapshot.data));
  
          
          this._log.info("Downloading server deltas");
          this._dav.GET("bookmarks-deltas.json", cont);
          resp = yield;
          this._checkStatus(resp.status, "Could not download deltas.");
          allDeltas = Crypto.PBEdecrypt(resp.responseText,
					this._cryptoId,
					status.deltasEncryption);
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
        break;
  
      case 404:
        this._log.info("Server has no status file, Initial upload to server");
  
        this._snapshot.data = this._store.wrap();
        this._snapshot.version = 0;
        this._snapshot.GUID = null; 

        this._fullUpload.async(this, cont);
        let uploadStatus = yield;
        if (!uploadStatus)
          return;
  
        this._log.info("Initial upload to server successful");
        this.snapshot.save();
  
        ret.status = 0;
        ret.formatVersion = STORAGE_FORMAT_VERSION;
        ret.maxVersion = this._snapshot.version;
        ret.snapVersion = this._snapshot.version;
        ret.snapEncryption = Crypto.defaultAlgorithm;
        ret.deltasEncryption = Crypto.defaultAlgorithm;
        ret.snapshot = eval(uneval(this._snapshot.data));
        ret.deltas = [];
        ret.updates = [];
        break;
  
      default:
        this._log.error("Could not get bookmarks.status: unknown HTTP status code " +
                        status);
        break;
      }

    } catch (e) {
      if (e != 'checkStatus failed' &&
          e != 'decrypt failed')
        this._log.error("Exception caught: " + e.message);

    } finally {
      generatorDone(this, self, onComplete, ret)
      yield; 
    }
    this._log.warn("generator not properly closed");
  },

  _fullUpload: function BmkEngine__fullUpload(onComplete) {
    let [self, cont] = yield;
    let ret = false;

    try {
      let data = Crypto.PBEencrypt(this._snapshot.serialize(),
				   this._cryptoId);
      this._dav.PUT("bookmarks-snapshot.json", data, cont);
      resp = yield;
      this._checkStatus(resp.status, "Could not upload snapshot.");

      this._dav.PUT("bookmarks-deltas.json", uneval([]), cont);
      resp = yield;
      this._checkStatus(resp.status, "Could not upload deltas.");

      let c = 0;
      for (GUID in this._snapshot.data)
        c++;

      this._dav.PUT("bookmarks-status.json",
                    uneval({GUID: this._snapshot.GUID,
                            formatVersion: STORAGE_FORMAT_VERSION,
                            snapVersion: this._snapshot.version,
                            maxVersion: this._snapshot.version,
                            snapEncryption: Crypto.defaultAlgorithm,
                            deltasEncryption: "none",
                            bookmarksCount: c}), cont);
      resp = yield;
      this._checkStatus(resp.status, "Could not upload status file.");

      this._log.info("Full upload to server successful");
      ret = true;

    } catch (e) {
      if (e != 'checkStatus failed')
        this._log.error("Exception caught: " + e.message);

    } finally {
      generatorDone(this, self, onComplete, ret)
      yield; 
    }
    this._log.warn("generator not properly closed");
  },

  
  
  
  
  
  
  
  
  

  
  

  
  
  

  
  
  
  
  
  
  
  
  
  

  _sync: function BmkEngine__sync(onComplete) {
    let [self, cont] = yield;
    let synced = false, locked = null;

    try {
      this._log.info("Beginning sync");
      this._os.notifyObservers(null, "bookmarks-sync:sync-start", "");

      this._dav.lock.async(this._dav, cont);
      locked = yield;

      if (locked)
        this._log.info("Lock acquired");
      else {
        this._log.warn("Could not acquire lock, aborting sync");
        return;
      }

      
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
      let ret = yield;

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
            deepEquals(serverChanges, serverDelta)))
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
	  let data = Crypto.PBEencrypt(serializeCommands(server.deltas),
				       this._cryptoId);
          this._dav.PUT("bookmarks-deltas.json", data, cont);
          let deltasPut = yield;

          let c = 0;
          for (GUID in this._snapshot.data)
            c++;

          this._dav.PUT("bookmarks-status.json",
                        uneval({GUID: this._snapshot.GUID,
                                formatVersion: STORAGE_FORMAT_VERSION,
                                snapVersion: server.snapVersion,
                                maxVersion: this._snapshot.version,
                                snapEncryption: server.snapEncryption,
                                deltasEncryption: Crypto.defaultAlgorithm,
                                Bookmarkscount: c}), cont);
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
      this._log.error("Exception caught: " + e.message);

    } finally {
      let ok = false;
      if (locked) {
        this._dav.unlock.async(this._dav, cont);
        ok = yield;
      }
      if (ok && synced) {
        this._os.notifyObservers(null, "bookmarks-sync:sync-end", "");
        generatorDone(this, self, onComplete, true);
      } else {
        this._os.notifyObservers(null, "bookmarks-sync:sync-error", "");
        generatorDone(this, self, onComplete, false);
      }
      yield; 
    }
    this._log.warn("generator not properly closed");
  },

  _resetServer: function BmkEngine__resetServer(onComplete) {
    let [self, cont] = yield;
    let done = false;

    try {
      this._log.debug("Resetting server data");
      this._os.notifyObservers(null, "bookmarks-sync:reset-server-start", "");

      this._dav.lock.async(this._dav, cont);
      let locked = yield;
      if (locked)
        this._log.debug("Lock acquired");
      else {
        this._log.warn("Could not acquire lock, aborting server reset");
        return;        
      }

      this._dav.DELETE("bookmarks-status.json", cont);
      let statusResp = yield;
      this._dav.DELETE("bookmarks-snapshot.json", cont);
      let snapshotResp = yield;
      this._dav.DELETE("bookmarks-deltas.json", cont);
      let deltasResp = yield;

      this._dav.unlock.async(this._dav, cont);
      let unlocked = yield;

      function ok(code) {
        if (code >= 200 && code < 300)
          return true;
        if (code == 404)
          return true;
        return false;
      }

      if (!(ok(statusResp.status) && ok(snapshotResp.status) &&
            ok(deltasResp.status))) {
        this._log.error("Could delete server data, response codes " +
                        statusResp.status + ", " + snapshotResp.status + ", " +
                        deltasResp.status);
        return;
      }

      this._log.debug("Server files deleted");
      done = true;
        
    } catch (e) {
      this._log.error("Exception caught: " + e.message);

    } finally {
      if (done) {
        this._log.debug("Server reset completed successfully");
        this._os.notifyObservers(null, "bookmarks-sync:reset-server-end", "");
      } else {
        this._log.debug("Server reset failed");
        this._os.notifyObservers(null, "bookmarks-sync:reset-server-error", "");
      }
      generatorDone(this, self, onComplete, done)
      yield; 
    }
    this._log.warn("generator not properly closed");
  },

  _resetClient: function BmkEngine__resetClient(onComplete) {
    let [self, cont] = yield;
    let done = false;

    try {
      this._log.debug("Resetting client state");
      this._os.notifyObservers(null, "bookmarks-sync:reset-client-start", "");

      this._snapshot.data = {};
      this._snapshot.version = -1;
      this.snapshot.save();
      done = true;

    } catch (e) {
      this._log.error("Exception caught: " + e.message);

    } finally {
      if (done) {
        this._log.debug("Client reset completed successfully");
        this._os.notifyObservers(null, "bookmarks-sync:reset-client-end", "");
      } else {
        this._log.debug("Client reset failed");
        this._os.notifyObservers(null, "bookmarks-sync:reset-client-error", "");
      }
      generatorDone(this, self, onComplete, done);
      yield; 
    }
    this._log.warn("generator not properly closed");
  },

  sync: function BmkEngine_sync(onComplete) {
    return this._sync.async(this, onComplete);
  },

  resetServer: function BmkEngine_resetServer(onComplete) {
    return this._resetServer.async(this, onComplete);
  },

  resetClient: function BmkEngine_resetClient(onComplete) {
    return this._resetClient.async(this, onComplete);
  }
};

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
