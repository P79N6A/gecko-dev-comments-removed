




































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
      url += '/';
    url += "0.3/user/";
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

  get outgoing() {
    if (!this._outgoing)
      this._outgoing = {};
    return this._outgoing;
  },

  
  _createRecord: function SyncEngine__createRecord(id) {
    let record = this._store.createRecord(id);
    record.uri = this.engineURL + id;
    record.encryption = this.cryptoMetaURL;
    return record;
  },

  
  
  
  
  _recordLike: function SyncEngine__recordLike(a, b) {
    if (a.parentid != b.parentid)
      return false;
    if (a.depth != b.depth)
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

    
    
    
    if (!this.lastSync) {
      this._log.info("First sync, uploading all items");

      
      this._tracker.clearChangedIDs();

      
      let all = this._store.getAllIDs();
      for (let id in all) {
        this._tracker.changedIDs[id] = true;
      }
    }

    this._tracker.disable(); 
  },

  
  _processIncoming: function SyncEngine__processIncoming() {
    let self = yield;

    this._log.debug("Downloading & applying server changes");

    let newitems = new Collection(this.engineURL);
    newitems.modified = this.lastSync;
    newitems.full = true;
    newitems.sort = "depthindex";
    yield newitems.get(self.cb);

    let mem = Cc["@mozilla.org/xpcom/memory-service;1"].getService(Ci.nsIMemory);
    this._lastSyncTmp = 0;
    let item;
    while ((item = yield newitems.iter.next(self.cb))) {
      if (mem.isLowMemory()) {
        this._log.warn("Low memory, forcing GC");
        Cu.forceGC();
        if (mem.isLowMemory()) {
          this._log.warn("Low memory, aborting sync!");
          throw "Low memory";
        }
      }
      yield item.decrypt(self.cb, ID.get('WeaveCryptoID').password);
      if (yield this._reconcile.async(this, self.cb, item))
        yield this._applyIncoming.async(this, self.cb, item);
      else {
        this._log.debug("Skipping reconciled incoming item " + item.id);
        if (this._lastSyncTmp < item.modified)
          this._lastSyncTmp = item.modified;
      }
    }
    if (this.lastSync < this._lastSyncTmp)
        this.lastSync = this._lastSyncTmp;
  },

  
  
  
  
  
  
  
  
  
  
  
  
  _reconcile: function SyncEngine__reconcile(item) {
    let self = yield;
    let ret = true;

    
    if (item.id in this._tracker.changedIDs) {
      
      let out = this._createRecord(item.id);
      if (Utils.deepEquals(item.cleartext, out.cleartext)) {
        this._tracker.removeChangedID(item.id);
        delete this.outgoing[item.id];
      } else {
        this._log.debug("Discarding server change due to conflict with local change");
      }
      self.done(false);
      return;
    }

    
    if (this._store.itemExists(item.id)) {
      self.done(true);
      return;
    }

    
    for (let id in this._tracker.changedIDs) {
      
      let out = (id in this.outgoing)?
        this.outgoing[id] : this._createRecord(id);

      
      if ([i for (i in this.outgoing)].length <= 100)
        this.outgoing[id] = out;

      if (this._recordLike(item, out)) {
        
        
        yield this._changeRecordRefs.async(this, self.cb, id, item.id);
        this._store.changeItemID(id, item.id);

        this._tracker.removeChangedID(item.id);
        delete this.outgoing[item.id];

        self.done(false);
        return;
      }
    }
    self.done(true);
  },

  
  _applyIncoming: function SyncEngine__applyIncoming(item) {
    let self = yield;
    this._log.debug("Applying incoming record");
    this._log.trace("Incoming:\n" + item);
    try {
      yield this._store.applyIncoming(self.cb, item);
      if (this._lastSyncTmp < item.modified)
        this._lastSyncTmp = item.modified;
    } catch (e) {
      this._log.warn("Error while applying incoming record: " +
                     (e.message? e.message : e));
    }
  },

  
  _uploadOutgoing: function SyncEngine__uploadOutgoing() {
    let self = yield;

    if (this.outgoing.length) {
      this._log.debug("Uploading client changes (" + this.outgoing.length + ")");

      
      let up = new Collection(this.engineURL);

      
      
      let depth = {};

      let out;
      while ((out = this.outgoing.pop())) {
        this._log.trace("Outgoing:\n" + out);
        yield out.encrypt(self.cb, ID.get('WeaveCryptoID').password);
        yield up.pushRecord(self.cb, out);
        this._store.wrapDepth(out.id, depth);
      }

      
      this._log.trace(depth.length + "outgoing depth records");
      for (let id in depth) {
        up.pushDepthRecord({id: id, depth: depth[id]});
      }
      

      
      yield up.post(self.cb);

      
      let mod = up.data.modified;
      if (typeof(mod) == "string")
        mod = parseInt(mod);
      if (mod > this.lastSync)
        this.lastSync = mod;
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
      yield this._processIncoming.async(this, self.cb);
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
