




































const EXPORTED_SYMBOLS = ['Engines', 'Engine', 'SyncEngine'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://weave/ext/Observers.js");
Cu.import("resource://weave/ext/Sync.js");
Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/constants.js");
Cu.import("resource://weave/util.js");
Cu.import("resource://weave/resource.js");
Cu.import("resource://weave/identity.js");
Cu.import("resource://weave/stores.js");
Cu.import("resource://weave/trackers.js");

Cu.import("resource://weave/base_records/wbo.js");
Cu.import("resource://weave/base_records/keys.js");
Cu.import("resource://weave/base_records/crypto.js");
Cu.import("resource://weave/base_records/collection.js");



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
      let name = engineObject.prototype.name;
      if (name in this._engines)
        this._log.error("Engine '" + name + "' is already registered!");
      else
        this._engines[name] = new engineObject();
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
  description: "An engine example - it doesn't actually sync anything",
  logName: "Engine",

  

  _storeObj: Store,
  _trackerObj: Tracker,

  get enabled() Svc.Prefs.get("engine." + this.name, null),
  set enabled(val) Svc.Prefs.set("engine." + this.name, !!val),

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
    this._notify = Utils.notify("weave:engine:");
    this._log = Log4Moz.repository.getLogger("Engine." + this.logName);
    let level = Svc.Prefs.get("log.logger.engine." + this.name, "Debug");
    this._log.level = Log4Moz.Level[level];

    this._tracker; 
    this._log.debug("Engine initialized");
  },

  sync: function Engine_sync() {
    if (!this._sync)
      throw "engine does not implement _sync method";

    let times = {};
    let wrapped = {};
    
    for (let _name in this) {
      let name = _name;

      
      if (name.search(/^_(.+Obj|notify)$/) == 0)
        continue;

      
      if (typeof this[name] == "function") {
        times[name] = [];
        wrapped[name] = this[name];

        
        this[name] = function() {
          let start = Date.now();
          try {
            return wrapped[name].apply(this, arguments);
          }
          finally {
            times[name].push(Date.now() - start);
          }
        };
      }
    }

    try {
      this._notify("sync", this.name, this._sync)();
    }
    finally {
      
      for (let [name, func] in Iterator(wrapped))
        this[name] = func;

      let stats = {};
      for (let [name, time] in Iterator(times)) {
        
        let num = time.length;
        if (num == 0)
          continue;

        
        let stat = {
          num: num,
          sum: 0
        };
        time.forEach(function(val) {
          if (val < stat.min || stat.min == null)
            stat.min = val;
          if (val > stat.max || stat.max == null)
            stat.max = val;
          stat.sum += val;
        });

        stat.avg = Number((stat.sum / num).toFixed(2));
        stats[name] = stat;
      }

      stats.toString = function() {
        let sums = [];
        for (let [name, stat] in Iterator(this))
          if (stat.sum != null)
            sums.push(name.replace(/^_/, "") + " " + stat.sum);

        
        let nameOrder = ["sync", "processIncoming", "uploadOutgoing",
          "syncStartup", "syncFinish"];
        let getPos = function(str) {
          let pos = nameOrder.indexOf(str.split(" ")[0]);
          return pos != -1 ? pos : Infinity;
        };
        let order = function(a, b) getPos(a) > getPos(b);

        return "Total (ms): " + sums.sort(order).join(", ");
      };

      this._log.info(stats);
    }
  },

  wipeServer: function Engine_wipeServer() {
    if (!this._wipeServer)
      throw "engine does not implement _wipeServer method";
    this._notify("wipe-server", this.name, this._wipeServer)();
  },

  


  resetClient: function Engine_resetClient() {
    if (!this._resetClient)
      throw "engine does not implement _resetClient method";

    this._notify("reset-client", this.name, this._resetClient)();
  },

  _wipeClient: function Engine__wipeClient() {
    this.resetClient();
    this._log.debug("Deleting all local data");
    this._store.wipe();
  },

  wipeClient: function Engine_wipeClient() {
    this._notify("wipe-client", this.name, this._wipeClient)();
  }
};

function SyncEngine() { this._init(); }
SyncEngine.prototype = {
  __proto__: Engine.prototype,

  _recordObj: CryptoWrapper,

  _init: function _init() {
    Engine.prototype._init.call(this);
    this.loadToFetch();
  },

  get storageURL() Svc.Prefs.get("clusterURL") + "0.5/" +
    ID.get("WeaveID").username + "/storage/",

  get engineURL() this.storageURL + this.name,

  get cryptoMetaURL() this.storageURL + "crypto/" + this.name,

  get lastSync() {
    return parseFloat(Svc.Prefs.get(this.name + ".lastSync", "0"));
  },
  set lastSync(value) {
    
    Svc.Prefs.reset(this.name + ".lastSync");
    
    Svc.Prefs.set(this.name + ".lastSync", value.toString());
  },
  resetLastSync: function SyncEngine_resetLastSync() {
    this._log.debug("Resetting " + this.name + " last sync time");
    Svc.Prefs.reset(this.name + ".lastSync");
    Svc.Prefs.set(this.name + ".lastSync", "0");
  },

  get toFetch() this._toFetch,
  set toFetch(val) {
    this._toFetch = val;
    Utils.jsonSave("toFetch/" + this.name, this, val);
  },

  loadToFetch: function loadToFetch() {
    
    this._toFetch = [];
    Utils.jsonLoad("toFetch/" + this.name, this, Utils.bind2(this, function(o)
      this._toFetch = o));
  },

  _makeUploadColl: function _makeUploadColl() {
    return new Collection(this.engineURL);
  },

  
  _createRecord: function SyncEngine__createRecord(id) {
    return this._store.createRecord(id, this.cryptoMetaURL);
  },

  
  
  _syncStartup: function SyncEngine__syncStartup() {
    this._log.debug("Ensuring server crypto records are there");

    let meta = CryptoMetas.get(this.cryptoMetaURL);
    if (!meta) {
      let symkey = Svc.Crypto.generateRandomKey();
      let pubkey = PubKeys.getDefaultKey();
      meta = new CryptoMeta(this.cryptoMetaURL);
      meta.generateIV();
      meta.addUnwrappedKey(pubkey, symkey);
      let res = new Resource(meta.uri);
      let resp = res.put(meta);
      if (!resp.success) {
        this._log.debug("Metarecord upload fail:" + resp);
        resp.failureCode = ENGINE_METARECORD_UPLOAD_FAIL;
        throw resp;
      }

      
      CryptoMetas.set(meta.uri, meta);
    }

    
    
    
    if (!this.lastSync) {
      this._log.info("First sync, uploading all items");
      this._tracker.clearChangedIDs();
      [i for (i in this._store.getAllIDs())]
        .forEach(function(id) this._tracker.changedIDs[id] = true, this);
    }

    let outnum = [i for (i in this._tracker.changedIDs)].length;
    this._log.info(outnum + " outgoing items pre-reconciliation");

    
    this._delete = {};
  },

  
  _processIncoming: function SyncEngine__processIncoming() {
    this._log.debug("Downloading & applying server changes");

    
    
    
    this._store.cache.enabled = true;
    this._store.cache.fifo = false; 
    this._store.cache.clear();

    let newitems = new Collection(this.engineURL, this._recordObj);
    newitems.newer = this.lastSync;
    newitems.full = true;
    newitems.sort = "index";
    newitems.limit = 300;

    let count = {applied: 0, reconciled: 0};
    let handled = [];
    newitems.recordHandler = Utils.bind2(this, function(item) {
      
      handled.push(item.id);

      try {
        item.decrypt(ID.get("WeaveCryptoID"));
        if (this._reconcile(item)) {
          count.applied++;
          this._tracker.ignoreAll = true;
          this._store.applyIncoming(item);
        } else {
          count.reconciled++;
          this._log.trace("Skipping reconciled incoming item " + item.id);
        }
      }
      catch(ex) {
        this._log.warn("Error processing record: " + Utils.exceptionStr(ex));
      }
      this._tracker.ignoreAll = false;
      Sync.sleep(0);
    });

    
    if (this.lastModified > this.lastSync) {
      let resp = newitems.get();
      if (!resp.success) {
        resp.failureCode = ENGINE_DOWNLOAD_FAIL;
        throw resp;
      }
    }

    
    if (handled.length == newitems.limit) {
      let guidColl = new Collection(this.engineURL);
      guidColl.newer = this.lastSync;
      guidColl.sort = "index";

      let guids = guidColl.get();
      if (!guids.success)
        throw guids;

      
      
      let extra = Utils.arraySub(guids.obj, handled);
      if (extra.length > 0)
        this.toFetch = extra.concat(Utils.arraySub(this.toFetch, extra));
    }

    
    if (this.toFetch.length > 0) {
      
      newitems.limit = 0;
      newitems.newer = 0;

      
      
      newitems.ids = this.toFetch.slice(0, 150);
      this.toFetch = this.toFetch.slice(150);

      
      let resp = newitems.get();
      if (!resp.success) {
        resp.failureCode = ENGINE_DOWNLOAD_FAIL;
        throw resp;
      }
        
    }

    if (this.lastSync < this.lastModified)
      this.lastSync = this.lastModified;

    this._log.info(["Records:", count.applied, "applied,", count.reconciled,
      "reconciled,", this.toFetch.length, "left to fetch"].join(" "));

    
    this._store.cache.clear();
    Cu.forceGC();
  },

  





  _findDupe: function _findDupe(item) {
    
  },

  _isEqual: function SyncEngine__isEqual(item) {
    let local = this._createRecord(item.id);
    if (this._log.level <= Log4Moz.Level.Trace)
      this._log.trace("Local record: " + local);
    if (item.parentid == local.parentid &&
        item.sortindex == local.sortindex &&
        item.deleted == local.deleted &&
        Utils.deepEquals(item.cleartext, local.cleartext)) {
      this._log.trace("Local record is the same");
      return true;
    } else {
      this._log.trace("Local record is different");
      return false;
    }
  },

  _deleteId: function _deleteId(id) {
    this._tracker.removeChangedID(id);

    
    if (this._delete.ids == null)
      this._delete.ids = [id];
    else
      this._delete.ids.push(id);
  },

  _handleDupe: function _handleDupe(item, dupeId) {
    
    if (dupeId < item.id) {
      this._deleteId(item.id);
      item.id = dupeId;
      this._tracker.changedIDs[dupeId] = true;
    }
    
    else {
      this._store.changeItemID(dupeId, item.id);
      this._deleteId(dupeId);
    }

    this._store.cache.clear(); 
  },

  
  
  
  
  
  
  
  
  
  
  
  
  _reconcile: function SyncEngine__reconcile(item) {
    if (this._log.level <= Log4Moz.Level.Trace)
      this._log.trace("Incoming: " + item);

    
    
    this._log.trace("Reconcile step 1");
    if (item.id in this._tracker.changedIDs) {
      if (this._isEqual(item))
        this._tracker.removeChangedID(item.id);
      return false;
    }

    
    
    this._log.trace("Reconcile step 2");
    if (this._store.itemExists(item.id))
      return !this._isEqual(item);

    
    this._log.trace("Reconcile step 2.5");
    if (item.deleted)
      return true;

    
    this._log.trace("Reconcile step 3");
    let dupeId = this._findDupe(item);
    if (dupeId)
      this._handleDupe(item, dupeId);

    
    return true;
  },

  
  _uploadOutgoing: function SyncEngine__uploadOutgoing() {
    let outnum = [i for (i in this._tracker.changedIDs)].length;
    if (outnum) {
      this._log.debug("Preparing " + outnum + " outgoing records");

      
      let up = this._makeUploadColl();
      let count = 0;

      
      let doUpload = Utils.bind2(this, function(desc) {
        this._log.info("Uploading " + desc + " of " + outnum + " records");
        let resp = up.post();
        if (!resp.success) {
          this._log.debug("Uploading records failed: " + resp);
          resp.failureCode = ENGINE_UPLOAD_FAIL;
          throw resp;
        }

        
        let modified = resp.headers["X-Weave-Timestamp"];
        if (modified > this.lastSync)
          this.lastSync = modified;

        up.clearRecords();
      });

      
      this._store.cache.enabled = false;

      for (let id in this._tracker.changedIDs) {
        let out = this._createRecord(id);
        if (this._log.level <= Log4Moz.Level.Trace)
          this._log.trace("Outgoing: " + out);

        out.encrypt(ID.get("WeaveCryptoID"));
        up.pushData(out);

        
        if ((++count % MAX_UPLOAD_RECORDS) == 0)
          doUpload((count - MAX_UPLOAD_RECORDS) + " - " + count + " out");

        Sync.sleep(0);
      }

      
      if (count % MAX_UPLOAD_RECORDS > 0)
        doUpload(count >= MAX_UPLOAD_RECORDS ? "last batch" : "all");

      this._store.cache.enabled = true;
    }
    this._tracker.clearChangedIDs();
  },

  
  
  _syncFinish: function SyncEngine__syncFinish() {
    this._log.trace("Finishing up sync");
    this._tracker.resetScore();

    for (let [key, val] in Iterator(this._delete)) {
      
      delete this._delete[key];

      
      let coll = new Collection(this.engineURL, this._recordObj);
      coll[key] = val;
      coll.delete();
    }
  },

  _sync: function SyncEngine__sync() {
    try {
      this._syncStartup();
      Observers.notify("weave:engine:sync:status", "process-incoming");
      this._processIncoming();
      Observers.notify("weave:engine:sync:status", "upload-outgoing");
      this._uploadOutgoing();
      this._syncFinish();
    }
    catch (e) {
      this._log.warn("Sync failed");
      throw e;
    }
  },

  _wipeServer: function SyncEngine__wipeServer() {
    new Resource(this.engineURL).delete();
    new Resource(this.cryptoMetaURL).delete();
  },

  _resetClient: function SyncEngine__resetClient() {
    this.resetLastSync();
    this.toFetch = [];
  }
};
