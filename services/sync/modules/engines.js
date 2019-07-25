







































const EXPORTED_SYMBOLS = ['Engines', 'Engine', 'SyncEngine',
                          'Tracker', 'Store'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://services-sync/async.js");
Cu.import("resource://services-sync/record.js");
Cu.import("resource://services-sync/constants.js");
Cu.import("resource://services-sync/ext/Observers.js");
Cu.import("resource://services-sync/identity.js");
Cu.import("resource://services-sync/log4moz.js");
Cu.import("resource://services-sync/resource.js");
Cu.import("resource://services-sync/util.js");

Cu.import("resource://services-sync/main.js");    












function Tracker(name) {
  name = name || "Unnamed";
  this.name = this.file = name.toLowerCase();

  this._log = Log4Moz.repository.getLogger("Tracker." + name);
  let level = Svc.Prefs.get("log.logger.engine." + this.name, "Debug");
  this._log.level = Log4Moz.Level[level];

  this._score = 0;
  this._ignored = [];
  this.ignoreAll = false;
  this.changedIDs = {};
  this.loadChangedIDs();
}
Tracker.prototype = {
  









  get score() {
    return this._score;
  },

  set score(value) {
    this._score = value;
    Observers.notify("weave:engine:score:updated", this.name);
  },

  
  resetScore: function T_resetScore() {
    this._score = 0;
  },

  saveChangedIDs: function T_saveChangedIDs() {
    Utils.delay(function() {
      Utils.jsonSave("changes/" + this.file, this, this.changedIDs);
    }, 1000, this, "_lazySave");
  },

  loadChangedIDs: function T_loadChangedIDs() {
    Utils.jsonLoad("changes/" + this.file, this, function(json) {
      if (json) {
        this.changedIDs = json;
      }
    });
  },

  
  
  

  ignoreID: function T_ignoreID(id) {
    this.unignoreID(id);
    this._ignored.push(id);
  },

  unignoreID: function T_unignoreID(id) {
    let index = this._ignored.indexOf(id);
    if (index != -1)
      this._ignored.splice(index, 1);
  },

  addChangedID: function addChangedID(id, when) {
    if (!id) {
      this._log.warn("Attempted to add undefined ID to tracker");
      return false;
    }
    if (this.ignoreAll || (id in this._ignored))
      return false;

    
    if (when == null)
      when = Math.floor(Date.now() / 1000);

    
    if ((this.changedIDs[id] || -Infinity) < when) {
      this._log.trace("Adding changed ID: " + [id, when]);
      this.changedIDs[id] = when;
      this.saveChangedIDs();
    }
    return true;
  },

  removeChangedID: function T_removeChangedID(id) {
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

  clearChangedIDs: function T_clearChangedIDs() {
    this._log.trace("Clearing changed ID list");
    this.changedIDs = {};
    this.saveChangedIDs();
  }
};








function Store(name) {
  name = name || "Unnamed";
  this.name = name.toLowerCase();

  this._log = Log4Moz.repository.getLogger("Store." + name);
  let level = Svc.Prefs.get("log.logger.engine." + this.name, "Debug");
  this._log.level = Log4Moz.Level[level];

  XPCOMUtils.defineLazyGetter(this, "_timer", function() {
    return Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
  });
}
Store.prototype = {

  _sleep: function _sleep(delay) {
    let cb = Async.makeSyncCallback();
    this._timer.initWithCallback({notify: cb}, delay,
                                 Ci.nsITimer.TYPE_ONE_SHOT);
    Async.waitForSyncCallback(cb);
  },

  applyIncomingBatch: function applyIncomingBatch(records) {
    let failed = [];
    for each (let record in records) {
      try {
        this.applyIncoming(record);
      } catch (ex) {
        this._log.warn("Failed to apply incoming record " + record.id);
        this._log.warn("Encountered exception: " + Utils.exceptionStr(ex));
        failed.push(record.id);
      }
    };
    return failed;
  },

  applyIncoming: function Store_applyIncoming(record) {
    if (record.deleted)
      this.remove(record);
    else if (!this.itemExists(record.id))
      this.create(record);
    else
      this.update(record);
  },

  

  create: function Store_create(record) {
    throw "override create in a subclass";
  },

  remove: function Store_remove(record) {
    throw "override remove in a subclass";
  },

  update: function Store_update(record) {
    throw "override update in a subclass";
  },

  itemExists: function Store_itemExists(id) {
    throw "override itemExists in a subclass";
  },

  createRecord: function Store_createRecord(id, collection) {
    throw "override createRecord in a subclass";
  },

  changeItemID: function Store_changeItemID(oldID, newID) {
    throw "override changeItemID in a subclass";
  },

  getAllIDs: function Store_getAllIDs() {
    throw "override getAllIDs in a subclass";
  },

  wipe: function Store_wipe() {
    throw "override wipe in a subclass";
  }
};




XPCOMUtils.defineLazyGetter(this, "Engines", function() {
  return new EngineManagerSvc();
});

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
    if (!engine) {
      this._log.debug("Could not get engine: " + name);
      if (Object.keys)
        this._log.debug("Engines are: " + JSON.stringify(Object.keys(this._engines)));
    }
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

    this._notify("sync", this.name, this._sync)();
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
  
  
  
  downloadLimit: null,
  
  
  
  guidFetchBatchSize: DEFAULT_GUID_FETCH_BATCH_SIZE,
  mobileGUIDFetchBatchSize: DEFAULT_MOBILE_GUID_FETCH_BATCH_SIZE,
  
  
  applyIncomingBatchSize: DEFAULT_STORE_BATCH_SIZE,

  get storageURL() Svc.Prefs.get("clusterURL") + SYNC_API_VERSION +
    "/" + ID.get("WeaveID").username + "/storage/",

  get engineURL() this.storageURL + this.name,

  get cryptoKeysURL() this.storageURL + "crypto/keys",

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
    this.lastSyncLocal = 0;
  },

  get toFetch() this._toFetch,
  set toFetch(val) {
    
    if (val + "" == this._toFetch) {
      return;
    }
    this._toFetch = val;
    Utils.delay(function () {
      Utils.jsonSave("toFetch/" + this.name, this, val);
    }, 0, this, "_toFetchDelay");
  },

  loadToFetch: function loadToFetch() {
    
    this._toFetch = [];
    Utils.jsonLoad("toFetch/" + this.name, this, function(toFetch) {
      if (toFetch) {
        this._toFetch = toFetch;
      }
    });
  },

  get previousFailed() this._previousFailed,
  set previousFailed(val) {
    
    if (val + "" == this._previousFailed) {
      return;
    }
    this._previousFailed = val;
    Utils.delay(function () {
      Utils.jsonSave("failed/" + this.name, this, val);
    }, 0, this, "_previousFailedDelay");
  },

  loadPreviousFailed: function loadPreviousFailed() {
    
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

  




  getChangedIDs: function getChangedIDs() {
    return this._tracker.changedIDs;
  },

  
  _createRecord: function SyncEngine__createRecord(id) {
    let record = this._store.createRecord(id, this.name);
    record.id = id;
    record.collection = this.name;
    return record;
  },

  
  _syncStartup: function SyncEngine__syncStartup() {

    
    let metaGlobal = Records.get(this.metaURL);
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
      this.wipeServer(true);
    }

    
    
    
    
    
    
    this.lastSyncLocal = Date.now();
    if (this.lastSync) {
      this._modified = this.getChangedIDs();
    } else {
      
      this._log.debug("First sync, uploading all items");
      this._modified = {};
      for (let id in this._store.getAllIDs())
        this._modified[id] = 0;
    }
    
    
    this._tracker.clearChangedIDs();
 
    
    
    this._modifiedIDs = [id for (id in this._modified)];
    this._log.info(this._modifiedIDs.length +
                   " outgoing items pre-reconciliation");

    
    this._delete = {};
  },

  
  _processIncoming: function SyncEngine__processIncoming() {
    this._log.trace("Downloading & applying server changes");

    
    let batchSize = Infinity;
    let newitems = new Collection(this.engineURL, this._recordObj);
    let isMobile = (Svc.Prefs.get("client.type") == "mobile");

    if (isMobile) {
      batchSize = MOBILE_BATCH_SIZE;
    }
    newitems.newer = this.lastSync;
    newitems.full = true;
    newitems.limit = batchSize;
    
    
    
    
    
    let count = {applied: 0, failed: 0, newFailed: 0, reconciled: 0};
    let handled = [];
    let applyBatch = [];
    let failed = [];
    let failedInPreviousSync = this.previousFailed;
    let fetchBatch = Utils.arrayUnion(this.toFetch, failedInPreviousSync);
    
    this.previousFailed = [];

    function doApplyBatch() {
      this._tracker.ignoreAll = true;
      failed = failed.concat(this._store.applyIncomingBatch(applyBatch));
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

    
    
    let self = this;
    newitems.recordHandler = function(item) {
      
      if (self.lastModified == null || item.modified > self.lastModified)
        self.lastModified = item.modified;

      
      item.collection = self.name;
      
      
      handled.push(item.id);

      try {
        try {
          item.decrypt();
        } catch (ex if Utils.isHMACMismatch(ex)) {
          let strategy = self.handleHMACMismatch(item, true);
          if (strategy == SyncEngine.kRecoveryStrategy.retry) {
            
            try {
              
              self._log.info("Trying decrypt again...");
              item.decrypt();
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
    }

    
    if (handled.length == newitems.limit) {
      let guidColl = new Collection(this.engineURL);
      
      
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

    while (fetchBatch.length) {
      
      
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
      if (this.lastSync < this.lastModified) {
        this.lastSync = this.lastModified;
      }
    }

    
    doApplyBatchAndPersistFailed.call(this);

    count.newFailed = Utils.arraySub(this.previousFailed, failedInPreviousSync).length;
    if (count.newFailed) {
      
      
      Observers.notify("weave:engine:sync:apply-failed", count, this.name);
    }
    this._log.info(["Records:",
                    count.applied, "applied,",
                    count.failed, "failed to apply,",
                    count.newFailed, "newly failed to apply,",
                    count.reconciled, "reconciled."].join(" "));
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
    if (item.id in this._modified) {
      
      if (this._isEqual(item)) {
        delete this._modified[item.id];
        return false;
      }

      
      let recordAge = Resource.serverTime - item.modified;
      let localAge = Date.now() / 1000 - this._modified[item.id];
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
    this._log.trace("Uploading local changes to server.");
    if (this._modifiedIDs.length) {
      this._log.trace("Preparing " + this._modifiedIDs.length +
                      " outgoing records");

      
      let up = new Collection(this.engineURL);
      let count = 0;

      
      let doUpload = Utils.bind2(this, function(desc) {
        this._log.info("Uploading " + desc + " of " +
                       this._modifiedIDs.length + " records");
        let resp = up.post();
        if (!resp.success) {
          this._log.debug("Uploading records failed: " + resp);
          resp.failureCode = ENGINE_UPLOAD_FAIL;
          throw resp;
        }

        
        let modified = resp.headers["x-weave-timestamp"];
        if (modified > this.lastSync)
          this.lastSync = modified;

        let failed_ids = [id for (id in resp.obj.failed)];
        if (failed_ids.length)
          this._log.debug("Records that will be uploaded again because "
                          + "the server couldn't store them: "
                          + failed_ids.join(", "));

        
        for each (let id in resp.obj.success) {
          delete this._modified[id];
        }

        up.clearRecords();
      });

      for each (let id in this._modifiedIDs) {
        try {
          let out = this._createRecord(id);
          if (this._log.level <= Log4Moz.Level.Trace)
            this._log.trace("Outgoing: " + out);

          out.encrypt();
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

  _syncCleanup: function _syncCleanup() {
    if (!this._modified)
      return;

    
    for (let [id, when] in Iterator(this._modified)) {
      this._tracker.addChangedID(id, when);
    }
    delete this._modified;
    delete this._modifiedIDs;
  },

  _sync: function SyncEngine__sync() {
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

  canDecrypt: function canDecrypt() {
    
    let canDecrypt = false;

    
    let test = new Collection(this.engineURL, this._recordObj);
    test.limit = 1;
    test.sort = "newest";
    test.full = true;
    test.recordHandler = function(record) {
      record.decrypt();
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
    this.previousFailed = [];
    this.toFetch = [];
  },

  wipeServer: function wipeServer() {
    new Resource(this.engineURL).delete();
    this._resetClient();
  },

  removeClientData: function removeClientData() {
    
    
  },

  















  handleHMACMismatch: function handleHMACMismatch(item, mayRetry) {
    
    return (Weave.Service.handleHMACEvent() && mayRetry) ?
           SyncEngine.kRecoveryStrategy.retry :
           SyncEngine.kRecoveryStrategy.error;
  }
};
