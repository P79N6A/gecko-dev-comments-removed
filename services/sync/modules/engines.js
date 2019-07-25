




































const EXPORTED_SYMBOLS = ['Engines', 'Engine', 'SyncEngine'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://weave/ext/Observers.js");
Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/constants.js");
Cu.import("resource://weave/util.js");
Cu.import("resource://weave/wrap.js");
Cu.import("resource://weave/resource.js");
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
  this._log = Log4Moz.repository.getLogger("Service.Engines");
  this._log.level = Log4Moz.Level[Svc.Prefs.get(
    "log.logger.service.engines", "Debug")];
}
EngineManagerSvc.prototype = {
  get: function EngMgr_get(name) {
    
    if (Utils.isArray(name)) {
      let engines = [];
      name.forEach(function(name) {
        let engine = this.get(name);
        if (engine)
          engines.push(engine);
      }, this);
      return engines;
    }

    let engine = this._engines[name];
    if (!engine)
      this._log.debug("Could not get engine: " + name);
    return engine;
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

  







  register: function EngMgr_register(engineObject) {
    if (Utils.isArray(engineObject))
      return engineObject.map(this.register, this);

    try {
      let engine = new engineObject();
      this._engines[engine.name] = engine;
    }
    catch(ex) {
      let mesg = ex.message ? ex.message : ex;
      let name = engineObject || "";
      name = name.prototype || "";
      name = name.name || "";

      let out = "Could not initialize engine '" + name + "': " + mesg;
      dump(out);
      this._log.error(out);

      return engineObject;
    }
  },
  unregister: function EngMgr_unregister(val) {
    let name = val;
    if (val instanceof Engine)
      name = val.name;
    delete this._engines[name];
  }
};

function Engine() { this._init(); }
Engine.prototype = {
  name: "engine",
  displayName: "Boring Engine",
  logName: "Engine",

  

  _storeObj: Store,
  _trackerObj: Tracker,

  get enabled() Utils.prefs.getBoolPref("engine." + this.name),
  get score() this._tracker.score,

  get _store() {
    if (!this.__store)
      this.__store = new this._storeObj();
    return this.__store;
  },

  get _tracker() {
    if (!this.__tracker)
      this.__tracker = new this._trackerObj();
    return this.__tracker;
  },

  _init: function Engine__init() {
    let levelPref = "log.logger.engine." + this.name;
    let level = "Debug";
    try { level = Utils.prefs.getCharPref(levelPref); }
    catch (e) {  }

    this._notify = Wrap.notify("weave:engine:");
    this._log = Log4Moz.repository.getLogger("Engine." + this.logName);
    this._log.level = Log4Moz.Level[level];

    this._tracker; 
    this._log.debug("Engine initialized");
  },

  sync: function Engine_sync(onComplete) {
    if (!this._sync)
      throw "engine does not implement _sync method";
    this._notify("sync", this.name, this._sync).async(this, onComplete);
  },

  wipeServer: function Engimne_wipeServer(onComplete) {
    if (!this._wipeServer)
      throw "engine does not implement _wipeServer method";
    this._notify("wipe-server", this.name, this._wipeServer).async(this, onComplete);
  },

  


  resetClient: function Engine_resetClient(onComplete) {
    if (!this._resetClient)
      throw "engine does not implement _resetClient method";

    this._notify("reset-client", this.name, this._resetClient).
      async(this, onComplete);
  },

  _wipeClient: function Engine__wipeClient() {
    let self = yield;

    yield this.resetClient(self.cb);

    this._log.debug("Deleting all local data");
    this._store.wipe();
  },

  wipeClient: function Engine_wipeClient(onComplete) {
    this._notify("wipe-client", this.name, this._wipeClient).async(this, onComplete);
  }
};

function SyncEngine() { this._init(); }
SyncEngine.prototype = {
  __proto__: Engine.prototype,

  _recordObj: CryptoWrapper,

  get baseURL() {
    let url = Svc.Prefs.get("clusterURL");
    if (!url)
      return null;
    if (url[url.length-1] != '/')
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
    return Svc.Prefs.get(this.name + ".lastSync", 0);
  },
  set lastSync(value) {
    Svc.Prefs.reset(this.name + ".lastSync");
    if (typeof(value) == "string")
      value = parseInt(value);
    Svc.Prefs.set(this.name + ".lastSync", value);
  },
  resetLastSync: function SyncEngine_resetLastSync() {
    this._log.debug("Resetting " + this.name + " last sync time");
    Svc.Prefs.reset(this.name + ".lastSync");
    Svc.Prefs.set(this.name + ".lastSync", 0);
  },

  
  _createRecord: function SyncEngine__createRecord(id) {
    return this._store.createRecord(id, this.cryptoMetaURL);
  },

  
  
  
  
  _recordLike: function SyncEngine__recordLike(a, b) {
    if (a.parentid != b.parentid)
      return false;
    if (a.depth != b.depth)
      return false;
    
    if (a.deleted || b.deleted)
      return false;
    return Utils.deepEquals(a.cleartext, b.cleartext);
  },

  _lowMemCheck: function SyncEngine__lowMemCheck() {
    if (Svc.Memory.isLowMemory()) {
      this._log.warn("Low memory, forcing GC");
      Cu.forceGC();
      if (Svc.Memory.isLowMemory()) {
        this._log.warn("Low memory, aborting sync!");
        throw "Low memory";
      }
    }
  },

  
  
  _syncStartup: function SyncEngine__syncStartup() {
    let self = yield;

    this._log.debug("Ensuring server crypto records are there");

    let meta = yield CryptoMetas.get(self.cb, this.cryptoMetaURL);
    if (!meta) {
      let symkey = Svc.Crypto.generateRandomKey();
      let pubkey = yield PubKeys.getDefaultKey(self.cb);
      meta = new CryptoMeta(this.cryptoMetaURL);
      meta.generateIV();
      yield meta.addUnwrappedKey(self.cb, pubkey, symkey);
      let res = new Resource(meta.uri);
      yield res.put(self.cb, meta.serialize());
    }

    
    
    
    if (!this.lastSync) {
      this._log.info("First sync, uploading all items");
      this._tracker.clearChangedIDs();
      [i for (i in this._store.getAllIDs())]
        .forEach(function(id) this._tracker.changedIDs[id] = true, this);
    }

    let outnum = [i for (i in this._tracker.changedIDs)].length;
    this._log.info(outnum + " outgoing items pre-reconciliation");
  },

  
  _processIncoming: function SyncEngine__processIncoming() {
    let self = yield;

    this._log.debug("Downloading & applying server changes");

    
    
    
    this._store.cache.enabled = true;
    this._store.cache.fifo = false; 
    this._store.cache.clear();

    let newitems = new Collection(this.engineURL, this._recordObj);
    newitems.newer = this.lastSync;
    newitems.full = true;
    newitems.sort = "depthindex";
    yield newitems.get(self.cb);

    let item;
    let count = {applied: 0, reconciled: 0};
    this._lastSyncTmp = 0;

    while ((item = yield newitems.iter.next(self.cb))) {
      this._lowMemCheck();
      try {
        yield item.decrypt(self.cb, ID.get('WeaveCryptoID').password);
        if (yield this._reconcile.async(this, self.cb, item)) {
          count.applied++;
          yield this._applyIncoming.async(this, self.cb, item);
        } else {
          count.reconciled++;
          this._log.trace("Skipping reconciled incoming item " + item.id);
          if (this._lastSyncTmp < item.modified)
            this._lastSyncTmp = item.modified;
        }
      } catch (e) {
	this._log.error("Could not process incoming record: " +
			Utils.exceptionStr(e));
      }
    }
    if (this.lastSync < this._lastSyncTmp)
        this.lastSync = this._lastSyncTmp;

    this._log.info("Applied " + count.applied + " records, reconciled " +
                    count.reconciled + " records");

    
    this._store.cache.clear();
    Cu.forceGC();
  },

  _isEqual: function SyncEngine__isEqual(item) {
    let local = this._createRecord(item.id);
    this._log.trace("Local record: \n" + local);
    if (item.parentid == local.parentid &&
        item.sortindex == local.sortindex &&
        Utils.deepEquals(item.cleartext, local.cleartext)) {
      this._log.trace("Local record is the same");
      return true;
    } else {
      this._log.trace("Local record is different");
      return false;
    }
  },

  
  
  
  
  
  
  
  
  
  
  
  
  _reconcile: function SyncEngine__reconcile(item) {
    let self = yield;
    let ret = true;

    
    
    this._log.trace("Reconcile step 1");
    if (item.id in this._tracker.changedIDs) {
      if (this._isEqual(item))
        this._tracker.removeChangedID(item.id);
      self.done(false);
      return;
    }

    
    
    this._log.trace("Reconcile step 2");
    if (this._store.itemExists(item.id)) {
      self.done(!this._isEqual(item));
      return;
    }

    
    this._log.trace("Reconcile step 2.5");
    if (item.deleted) {
      self.done(true);
      return;
    }

    
    this._log.trace("Reconcile step 3");
    for (let id in this._tracker.changedIDs) {
      let out = this._createRecord(id);
      if (this._recordLike(item, out)) {
        this._store.changeItemID(id, item.id);
        this._tracker.removeChangedID(id);
        this._tracker.removeChangedID(item.id);
        this._store.cache.clear(); 
        self.done(false);
        return;
      }
    }
    self.done(true);
  },

  
  _applyIncoming: function SyncEngine__applyIncoming(item) {
    let self = yield;
    this._log.trace("Incoming:\n" + item);
    try {
      this._tracker.ignoreAll = true;
      yield this._store.applyIncoming(self.cb, item);
      if (this._lastSyncTmp < item.modified)
        this._lastSyncTmp = item.modified;
    } catch (e) {
      this._log.warn("Error while applying incoming record: " +
                     (e.message? e.message : e));
    } finally {
      this._tracker.ignoreAll = false;
    }
  },

  
  _uploadOutgoing: function SyncEngine__uploadOutgoing() {
    let self = yield;

    let outnum = [i for (i in this._tracker.changedIDs)].length;
    this._log.debug("Preparing " + outnum + " outgoing records");
    if (outnum) {
      
      let up = new Collection(this.engineURL);
      let meta = {};

      
      this._store.cache.enabled = false;

      for (let id in this._tracker.changedIDs) {
        let out = this._createRecord(id);
        this._log.trace("Outgoing:\n" + out);
        if (!out.deleted)
          this._store.createMetaRecords(out.id, meta);
        yield out.encrypt(self.cb, ID.get('WeaveCryptoID').password);
        up.pushData(JSON.parse(out.serialize())); 
      }

      this._store.cache.enabled = true;

      
      
      let count = 0;
      for each (let obj in meta) {
          if (!(obj.id in this._tracker.changedIDs)) {
            up.pushData(obj);
            count++;
          }
      }

      this._log.info("Uploading " + outnum + " records + " + count + " index/depth records)");
      
      yield up.post(self.cb);

      
      let mod = up.data.modified;
      if (mod > this.lastSync)
        this.lastSync = mod;
    }
    this._tracker.clearChangedIDs();
  },

  
  
  _syncFinish: function SyncEngine__syncFinish(error) {
    let self = yield;
    this._log.debug("Finishing up sync");
    this._tracker.resetScore();
  },

  _sync: function SyncEngine__sync() {
    let self = yield;

    try {
      yield this._syncStartup.async(this, self.cb);
      Observers.notify("weave:engine:sync:status", "process-incoming");
      yield this._processIncoming.async(this, self.cb);
      Observers.notify("weave:engine:sync:status", "upload-outgoing");
      yield this._uploadOutgoing.async(this, self.cb);
      yield this._syncFinish.async(this, self.cb);
    }
    catch (e) {
      this._log.warn("Sync failed");
      throw e;
    }
  },

  _wipeServer: function SyncEngine__wipeServer() {
    let self = yield;
    let all = new Resource(this.engineURL);
    yield all.delete(self.cb);
    let crypto = new Resource(this.cryptoMetaURL);
    yield crypto.delete(self.cb);
  },

  _resetClient: function SyncEngine__resetClient() {
    let self = yield;
    this.resetLastSync();
  }
};
