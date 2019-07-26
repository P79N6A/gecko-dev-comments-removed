



"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/Log.jsm", this);
Cu.import("resource://gre/modules/osfile.jsm", this)
Cu.import("resource://gre/modules/Promise.jsm", this);
Cu.import("resource://gre/modules/Services.jsm", this);
Cu.import("resource://gre/modules/Task.jsm", this);
Cu.import("resource://gre/modules/Timer.jsm", this);
Cu.import("resource://gre/modules/XPCOMUtils.jsm", this);
Cu.import("resource://services-common/utils.js", this);

this.EXPORTED_SYMBOLS = [
  "CrashManager",
];








const AGGREGATE_STARTUP_DELAY_MS = 57000;

const MILLISECONDS_IN_DAY = 24 * 60 * 60 * 1000;






























this.CrashManager = function (options) {
  for (let k of ["pendingDumpsDir", "submittedDumpsDir", "eventsDirs",
    "storeDir"]) {
    if (!(k in options)) {
      throw new Error("Required key not present in options: " + k);
    }
  }

  this._log = Log.repository.getLogger("Crashes.CrashManager");

  for (let k in options) {
    let v = options[k];

    switch (k) {
      case "pendingDumpsDir":
        this._pendingDumpsDir = v;
        break;

      case "submittedDumpsDir":
        this._submittedDumpsDir = v;
        break;

      case "eventsDirs":
        this._eventsDirs = v;
        break;

      case "storeDir":
        this._storeDir = v;
        break;

      case "telemetryStoreSizeKey":
        this._telemetryStoreSizeKey = v;
        break;

      default:
        throw new Error("Unknown property in options: " + k);
    }
  }

  
  
  this._aggregatePromise = null;

  
  this._store = null;

  
  this._storeTimer = null;

  
  
  this._storeProtectedCount = 0;
};

this.CrashManager.prototype = Object.freeze({
  DUMP_REGEX: /^([0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12})\.dmp$/i,
  SUBMITTED_REGEX: /^bp-(?:hr-)?([0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12})\.txt$/i,
  ALL_REGEX: /^(.*)$/,

  
  
  STORE_EXPIRATION_MS: 60 * 1000,

  
  PURGE_OLDER_THAN_DAYS: 180,

  
  
  EVENT_FILE_SUCCESS: "ok",
  
  EVENT_FILE_ERROR_MALFORMED: "malformed",
  
  EVENT_FILE_ERROR_UNKNOWN_EVENT: "unknown-event",

  



















  pendingDumps: function () {
    return this._getDirectoryEntries(this._pendingDumpsDir, this.DUMP_REGEX);
  },

  



















  submittedDumps: function () {
    return this._getDirectoryEntries(this._submittedDumpsDir,
                                     this.SUBMITTED_REGEX);
  },

  














  aggregateEventsFiles: function () {
    if (this._aggregatePromise) {
      return this._aggregatePromise;
    }

    return this._aggregatePromise = Task.spawn(function* () {
      if (this._aggregatePromise) {
        return this._aggregatePromise;
      }

      try {
        let unprocessedFiles = yield this._getUnprocessedEventsFiles();

        let deletePaths = [];
        let needsSave = false;

        this._storeProtectedCount++;
        for (let entry of unprocessedFiles) {
          try {
            let result = yield this._processEventFile(entry);

            switch (result) {
              case this.EVENT_FILE_SUCCESS:
                needsSave = true;
                

              case this.EVENT_FILE_ERROR_MALFORMED:
                deletePaths.push(entry.path);
                break;

              case this.EVENT_FILE_ERROR_UNKNOWN_EVENT:
                break;

              default:
                Cu.reportError("Unhandled crash event file return code. Please " +
                               "file a bug: " + result);
            }
          } catch (ex if ex instanceof OS.File.Error) {
            this._log.warn("I/O error reading " + entry.path + ": " +
                           CommonUtils.exceptionStr(ex));
          } catch (ex) {
            
            
            
            
            
            
            Cu.reportError("Exception when processing crash event file: " +
                           CommonUtils.exceptionStr(ex));
            deletePaths.push(entry.path);
          }
        }

        if (needsSave) {
          let store = yield this._getStore();
          yield store.save();
        }

        for (let path of deletePaths) {
          try {
            yield OS.File.remove(path);
          } catch (ex) {
            this._log.warn("Error removing event file (" + path + "): " +
                           CommonUtils.exceptionStr(ex));
          }
        }

        return unprocessedFiles.length;

      } finally {
        this._aggregatePromise = false;
        this._storeProtectedCount--;
      }
    }.bind(this));
  },

  






  pruneOldCrashes: function (date) {
    return Task.spawn(function* () {
      let store = yield this._getStore();
      store.pruneOldCrashes(date);
      yield store.save();
    }.bind(this));
  },

  


  runMaintenanceTasks: function () {
    return Task.spawn(function* () {
      yield this.aggregateEventsFiles();

      let offset = this.PURGE_OLDER_THAN_DAYS * MILLISECONDS_IN_DAY;
      yield this.pruneOldCrashes(new Date(Date.now() - offset));
    }.bind(this));
  },

  





  scheduleMaintenance: function (delay) {
    let deferred = Promise.defer();

    setTimeout(() => {
      this.runMaintenanceTasks().then(deferred.resolve, deferred.reject);
    }, delay);

    return deferred.promise;
  },

  




  _getUnprocessedEventsFiles: function () {
    return Task.spawn(function* () {
      let entries = [];

      for (let dir of this._eventsDirs) {
        for (let e of yield this._getDirectoryEntries(dir, this.ALL_REGEX)) {
          entries.push(e);
        }
      }

      entries.sort((a, b) => { return a.date - b.date; });

      return entries;
    }.bind(this));
  },

  
  _processEventFile: function (entry) {
    return Task.spawn(function* () {
      let data = yield OS.File.read(entry.path);
      let store = yield this._getStore();

      let decoder = new TextDecoder();
      data = decoder.decode(data);

      let type, time, payload;
      let start = 0;
      for (let i = 0; i < 2; i++) {
        let index = data.indexOf("\n", start);
        if (index == -1) {
          return this.EVENT_FILE_ERROR_MALFORMED;
        }

        let sub = data.substring(start, index);
        switch (i) {
          case 0:
            type = sub;
            break;
          case 1:
            time = sub;
            try {
              time = parseInt(time, 10);
            } catch (ex) {
              return this.EVENT_FILE_ERROR_MALFORMED;
            }
        }

        start = index + 1;
      }
      let date = new Date(time * 1000);
      let payload = data.substring(start);

      return this._handleEventFilePayload(store, entry, type, date, payload);
    }.bind(this));
  },

  _handleEventFilePayload: function (store, entry, type, date, payload) {
      
      
      

      let eventMap = {
        "crash.main.1": "addMainProcessCrash",
        "crash.plugin.1": "addPluginCrash",
        "hang.plugin.1": "addPluginHang",
      };

      if (type in eventMap) {
        let lines = payload.split("\n");
        if (lines.length > 1) {
          this._log.warn("Multiple lines unexpected in payload for " +
                         entry.path);
          return this.EVENT_FILE_ERROR_MALFORMED;
        }

        store[eventMap[type]](payload, date);
        return this.EVENT_FILE_SUCCESS;
      }

      

      return this.EVENT_FILE_ERROR_UNKNOWN_EVENT;
  },

  






  _getDirectoryEntries: function (path, re) {
    return Task.spawn(function* () {
      try {
        yield OS.File.stat(path);
      } catch (ex if ex instanceof OS.File.Error && ex.becauseNoSuchFile) {
          return [];
      }

      let it = new OS.File.DirectoryIterator(path);
      let entries = [];

      try {
        yield it.forEach((entry, index, it) => {
          if (entry.isDir) {
            return;
          }

          let match = re.exec(entry.name);
          if (!match) {
            return;
          }

          return OS.File.stat(entry.path).then((info) => {
            entries.push({
              path: entry.path,
              id: match[1],
              date: info.lastModificationDate,
            });
          });
        });
      } finally {
        it.close();
      }

      entries.sort((a, b) => { return a.date - b.date; });

      return entries;
    }.bind(this));
  },

  _getStore: function () {
    return Task.spawn(function* () {
      if (!this._store) {
        let store = new CrashStore(this._storeDir, this._telemetryStoreSizeKey);
        yield store.load();

        this._store = store;
        this._storeTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
      }

      
      
      
      
      
      this._storeTimer.cancel();

      
      
      let timerCB = function () {
        if (this._storeProtectedCount) {
          this._storeTimer.initWithCallback(timerCB, this.STORE_EXPIRATION_MS,
                                            this._storeTimer.TYPE_ONE_SHOT);
          return;
        }

        
        
        
        this._store = null;
        this._storeTimer = null;
      }.bind(this);

      this._storeTimer.initWithCallback(timerCB, this.STORE_EXPIRATION_MS,
                                        this._storeTimer.TYPE_ONE_SHOT);

      return this._store;
    }.bind(this));
  },

  




  getCrashes: function () {
    return Task.spawn(function* () {
      let store = yield this._getStore();

      return store.crashes;
    }.bind(this));
  },
});

let gCrashManager;



















function CrashStore(storeDir, telemetrySizeKey) {
  this._storeDir = storeDir;
  this._telemetrySizeKey = telemetrySizeKey;

  this._storePath = OS.Path.join(storeDir, "store.json.mozlz4");

  
  this._data = null;
}

CrashStore.prototype = Object.freeze({
  TYPE_MAIN_CRASH: "main-crash",
  TYPE_PLUGIN_CRASH: "plugin-crash",
  TYPE_PLUGIN_HANG: "plugin-hang",

  




  load: function () {
    return Task.spawn(function* () {
      this._data = {
        v: 1,
        crashes: new Map(),
        corruptDate: null,
      };

      try {
        let decoder = new TextDecoder();
        let data = yield OS.File.read(this._storePath, null, {compression: "lz4"});
        data = JSON.parse(decoder.decode(data));

        if (data.corruptDate) {
          this._data.corruptDate = new Date(data.corruptDate);
        }

        for (let id in data.crashes) {
          let crash = data.crashes[id];
          let denormalized = this._denormalize(crash);
          this._data.crashes.set(id, denormalized);
        }
      } catch (ex if ex instanceof OS.File.Error && ex.becauseNoSuchFile) {
        
      } catch (ex) {
        
        
        
        
        
        
        this._data.corruptDate = new Date();
      }
    }.bind(this));
  },

  




  save: function () {
    return Task.spawn(function* () {
      if (!this._data) {
        return;
      }

      let normalized = {
        v: 1,
        crashes: {},
        corruptDate: null,
      };

      if (this._data.corruptDate) {
        normalized.corruptDate = this._data.corruptDate.getTime();
      }

      for (let [id, crash] of this._data.crashes) {
        let c = this._normalize(crash);
        normalized.crashes[id] = c;
      }

      let encoder = new TextEncoder();
      let data = encoder.encode(JSON.stringify(normalized));
      let size = yield OS.File.writeAtomic(this._storePath, data, {
                                           tmpPath: this._storePath + ".tmp",
                                           compression: "lz4"});
      if (this._telemetrySizeKey) {
        Services.telemetry.getHistogramById(this._telemetrySizeKey).add(size);
      }
    }.bind(this));
  },

  










  _normalize: function (o) {
    let normalized = {};

    for (let k in o) {
      let v = o[k];
      if (v && k.endsWith("Date")) {
        normalized[k] = v.getTime();
      } else {
        normalized[k] = v;
      }
    }

    return normalized;
  },

  


  _denormalize: function (o) {
    let n = {};

    for (let k in o) {
      let v = o[k];
      if (v && k.endsWith("Date")) {
        n[k] = new Date(parseInt(v, 10));
      } else {
        n[k] = v;
      }
    }

    return n;
  },

  










  pruneOldCrashes: function (date) {
    for (let crash of this.crashes) {
      let newest = crash.newestDate;
      if (!newest || newest.getTime() < date.getTime()) {
        this._data.crashes.delete(crash.id);
      }
    }
  },

  




  get corruptDate() {
    return this._data.corruptDate;
  },

  


  get crashesCount() {
    return this._data.crashes.size;
  },

  




  get crashes() {
    let crashes = [];
    for (let [id, crash] of this._data.crashes) {
      crashes.push(new CrashRecord(crash));
    }

    return crashes;
  },

  





  getCrash: function (id) {
    for (let crash of this.crashes) {
      if (crash.id == id) {
        return crash;
      }
    }

    return null;
  },

  _ensureCrashRecord: function (id) {
    if (!this._data.crashes.has(id)) {
      this._data.crashes.set(id, {
        id: id,
        type: null,
        crashDate: null,
      });
    }

    return this._data.crashes.get(id);
  },

  





  addMainProcessCrash: function (id, date) {
    let r = this._ensureCrashRecord(id);
    r.type = this.TYPE_MAIN_CRASH;
    r.crashDate = date;
  },

  





  addPluginCrash: function (id, date) {
    let r = this._ensureCrashRecord(id);
    r.type = this.TYPE_PLUGIN_CRASH;
    r.crashDate = date;
  },

  





  addPluginHang: function (id, date) {
    let r = this._ensureCrashRecord(id);
    r.type = this.TYPE_PLUGIN_HANG;
    r.crashDate = date;
  },

  get mainProcessCrashes() {
    let crashes = [];
    for (let crash of this.crashes) {
      if (crash.isMainProcessCrash) {
        crashes.push(crash);
      }
    }

    return crashes;
  },

  get pluginCrashes() {
    let crashes = [];
    for (let crash of this.crashes) {
      if (crash.isPluginCrash) {
        crashes.push(crash);
      }
    }

    return crashes;
  },

  get pluginHangs() {
    let crashes = [];
    for (let crash of this.crashes) {
      if (crash.isPluginHang) {
        crashes.push(crash);
      }
    }

    return crashes;
  },
});













function CrashRecord(o) {
  this._o = o;
}

CrashRecord.prototype = Object.freeze({
  get id() {
    return this._o.id;
  },

  get crashDate() {
    return this._o.crashDate;
  },

  





  get newestDate() {
    
    return this._o.crashDate;
  },

  get type() {
    return this._o.type;
  },

  get isMainProcessCrash() {
    return this._o.type == CrashStore.prototype.TYPE_MAIN_CRASH;
  },

  get isPluginCrash() {
    return this._o.type == CrashStore.prototype.TYPE_PLUGIN_CRASH;
  },

  get isPluginHang() {
    return this._o.type == CrashStore.prototype.TYPE_PLUGIN_HANG;
  },
});







XPCOMUtils.defineLazyGetter(this.CrashManager, "Singleton", function () {
  if (gCrashManager) {
    return gCrashManager;
  }

  let crPath = OS.Path.join(OS.Constants.Path.userApplicationDataDir,
                            "Crash Reports");
  let storePath = OS.Path.join(OS.Constants.Path.profileDir, "crashes");

  gCrashManager = new CrashManager({
    pendingDumpsDir: OS.Path.join(crPath, "pending"),
    submittedDumpsDir: OS.Path.join(crPath, "submitted"),
    eventsDirs: [OS.Path.join(crPath, "events"), OS.Path.join(storePath, "events")],
    storeDir: storePath,
    telemetryStoreSizeKey: "CRASH_STORE_COMPRESSED_BYTES",
  });

  
  
  
  
  
  
  
  
  
  gCrashManager.scheduleMaintenance(AGGREGATE_STARTUP_DELAY_MS);

  return gCrashManager;
});
