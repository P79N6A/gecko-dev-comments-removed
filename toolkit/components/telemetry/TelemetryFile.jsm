




"use strict";

this.EXPORTED_SYMBOLS = ["TelemetryFile"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm", this);
Cu.import("resource://gre/modules/XPCOMUtils.jsm", this);
Cu.import("resource://gre/modules/osfile.jsm", this);
Cu.import("resource://gre/modules/Task.jsm", this);
Cu.import("resource://gre/modules/Promise.jsm", this);

XPCOMUtils.defineLazyModuleGetter(this, 'Deprecated',
  'resource://gre/modules/Deprecated.jsm');

const Telemetry = Services.telemetry;



const MAX_PING_FILE_AGE = 14 * 24 * 60 * 60 * 1000; 



const OVERDUE_PING_FILE_AGE = 7 * 24 * 60 * 60 * 1000; 


const MAX_LRU_PINGS = 17;



let pingsLoaded = 0;



let pingsDiscarded = 0;



let pingsOverdue = 0;


let pendingPings = [];

let isPingDirectoryCreated = false;

this.TelemetryFile = {

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

  









  savePingToFile: function(ping, file, overwrite) {
    return Task.spawn(function*() {
      try {
        let pingString = JSON.stringify(ping);
        yield OS.File.writeAtomic(file, pingString, {tmpPath: file + ".tmp",
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

  







  addPendingPing: function(aPingPath) {
    
    
    
    return loadPingFile(aPingPath).then(ping => {
        
        pendingPings.push(ping);
        
        Telemetry.getHistogramById("READ_SAVED_PING_SUCCESS").add(1);
        
        return this.savePing(ping, false);
      });
  },

  





  cleanupPingFile: function(ping) {
    return OS.File.remove(pingFilePath(ping));
  },

  







  loadSavedPings: function() {
    return Task.spawn(function*() {
      let directory = TelemetryFile.pingDirectoryPath;
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

  




  popPendingPings: function*() {
    while (pendingPings.length > 0) {
      let data = pendingPings.pop();
      yield data;
    }
  },

  testLoadHistograms: function(file) {
    pingsLoaded = 0;
    return this.loadHistograms(file.path);
  }
};


function pingFilePath(ping) {
  
  let pingIdentifier = (ping.slug) ? ping.slug : ping.id;
  return OS.Path.join(TelemetryFile.pingDirectoryPath, pingIdentifier);
}

function getPingDirectory() {
  return Task.spawn(function*() {
    let directory = TelemetryFile.pingDirectoryPath;

    if (!isPingDirectoryCreated) {
      yield OS.File.makeDir(directory, { unixMode: OS.Constants.S_IRWXU });
      isPingDirectoryCreated = true;
    }

    return directory;
  });
}







let loadPingFile = Task.async(function* (aFilePath) {
  let array = yield OS.File.read(aFilePath);
  let decoder = new TextDecoder();
  let string = decoder.decode(array);

  let ping = JSON.parse(string);
  
  if (typeof(ping.payload) == "string") {
    ping.payload = JSON.parse(ping.payload);
  }
  return ping;
});

function addToPendingPings(file) {
  function onLoad(success) {
    let success_histogram = Telemetry.getHistogramById("READ_SAVED_PING_SUCCESS");
    success_histogram.add(success);
  }

  return loadPingFile(file).then(ping => {
      pendingPings.push(ping);
      onLoad(true);
    },
    () => {
      onLoad(false);
      return OS.File.remove(file);
    });
}
