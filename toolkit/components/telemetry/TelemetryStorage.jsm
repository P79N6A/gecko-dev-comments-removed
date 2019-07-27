




"use strict";

this.EXPORTED_SYMBOLS = ["TelemetryStorage"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://gre/modules/Services.jsm", this);
Cu.import("resource://gre/modules/XPCOMUtils.jsm", this);
Cu.import("resource://gre/modules/osfile.jsm", this);
Cu.import("resource://gre/modules/Task.jsm", this);
Cu.import("resource://gre/modules/TelemetryUtils.jsm", this);
Cu.import("resource://gre/modules/Promise.jsm", this);

XPCOMUtils.defineLazyModuleGetter(this, 'Deprecated',
  'resource://gre/modules/Deprecated.jsm');

const LOGGER_NAME = "Toolkit.Telemetry";
const LOGGER_PREFIX = "TelemetryStorage::";

const Telemetry = Services.telemetry;
const Utils = TelemetryUtils;


const DATAREPORTING_DIR = "datareporting";
const PINGS_ARCHIVE_DIR = "archived";
const ABORTED_SESSION_FILE_NAME = "aborted-session-ping";
XPCOMUtils.defineLazyGetter(this, "gDataReportingDir", function() {
  return OS.Path.join(OS.Constants.Path.profileDir, DATAREPORTING_DIR);
});
XPCOMUtils.defineLazyGetter(this, "gPingsArchivePath", function() {
  return OS.Path.join(gDataReportingDir, PINGS_ARCHIVE_DIR);
});
XPCOMUtils.defineLazyGetter(this, "gAbortedSessionFilePath", function() {
  return OS.Path.join(gDataReportingDir, ABORTED_SESSION_FILE_NAME);
});



const MAX_PING_FILE_AGE = 14 * 24 * 60 * 60 * 1000; 



const OVERDUE_PING_FILE_AGE = 7 * 24 * 60 * 60 * 1000; 


const MAX_LRU_PINGS = 50;


const MAX_ARCHIVED_PINGS_RETENTION_MS = 180 * 24 * 60 * 60 * 1000;  


const ARCHIVE_QUOTA_BYTES = 120 * 1024 * 1024; 


const ARCHIVE_SIZE_PROBE_SPECIAL_VALUE = 300;



let pingsLoaded = 0;



let pingsDiscarded = 0;



let pingsOverdue = 0;


let pendingPings = [];

let isPingDirectoryCreated = false;




let Policy = {
  now: () => new Date(),
  getArchiveQuota: () => ARCHIVE_QUOTA_BYTES,
};

this.TelemetryStorage = {
  get MAX_PING_FILE_AGE() {
    return MAX_PING_FILE_AGE;
  },

  get OVERDUE_PING_FILE_AGE() {
    return OVERDUE_PING_FILE_AGE;
  },

  get MAX_LRU_PINGS() {
    return MAX_LRU_PINGS;
  },

  get pingDirectoryPath() {
    return OS.Path.join(OS.Constants.Path.profileDir, "saved-telemetry-pings");
  },

  




  shutdown: function() {
    return TelemetryStorageImpl.shutdown();
  },

  





  saveArchivedPing: function(ping) {
    return TelemetryStorageImpl.saveArchivedPing(ping);
  },

  





  loadArchivedPing: function(id) {
    return TelemetryStorageImpl.loadArchivedPing(id);
  },

  





  runCleanPingArchiveTask: function() {
    return TelemetryStorageImpl.runCleanPingArchiveTask();
  },

  


  reset: function() {
    return TelemetryStorageImpl.reset();
  },

  


  testCleanupTaskPromise: function() {
    return (TelemetryStorageImpl._cleanArchiveTask || Promise.resolve());
  },

  






  loadArchivedPingList: function() {
    return TelemetryStorageImpl.loadArchivedPingList();
  },

  






  saveAbortedSessionPing: function(ping) {
    return TelemetryStorageImpl.saveAbortedSessionPing(ping);
  },

  





  loadAbortedSessionPing: function() {
    return TelemetryStorageImpl.loadAbortedSessionPing();
  },

  




  removeAbortedSessionPing: function() {
    return TelemetryStorageImpl.removeAbortedSessionPing();
  },

  









  savePingToFile: function(ping, file, overwrite) {
    return TelemetryStorageImpl.savePingToFile(ping, file, overwrite);
  },

  







  savePing: function(ping, overwrite) {
    return TelemetryStorageImpl.savePing(ping, overwrite);
  },

  






  addPendingPing: function(pingData) {
    return TelemetryStorageImpl.addPendingPing(pingData);
  },

  








  addPendingPingFromFile: function(pingPath) {
    return TelemetryStorageImpl.addPendingPingFromFile(pingPath);
  },

  





  cleanupPingFile: function(ping) {
    return TelemetryStorageImpl.cleanupPingFile(ping);
  },

  







  loadSavedPings: function() {
    return TelemetryStorageImpl.loadSavedPings();
  },

  








  loadHistograms: function loadHistograms(file) {
    return TelemetryStorageImpl.loadHistograms(file);
  },

  


  get pingsLoaded() {
    return TelemetryStorageImpl.pingsLoaded;
  },

  



  get pingsOverdue() {
    return TelemetryStorageImpl.pingsOverdue;
  },

  



  get pingsDiscarded() {
    return TelemetryStorageImpl.pingsDiscarded;
  },

  




  popPendingPings: function*() {
    while (pendingPings.length > 0) {
      let data = pendingPings.pop();
      yield data;
    }
  },

  testLoadHistograms: function(file) {
    return TelemetryStorageImpl.testLoadHistograms(file);
  },

  





  loadPingFile: Task.async(function* (aFilePath) {
    return TelemetryStorageImpl.loadPingFile(aFilePath);
  }),

  






  _testGetArchivedPingPath: function(aPingId, aDate, aType) {
    return getArchivedPingPath(aPingId, aDate, aType);
  },

  









  _testGetArchivedPingDataFromFileName: function(aFileName) {
    return TelemetryStorageImpl._getArchivedPingDataFromFileName(aFileName);
  },
};








function SaveSerializer() {
  this._queuedOperations = [];
  this._queuedInProgress = false;
  this._log = Log.repository.getLoggerWithMessagePrefix(LOGGER_NAME, LOGGER_PREFIX);
}

SaveSerializer.prototype = {
  






  enqueueTask: function (aFunction) {
    let promise = new Promise((resolve, reject) =>
      this._queuedOperations.push([aFunction, resolve, reject]));

    if (this._queuedOperations.length == 1) {
      this._popAndPerformQueuedOperation();
    }
    return promise;
  },

  



  flushTasks: function () {
    let dummyTask = () => new Promise(resolve => resolve());
    return this.enqueueTask(dummyTask);
  },

  



  _popAndPerformQueuedOperation: function () {
    if (!this._queuedOperations.length || this._queuedInProgress) {
      return;
    }

    this._log.trace("_popAndPerformQueuedOperation - Performing queued operation.");
    let [func, resolve, reject] = this._queuedOperations.shift();
    let promise;

    try {
      this._queuedInProgress = true;
      promise = func();
    } catch (ex) {
      this._log.warn("_popAndPerformQueuedOperation - Queued operation threw during execution. ",
                     ex);
      this._queuedInProgress = false;
      reject(ex);
      this._popAndPerformQueuedOperation();
      return;
    }

    if (!promise || typeof(promise.then) != "function") {
      let msg = "Queued operation did not return a promise: " + func;
      this._log.warn("_popAndPerformQueuedOperation - " + msg);

      this._queuedInProgress = false;
      reject(new Error(msg));
      this._popAndPerformQueuedOperation();
      return;
    }

    promise.then(result => {
        this._queuedInProgress = false;
        resolve(result);
        this._popAndPerformQueuedOperation();
      },
      error => {
        this._log.warn("_popAndPerformQueuedOperation - Failure when performing queued operation.",
                       error);
        this._queuedInProgress = false;
        reject(error);
        this._popAndPerformQueuedOperation();
      });
  },
};

let TelemetryStorageImpl = {
  _logger: null,
  
  _abortedSessionSerializer: new SaveSerializer(),

  
  
  _archivedPings: new Map(),
  
  _scanArchiveTask: null,
  
  _cleanArchiveTask: null,
  
  _scannedArchiveDirectory: false,

  
  _shutdown: false,

  get _log() {
    if (!this._logger) {
      this._logger = Log.repository.getLoggerWithMessagePrefix(LOGGER_NAME, LOGGER_PREFIX);
    }

    return this._logger;
  },

  




  shutdown: Task.async(function*() {
    this._shutdown = true;
    yield this._abortedSessionSerializer.flushTasks();
    yield this.savePendingPings();
    
    
    yield this._cleanArchiveTask;
  }),

  





  saveArchivedPing: Task.async(function*(ping) {
    const creationDate = new Date(ping.creationDate);
    if (this._archivedPings.has(ping.id)) {
      const data = this._archivedPings.get(ping.id);
      if (data.timestampCreated > creationDate.getTime()) {
        this._log.error("saveArchivedPing - trying to overwrite newer ping with the same id");
        return Promise.reject(new Error("trying to overwrite newer ping with the same id"));
      } else {
        this._log.warn("saveArchivedPing - overwriting older ping with the same id");
      }
    }

    
    const filePath = getArchivedPingPath(ping.id, creationDate, ping.type) + "lz4";
    yield OS.File.makeDir(OS.Path.dirname(filePath), { ignoreExisting: true,
                                                       from: OS.Constants.Path.profileDir });
    yield this.savePingToFile(ping, filePath,  true,  true);

    this._archivedPings.set(ping.id, {
      timestampCreated: creationDate.getTime(),
      type: ping.type,
    });

    Telemetry.getHistogramById("TELEMETRY_ARCHIVE_SESSION_PING_COUNT").add();
  }),

  





  loadArchivedPing: Task.async(function*(id) {
    this._log.trace("loadArchivedPing - id: " + id);

    const data = this._archivedPings.get(id);
    if (!data) {
      this._log.trace("loadArchivedPing - no ping with id: " + id);
      return Promise.reject(new Error("TelemetryStorage.loadArchivedPing - no ping with id " + id));
    }

    const path = getArchivedPingPath(id, new Date(data.timestampCreated), data.type);
    const pathCompressed = path + "lz4";

    try {
      
      this._log.trace("loadArchivedPing - loading ping from: " + pathCompressed);
      return yield this.loadPingFile(pathCompressed,  true);
    } catch (ex if ex.becauseNoSuchFile) {
      
      this._log.trace("loadArchivedPing - compressed ping not found, loading: " + path);
      return yield this.loadPingFile(path,  false);
    }
  }),

  







  _removeArchivedPing: Task.async(function*(id, timestampCreated, type) {
    this._log.trace("_removeArchivedPing - id: " + id + ", timestampCreated: " + timestampCreated + ", type: " + type);
    const path = getArchivedPingPath(id, new Date(timestampCreated), type);
    const pathCompressed = path + "lz4";

    this._log.trace("_removeArchivedPing - removing ping from: " + path);
    yield OS.File.remove(path, {ignoreAbsent: true});
    yield OS.File.remove(pathCompressed, {ignoreAbsent: true});
  }),

  




  runCleanPingArchiveTask: function() {
    
    if (this._cleanArchiveTask) {
      return this._cleanArchiveTask;
    }

    
    let clear = () => this._cleanArchiveTask = null;
    
    this._cleanArchiveTask = this._cleanArchive().then(clear, clear);
    return this._cleanArchiveTask;
  },

  



  _purgeOldPings: Task.async(function*() {
    this._log.trace("_purgeOldPings");

    const nowDate = Policy.now();
    const startTimeStamp = nowDate.getTime();
    let dirIterator = new OS.File.DirectoryIterator(gPingsArchivePath);
    let subdirs = (yield dirIterator.nextBatch()).filter(e => e.isDir);

    
    let newestRemovedMonthTimestamp = null;
    let evictedDirsCount = 0;
    let maxDirAgeInMonths = 0;

    
    for (let dir of subdirs) {
      if (this._shutdown) {
        this._log.trace("_purgeOldPings - Terminating the clean up task due to shutdown");
        return;
      }

      if (!isValidArchiveDir(dir.name)) {
        this._log.warn("_purgeOldPings - skipping invalidly named subdirectory " + dir.path);
        continue;
      }

      const archiveDate = getDateFromArchiveDir(dir.name);
      if (!archiveDate) {
        this._log.warn("_purgeOldPings - skipping invalid subdirectory date " + dir.path);
        continue;
      }

      
      if ((startTimeStamp - archiveDate.getTime()) > MAX_ARCHIVED_PINGS_RETENTION_MS) {
        try {
          yield OS.File.removeDir(dir.path);
          evictedDirsCount++;

          
          newestRemovedMonthTimestamp = Math.max(archiveDate, newestRemovedMonthTimestamp);
        } catch (ex) {
          this._log.error("_purgeOldPings - Unable to remove " + dir.path, ex);
        }
      } else {
        
        const dirAgeInMonths = Utils.getElapsedTimeInMonths(archiveDate, nowDate);
        maxDirAgeInMonths = Math.max(dirAgeInMonths, maxDirAgeInMonths);
      }
    }

    
    yield this.loadArchivedPingList();

    
    
    if (newestRemovedMonthTimestamp) {
      
      for (let [id, info] of this._archivedPings) {
        const timestampCreated = new Date(info.timestampCreated);
        if (timestampCreated.getTime() > newestRemovedMonthTimestamp) {
          continue;
        }
        
        this._archivedPings.delete(id);
      }
    }

    const endTimeStamp = Policy.now().getTime();

    
    Telemetry.getHistogramById("TELEMETRY_ARCHIVE_EVICTED_OLD_DIRS")
             .add(evictedDirsCount);
    Telemetry.getHistogramById("TELEMETRY_ARCHIVE_EVICTING_DIRS_MS")
             .add(Math.ceil(endTimeStamp - startTimeStamp));
    Telemetry.getHistogramById("TELEMETRY_ARCHIVE_OLDEST_DIRECTORY_AGE")
             .add(maxDirAgeInMonths);
  }),

  



  _enforceArchiveQuota: Task.async(function*() {
    this._log.trace("_enforceArchiveQuota");
    const startTimeStamp = Policy.now().getTime();

    
    let pingList = [for (p of this._archivedPings) {
      id: p[0],
      timestampCreated: p[1].timestampCreated,
      type: p[1].type,
    }];

    pingList.sort((a, b) => b.timestampCreated - a.timestampCreated);

    
    const SAFE_QUOTA = Policy.getArchiveQuota() * 0.9;
    
    
    let lastPingIndexToKeep = null;
    let archiveSizeInBytes = 0;

    
    for (let i = 0; i < pingList.length; i++) {
      if (this._shutdown) {
        this._log.trace("_enforceArchiveQuota - Terminating the clean up task due to shutdown");
        return;
      }

      let ping = pingList[i];

      
      const fileSize =
        yield getArchivedPingSize(ping.id, new Date(ping.timestampCreated), ping.type);
      if (!fileSize) {
        this._log.warn("_enforceArchiveQuota - Unable to find the size of ping " + ping.id);
        continue;
      }

      archiveSizeInBytes += fileSize;

      if (archiveSizeInBytes < SAFE_QUOTA) {
        
        
        lastPingIndexToKeep = i;
      } else if (archiveSizeInBytes > Policy.getArchiveQuota()) {
        
        break;
      }
    }

    
    Telemetry.getHistogramById("TELEMETRY_ARCHIVE_CHECKING_OVER_QUOTA_MS")
             .add(Math.round(Policy.now().getTime() - startTimeStamp));

    let submitProbes = (sizeInMB, evictedPings, elapsedMs) => {
      Telemetry.getHistogramById("TELEMETRY_ARCHIVE_SIZE_MB").add(sizeInMB);
      Telemetry.getHistogramById("TELEMETRY_ARCHIVE_EVICTED_OVER_QUOTA").add(evictedPings);
      Telemetry.getHistogramById("TELEMETRY_ARCHIVE_EVICTING_OVER_QUOTA_MS").add(elapsedMs);
    };

    
    if (archiveSizeInBytes < Policy.getArchiveQuota()) {
      submitProbes(Math.round(archiveSizeInBytes / 1024 / 1024), 0, 0);
      return;
    }

    this._log.info("_enforceArchiveQuota - archive size: " + archiveSizeInBytes + "bytes"
                   + ", safety quota: " + SAFE_QUOTA + "bytes");

    let pingsToPurge = pingList.slice(lastPingIndexToKeep + 1);

    
    for (let ping of pingsToPurge) {
      if (this._shutdown) {
        this._log.trace("_enforceArchiveQuota - Terminating the clean up task due to shutdown");
        return;
      }

      
      
      yield this._removeArchivedPing(ping.id, ping.timestampCreated, ping.type);

      
      this._archivedPings.delete(ping.id);
    }

    const endTimeStamp = Policy.now().getTime();
    submitProbes(ARCHIVE_SIZE_PROBE_SPECIAL_VALUE, pingsToPurge.length,
                 Math.ceil(endTimeStamp - startTimeStamp));
  }),

  _cleanArchive: Task.async(function*() {
    this._log.trace("cleanArchiveTask");

    if (!(yield OS.File.exists(gPingsArchivePath))) {
      return;
    }

    
    try {
      yield this._purgeOldPings();
    } catch (ex) {
      this._log.error("_cleanArchive - There was an error removing old directories", ex);
    }

    
    yield this._enforceArchiveQuota();
  }),

  


  reset: function() {
    this._shutdown = false;
    this._scannedArchiveDirectory = false;
    this._archivedPings = new Map();
  },

  






  loadArchivedPingList: function() {
    
    if (this._scanArchiveTask) {
      return this._scanArchiveTask;
    }

    if (this._scannedArchiveDirectory) {
      this._log.trace("loadArchivedPingList - Archive already scanned, hitting cache.");
      return Promise.resolve(this._archivedPings);
    }

    
    let clear = pingList => {
      this._scanArchiveTask = null;
      return pingList;
    };
    
    this._scanArchiveTask = this._scanArchive().then(clear, clear);
    return this._scanArchiveTask;
  },

  _scanArchive: Task.async(function*() {
    this._log.trace("_scanArchive");

    let submitProbes = (pingCount, dirCount) => {
      Telemetry.getHistogramById("TELEMETRY_ARCHIVE_SCAN_PING_COUNT")
               .add(pingCount);
      Telemetry.getHistogramById("TELEMETRY_ARCHIVE_DIRECTORIES_COUNT")
               .add(dirCount);
    };

    if (!(yield OS.File.exists(gPingsArchivePath))) {
      submitProbes(0, 0);
      return new Map();
    }

    let dirIterator = new OS.File.DirectoryIterator(gPingsArchivePath);
    let subdirs =
      (yield dirIterator.nextBatch()).filter(e => e.isDir).filter(e => isValidArchiveDir(e.name));

    
    for (let dir of subdirs) {
      this._log.trace("_scanArchive - checking in subdir: " + dir.path);
      let pingIterator = new OS.File.DirectoryIterator(dir.path);
      let pings = (yield pingIterator.nextBatch()).filter(e => !e.isDir);

      
      for (let p of pings) {
        
        let data = this._getArchivedPingDataFromFileName(p.name);
        if (!data) {
          continue;
        }

        
        if (this._archivedPings.has(data.id)) {
          const overwrite = data.timestamp > this._archivedPings.get(data.id).timestampCreated;
          this._log.warn("_scanArchive - have seen this id before: " + data.id +
                         ", overwrite: " + overwrite);
          if (!overwrite) {
            continue;
          }

          yield this._removeArchivedPing(data.id, data.timestampCreated, data.type)
                    .catch((e) => this._log.warn("_scanArchive - failed to remove ping", e));
        }

        this._archivedPings.set(data.id, {
          timestampCreated: data.timestamp,
          type: data.type,
        });
      }
    }

    
    this._scannedArchiveDirectory = true;
    
    submitProbes(this._archivedPings.size, subdirs.length);
    return this._archivedPings;
  }),

  











  savePingToFile: function(ping, filePath, overwrite, compress = false) {
    return Task.spawn(function*() {
      try {
        let pingString = JSON.stringify(ping);
        let options = { tmpPath: filePath + ".tmp", noOverwrite: !overwrite };
        if (compress) {
          options.compression = "lz4";
        }
        yield OS.File.writeAtomic(filePath, pingString, options);
      } catch(e if e.becauseExists) {
      }
    })
  },

  







  savePing: function(ping, overwrite) {
    return Task.spawn(function*() {
      yield getPingDirectory();
      let file = pingFilePath(ping);
      yield this.savePingToFile(ping, file, overwrite);
    }.bind(this));
  },

  




  savePendingPings: function() {
    let p = [for (ping of pendingPings) this.savePing(ping, false).catch(ex => {
      this._log.error("savePendingPings - failed to save pending pings.");
    })];
    return Promise.all(p);
  },

  








  addPendingPingFromFile: function(pingPath) {
    
    
    
    return this.loadPingFile(pingPath).then(ping => {
      
      Telemetry.getHistogramById("READ_SAVED_PING_SUCCESS").add(1);
      return this.addPendingPing(ping);
    });
  },

  







  addPendingPing: function(ping) {
    
    pendingPings.push(ping);
    
    return this.savePing(ping, false);
  },

  





  cleanupPingFile: function(ping) {
    return OS.File.remove(pingFilePath(ping));
  },

  







  loadSavedPings: function() {
    return Task.spawn(function*() {
      let directory = TelemetryStorage.pingDirectoryPath;
      let iter = new OS.File.DirectoryIterator(directory);
      let exists = yield iter.exists();

      if (exists) {
        let entries = yield iter.nextBatch();
        let sortedEntries = [];

        for (let entry of entries) {
          if (entry.isDir) {
            continue;
          }

          let info = yield OS.File.stat(entry.path);
          sortedEntries.push({entry:entry, lastModificationDate: info.lastModificationDate});
        }

        sortedEntries.sort(function compare(a, b) {
          return b.lastModificationDate - a.lastModificationDate;
        });

        let count = 0;
        let result = [];

        
        for (let i = 0; i < MAX_LRU_PINGS && i < sortedEntries.length; i++) {
          let entry = sortedEntries[i].entry;
          result.push(this.loadHistograms(entry.path))
        }

        for (let i = MAX_LRU_PINGS; i < sortedEntries.length; i++) {
          let entry = sortedEntries[i].entry;
          OS.File.remove(entry.path);
        }

        yield Promise.all(result);

        Services.telemetry.getHistogramById('TELEMETRY_FILES_EVICTED').
          add(sortedEntries.length - MAX_LRU_PINGS);
      }

      yield iter.close();
    }.bind(this));
  },

  








  loadHistograms: function loadHistograms(file) {
    return OS.File.stat(file).then(function(info){
      let now = Date.now();
      if (now - info.lastModificationDate > MAX_PING_FILE_AGE) {
        
        pingsDiscarded++;
        return OS.File.remove(file);
      }

      
      if (now - info.lastModificationDate > OVERDUE_PING_FILE_AGE) {
        pingsOverdue++;
      }

      pingsLoaded++;
      return addToPendingPings(file);
    });
  },

  


  get pingsLoaded() {
    return pingsLoaded;
  },

  



  get pingsOverdue() {
    return pingsOverdue;
  },

  



  get pingsDiscarded() {
    return pingsDiscarded;
  },

  testLoadHistograms: function(file) {
    pingsLoaded = 0;
    return this.loadHistograms(file.path);
  },

  






  loadPingFile: Task.async(function* (aFilePath, aCompressed = false) {
    let options = {};
    if (aCompressed) {
      options.compression = "lz4";
    }
    let array = yield OS.File.read(aFilePath, options);
    let decoder = new TextDecoder();
    let string = decoder.decode(array);

    let ping = JSON.parse(string);
    
    if (typeof(ping.payload) == "string") {
      ping.payload = JSON.parse(ping.payload);
    }
    return ping;
  }),

  











  _getArchivedPingDataFromFileName: function(fileName) {
    
    let parts = fileName.split(".");
    if (parts.length != 4) {
      this._log.trace("_getArchivedPingDataFromFileName - should have 4 parts");
      return null;
    }

    let [timestamp, uuid, type, extension] = parts;
    if (extension != "json" && extension != "jsonlz4") {
      this._log.trace("_getArchivedPingDataFromFileName - should have 'json' or 'jsonlz4' extension");
      return null;
    }

    
    timestamp = parseInt(timestamp);
    if (Number.isNaN(timestamp)) {
      this._log.trace("_getArchivedPingDataFromFileName - should have a valid timestamp");
      return null;
    }

    
    const uuidRegex = /^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/i;
    if (!uuidRegex.test(uuid)) {
      this._log.trace("_getArchivedPingDataFromFileName - should have a valid id");
      return null;
    }

    
    const typeRegex = /^[a-z0-9][a-z0-9-]+[a-z0-9]$/i;
    if (!typeRegex.test(type)) {
      this._log.trace("_getArchivedPingDataFromFileName - should have a valid type");
      return null;
    }

    return {
      timestamp: timestamp,
      id: uuid,
      type: type,
    };
  },

  saveAbortedSessionPing: Task.async(function*(ping) {
    this._log.trace("saveAbortedSessionPing - ping path: " + gAbortedSessionFilePath);
    yield OS.File.makeDir(gDataReportingDir, { ignoreExisting: true });

    return this._abortedSessionSerializer.enqueueTask(() =>
      this.savePingToFile(ping, gAbortedSessionFilePath, true));
  }),

  loadAbortedSessionPing: Task.async(function*() {
    let ping = null;
    try {
      ping = yield this.loadPingFile(gAbortedSessionFilePath);
    } catch (ex if ex.becauseNoSuchFile) {
      this._log.trace("loadAbortedSessionPing - no such file");
    } catch (ex) {
      this._log.error("loadAbortedSessionPing - error removing ping", ex)
    }
    return ping;
  }),

  removeAbortedSessionPing: function() {
    return this._abortedSessionSerializer.enqueueTask(Task.async(function*() {
      try {
        yield OS.File.remove(gAbortedSessionFilePath, { ignoreAbsent: false });
        this._log.trace("removeAbortedSessionPing - success");
      } catch (ex if ex.becauseNoSuchFile) {
        this._log.trace("removeAbortedSessionPing - no such file");
      } catch (ex) {
        this._log.error("removeAbortedSessionPing - error removing ping", ex)
      }
    }.bind(this)));
  },
};



function pingFilePath(ping) {
  
  let pingIdentifier = (ping.slug) ? ping.slug : ping.id;
  return OS.Path.join(TelemetryStorage.pingDirectoryPath, pingIdentifier);
}

function getPingDirectory() {
  return Task.spawn(function*() {
    let directory = TelemetryStorage.pingDirectoryPath;

    if (!isPingDirectoryCreated) {
      yield OS.File.makeDir(directory, { unixMode: OS.Constants.S_IRWXU });
      isPingDirectoryCreated = true;
    }

    return directory;
  });
}

function addToPendingPings(file) {
  function onLoad(success) {
    let success_histogram = Telemetry.getHistogramById("READ_SAVED_PING_SUCCESS");
    success_histogram.add(success);
  }

  return TelemetryStorage.loadPingFile(file).then(ping => {
      pendingPings.push(ping);
      onLoad(true);
    },
    () => {
      onLoad(false);
      return OS.File.remove(file);
    });
}








function getArchivedPingPath(aPingId, aDate, aType) {
  
  let addLeftPadding = value => (value < 10) ? ("0" + value) : value;
  
  
  let archivedPingDir = OS.Path.join(gPingsArchivePath,
    aDate.getFullYear() + '-' + addLeftPadding(aDate.getMonth() + 1));
  
  let fileName = [aDate.getTime(), aPingId, aType, "json"].join(".");
  return OS.Path.join(archivedPingDir, fileName);
}





let getArchivedPingSize = Task.async(function*(aPingId, aDate, aType) {
  const path = getArchivedPingPath(aPingId, aDate, aType);
  let filePaths = [ path + "lz4", path ];

  for (let path of filePaths) {
    try {
      return (yield OS.File.stat(path)).size;
    } catch (e) {}
  }

  
  return 0;
});






function isValidArchiveDir(aDirName) {
  const dirRegEx = /^[0-9]{4}-[0-9]{2}$/;
  return dirRegEx.test(aDirName);
}







function getDateFromArchiveDir(aDirName) {
  let [year, month] = aDirName.split("-");
  year = parseInt(year);
  month = parseInt(month);
  
  if (!Number.isFinite(month) || !Number.isFinite(year) || month < 1 || month > 12) {
    return null;
  }
  return new Date(year, month - 1, 1, 0, 0, 0);
}
