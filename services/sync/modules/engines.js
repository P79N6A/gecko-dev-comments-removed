




































const EXPORTED_SYMBOLS = ['Engines', 'NewEngine', 'Engine', 'SyncEngine', 'BlobEngine'];

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
Cu.import("resource://weave/resource.js");
Cu.import("resource://weave/clientData.js");
Cu.import("resource://weave/identity.js");
Cu.import("resource://weave/stores.js");
Cu.import("resource://weave/trackers.js");
Cu.import("resource://weave/async.js");

Cu.import("resource://weave/base_records/wbo.js");
Cu.import("resource://weave/base_records/keys.js");
Cu.import("resource://weave/base_records/crypto.js");
Cu.import("resource://weave/base_records/collection.js");

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
  getEnabled: function EngMgr_getEnabled() {
    let ret = [];
    for (key in this._engines) {
      if(this._engines[key].enabled)
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

function Engine() { }
Engine.prototype = {
  _notify: Wrap.notify,

  
  get name() { throw "name property must be overridden in subclasses"; },

  
  get displayName() { throw "displayName property must be overriden in subclasses"; },

  
  get logName() { throw "logName property must be overridden in subclasses"; },

  
  get serverPrefix() { throw "serverPrefix property must be overridden in subclasses"; },

  get enabled() {
    return Utils.prefs.getBoolPref("engine." + this.name);
  },

  get _os() {
    let os = Cc["@mozilla.org/observer-service;1"].
      getService(Ci.nsIObserverService);
    this.__defineGetter__("_os", function() os);
    return os;
  },

  get _json() {
    let json = Cc["@mozilla.org/dom/json;1"].
      createInstance(Ci.nsIJSON);
    this.__defineGetter__("_json", function() json);
    return json;
  },

  get score() this._tracker.score,

  
  get _store() {
    let store = new Store();
    this.__defineGetter__("_store", function() store);
    return store;
  },

  get _tracker() {
    let tracker = new Tracker();
    this.__defineGetter__("_tracker", function() tracker);
    return tracker;
  },

  get engineId() {
    let id = ID.get('Engine:' + this.name);
    if (!id) {
      
      let masterID = ID.get('WeaveID');

      id = new Identity(this.logName, masterID.username, masterID.password);
      ID.set('Engine:' + this.name, id);
    }
    return id;
  },

  _init: function Engine__init() {
    let levelPref = "log.logger.service.engine." + this.name;
    let level = "Debug";
    try { level = Utils.prefs.getCharPref(levelPref); }
    catch (e) {  }

    this._log = Log4Moz.repository.getLogger("Engine." + this.logName);
    this._log.level = Log4Moz.Level[level];
    this._osPrefix = "weave:" + this.name + ":";

    this._tracker; 

    this._log.debug("Engine initialized");
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
    throw "_resetServer needs to be subclassed";
  },

  _resetClient: function Engine__resetClient() {
    let self = yield;
    this._log.debug("Resetting client state");
    this._store.wipe();
    this._log.debug("Client reset completed successfully");
  },

  _sync: function Engine__sync() {
    let self = yield;
    throw "_sync needs to be subclassed";
  },

  _share: function Engine__share(guid, username) {
    let self = yield;
    


    self.done();
  },

  _stopSharing: function Engine__stopSharing(guid, username) {
    let self = yield;
    


    self.done();
  },

  sync: function Engine_sync(onComplete) {
    return this._sync.async(this, onComplete);
  },

  share: function Engine_share(onComplete, guid, username) {
    return this._share.async(this, onComplete, guid, username);
  },

  stopSharing: function Engine_share(onComplete, guid, username) {
    return this._stopSharing.async(this, onComplete, guid, username);
  },

  resetServer: function Engimne_resetServer(onComplete) {
    this._notify("reset-server", "", this._resetServer).async(this, onComplete);
  },

  resetClient: function Engine_resetClient(onComplete) {
    this._notify("reset-client", "", this._resetClient).async(this, onComplete);
  }
};

function SyncEngine() {  }
SyncEngine.prototype = {
  __proto__: Engine.prototype,

  get baseURL() {
    let url = Utils.prefs.getCharPref("serverURL");
    if (url && url[url.length-1] != '/')
      url = url + '/';
    return url;
  },

  get engineURL() {
    return this.baseURL + ID.get('WeaveID').username + '/' + this.name + '/';
  },

  get cryptoMetaURL() {
    return this.baseURL + ID.get('WeaveID').username + '/crypto/' + this.name;
  },

  get lastSync() {
    try {
      return Utils.prefs.getCharPref(this.name + ".lastSync");
    } catch (e) {
      return 0;
    }
  },
  set lastSync(value) {
    Utils.prefs.setCharPref(this.name + ".lastSync", value);
  },

  
  

  get incoming() {
    if (!this._incoming)
      this._incoming = [];
    return this._incoming;
  },

  get outgoing() {
    if (!this._outgoing)
      this._outgoing = [];
    return this._outgoing;
  },

  
  
  
  _createRecord: function SyncEngine__createRecord(id, encrypt) {
    let self = yield;

    let record = new CryptoWrapper();
    record.uri = this.engineURL + id;
    record.encryption = this.cryptoMetaURL;
    record.cleartext = this._store.wrapItem(id);

    if (record.cleartext && record.cleartext.parentid)
        record.parentid = record.cleartext.parentid;

    if (encrypt || encrypt == undefined)
      yield record.encrypt(self.cb, ID.get('WeaveCryptoID').password);

    self.done(record);
  },

  
  
  
  
  _recordLike: function SyncEngine__recordLike(a, b) {
    if (a.parentid != b.parentid)
      return false;
    return Utils.deepEquals(a.cleartext, b.cleartext);
  },

  _changeRecordRefs: function SyncEngine__changeRecordRefs(oldID, newID) {
    let self = yield;
    for each (let rec in this.outgoing) {
      if (rec.parentid == oldID)
        rec.parentid = newID;
    }
  },

  _recDepth: function SyncEngine__recDepth(rec) {
    
    if (rec.depth)
      return rec.depth;

    
    if (!rec.parentid)
      return 0;

    
    for each (let inc in this.incoming) {
      if (inc.id == rec.parentid) {
        rec.depth = this._recDepth(inc) + 1;
        return rec.depth;
      }
    }

    
    return 0;
  },

  
  
  _syncStartup: function SyncEngine__syncStartup() {
    let self = yield;

    this._log.debug("Ensuring server crypto records are there");

    let meta = yield CryptoMetas.get(self.cb, this.cryptoMetaURL);
    if (!meta) {
      let cryptoSvc = Cc["@labs.mozilla.com/Weave/Crypto;1"].
        getService(Ci.IWeaveCrypto);
      let symkey = cryptoSvc.generateRandomKey();
      let pubkey = yield PubKeys.getDefaultKey(self.cb);
      meta = new CryptoMeta(this.cryptoMetaURL);
      meta.generateIV();
      yield meta.addUnwrappedKey(self.cb, pubkey, symkey);
      yield meta.put(self.cb);
    }
    this._tracker.disable();
  },

  
  _generateOutgoing: function SyncEngine__generateOutgoing() {
    let self = yield;

    this._log.debug("Calculating client changes");

    
    
    
    if (!this.lastSync) {
      this._log.info("First sync, uploading all items");

      
      this._tracker.clearChangedIDs();

      
      let all = this._store.getAllIDs();
      for (let id in all) {
        this._tracker.changedIDs[id] = true;
      }
    }

    

    
    
    
    this._store.cacheItemsHint();

    
    
    
    for (let id in this._tracker.changedIDs) {
      this.outgoing.push(yield this._createRecord.async(this, self.cb, id, false));
    }

    this._store.clearItemCacheHint();
  },

  
  _fetchIncoming: function SyncEngine__fetchIncoming() {
    let self = yield;

    this._log.debug("Downloading server changes");

    let newitems = new Collection(this.engineURL);
    newitems.modified = this.lastSync;
    newitems.full = true;
    yield newitems.get(self.cb);

    let item;
    while ((item = yield newitems.iter.next(self.cb))) {
      this.incoming.push(item);
    }
  },

  
  
  _processIncoming: function SyncEngine__processIncoming() {
    let self = yield;

    this._log.debug("Decrypting and sorting incoming changes");

    for each (let inc in this.incoming) {
      yield inc.decrypt(self.cb, ID.get('WeaveCryptoID').password);
      this._recDepth(inc); 
    }
    this.incoming.sort(function(a, b) {
      if ((typeof(a.depth) == "number" && typeof(b.depth) == "undefined") ||
          (typeof(a.depth) == "number" && b.depth == null) ||
          (a.depth > b.depth))
        return 1;
      if ((typeof(a.depth) == "undefined" && typeof(b.depth) == "number") ||
          (a.depth == null && typeof(b.depth) == "number") ||
          (a.depth < b.depth))
        return -1;
      if (a.cleartext && b.cleartext) {
        if (a.cleartext.index > b.cleartext.index)
          return 1;
        if (a.cleartext.index < b.cleartext.index)
          return -1;
      }
      return 0;
    });
  },

  
  
  
  
  
  
  
  
  
  
  
  
  _reconcile: function SyncEngine__reconcile() {
    let self = yield;

    this._log.debug("Reconciling server/client changes");

    this._log.debug(this.incoming.length + " items coming in, " +
                    this.outgoing.length + " items going out");

    
    let conflicts = [];
    for (let i = 0; i < this.incoming.length; i++) {
      for (let o = 0; o < this.outgoing.length; o++) {
        if (this.incoming[i].id == this.outgoing[o].id) {
          
          
          if (!Utils.deepEquals(this.incoming[i].cleartext,
                                this.outgoing[o].cleartext))
            conflicts.push({in: this.incoming[i], out: this.outgoing[o]});
          else
            delete this.outgoing[o];
          delete this.incoming[i];
          break;
        }
      }
      this._outgoing = this.outgoing.filter(function(n) n); 
    }
    this._incoming = this.incoming.filter(function(n) n); 
    if (conflicts.length)
      this._log.debug("Conflicts found.  Conflicting server changes discarded");

    
    for (let i = 0; i < this.incoming.length; i++) {
      for (let o = 0; o < this.outgoing.length; o++) {
        if (this._recordLike(this.incoming[i], this.outgoing[o])) {
          
          yield this._changeRecordRefs.async(this, self.cb,
                                             this.outgoing[o].id,
                                             this.incoming[i].id);
          
          this._store.changeItemID(this.outgoing[o].id,
                                   this.incoming[i].id);
          delete this.incoming[i];
          delete this.outgoing[o];
          break;
        }
      }
      this._outgoing = this.outgoing.filter(function(n) n); 
    }
    this._incoming = this.incoming.filter(function(n) n); 

    this._log.debug("Reconciliation complete");
    this._log.debug(this.incoming.length + " items coming in, " +
                    this.outgoing.length + " items going out");
  },

  
  _applyIncoming: function SyncEngine__applyIncoming() {
    let self = yield;
    if (this.incoming.length) {
      this._log.debug("Applying server changes");
      let inc;
      while ((inc = this.incoming.shift())) {
        this._log.trace("Incoming record: " + this._json.encode(inc.cleartext));
        try {
          yield this._store.applyIncoming(self.cb, inc);
          if (inc.modified > this.lastSync)
            this.lastSync = inc.modified;
        } catch (e) {
          this._log.warn("Error while applying incoming record: " +
                         (e.message? e.message : e));
        }
      }
    }
  },

  
  _uploadOutgoing: function SyncEngine__uploadOutgoing() {
    let self = yield;
    if (this.outgoing.length) {
      this._log.debug("Uploading client changes");
      let up = new Collection(this.engineURL);
      let out;
      while ((out = this.outgoing.pop())) {
        this._log.trace("Outgoing record: " + this._json.encode(out.cleartext));
        yield out.encrypt(self.cb, ID.get('WeaveCryptoID').password);
        yield up.pushRecord(self.cb, out);
      }
      yield up.post(self.cb);
      if (up.data.modified > this.lastSync)
        this.lastSync = up.data.modified;
    }
    this._tracker.clearChangedIDs();
  },

  
  
  _syncFinish: function SyncEngine__syncFinish(error) {
    let self = yield;
    this._log.debug("Finishing up sync");
    this._tracker.resetScore();
    this._tracker.enable();
  },

  _sync: function SyncEngine__sync() {
    let self = yield;

    try {
      yield this._syncStartup.async(this, self.cb);

      
      yield this._generateOutgoing.async(this, self.cb);
      yield this._fetchIncoming.async(this, self.cb);

      
      yield this._processIncoming.async(this, self.cb);
      yield this._reconcile.async(this, self.cb);

      
      yield this._applyIncoming.async(this, self.cb);
      yield this._uploadOutgoing.async(this, self.cb);

      yield this._syncFinish.async(this, self.cb);
    }
    catch (e) {
      this._log.warn("Sync failed");
      throw e;
    }
    finally {
      this._tracker.enable();
    }
  },

  _resetServer: function SyncEngine__resetServer() {
    let self = yield;
    let all = new Resource(this.engineURL);
    yield all.delete(self.cb);
  }
};

function BlobEngine() {
  
  
}
BlobEngine.prototype = {
  __proto__: Engine.prototype,

  get _profileID() {
    return ClientData.GUID;
  },

  _init: function BlobEngine__init() {
    
    this.__proto__.__proto__.__proto__.__proto__._init.call(this);
    this._keys = new Keychain(this.serverPrefix);
    this._file = new Resource(this.serverPrefix + "data");
    this._file.pushFilter(new JsonFilter());
    this._file.pushFilter(new CryptoFilter(this.engineId));
  },

  _initialUpload: function BlobEngine__initialUpload() {
    let self = yield;
    this._log.info("Initial upload to server");
    yield this._keys.initialize(self.cb, this.engineId);
    this._file.data = {};
    yield this._merge.async(this, self.cb);
    yield this._file.put(self.cb);
  },

  
  
  _merge: function BlobEngine__merge() {
    let self = yield;
    this._file.data[this._profileID] = this._store.wrap();
  },

  
  
  
  
  
  
  _sync: function BlobEngine__sync() {
    let self = yield;

    this._log.info("Beginning sync");
    this._os.notifyObservers(null, "weave:service:sync:engine:start", this.name);

    
    
    

    try {
      if ("none" != Utils.prefs.getCharPref("encryption"))
        yield this._keys.getKeyAndIV(self.cb, this.engineId);
      yield this._file.get(self.cb);
      yield this._merge.async(this, self.cb);
      yield this._file.put(self.cb);

    } catch (e if e.status == 404) {
      yield this._initialUpload.async(this, self.cb);
    }

    this._log.info("Sync complete");
    this._os.notifyObservers(null, "weave:service:sync:engine:success", this.name);
    self.done(true);
  }
};

function HeuristicEngine() {
}
HeuristicEngine.prototype = {
  __proto__: new Engine(),

  get _remote() {
    let remote = new RemoteStore(this);
    this.__defineGetter__("_remote", function() remote);
    return remote;
  },

  get _snapshot() {
    let snap = new SnapshotStore(this.name);
    this.__defineGetter__("_snapshot", function() snap);
    return snap;
  },

  _resetServer: function SyncEngine__resetServer() {
    let self = yield;
    yield this._remote.wipe(self.cb);
  },

  _resetClient: function SyncEngine__resetClient() {
    let self = yield;
    this._log.debug("Resetting client state");
    this._snapshot.wipe();
    this._store.wipe();
    this._log.debug("Client reset completed successfully");
  },

  _initialUpload: function HeuristicEngine__initialUpload() {
    let self = yield;
    this._log.info("Initial upload to server");
    this._snapshot.data = this._store.wrap();
    this._snapshot.version = 0;
    this._snapshot.GUID = null; 
    yield this._remote.initialize(self.cb, this._snapshot);
    this._snapshot.save();
  },

  _sync: function HeuristicEngine__sync() {
    let self = yield;
  }
};
