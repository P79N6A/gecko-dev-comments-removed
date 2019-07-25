



































const EXPORTED_SYMBOLS = ['Engines', 'Engine',
                          'BookmarksEngine', 'HistoryEngine', 'CookieEngine',
                          'PasswordEngine', 'FormEngine'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/constants.js");
Cu.import("resource://weave/util.js");
Cu.import("resource://weave/wrap.js");
Cu.import("resource://weave/crypto.js");
Cu.import("resource://weave/dav.js");
Cu.import("resource://weave/remote.js");
Cu.import("resource://weave/identity.js");
Cu.import("resource://weave/stores.js");
Cu.import("resource://weave/syncCores.js");
Cu.import("resource://weave/trackers.js");
Cu.import("resource://weave/async.js");

Function.prototype.async = Async.sugar;



Utils.lazy(this, 'Engines', EngineManagerSvc);

function EngineManagerSvc() {
  this._engines = {};
}
EngineManagerSvc.prototype = {
  get: function EngMgr_get(name) {
    return this._engines[name];
  },
  getAll: function EngMgr_getAll() {
    let ret = [];
    for (key in this._engines) {
      ret.push(this._engines[key]);
    }
    return ret;
  },
  register: function EngMgr_register(engine) {
    this._engines[engine.name] = engine;
  },
  unregister: function EngMgr_unregister(val) {
    let name = val;
    if (val instanceof Engine)
      name = val.name;
    delete this._engines[name];
  }
};

function Engine() {}
Engine.prototype = {
  _notify: Wrap.notify,

  
  get name() { throw "name property must be overridden in subclasses"; },

  
  get logName() { throw "logName property must be overridden in subclasses"; },

  
  get serverPrefix() { throw "serverPrefix property must be overridden in subclasses"; },

  get _remote() {
    if (!this.__remote)
      this.__remote = new RemoteStore(this.serverPrefix, 'Engine:' + this.name);
    return this.__remote;
  },

  get enabled() {
    return Utils.prefs.getBoolPref("engine." + this.name);
  },

  __os: null,
  get _os() {
    if (!this.__os)
      this.__os = Cc["@mozilla.org/observer-service;1"]
        .getService(Ci.nsIObserverService);
    return this.__os;
  },

  __json: null,
  get _json() {
    if (!this.__json)
      this.__json = Cc["@mozilla.org/dom/json;1"].
        createInstance(Ci.nsIJSON);
    return this.__json;
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

  __tracker: null,
  get _tracker() {
    if (!this.__tracker)
      this.__tracker = new Tracker();
    return this.__tracker;
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

  get _pbeId() {
    let id = ID.get('Engine:PBE:' + this.name);
    if (!id)
      id = ID.get('Engine:PBE:default');
    if (!id)
      throw "No identity found for engine PBE!";
    return id;
  },

  get _engineId() {
    let id = ID.get('Engine:' + this.name)
    if (!id ||
        id.username != this._pbeId.username || id.realm != this._pbeId.realm) {
      let password = null;
      if (id)
        password = id.password;
      id = new Identity(this._pbeId.realm + ' - ' + this.logName,
                        this._pbeId.username, password);
      ID.set('Engine:' + this.name, id);
    }
    return id;
  },

  _init: function Engine__init() {
    this._log = Log4Moz.Service.getLogger("Service." + this.logName);
    this._log.level =
      Log4Moz.Level[Utils.prefs.getCharPref("log.logger.service.engine")];
    this._osPrefix = "weave:" + this.name + ":";
    this._snapshot.load();
  },

  _getSymKey: function Engine__getSymKey() {
    let self = yield;

    if ("none" == Utils.prefs.getCharPref("encryption"))
      return;

    this._remote.keys.get(self.cb);
    yield;
    let keys = this._remote.keys.data;

    if (!keys || !keys.ring || !keys.ring[this._engineId.userHash])
      throw "Keyring does not contain a key for this user";

    Crypto.RSAdecrypt.async(Crypto, self.cb,
                            keys.ring[this._engineId.userHash], this._pbeId);
    let symkey = yield;
    this._engineId.setTempPassword(symkey);

    self.done();
  },

  _serializeCommands: function Engine__serializeCommands(commands) {
    let json = this._json.encode(commands);
    
    return json;
  },

  _serializeConflicts: function Engine__serializeConflicts(conflicts) {
    let json = this._json.encode(conflicts);
    
    return json;
  },

  _resetServer: function Engine__resetServer() {
    let self = yield;
    this._log.debug("Resetting server data");
    this._remote.status.delete(self.cb);
    yield;
    this._remote.keys.delete(self.cb);
    yield;
    this._remote.snapshot.delete(self.cb);
    yield;
    this._remote.deltas.delete(self.cb);
    yield;
    this._log.debug("Server files deleted");
  },

  _resetClient: function Engine__resetClient() {
    let self = yield;
    this._log.debug("Resetting client state");
    this._snapshot.wipe();
    this._store.wipe();
    this._log.debug("Client reset completed successfully");
  },

  
  
  
  
  
  
  
  
  

  
  

  
  
  

  
  
  
  
  
  
  
  
  
  

  _sync: function Engine__sync() {
    let self = yield;

    this._log.info("Beginning sync");

    
    DAV.MKCOL(this.serverPrefix, self.cb);
    let ret = yield;
    if (!ret)
      throw "Could not create remote folder";

    
    this._getServerData.async(this, self.cb);
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
    this._core.detectUpdates(self.cb, this._snapshot.data, localJson.data);
    let localUpdates = yield;

    this._log.trace("local json:\n" + localJson.serialize());
    this._log.trace("Local updates: " + this._serializeCommands(localUpdates));
    this._log.trace("Server updates: " + this._serializeCommands(server.updates));

    if (server.updates.length == 0 && localUpdates.length == 0) {
      this._snapshot.version = server.maxVersion;
      this._log.info("Sync complete: no changes needed on client or server");
      self.done(true);
      return;
    }

    

    this._log.info("Reconciling client/server updates");
    this._core.reconcile(self.cb, localUpdates, server.updates);
    ret = yield;

    let clientChanges = ret.propagations[0];
    let serverChanges = ret.propagations[1];
    let clientConflicts = ret.conflicts[0];
    let serverConflicts = ret.conflicts[1];

    this._log.info("Changes for client: " + clientChanges.length);
    this._log.info("Predicted changes for server: " + serverChanges.length);
    this._log.info("Client conflicts: " + clientConflicts.length);
    this._log.info("Server conflicts: " + serverConflicts.length);
    this._log.trace("Changes for client: " + this._serializeCommands(clientChanges));
    this._log.trace("Predicted changes for server: " + this._serializeCommands(serverChanges));
    this._log.trace("Client conflicts: " + this._serializeConflicts(clientConflicts));
    this._log.trace("Server conflicts: " + this._serializeConflicts(serverConflicts));

    if (!(clientChanges.length || serverChanges.length ||
          clientConflicts.length || serverConflicts.length)) {
      this._log.info("Sync complete: no changes needed on client or server");
      this._snapshot.data = localJson.data;
      this._snapshot.version = server.maxVersion;
      this._snapshot.save();
      self.done(true);
      return;
    }

    if (clientConflicts.length || serverConflicts.length) {
      this._log.warn("Conflicts found!  Discarding server changes");
    }

    let savedSnap = Utils.deepCopy(this._snapshot.data);
    let savedVersion = this._snapshot.version;
    let newSnapshot;

    
    if (clientChanges.length) {
      this._log.info("Applying changes locally");
      
      

      localJson.applyCommands.async(localJson, self.cb, clientChanges);
      yield;
      this._snapshot.data = localJson.data;
      this._snapshot.version = server.maxVersion;
      this._store.applyCommands.async(this._store, self.cb, clientChanges);
      yield;
      newSnapshot = this._store.wrap();

      this._core.detectUpdates(self.cb, this._snapshot.data, newSnapshot);
      let diff = yield;
      if (diff.length != 0) {
        this._log.warn("Commands did not apply correctly");
        this._log.debug("Diff from snapshot+commands -> " +
                        "new snapshot after commands:\n" +
                        this._serializeCommands(diff));
        
        this._snapshot.data = Utils.deepCopy(savedSnap);
        this._snapshot.version = savedVersion;
      }
      this._snapshot.save();
    }

    

    
    
    

    newSnapshot = this._store.wrap();
    this._core.detectUpdates(self.cb, server.snapshot, newSnapshot);
    let serverDelta = yield;

    
    if (!(serverConflicts.length ||
          Utils.deepEquals(serverChanges, serverDelta)))
      this._log.warn("Predicted server changes differ from " +
                     "actual server->client diff (can be ignored in many cases)");

    this._log.info("Actual changes for server: " + serverDelta.length);
    this._log.debug("Actual changes for server: " +
                    this._serializeCommands(serverDelta));

    if (serverDelta.length) {
      this._log.info("Uploading changes to server");

      this._snapshot.data = newSnapshot;
      this._snapshot.version = ++server.maxVersion;

      server.deltas.push(serverDelta);

      if (server.formatVersion != ENGINE_STORAGE_FORMAT_VERSION ||
          this._encryptionChanged) {
        this._fullUpload.async(this, self.cb);
        let status = yield;
        if (!status)
          this._log.error("Could not upload files to server"); 

      } else {
        this._remote.deltas.put(self.cb, this._serializeCommands(server.deltas));
        yield;

        let c = 0;
        for (GUID in this._snapshot.data)
          c++;

        this._remote.status.put(self.cb,
                                {GUID: this._snapshot.GUID,
                                 formatVersion: ENGINE_STORAGE_FORMAT_VERSION,
                                 snapVersion: server.snapVersion,
                                 maxVersion: this._snapshot.version,
                                 snapEncryption: server.snapEncryption,
                                 deltasEncryption: Crypto.defaultAlgorithm,
                                 itemCount: c});

        this._log.info("Successfully updated deltas and status on server");
        this._snapshot.save();
      }
    }

    this._log.info("Sync complete");
    self.done(true);
  },

  























  _getServerData: function BmkEngine__getServerData() {
    let self = yield;
    let status;

    try {
      this._log.debug("Getting status file from server");
      this._remote.status.get(self.cb);
      status = yield;
      this._log.info("Got status file from server");

    } catch (e if e.message.status == 404) {
      this._log.info("Server has no status file, Initial upload to server");

      this._snapshot.data = this._store.wrap();
      this._snapshot.version = 0;
      this._snapshot.GUID = null; 

      this._fullUpload.async(this, self.cb);
      let uploadStatus = yield;
      if (!uploadStatus)
        throw "Initial upload failed";

      this._log.info("Initial upload to server successful");
      this._snapshot.save();

      self.done({status: 0,
                 formatVersion: ENGINE_STORAGE_FORMAT_VERSION,
                 maxVersion: this._snapshot.version,
                 snapVersion: this._snapshot.version,
                 snapEncryption: Crypto.defaultAlgorithm,
                 deltasEncryption: Crypto.defaultAlgorithm,
                 snapshot: Utils.deepCopy(this._snapshot.data),
                 deltas: [],
                 updates: []});
      return;
    }

    let ret = {status: -1,
               formatVersion: null, maxVersion: null, snapVersion: null,
               snapEncryption: null, deltasEncryption: null,
               snapshot: null, deltas: null, updates: null};
    let deltas, allDeltas;
    let snap = new SnapshotStore();

    
    if (status.formatVersion > ENGINE_STORAGE_FORMAT_VERSION) {
      this._log.error("Server uses storage format v" + status.formatVersion +
                      ", this client understands up to v" + ENGINE_STORAGE_FORMAT_VERSION);
      throw "Incompatible server format for engine data";
    }

    this._getSymKey.async(this, self.cb);
    yield;

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
      this._log.trace("Local snapshot version < server snapVersion");

      if (this._snapshot.version >= 0)
        this._log.info("Local snapshot is out of date");

      this._log.info("Downloading server snapshot");
      this._remote.snapshot.get(self.cb); 
      snap.data = yield;

      this._log.info("Downloading server deltas");
      this._remote.deltas.get(self.cb); 
      allDeltas = yield;
      deltas = allDeltas;

    } else if (this._snapshot.version >= status.snapVersion &&
               this._snapshot.version < status.maxVersion) {
      this._log.trace("Server snapVersion <= local snapshot version < server maxVersion");
      snap.data = Utils.deepCopy(this._snapshot.data);

      this._log.info("Downloading server deltas");
      this._remote.deltas.get(self.cb); 
      allDeltas = yield;
      deltas = allDeltas.slice(this._snapshot.version - status.snapVersion);

    } else if (this._snapshot.version == status.maxVersion) {
      this._log.trace("Local snapshot version == server maxVersion");
      snap.data = Utils.deepCopy(this._snapshot.data);

      
      this._log.info("Downloading server deltas");
      this._remote.deltas.get(self.cb); 
      allDeltas = yield;
      deltas = [];

    } else { 
      this._log.error("Server snapshot is older than local snapshot");
      throw "Server snapshot is older than local snapshot";
    }

    try {
      for (var i = 0; i < deltas.length; i++) {
        snap.applyCommands.async(snap, self.cb, deltas[i]);
        yield;
      }
    } catch (e) {
      this._log.error("Error applying remote deltas to saved snapshot");
      this._log.error("Clearing local snapshot; next sync will merge");
      this._log.debug("Exception: " + Utils.exceptionStr(e));
      this._log.trace("Stack:\n" + Utils.stackTrace(e));
      this._snapshot.wipe();
      throw e;
    }

    ret.status = 0;
    ret.formatVersion = status.formatVersion;
    ret.maxVersion = status.maxVersion;
    ret.snapVersion = status.snapVersion;
    ret.snapEncryption = status.snapEncryption;
    ret.deltasEncryption = status.deltasEncryption;
    ret.snapshot = snap.data;
    ret.deltas = allDeltas;
    this._core.detectUpdates(self.cb, this._snapshot.data, snap.data);
    ret.updates = yield;

    self.done(ret)
  },

  _fullUpload: function Engine__fullUpload() {
    let self = yield;
    let ret = false;

    Crypto.PBEkeygen.async(Crypto, self.cb);
    let symkey = yield;
    this._engineId.setTempPassword(symkey);
    if (!this._engineId.password)
      throw "Could not generate a symmetric encryption key";

    let enckey = this._engineId.password;
    if ("none" != Utils.prefs.getCharPref("encryption")) {
      Crypto.RSAencrypt.async(Crypto, self.cb,
                              this._engineId.password, this._pbeId);
      enckey = yield;
    }

    if (!enckey)
      throw "Could not encrypt symmetric encryption key";

    let keys = {ring: {}};
    keys.ring[this._engineId.userHash] = enckey;
    this._remote.keys.put(self.cb, keys);
    yield;

    this._remote.snapshot.put(self.cb, this._snapshot.wrap());
    yield;
    this._remote.deltas.put(self.cb, []);
    yield;

    let c = 0;
    for (GUID in this._snapshot.data)
      c++;

    this._remote.status.put(self.cb,
                            {GUID: this._snapshot.GUID,
                             formatVersion: ENGINE_STORAGE_FORMAT_VERSION,
                             snapVersion: this._snapshot.version,
                             maxVersion: this._snapshot.version,
                             snapEncryption: Crypto.defaultAlgorithm,
                             deltasEncryption: "none",
                             itemCount: c});
    yield;

    this._log.info("Full upload to server successful");
    ret = true;
    self.done(ret);
  },

  _share: function Engine__share(username) {
    let self = yield;
    let prefix = DAV.defaultPrefix;

    this._log.debug("Sharing bookmarks with " + username);

    this._getSymKey.async(this, self.cb);
    yield;

    
    DAV.GET(this.keysFile, self.cb);
    let ret = yield;
    Utils.ensureStatus(ret.status, "Could not get keys file.");
    let keys = this._json.decode(ret.responseText);

    
    let hash = Utils.sha1(username);
    let serverURL = Utils.prefs.getCharPref("serverURL");

    try {
      DAV.defaultPrefix = "user/" + hash + "/";  
      DAV.GET("public/pubkey", self.cb);
      ret = yield;
    }
    catch (e) { throw e; }
    finally { DAV.defaultPrefix = prefix; }

    Utils.ensureStatus(ret.status, "Could not get public key for " + username);

    let id = new Identity();
    id.pubkey = ret.responseText;

    
    Crypto.RSAencrypt.async(Crypto, self.cb, this._engineId.password, id);
    let enckey = yield;
    if (!enckey)
      throw "Could not encrypt symmetric encryption key";

    keys.ring[hash] = enckey;
    DAV.PUT(this.keysFile, this._json.encode(keys), self.cb);
    ret = yield;
    Utils.ensureStatus(ret.status, "Could not upload keyring file.");

    this._createShare(username, username);

    this._log.debug("All done sharing!");

    self.done(true);
  },

  
  _createShare: function Engine__createShare(id, title) {
    let bms = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
      getService(Ci.nsINavBookmarksService);
    let ans = Cc["@mozilla.org/browser/annotation-service;1"].
      getService(Ci.nsIAnnotationService);

    let root;
    let a = ans.getItemsWithAnnotation("weave/mounted-shares-folder", {});
    if (a.length == 1)
      root = a[0];

    if (!root) {
      root = bms.createFolder(bms.toolbarFolder, "Shared Folders",
                              bms.DEFAULT_INDEX);
      ans.setItemAnnotation(root, "weave/mounted-shares-folder", true, 0,
                            ans.EXPIRE_NEVER);
    }

    let item;
    a = ans.getItemsWithAnnotation("weave/mounted-share-id", {});
    for (let i = 0; i < a.length; i++) {
      if (ans.getItemAnnotation(a[i], "weave/mounted-share-id") == id) {
        item = a[i];
        break;
      }
    }

    if (!item) {
      let newId = bms.createFolder(root, title, bms.DEFAULT_INDEX);
      ans.setItemAnnotation(newId, "weave/mounted-share-id", id, 0,
                            ans.EXPIRE_NEVER);
    }
  },

  sync: function Engine_sync(onComplete) {
    return this._sync.async(this, onComplete);
  },

  share: function Engine_share(onComplete, username) {
    return this._share.async(this, onComplete, username);
  },

  resetServer: function Engine_resetServer(onComplete) {
    this._notify("reset-server", this._resetServer).async(this, onComplete);
  },

  resetClient: function Engine_resetClient(onComplete) {
    this._notify("reset-client", this._resetClient).async(this, onComplete);
  }
};

function BookmarksEngine(pbeId) {
  this._init(pbeId);
}
BookmarksEngine.prototype = {
  get name() { return "bookmarks"; },
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
  },

  __tracker: null,
  get _tracker() {
    if (!this.__tracker)
      this.__tracker = new BookmarksTracker();
    return this.__tracker;
  },

  syncMounts: function BmkEngine_syncMounts(onComplete) {
    this._syncMounts.async(this, onComplete);
  },
  _syncMounts: function BmkEngine__syncMounts() {
    let self = yield;
    let mounts = this._store.findMounts();

    for (i = 0; i < mounts.length; i++) {
      try {
        this._syncOneMount.async(this, self.cb, mounts[i]);
        yield;
      } catch (e) {
        this._log.warn("Could not sync shared folder from " + mounts[i].userid);
        this._log.trace(Utils.stackTrace(e));
      }
    }
  },

  _syncOneMount: function BmkEngine__syncOneMount(mountData) {
    let self = yield;
    let user = mountData.userid;
    let prefix = DAV.defaultPrefix;
    let serverURL = Utils.prefs.getCharPref("serverURL");
    let snap = new SnapshotStore();

    this._log.debug("Syncing shared folder from user " + user);

    try {
      let hash = Utils.sha1(user);
      DAV.defaultPrefix = "user/" + hash + "/";  

      this._getSymKey.async(this, self.cb);
      yield;

      this._log.trace("Getting status file for " + user);
      DAV.GET(this.statusFile, self.cb);
      let resp = yield;
      Utils.ensureStatus(resp.status, "Could not download status file.");
      let status = this._json.decode(resp.responseText);

      this._log.trace("Downloading server snapshot for " + user);
      DAV.GET(this.snapshotFile, self.cb);
      resp = yield;
      Utils.ensureStatus(resp.status, "Could not download snapshot.");
      Crypto.PBEdecrypt.async(Crypto, self.cb, resp.responseText,
    			        this._engineId, status.snapEncryption);
      let data = yield;
      snap.data = this._json.decode(data);

      this._log.trace("Downloading server deltas for " + user);
      DAV.GET(this.deltasFile, self.cb);
      resp = yield;
      Utils.ensureStatus(resp.status, "Could not download deltas.");
      Crypto.PBEdecrypt.async(Crypto, self.cb, resp.responseText,
    			        this._engineId, status.deltasEncryption);
      data = yield;
      deltas = this._json.decode(data);
    }
    catch (e) { throw e; }
    finally { DAV.defaultPrefix = prefix; }

    
    for (var i = 0; i < deltas.length; i++) {
      snap.applyCommands.async(snap, self.cb, deltas[i]);
      yield;
    }

    
    for (let guid in snap.data) {
      if (snap.data[guid].type != "bookmark")
        delete snap.data[guid];
      else
        snap.data[guid].parentGUID = mountData.rootGUID;
    }

    this._log.trace("Got bookmarks fror " + user + ", comparing with local copy");
    this._core.detectUpdates(self.cb, mountData.snapshot, snap.data);
    let diff = yield;

    
    this._log.trace("Applying changes to folder from " + user);
    this._store.applyCommands.async(this._store, self.cb, diff);
    yield;

    this._log.trace("Shared folder from " + user + " successfully synced!");
  }
};
BookmarksEngine.prototype.__proto__ = new Engine();

function HistoryEngine(pbeId) {
  this._init(pbeId);
}
HistoryEngine.prototype = {
  get name() { return "history"; },
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
  },

  __tracker: null,
  get _tracker() {
    if (!this.__tracker)
      this.__tracker = new HistoryTracker();
    return this.__tracker;
  }
};
HistoryEngine.prototype.__proto__ = new Engine();

function CookieEngine(pbeId) {
  this._init(pbeId);
}
CookieEngine.prototype = {
  get name() { return "cookies"; },
  get logName() { return "CookieEngine"; },
  get serverPrefix() { return "user-data/cookies/"; },

  __core: null,
  get _core() {
    if (!this.__core)
      this.__core = new CookieSyncCore();
    return this.__core;
  },

  __store: null,
  get _store() {
    if (!this.__store)
      this.__store = new CookieStore();
    return this.__store;
  },

  __tracker: null,
  get _tracker() {
    if (!this.__tracker)
      this.__tracker = new CookieTracker();
    return this.__tracker;
  }
};
CookieEngine.prototype.__proto__ = new Engine();

function PasswordEngine(pbeId) {
  this._init(pbeId);
}
PasswordEngine.prototype = {
  get name() { return "passwords"; },
  get logName() { return "PasswordEngine"; },
  get serverPrefix() { return "user-data/passwords/"; },

  __core: null,
  get _core() {
    if (!this.__core) {
      this.__core = new PasswordSyncCore();
      this.__core._hashLoginInfo = this._hashLoginInfo;
    }
    return this.__core;
  },

  __store: null,
  get _store() {
    if (!this.__store) {
      this.__store = new PasswordStore();
      this.__store._hashLoginInfo = this._hashLoginInfo;
    }
    return this.__store;
  },

  








  _hashLoginInfo : function (aLogin) {
    var loginKey = aLogin.hostname      + ":" +
                   aLogin.formSubmitURL + ":" +
                   aLogin.httpRealm     + ":" +
                   aLogin.username      + ":" +
                   aLogin.password      + ":" +
                   aLogin.usernameField + ":" +
                   aLogin.passwordField;

    return Utils.sha1(loginKey);
  }
};
PasswordEngine.prototype.__proto__ = new Engine();

function FormEngine(pbeId) {
  this._init(pbeId);
}
FormEngine.prototype = {
  get name() { return "forms"; },
  get logName() { return "FormEngine"; },
  get serverPrefix() { return "user-data/forms/"; },

  __core: null,
  get _core() {
    if (!this.__core)
      this.__core = new FormSyncCore();
    return this.__core;
  },

  __store: null,
  get _store() {
    if (!this.__store)
      this.__store = new FormStore();
    return this.__store;
  },

  __tracker: null,
  get _tracker() {
    if (!this.__tracker)
      this.__tracker = new FormsTracker();
    return this.__tracker;
  }
};
FormEngine.prototype.__proto__ = new Engine();
