




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
Cu.import("resource://gre/modules/Promise.jsm", this);

XPCOMUtils.defineLazyModuleGetter(this, 'Deprecated',
  'resource://gre/modules/Deprecated.jsm');

const LOGGER_NAME = "Toolkit.Telemetry";
const LOGGER_PREFIX = "TelemetryStorage::";

const Telemetry = Services.telemetry;


const DATAREPORTING_DIR = "datareporting";
const PINGS_ARCHIVE_DIR = "archived";
XPCOMUtils.defineLazyGetter(this, "gPingsArchivePath", function() {
  return OS.Path.join(OS.Constants.Path.profileDir, DATAREPORTING_DIR, PINGS_ARCHIVE_DIR);
});



const MAX_PING_FILE_AGE = 14 * 24 * 60 * 60 * 1000; 



const OVERDUE_PING_FILE_AGE = 7 * 24 * 60 * 60 * 1000; 


const MAX_LRU_PINGS = 50;



let pingsLoaded = 0;



let pingsDiscarded = 0;



let pingsOverdue = 0;


let pendingPings = [];

let isPingDirectoryCreated = false;

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

  





  saveArchivedPing: function(ping) {
    return TelemetryStorageImpl.saveArchivedPing(ping);
  },

  







  loadArchivedPing: function(id, timestampCreated, type) {
    return TelemetryStorageImpl.loadArchivedPing(id, timestampCreated, type);
  },

  






  loadArchivedPingList: function() {
    return TelemetryStorageImpl.loadArchivedPingList();
  },

  









  savePingToFile: function(ping, file, overwrite) {
    return TelemetryStorageImpl.savePingToFile(ping, file, overwrite);
  },

  







  savePing: function(ping, overwrite) {
    return TelemetryStorageImpl.savePing(ping, overwrite);
  },

  





  savePendingPings: function(sessionPing) {
    return TelemetryStorageImpl.savePendingPings(sessionPing);
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
};

let TelemetryStorageImpl = {
  _logger: null,

  get _log() {
    if (!this._logger) {
      this._logger = Log.repository.getLoggerWithMessagePrefix(LOGGER_NAME, LOGGER_PREFIX);
    }

    return this._logger;
  },

  





  saveArchivedPing: Task.async(function*(ping) {
    const creationDate = new Date(ping.creationDate);
    const filePath = getArchivedPingPath(ping.id, creationDate, ping.type);
    yield OS.File.makeDir(OS.Path.dirname(filePath), { ignoreExisting: true,
                                                       from: OS.Constants.Path.profileDir });
    yield TelemetryStorage.savePingToFile(ping, filePath, true);
  }),

  







  loadArchivedPing: function(id, timestampCreated, type) {
    this._log.trace("loadArchivedPing - id: " + id + ", timestampCreated: " + timestampCreated + ", type: " + type);
    const path = getArchivedPingPath(id, new Date(timestampCreated), type);
    this._log.trace("loadArchivedPing - loading ping from: " + path);
    return this.loadPingFile(path);
  },

  







  _removeArchivedPing: function(id, timestampCreated, type) {
    this._log.trace("_removeArchivedPing - id: " + id + ", timestampCreated: " + timestampCreated + ", type: " + type);
    const path = getArchivedPingPath(id, new Date(timestampCreated), type);
    this._log.trace("_removeArchivedPing - removing ping from: " + path);
    return OS.File.remove(path);
  },

  






  loadArchivedPingList: Task.async(function*() {
    this._log.trace("loadArchivedPingList");

    if (!(yield OS.File.exists(gPingsArchivePath))) {
      return new Map();
    }

    let archivedPings = new Map();
    let dirIterator = new OS.File.DirectoryIterator(gPingsArchivePath);
    let subdirs = (yield dirIterator.nextBatch()).filter(e => e.isDir);

    
    for (let dir of subdirs) {
      const dirRegEx = /^[0-9]{4}-[0-9]{2}$/;
      if (!dirRegEx.test(dir.name)) {
        this._log.warn("loadArchivedPingList - skipping invalidly named subdirectory " + dir.path);
        continue;
      }

      this._log.trace("loadArchivedPingList - checking in subdir: " + dir.path);
      let pingIterator = new OS.File.DirectoryIterator(dir.path);
      let pings = (yield pingIterator.nextBatch()).filter(e => !e.isDir);

      
      for (let p of pings) {
        
        let data = this._getArchivedPingDataFromFileName(p.name);
        if (!data) {
          continue;
        }

        
        if (archivedPings.has(data.id)) {
          const overwrite = data.timestamp > archivedPings.get(data.id).timestampCreated;
          this._log.warn("loadArchivedPingList - have seen this id before: " + data.id +
                         ", overwrite: " + overwrite);
          if (!overwrite) {
            continue;
          }

          yield this._removeArchivedPing(data.id, data.timestampCreated, data.type)
                    .catch((e) => this._log.warn("loadArchivedPingList - failed to remove ping", e));
        }

        archivedPings.set(data.id, {
          timestampCreated: data.timestamp,
          type: data.type,
        });
      }
    }

    return archivedPings;
  }),

  









  savePingToFile: function(ping, filePath, overwrite) {
    return Task.spawn(function*() {
      try {
        let pingString = JSON.stringify(ping);
        yield OS.File.writeAtomic(filePath, pingString, {tmpPath: filePath + ".tmp",
                                  noOverwrite: !overwrite});
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

  





  savePendingPings: function(sessionPing) {
    let p = pendingPings.reduce((p, ping) => {
      
      
      p.push(this.savePing(ping, false));
      return p;}, [this.savePing(sessionPing, true)]);

    pendingPings = [];
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

  





  loadPingFile: Task.async(function* (aFilePath) {
    let array = yield OS.File.read(aFilePath);
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
    if (extension != "json") {
      this._log.trace("_getArchivedPingDataFromFileName - should have a 'json' extension");
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
