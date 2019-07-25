




































const EXPORTED_SYMBOLS = ['Engines', 'Engine', 'SyncEngine'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://services-sync/base_records/collection.js");
Cu.import("resource://services-sync/base_records/crypto.js");
Cu.import("resource://services-sync/base_records/keys.js");
Cu.import("resource://services-sync/base_records/wbo.js");
Cu.import("resource://services-sync/constants.js");
Cu.import("resource://services-sync/ext/Observers.js");
Cu.import("resource://services-sync/ext/Sync.js");
Cu.import("resource://services-sync/identity.js");
Cu.import("resource://services-sync/log4moz.js");
Cu.import("resource://services-sync/resource.js");
Cu.import("resource://services-sync/stores.js");
Cu.import("resource://services-sync/trackers.js");
Cu.import("resource://services-sync/util.js");



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
    return [engine for ([name, engine] in Iterator(Engines._engines))];
  },
  getEnabled: function EngMgr_getEnabled() {
    return this.getAll().filter(function(engine) engine.enabled);
  },

  







  register: function EngMgr_register(engineObject) {
    if (Utils.isArray(engineObject))
      return engineObject.map(this.register, this);

    try {
      let engine = new engineObject();
      let name = engine.name;
      if (name in this._engines)
        this._log.error("Engine '" + name + "' is already registered!");
      else
        this._engines[name] = engine;
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

function Engine(name) {
  this.Name = name || "Unnamed";
  this.name = name.toLowerCase();

  this._notify = Utils.notify("weave:engine:");
  this._log = Log4Moz.repository.getLogger("Engine." + this.Name);
  let level = Svc.Prefs.get("log.logger.engine." + this.name, "Debug");
  this._log.level = Log4Moz.Level[level];

  this._tracker; 
  this._log.debug("Engine initialized");
}
Engine.prototype = {
  
  _storeObj: Store,
  _trackerObj: Tracker,

  get prefName() this.name,
  get enabled() Svc.Prefs.get("engine." + this.prefName, false),
  set enabled(val) Svc.Prefs.set("engine." + this.prefName, !!val),

  get score() this._tracker.score,

  get _store() {
    let store = new this._storeObj(this.Name);
    this.__defineGetter__("_store", function() store);
    return store;
  },

  get _tracker() {
    let tracker = new this._trackerObj(this.Name);
    this.__defineGetter__("_tracker", function() tracker);
    return tracker;
  },

  sync: function Engine_sync() {
    if (!this.enabled)
      return;

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
          if (stat.min == null || val < stat.min)
            stat.min = val;
          if (stat.max == null || val > stat.max)
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

      this._log.debug(stats);
    }
  },

  


  resetClient: function Engine_resetClient() {
    if (!this._resetClient)
      throw "engine does not implement _resetClient method";

    this._notify("reset-client", this.name, this._resetClient)();
  },

  _wipeClient: function Engine__wipeClient() {
    this.resetClient();
    this._log.debug("Deleting all local data");
    this._tracker.ignoreAll = true;
    this._store.wipe();
    this._tracker.ignoreAll = false;
    this._tracker.clearChangedIDs();
  },

  wipeClient: function Engine_wipeClient() {
    this._notify("wipe-client", this.name, this._wipeClient)();
  }
};

function SyncEngine(name) {
  Engine.call(this, name || "SyncEngine");
  this.loadToFetch();
}
SyncEngine.prototype = {
  __proto__: Engine.prototype,
  _recordObj: CryptoWrapper,
  version: 1,

  get storageURL() Svc.Prefs.get("clusterURL") + Svc.Prefs.get("storageAPI") +
    "/" + ID.get("WeaveID").username + "/storage/",

  get engineURL() this.storageURL + this.name,

  get cryptoMetaURL() this.storageURL + "crypto/" + this.name,

  get metaURL() this.storageURL + "meta/global",

  get syncID() {
    
    let syncID = Svc.Prefs.get(this.name + ".syncID", "");
    return syncID == "" ? this.syncID = Utils.makeGUID() : syncID;
  },
  set syncID(value) {
    Svc.Prefs.set(this.name + ".syncID", value);
  },

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

  
  _createRecord: function SyncEngine__createRecord(id) {
    let record = this._store.createRecord(id);
    record.id = id;
    record.encryption = this.cryptoMetaURL;
    return record;
  },

  
  
  _syncStartup: function SyncEngine__syncStartup() {
    this._log.trace("Ensuring server crypto records are there");

    
    let meta = CryptoMetas.get(this.cryptoMetaURL);
    if (meta) {
      try {
        let pubkey = PubKeys.getDefaultKey();
        let privkey = PrivKeys.get(pubkey.privateKeyUri);
        meta.getKey(privkey, ID.get("WeaveCryptoID"));
      }
      catch(ex) {
        
        this._log.debug("Purging bad data after failed unwrap crypto: " + ex);
        meta = null;
      }
    }
    
    else if (CryptoMetas.response.status != 404) {
      let resp = CryptoMetas.response;
      resp.failureCode = ENGINE_METARECORD_DOWNLOAD_FAIL;
      throw resp;
    }

    
    let metaGlobal = Records.get(this.metaURL);
    let engines = metaGlobal.payload.engines || {};
    let engineData = engines[this.name] || {};

    
    if ((engineData.version || 0) < this.version) {
      this._log.debug("Old engine data: " + [engineData.version, this.version]);

      
      meta = null;
      this.syncID = "";

      
      engineData.version = this.version;
      engineData.syncID = this.syncID;

      
      engines[this.name] = engineData;
      metaGlobal.payload.engines = engines;
      metaGlobal.changed = true;
    }
    
    else if (engineData.version > this.version) {
      let error = new String("New data: " + [engineData.version, this.version]);
      error.failureCode = VERSION_OUT_OF_DATE;
      throw error;
    }
    
    else if (engineData.syncID != this.syncID) {
      this._log.debug("Engine syncIDs: " + [engineData.syncID, this.syncID]);
      this.syncID = engineData.syncID;
      this._resetClient();
    };

    
    if (meta == null) {
      new Resource(this.engineURL).delete();
      this._resetClient();

      
      let symkey = Svc.Crypto.generateRandomKey();
      let pubkey = PubKeys.getDefaultKey();
      meta = new CryptoMeta(this.cryptoMetaURL);
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
      this._log.debug("First sync, uploading all items");
      for (let id in this._store.getAllIDs())
        this._tracker.addChangedID(id, 0);
    }

    let outnum = [i for (i in this._tracker.changedIDs)].length;
    this._log.info(outnum + " outgoing items pre-reconciliation");

    
    this._delete = {};
  },

  
  _processIncoming: function SyncEngine__processIncoming() {
    this._log.trace("Downloading & applying server changes");

    
    
    
    let fetchNum = Infinity;

    let newitems = new Collection(this.engineURL, this._recordObj);
    if (Svc.Prefs.get("client.type") == "mobile") {
      fetchNum = 50;
      newitems.sort = "index";
    }
    newitems.newer = this.lastSync;
    newitems.full = true;
    newitems.limit = fetchNum;

    let count = {applied: 0, reconciled: 0};
    let handled = [];
    newitems.recordHandler = Utils.bind2(this, function(item) {
      
      if (this.lastModified == null || item.modified > this.lastModified)
        this.lastModified = item.modified;

      
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

        
        if (this._store.itemExists(item.id))
          this._tracker.addChangedID(item.id, 0);
      }
      this._tracker.ignoreAll = false;
      Sync.sleep(0);
    });

    
    if (this.lastModified == null || this.lastModified > this.lastSync) {
      let resp = newitems.get();
      if (!resp.success) {
        resp.failureCode = ENGINE_DOWNLOAD_FAIL;
        throw resp;
      }

      
      fetchNum -= handled.length;
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

    
    while (this.toFetch.length > 0 && fetchNum > 0) {
      
      newitems.limit = 0;
      newitems.newer = 0;

      
      let minFetch = Math.min(150, this.toFetch.length, fetchNum);
      newitems.ids = this.toFetch.slice(0, minFetch);
      this.toFetch = this.toFetch.slice(minFetch);
      fetchNum -= minFetch;

      
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
  },

  





  _findDupe: function _findDupe(item) {
    
  },

  _isEqual: function SyncEngine__isEqual(item) {
    let local = this._createRecord(item.id);
    if (this._log.level <= Log4Moz.Level.Trace)
      this._log.trace("Local record: " + local);
    if (Utils.deepEquals(item.cleartext, local.cleartext)) {
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
    
    let preferLocal = dupeId.length < item.id.length ||
      (dupeId.length == item.id.length && dupeId < item.id);

    if (preferLocal) {
      this._log.trace("Preferring local id: " + [dupeId, item.id]);
      this._deleteId(item.id);
      item.id = dupeId;
      this._tracker.addChangedID(dupeId, 0);
    }
    else {
      this._log.trace("Switching local id to incoming: " + [item.id, dupeId]);
      this._store.changeItemID(dupeId, item.id);
      this._deleteId(dupeId);
    }
  },

  
  
  _reconcile: function SyncEngine__reconcile(item) {
    if (this._log.level <= Log4Moz.Level.Trace)
      this._log.trace("Incoming: " + item);

    this._log.trace("Reconcile step 1: Check for conflicts");
    if (item.id in this._tracker.changedIDs) {
      
      if (this._isEqual(item)) {
        this._tracker.removeChangedID(item.id);
        return false;
      }

      
      let recordAge = Resource.serverTime - item.modified;
      let localAge = Date.now() / 1000 - this._tracker.changedIDs[item.id];
      this._log.trace("Record age vs local age: " + [recordAge, localAge]);

      
      return recordAge < localAge;
    }

    this._log.trace("Reconcile step 2: Check for updates");
    if (this._store.itemExists(item.id))
      return !this._isEqual(item);

    this._log.trace("Reconcile step 2.5: Don't dupe deletes");
    if (item.deleted)
      return true;

    this._log.trace("Reconcile step 3: Find dupes");
    let dupeId = this._findDupe(item);
    if (dupeId)
      this._handleDupe(item, dupeId);

    
    return true;
  },

  
  _uploadOutgoing: function SyncEngine__uploadOutgoing() {
    let outnum = [i for (i in this._tracker.changedIDs)].length;
    if (outnum) {
      this._log.trace("Preparing " + outnum + " outgoing records");

      
      let up = new Collection(this.engineURL);
      let count = 0;

      
      let doUpload = Utils.bind2(this, function(desc) {
        this._log.info("Uploading " + desc + " of " + outnum + " records");
        let resp = up.post();
        if (!resp.success) {
          this._log.debug("Uploading records failed: " + resp);
          resp.failureCode = ENGINE_UPLOAD_FAIL;
          throw resp;
        }

        
        let modified = resp.headers["x-weave-timestamp"];
        if (modified > this.lastSync)
          this.lastSync = modified;

        up.clearRecords();
      });

      for (let id in this._tracker.changedIDs) {
        try {
          let out = this._createRecord(id);
          if (this._log.level <= Log4Moz.Level.Trace)
            this._log.trace("Outgoing: " + out);

          out.encrypt(ID.get("WeaveCryptoID"));
          up.pushData(out);
        }
        catch(ex) {
          this._log.warn("Error creating record: " + Utils.exceptionStr(ex));
        }

        
        if ((++count % MAX_UPLOAD_RECORDS) == 0)
          doUpload((count - MAX_UPLOAD_RECORDS) + " - " + count + " out");

        Sync.sleep(0);
      }

      
      if (count % MAX_UPLOAD_RECORDS > 0)
        doUpload(count >= MAX_UPLOAD_RECORDS ? "last batch" : "all");
    }
    this._tracker.clearChangedIDs();
  },

  
  
  _syncFinish: function SyncEngine__syncFinish() {
    this._log.trace("Finishing up sync");
    this._tracker.resetScore();

    let doDelete = Utils.bind2(this, function(key, val) {
      let coll = new Collection(this.engineURL, this._recordObj);
      coll[key] = val;
      coll.delete();
    });

    for (let [key, val] in Iterator(this._delete)) {
      
      delete this._delete[key];

      
      if (key != "ids" || val.length <= 100)
        doDelete(key, val);
      else {
        
        while (val.length > 0) {
          doDelete(key, val.slice(0, 100));
          val = val.slice(100);
        }
      }
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

  _testDecrypt: function _testDecrypt() {
    
    let canDecrypt = false;

    
    let test = new Collection(this.engineURL, this._recordObj);
    test.limit = 1;
    test.sort = "newest";
    test.full = true;
    test.recordHandler = function(record) {
      record.decrypt(ID.get("WeaveCryptoID"));
      canDecrypt = true;
    };

    
    try {
      this._log.trace("Trying to decrypt a record from the server..");
      test.get();
    }
    catch(ex) {
      this._log.debug("Failed test decrypt: " + Utils.exceptionStr(ex));
    }

    return canDecrypt;
  },

  _resetClient: function SyncEngine__resetClient() {
    this.resetLastSync();
    this.toFetch = [];
  }
};
