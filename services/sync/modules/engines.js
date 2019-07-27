



this.EXPORTED_SYMBOLS = [
  "EngineManager",
  "Engine",
  "SyncEngine",
  "Tracker",
  "Store"
];

const {classes: Cc, interfaces: Ci, results: Cr, utils: Cu} = Components;

Cu.import("resource://services-common/async.js");
Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://services-common/observers.js");
Cu.import("resource://services-common/utils.js");
Cu.import("resource://services-sync/constants.js");
Cu.import("resource://services-sync/identity.js");
Cu.import("resource://services-sync/record.js");
Cu.import("resource://services-sync/resource.js");
Cu.import("resource://services-sync/util.js");












this.Tracker = function Tracker(name, engine) {
  if (!engine) {
    throw new Error("Tracker must be associated with an Engine instance.");
  }

  name = name || "Unnamed";
  this.name = this.file = name.toLowerCase();
  this.engine = engine;

  this._log = Log.repository.getLogger("Sync.Tracker." + name);
  let level = Svc.Prefs.get("log.logger.engine." + this.name, "Debug");
  this._log.level = Log.Level[level];

  this._score = 0;
  this._ignored = [];
  this.ignoreAll = false;
  this.changedIDs = {};
  this.loadChangedIDs();

  Svc.Obs.add("weave:engine:start-tracking", this);
  Svc.Obs.add("weave:engine:stop-tracking", this);

  Svc.Prefs.observe("engine." + this.engine.prefName, this);
};

Tracker.prototype = {
  









  get score() {
    return this._score;
  },

  set score(value) {
    this._score = value;
    Observers.notify("weave:engine:score:updated", this.name);
  },

  
  resetScore: function () {
    this._score = 0;
  },

  persistChangedIDs: true,

  



  saveChangedIDs: function (cb) {
    if (!this.persistChangedIDs) {
      this._log.debug("Not saving changedIDs.");
      return;
    }
    Utils.namedTimer(function () {
      this._log.debug("Saving changed IDs to " + this.file);
      Utils.jsonSave("changes/" + this.file, this, this.changedIDs, cb);
    }, 1000, this, "_lazySave");
  },

  loadChangedIDs: function (cb) {
    Utils.jsonLoad("changes/" + this.file, this, function(json) {
      if (json && (typeof(json) == "object")) {
        this.changedIDs = json;
      } else {
        this._log.warn("Changed IDs file " + this.file + " contains non-object value.");
        json = null;
      }
      if (cb) {
        cb.call(this, json);
      }
    });
  },

  
  
  

  ignoreID: function (id) {
    this.unignoreID(id);
    this._ignored.push(id);
  },

  unignoreID: function (id) {
    let index = this._ignored.indexOf(id);
    if (index != -1)
      this._ignored.splice(index, 1);
  },

  addChangedID: function (id, when) {
    if (!id) {
      this._log.warn("Attempted to add undefined ID to tracker");
      return false;
    }

    if (this.ignoreAll || (id in this._ignored)) {
      return false;
    }

    
    if (when == null) {
      when = Math.floor(Date.now() / 1000);
    }

    
    if ((this.changedIDs[id] || -Infinity) < when) {
      this._log.trace("Adding changed ID: " + id + ", " + when);
      this.changedIDs[id] = when;
      this.saveChangedIDs(this.onSavedChangedIDs);
    }

    return true;
  },

  removeChangedID: function (id) {
    if (!id) {
      this._log.warn("Attempted to remove undefined ID to tracker");
      return false;
    }
    if (this.ignoreAll || (id in this._ignored))
      return false;
    if (this.changedIDs[id] != null) {
      this._log.trace("Removing changed ID " + id);
      delete this.changedIDs[id];
      this.saveChangedIDs();
    }
    return true;
  },

  clearChangedIDs: function () {
    this._log.trace("Clearing changed ID list");
    this.changedIDs = {};
    this.saveChangedIDs();
  },

  _isTracking: false,

  
  startTracking: function () {
  },

  stopTracking: function () {
  },

  engineIsEnabled: function () {
    if (!this.engine) {
      
      return true;
    }
    return this.engine.enabled;
  },

  onEngineEnabledChanged: function (engineEnabled) {
    if (engineEnabled == this._isTracking) {
      return;
    }

    if (engineEnabled) {
      this.startTracking();
      this._isTracking = true;
    } else {
      this.stopTracking();
      this._isTracking = false;
      this.clearChangedIDs();
    }
  },

  observe: function (subject, topic, data) {
    switch (topic) {
      case "weave:engine:start-tracking":
        if (!this.engineIsEnabled()) {
          return;
        }
        this._log.trace("Got start-tracking.");
        if (!this._isTracking) {
          this.startTracking();
          this._isTracking = true;
        }
        return;
      case "weave:engine:stop-tracking":
        this._log.trace("Got stop-tracking.");
        if (this._isTracking) {
          this.stopTracking();
          this._isTracking = false;
        }
        return;
      case "nsPref:changed":
        if (data == PREFS_BRANCH + "engine." + this.engine.prefName) {
          this.onEngineEnabledChanged(this.engine.enabled);
        }
        return;
    }
  }
};























this.Store = function Store(name, engine) {
  if (!engine) {
    throw new Error("Store must be associated with an Engine instance.");
  }

  name = name || "Unnamed";
  this.name = name.toLowerCase();
  this.engine = engine;

  this._log = Log.repository.getLogger("Sync.Store." + name);
  let level = Svc.Prefs.get("log.logger.engine." + this.name, "Debug");
  this._log.level = Log.Level[level];

  XPCOMUtils.defineLazyGetter(this, "_timer", function() {
    return Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
  });
}
Store.prototype = {

  _sleep: function _sleep(delay) {
    let cb = Async.makeSyncCallback();
    this._timer.initWithCallback(cb, delay, Ci.nsITimer.TYPE_ONE_SHOT);
    Async.waitForSyncCallback(cb);
  },

  














  applyIncomingBatch: function (records) {
    let failed = [];
    for each (let record in records) {
      try {
        this.applyIncoming(record);
      } catch (ex if (ex.code == Engine.prototype.eEngineAbortApplyIncoming)) {
        
        
        
        throw ex.cause;
      } catch (ex) {
        this._log.warn("Failed to apply incoming record " + record.id);
        this._log.warn("Encountered exception: " + Utils.exceptionStr(ex));
        failed.push(record.id);
      }
    };
    return failed;
  },

  












  applyIncoming: function (record) {
    if (record.deleted)
      this.remove(record);
    else if (!this.itemExists(record.id))
      this.create(record);
    else
      this.update(record);
  },

  

  








  create: function (record) {
    throw "override create in a subclass";
  },

  








  remove: function (record) {
    throw "override remove in a subclass";
  },

  








  update: function (record) {
    throw "override update in a subclass";
  },

  









  itemExists: function (id) {
    throw "override itemExists in a subclass";
  },

  













  createRecord: function (id, collection) {
    throw "override createRecord in a subclass";
  },

  







  changeItemID: function (oldID, newID) {
    throw "override changeItemID in a subclass";
  },

  





  getAllIDs: function () {
    throw "override getAllIDs in a subclass";
  },

  









  wipe: function () {
    throw "override wipe in a subclass";
  }
};

this.EngineManager = function EngineManager(service) {
  this.service = service;

  this._engines = {};

  
  this._declined = new Set();
  this._log = Log.repository.getLogger("Sync.EngineManager");
  this._log.level = Log.Level[Svc.Prefs.get("log.logger.service.engines", "Debug")];
}
EngineManager.prototype = {
  get: function (name) {
    
    if (Array.isArray(name)) {
      let engines = [];
      name.forEach(function(name) {
        let engine = this.get(name);
        if (engine) {
          engines.push(engine);
        }
      }, this);
      return engines;
    }

    let engine = this._engines[name];
    if (!engine) {
      this._log.debug("Could not get engine: " + name);
      if (Object.keys) {
        this._log.debug("Engines are: " + JSON.stringify(Object.keys(this._engines)));
      }
    }
    return engine;
  },

  getAll: function () {
    return [engine for ([name, engine] in Iterator(this._engines))];
  },

  


  getEnabled: function () {
    return this.getAll()
               .filter((engine) => engine.enabled)
               .sort((a, b) => a.syncPriority - b.syncPriority);
  },

  get enabledEngineNames() {
    return [e.name for each (e in this.getEnabled())];
  },

  persistDeclined: function () {
    Svc.Prefs.set("declinedEngines", [...this._declined].join(","));
  },

  


  getDeclined: function () {
    return [...this._declined];
  },

  setDeclined: function (engines) {
    this._declined = new Set(engines);
    this.persistDeclined();
  },

  isDeclined: function (engineName) {
    return this._declined.has(engineName);
  },

  


  decline: function (engines) {
    for (let e of engines) {
      this._declined.add(e);
    }
    this.persistDeclined();
  },

  undecline: function (engines) {
    for (let e of engines) {
      this._declined.delete(e);
    }
    this.persistDeclined();
  },

  




  declineDisabled: function () {
    for (let e of this.getAll()) {
      if (!e.enabled) {
        this._log.debug("Declining disabled engine " + e.name);
        this._declined.add(e.name);
      }
    }
    this.persistDeclined();
  },

  







  register: function (engineObject) {
    if (Array.isArray(engineObject)) {
      return engineObject.map(this.register, this);
    }

    try {
      let engine = new engineObject(this.service);
      let name = engine.name;
      if (name in this._engines) {
        this._log.error("Engine '" + name + "' is already registered!");
      } else {
        this._engines[name] = engine;
      }
    } catch (ex) {
      this._log.error(CommonUtils.exceptionStr(ex));

      let mesg = ex.message ? ex.message : ex;
      let name = engineObject || "";
      name = name.prototype || "";
      name = name.name || "";

      let out = "Could not initialize engine '" + name + "': " + mesg;
      this._log.error(out);

      return engineObject;
    }
  },

  unregister: function (val) {
    let name = val;
    if (val instanceof Engine) {
      name = val.name;
    }
    delete this._engines[name];
  },

  clear: function () {
    for (let name in this._engines) {
      delete this._engines[name];
    }
  },
};

this.Engine = function Engine(name, service) {
  if (!service) {
    throw new Error("Engine must be associated with a Service instance.");
  }

  this.Name = name || "Unnamed";
  this.name = name.toLowerCase();
  this.service = service;

  this._notify = Utils.notify("weave:engine:");
  this._log = Log.repository.getLogger("Sync.Engine." + this.Name);
  let level = Svc.Prefs.get("log.logger.engine." + this.name, "Debug");
  this._log.level = Log.Level[level];

  this._tracker; 
  this._log.debug("Engine initialized");
}
Engine.prototype = {
  
  _storeObj: Store,
  _trackerObj: Tracker,

  
  
  eEngineAbortApplyIncoming: "error.engine.abort.applyincoming",

  get prefName() {
    return this.name;
  },

  get enabled() {
    return Svc.Prefs.get("engine." + this.prefName, false);
  },

  set enabled(val) {
    Svc.Prefs.set("engine." + this.prefName, !!val);
  },

  get score() {
    return this._tracker.score;
  },

  get _store() {
    let store = new this._storeObj(this.Name, this);
    this.__defineGetter__("_store", () => store);
    return store;
  },

  get _tracker() {
    let tracker = new this._trackerObj(this.Name, this);
    this.__defineGetter__("_tracker", () => tracker);
    return tracker;
  },

  sync: function () {
    if (!this.enabled) {
      return;
    }

    if (!this._sync) {
      throw "engine does not implement _sync method";
    }

    this._notify("sync", this.name, this._sync)();
  },

  


  resetClient: function () {
    if (!this._resetClient) {
      throw "engine does not implement _resetClient method";
    }

    this._notify("reset-client", this.name, this._resetClient)();
  },

  _wipeClient: function () {
    this.resetClient();
    this._log.debug("Deleting all local data");
    this._tracker.ignoreAll = true;
    this._store.wipe();
    this._tracker.ignoreAll = false;
    this._tracker.clearChangedIDs();
  },

  wipeClient: function () {
    this._notify("wipe-client", this.name, this._wipeClient)();
  }
};

this.SyncEngine = function SyncEngine(name, service) {
  Engine.call(this, name || "SyncEngine", service);

  this.loadToFetch();
  this.loadPreviousFailed();
}



SyncEngine.kRecoveryStrategy = {
  ignore: "ignore",
  retry:  "retry",
  error:  "error"
};

SyncEngine.prototype = {
  __proto__: Engine.prototype,
  _recordObj: CryptoWrapper,
  version: 1,

  
  _defaultSort: undefined,

  
  
  
  
  
  syncPriority: 0,

  
  
  downloadLimit: null,

  
  
  guidFetchBatchSize: DEFAULT_GUID_FETCH_BATCH_SIZE,
  mobileGUIDFetchBatchSize: DEFAULT_MOBILE_GUID_FETCH_BATCH_SIZE,

  
  applyIncomingBatchSize: DEFAULT_STORE_BATCH_SIZE,

  get storageURL() {
    return this.service.storageURL;
  },

  get engineURL() {
    return this.storageURL + this.name;
  },

  get cryptoKeysURL() {
    return this.storageURL + "crypto/keys";
  },

  get metaURL() {
    return this.storageURL + "meta/global";
  },

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
  resetLastSync: function () {
    this._log.debug("Resetting " + this.name + " last sync time");
    Svc.Prefs.reset(this.name + ".lastSync");
    Svc.Prefs.set(this.name + ".lastSync", "0");
    this.lastSyncLocal = 0;
  },

  get toFetch() {
    return this._toFetch;
  },
  set toFetch(val) {
    let cb = (error) => this._log.error(Utils.exceptionStr(error));
    
    if (val + "" == this._toFetch) {
      return;
    }
    this._toFetch = val;
    Utils.namedTimer(function () {
      Utils.jsonSave("toFetch/" + this.name, this, val, cb);
    }, 0, this, "_toFetchDelay");
  },

  loadToFetch: function () {
    
    this._toFetch = [];
    Utils.jsonLoad("toFetch/" + this.name, this, function(toFetch) {
      if (toFetch) {
        this._toFetch = toFetch;
      }
    });
  },

  get previousFailed() {
    return this._previousFailed;
  },
  set previousFailed(val) {
    let cb = (error) => this._log.error(Utils.exceptionStr(error));
    
    if (val + "" == this._previousFailed) {
      return;
    }
    this._previousFailed = val;
    Utils.namedTimer(function () {
      Utils.jsonSave("failed/" + this.name, this, val, cb);
    }, 0, this, "_previousFailedDelay");
  },

  loadPreviousFailed: function () {
    
    this._previousFailed = [];
    Utils.jsonLoad("failed/" + this.name, this, function(previousFailed) {
      if (previousFailed) {
        this._previousFailed = previousFailed;
      }
    });
  },

  


  get lastSyncLocal() {
    return parseInt(Svc.Prefs.get(this.name + ".lastSyncLocal", "0"), 10);
  },
  set lastSyncLocal(value) {
    
    Svc.Prefs.set(this.name + ".lastSyncLocal", value.toString());
  },

  




  getChangedIDs: function () {
    return this._tracker.changedIDs;
  },

  
  _createRecord: function (id) {
    let record = this._store.createRecord(id, this.name);
    record.id = id;
    record.collection = this.name;
    return record;
  },

  
  _syncStartup: function () {

    
    let metaGlobal = this.service.recordManager.get(this.metaURL);
    let engines = metaGlobal.payload.engines || {};
    let engineData = engines[this.name] || {};

    let needsWipe = false;

    
    if ((engineData.version || 0) < this.version) {
      this._log.debug("Old engine data: " + [engineData.version, this.version]);

      
      needsWipe = true;
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

    
    
    if (needsWipe) {
      this.wipeServer();
    }

    
    
    
    
    
    
    this.lastSyncLocal = Date.now();
    if (this.lastSync) {
      this._modified = this.getChangedIDs();
    } else {
      
      this._log.debug("First sync, uploading all items");
      this._modified = {};
      for (let id in this._store.getAllIDs()) {
        this._modified[id] = 0;
      }
    }
    
    
    this._tracker.clearChangedIDs();

    this._log.info(Object.keys(this._modified).length +
                   " outgoing items pre-reconciliation");

    
    this._delete = {};
  },

  



  _itemSource: function () {
    return new Collection(this.engineURL, this._recordObj, this.service);
  },

  




  _processIncoming: function (newitems) {
    this._log.trace("Downloading & applying server changes");

    
    let batchSize = this.downloadLimit || Infinity;
    let isMobile = (Svc.Prefs.get("client.type") == "mobile");

    if (!newitems) {
      newitems = this._itemSource();
    }

    if (this._defaultSort) {
      newitems.sort = this._defaultSort;
    }

    if (isMobile) {
      batchSize = MOBILE_BATCH_SIZE;
    }
    newitems.newer = this.lastSync;
    newitems.full  = true;
    newitems.limit = batchSize;

    
    
    
    
    let count = {applied: 0, failed: 0, newFailed: 0, reconciled: 0};
    let handled = [];
    let applyBatch = [];
    let failed = [];
    let failedInPreviousSync = this.previousFailed;
    let fetchBatch = Utils.arrayUnion(this.toFetch, failedInPreviousSync);
    
    this.previousFailed = [];

    
    
    
    let aborting = undefined;

    function doApplyBatch() {
      this._tracker.ignoreAll = true;
      try {
        failed = failed.concat(this._store.applyIncomingBatch(applyBatch));
      } catch (ex) {
        
        
        this._log.warn("Got exception " + Utils.exceptionStr(ex) +
                       ", aborting processIncoming.");
        aborting = ex;
      }
      this._tracker.ignoreAll = false;
      applyBatch = [];
    }

    function doApplyBatchAndPersistFailed() {
      
      if (applyBatch.length) {
        doApplyBatch.call(this);
      }
      
      if (failed.length) {
        this.previousFailed = Utils.arrayUnion(failed, this.previousFailed);
        count.failed += failed.length;
        this._log.debug("Records that failed to apply: " + failed);
        failed = [];
      }
    }

    let key = this.service.collectionKeys.keyForCollection(this.name);

    
    
    let self = this;

    newitems.recordHandler = function(item) {
      if (aborting) {
        return;
      }

      
      if (self.lastModified == null || item.modified > self.lastModified)
        self.lastModified = item.modified;

      
      item.collection = self.name;

      
      handled.push(item.id);

      try {
        try {
          item.decrypt(key);
        } catch (ex if Utils.isHMACMismatch(ex)) {
          let strategy = self.handleHMACMismatch(item, true);
          if (strategy == SyncEngine.kRecoveryStrategy.retry) {
            
            try {
              
              self._log.info("Trying decrypt again...");
              key = self.service.collectionKeys.keyForCollection(self.name);
              item.decrypt(key);
              strategy = null;
            } catch (ex if Utils.isHMACMismatch(ex)) {
              strategy = self.handleHMACMismatch(item, false);
            }
          }

          switch (strategy) {
            case null:
              
              break;
            case SyncEngine.kRecoveryStrategy.retry:
              self._log.debug("Ignoring second retry suggestion.");
              
            case SyncEngine.kRecoveryStrategy.error:
              self._log.warn("Error decrypting record: " + Utils.exceptionStr(ex));
              failed.push(item.id);
              return;
            case SyncEngine.kRecoveryStrategy.ignore:
              self._log.debug("Ignoring record " + item.id +
                              " with bad HMAC: already handled.");
              return;
          }
        }
      } catch (ex) {
        self._log.warn("Error decrypting record: " + Utils.exceptionStr(ex));
        failed.push(item.id);
        return;
      }

      let shouldApply;
      try {
        shouldApply = self._reconcile(item);
      } catch (ex if (ex.code == Engine.prototype.eEngineAbortApplyIncoming)) {
        self._log.warn("Reconciliation failed: aborting incoming processing.");
        failed.push(item.id);
        aborting = ex.cause;
      } catch (ex) {
        self._log.warn("Failed to reconcile incoming record " + item.id);
        self._log.warn("Encountered exception: " + Utils.exceptionStr(ex));
        failed.push(item.id);
        return;
      }

      if (shouldApply) {
        count.applied++;
        applyBatch.push(item);
      } else {
        count.reconciled++;
        self._log.trace("Skipping reconciled incoming item " + item.id);
      }

      if (applyBatch.length == self.applyIncomingBatchSize) {
        doApplyBatch.call(self);
      }
      self._store._sleep(0);
    };

    
    if (this.lastModified == null || this.lastModified > this.lastSync) {
      let resp = newitems.get();
      doApplyBatchAndPersistFailed.call(this);
      if (!resp.success) {
        resp.failureCode = ENGINE_DOWNLOAD_FAIL;
        throw resp;
      }

      if (aborting) {
        throw aborting;
      }
    }

    
    if (handled.length == newitems.limit) {
      let guidColl = new Collection(this.engineURL, null, this.service);

      
      guidColl.limit = this.downloadLimit;
      guidColl.newer = this.lastSync;

      
      guidColl.sort  = "index";

      let guids = guidColl.get();
      if (!guids.success)
        throw guids;

      
      
      let extra = Utils.arraySub(guids.obj, handled);
      if (extra.length > 0) {
        fetchBatch = Utils.arrayUnion(extra, fetchBatch);
        this.toFetch = Utils.arrayUnion(extra, this.toFetch);
      }
    }

    
    
    if (this.lastSync < this.lastModified) {
      this.lastSync = this.lastModified;
    }

    
    
    
    batchSize = isMobile ? this.mobileGUIDFetchBatchSize :
                           this.guidFetchBatchSize;

    while (fetchBatch.length && !aborting) {
      
      
      newitems.limit = 0;
      newitems.newer = 0;
      newitems.ids = fetchBatch.slice(0, batchSize);

      
      let resp = newitems.get();
      if (!resp.success) {
        resp.failureCode = ENGINE_DOWNLOAD_FAIL;
        throw resp;
      }

      
      
      fetchBatch = fetchBatch.slice(batchSize);
      this.toFetch = Utils.arraySub(this.toFetch, newitems.ids);
      this.previousFailed = Utils.arrayUnion(this.previousFailed, failed);
      if (failed.length) {
        count.failed += failed.length;
        this._log.debug("Records that failed to apply: " + failed);
      }
      failed = [];

      if (aborting) {
        throw aborting;
      }

      if (this.lastSync < this.lastModified) {
        this.lastSync = this.lastModified;
      }
    }

    
    doApplyBatchAndPersistFailed.call(this);

    count.newFailed = Utils.arraySub(this.previousFailed, failedInPreviousSync).length;
    count.succeeded = Math.max(0, count.applied - count.failed);
    this._log.info(["Records:",
                    count.applied, "applied,",
                    count.succeeded, "successfully,",
                    count.failed, "failed to apply,",
                    count.newFailed, "newly failed to apply,",
                    count.reconciled, "reconciled."].join(" "));
    Observers.notify("weave:engine:sync:applied", count, this.name);
  },

  





  _findDupe: function (item) {
    
  },

  _deleteId: function (id) {
    this._tracker.removeChangedID(id);

    
    if (this._delete.ids == null)
      this._delete.ids = [id];
    else
      this._delete.ids.push(id);
  },

  









  _reconcile: function (item) {
    if (this._log.level <= Log.Level.Trace) {
      this._log.trace("Incoming: " + item);
    }

    
    
    
    let existsLocally   = this._store.itemExists(item.id);
    let locallyModified = item.id in this._modified;

    
    let remoteAge = AsyncResource.serverTime - item.modified;
    let localAge  = locallyModified ?
      (Date.now() / 1000 - this._modified[item.id]) : null;
    let remoteIsNewer = remoteAge < localAge;

    this._log.trace("Reconciling " + item.id + ". exists=" +
                    existsLocally + "; modified=" + locallyModified +
                    "; local age=" + localAge + "; incoming age=" +
                    remoteAge);

    
    
    if (item.deleted) {
      
      
      
      if (!existsLocally) {
        this._log.trace("Ignoring incoming item because it was deleted and " +
                        "the item does not exist locally.");
        return false;
      }

      
      
      
      
      if (!locallyModified) {
        this._log.trace("Applying incoming delete because the local item " +
                        "exists and isn't modified.");
        return true;
      }

      
      
      
      
      
      this._log.trace("Incoming record is deleted but we had local changes. " +
                      "Applying the youngest record.");
      return remoteIsNewer;
    }

    
    
    
    
    
    
    
    
    if (!existsLocally) {
      let dupeID = this._findDupe(item);
      if (dupeID) {
        this._log.trace("Local item " + dupeID + " is a duplicate for " +
                        "incoming item " + item.id);

        
        this._deleteId(dupeID);

        
        
        
        existsLocally = this._store.itemExists(dupeID);

        
        
        
        this._log.debug("Switching local ID to incoming: " + dupeID + " -> " +
                        item.id);
        this._store.changeItemID(dupeID, item.id);

        
        
        if (dupeID in this._modified) {
          locallyModified = true;
          localAge = Date.now() / 1000 - this._modified[dupeID];
          remoteIsNewer = remoteAge < localAge;

          this._modified[item.id] = this._modified[dupeID];
          delete this._modified[dupeID];
        } else {
          locallyModified = false;
          localAge = null;
        }

        this._log.debug("Local item after duplication: age=" + localAge +
                        "; modified=" + locallyModified + "; exists=" +
                        existsLocally);
      } else {
        this._log.trace("No duplicate found for incoming item: " + item.id);
      }
    }

    
    
    

    if (!existsLocally) {
      
      
      
      if (!locallyModified) {
        this._log.trace("Applying incoming because local item does not exist " +
                        "and was not deleted.");
        return true;
      }

      
      
      
      if (remoteIsNewer) {
        this._log.trace("Applying incoming because local item was deleted " +
                        "before the incoming item was changed.");
        delete this._modified[item.id];
        return true;
      }

      this._log.trace("Ignoring incoming item because the local item's " +
                      "deletion is newer.");
      return false;
    }

    
    
    
    
    
    
    let localRecord = this._createRecord(item.id);
    let recordsEqual = Utils.deepEquals(item.cleartext,
                                        localRecord.cleartext);

    
    
    
    if (recordsEqual) {
      this._log.trace("Ignoring incoming item because the local item is " +
                      "identical.");

      delete this._modified[item.id];
      return false;
    }

    

    
    if (!locallyModified) {
      this._log.trace("Applying incoming record because no local conflicts.");
      return true;
    }

    
    
    
    
    this._log.warn("DATA LOSS: Both local and remote changes to record: " +
                   item.id);
    return remoteIsNewer;
  },

  
  _uploadOutgoing: function () {
    this._log.trace("Uploading local changes to server.");

    let modifiedIDs = Object.keys(this._modified);
    if (modifiedIDs.length) {
      this._log.trace("Preparing " + modifiedIDs.length +
                      " outgoing records");

      
      let up = new Collection(this.engineURL, null, this.service);
      let count = 0;

      
      let doUpload = Utils.bind2(this, function(desc) {
        this._log.info("Uploading " + desc + " of " + modifiedIDs.length +
                       " records");
        let resp = up.post();
        if (!resp.success) {
          this._log.debug("Uploading records failed: " + resp);
          resp.failureCode = ENGINE_UPLOAD_FAIL;
          throw resp;
        }

        
        let modified = resp.headers["x-weave-timestamp"];
        if (modified > this.lastSync)
          this.lastSync = modified;

        let failed_ids = Object.keys(resp.obj.failed);
        if (failed_ids.length)
          this._log.debug("Records that will be uploaded again because "
                          + "the server couldn't store them: "
                          + failed_ids.join(", "));

        
        for each (let id in resp.obj.success) {
          delete this._modified[id];
        }

        up.clearRecords();
      });

      for each (let id in modifiedIDs) {
        try {
          let out = this._createRecord(id);
          if (this._log.level <= Log.Level.Trace)
            this._log.trace("Outgoing: " + out);

          out.encrypt(this.service.collectionKeys.keyForCollection(this.name));
          up.pushData(out);
        }
        catch(ex) {
          this._log.warn("Error creating record: " + Utils.exceptionStr(ex));
        }

        
        if ((++count % MAX_UPLOAD_RECORDS) == 0)
          doUpload((count - MAX_UPLOAD_RECORDS) + " - " + count + " out");

        this._store._sleep(0);
      }

      
      if (count % MAX_UPLOAD_RECORDS > 0)
        doUpload(count >= MAX_UPLOAD_RECORDS ? "last batch" : "all");
    }
  },

  
  
  _syncFinish: function () {
    this._log.trace("Finishing up sync");
    this._tracker.resetScore();

    let doDelete = Utils.bind2(this, function(key, val) {
      let coll = new Collection(this.engineURL, this._recordObj, this.service);
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

  _syncCleanup: function () {
    if (!this._modified) {
      return;
    }

    
    for (let [id, when] in Iterator(this._modified)) {
      this._tracker.addChangedID(id, when);
    }
    this._modified = {};
  },

  _sync: function () {
    try {
      this._syncStartup();
      Observers.notify("weave:engine:sync:status", "process-incoming");
      this._processIncoming();
      Observers.notify("weave:engine:sync:status", "upload-outgoing");
      this._uploadOutgoing();
      this._syncFinish();
    } finally {
      this._syncCleanup();
    }
  },

  canDecrypt: function () {
    
    let canDecrypt = false;

    
    let test = new Collection(this.engineURL, this._recordObj, this.service);
    test.limit = 1;
    test.sort = "newest";
    test.full = true;

    let key = this.service.collectionKeys.keyForCollection(this.name);
    test.recordHandler = function recordHandler(record) {
      record.decrypt(key);
      canDecrypt = true;
    }.bind(this);

    
    try {
      this._log.trace("Trying to decrypt a record from the server..");
      test.get();
    }
    catch(ex) {
      this._log.debug("Failed test decrypt: " + Utils.exceptionStr(ex));
    }

    return canDecrypt;
  },

  _resetClient: function () {
    this.resetLastSync();
    this.previousFailed = [];
    this.toFetch = [];
  },

  wipeServer: function () {
    let response = this.service.resource(this.engineURL).delete();
    if (response.status != 200 && response.status != 404) {
      throw response;
    }
    this._resetClient();
  },

  removeClientData: function () {
    
    
  },

  















  handleHMACMismatch: function (item, mayRetry) {
    
    return (this.service.handleHMACEvent() && mayRetry) ?
           SyncEngine.kRecoveryStrategy.retry :
           SyncEngine.kRecoveryStrategy.error;
  }
};
