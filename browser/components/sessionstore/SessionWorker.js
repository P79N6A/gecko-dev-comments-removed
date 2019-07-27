







"use strict";

importScripts("resource://gre/modules/osfile.jsm");

let PromiseWorker = require("resource://gre/modules/workers/PromiseWorker.js");

let File = OS.File;
let Encoder = new TextEncoder();
let Decoder = new TextDecoder();

let worker = new PromiseWorker.AbstractWorker();
worker.dispatch = function(method, args = []) {
  return Agent[method](...args);
};
worker.postMessage = function(result, ...transfers) {
  self.postMessage(result, ...transfers);
};
worker.close = function() {
  self.close();
};

self.addEventListener("message", msg => worker.handleMessage(msg));







const STATE_CLEAN = "clean";






const STATE_RECOVERY = "recovery";





const STATE_RECOVERY_BACKUP = "recoveryBackup";






const STATE_UPGRADE_BACKUP = "upgradeBackup";




const STATE_EMPTY = "empty";

let Agent = {
  
  Paths: null,

  








  state: null,

  


  maxUpgradeBackups: null,

  







  init: function (origin, paths, maxUpgradeBackups) {
    if (!(origin in paths || origin == STATE_EMPTY)) {
      throw new TypeError("Invalid origin: " + origin);
    }
    this.state = origin;
    this.Paths = paths;
    this.maxUpgradeBackups = maxUpgradeBackups || 3;
    this.upgradeBackupNeeded = paths.nextUpgradeBackup != paths.upgradeBackup;
    return {result: true};
  },

  












  write: function (state, options = {}) {
    let exn;
    let telemetry = {};

    let stateString = JSON.stringify(state);
    let data = Encoder.encode(stateString);
    let startWriteMs, stopWriteMs;

    try {

      if (this.state == STATE_CLEAN || this.state == STATE_EMPTY) {
        
        
        
        File.makeDir(this.Paths.backups);
      }

      if (this.state == STATE_CLEAN) {
        
        
        File.move(this.Paths.clean, this.Paths.cleanBackup);
      }

      startWriteMs = Date.now();

      if (options.isFinalWrite) {
        
        
        
        
        
        File.writeAtomic(this.Paths.clean, data, {
          tmpPath: this.Paths.clean + ".tmp"
        });
      } else if (this.state == STATE_RECOVERY) {
        
        
        
        
        
        
        File.writeAtomic(this.Paths.recovery, data, {
          tmpPath: this.Paths.recovery + ".tmp",
          backupTo: this.Paths.recoveryBackup
        });
      } else {
        
        
        
        File.writeAtomic(this.Paths.recovery, data, {
          tmpPath: this.Paths.recovery + ".tmp"
        });
      }

      stopWriteMs = Date.now();

    } catch (ex) {
      
      exn = exn || ex;
    }

    
    let upgradeBackupComplete = false;
    if (this.upgradeBackupNeeded
      && (this.state == STATE_CLEAN || this.state == STATE_UPGRADE_BACKUP)) {
      try {
        
        let path = this.state == STATE_CLEAN ? this.Paths.cleanBackup : this.Paths.upgradeBackup;
        File.copy(path, this.Paths.nextUpgradeBackup);
        this.upgradeBackupNeeded = false;
        upgradeBackupComplete = true;
      } catch (ex) {
        
        exn = exn || ex;
      }

      
      let iterator;
      let backups = [];  
      let upgradeBackupPrefix = this.Paths.upgradeBackupPrefix;  

      try {
        iterator = new File.DirectoryIterator(this.Paths.backups);
        iterator.forEach(function (file) {
          if (file.path.startsWith(upgradeBackupPrefix)) {
            backups.push(file.path);
          }
        }, this);
      } catch (ex) {
          
          exn = exn || ex;
      } finally {
        if (iterator) {
          iterator.close();
        }
      }

      
      if (backups.length > this.maxUpgradeBackups) {
        
        backups.sort().forEach((file, i) => {
          
          if (i < backups.length - this.maxUpgradeBackups) {
            File.remove(file);
          }
        });
      }
    }

    if (options.performShutdownCleanup && !exn) {

      
      
      
      

      
      
      File.remove(this.Paths.recoveryBackup);
      File.remove(this.Paths.recovery);
    }

    this.state = STATE_RECOVERY;

    if (exn) {
      throw exn;
    }

    return {
      result: {
        upgradeBackup: upgradeBackupComplete
      },
      telemetry: {
        FX_SESSION_RESTORE_WRITE_FILE_MS: stopWriteMs - startWriteMs,
        FX_SESSION_RESTORE_FILE_SIZE_BYTES: data.byteLength,
      }
    };
  },

  





  gatherTelemetry: function (stateString) {
    return Statistics.collect(stateString);
  },

  


  wipe: function () {

    
    let exn = null;

    
    try {
      File.remove(this.Paths.clean);
    } catch (ex) {
      
      exn = exn || ex;
    }

    
    try {
      this._wipeFromDir(this.Paths.backups, null);
    } catch (ex) {
      exn = exn || ex;
    }

    try {
      File.removeDir(this.Paths.backups);
    } catch (ex) {
      exn = exn || ex;
    }

    
    try {
      this._wipeFromDir(OS.Constants.Path.profileDir, "sessionstore.bak");
    } catch (ex) {
      exn = exn || ex;
    }


    this.state = STATE_EMPTY;
    if (exn) {
      throw exn;
    }

    return { result: true };
  },

  






  _wipeFromDir: function(path, prefix) {
    
    if (typeof prefix == "undefined" || prefix == "") {
      throw new TypeError();
    }

    let exn = null;

    let iterator = new File.DirectoryIterator(path);
    if (!iterator.exists()) {
      return;
    }
    for (let entry in iterator) {
      if (entry.isDir) {
        continue;
      }
      if (!prefix || entry.name.startsWith(prefix)) {
        try {
          File.remove(entry.path);
        } catch (ex) {
          
          exn = exn || ex;
        }
      }
    }

    if (exn) {
      throw exn;
    }
  },
};

function isNoSuchFileEx(aReason) {
  return aReason instanceof OS.File.Error && aReason.becauseNoSuchFile;
}





function getByteLength(str) {
  return Encoder.encode(JSON.stringify(str)).byteLength;
}




let Statistics = {
  collect: function(stateString) {
    let start = Date.now();
    let TOTAL_PREFIX = "FX_SESSION_RESTORE_TOTAL_";
    let INDIVIDUAL_PREFIX = "FX_SESSION_RESTORE_INDIVIDUAL_";
    let SIZE_SUFFIX = "_SIZE_BYTES";

    let state = JSON.parse(stateString);

    
    let subsets = {};
    this.gatherSimpleData(state, subsets);
    this.gatherComplexData(state, subsets);

    
    let telemetry = {};
    for (let k of Object.keys(subsets)) {
      let obj = subsets[k];
      telemetry[TOTAL_PREFIX + k + SIZE_SUFFIX] = getByteLength(obj);

      if (Array.isArray(obj)) {
        let size = obj.map(getByteLength);
        telemetry[INDIVIDUAL_PREFIX + k + SIZE_SUFFIX] = size;
      }
    }

    let stop = Date.now();
    telemetry["FX_SESSION_RESTORE_EXTRACTING_STATISTICS_DURATION_MS"] = stop - start;
    return {
      telemetry: telemetry
    };
  },

  



  gatherSimpleData: function(state, subsets) {
    
    subsets.OPEN_WINDOWS = state.windows;

    
    subsets.CLOSED_WINDOWS = state._closedWindows;

    
    
    subsets.CLOSED_TABS_IN_OPEN_WINDOWS = [];

    
    
    subsets.COOKIES = [];

    for (let winData of state.windows) {
      let closedTabs = winData._closedTabs || [];
      subsets.CLOSED_TABS_IN_OPEN_WINDOWS.push(...closedTabs);

      let cookies = winData.cookies || [];
      subsets.COOKIES.push(...cookies);
    }

    for (let winData of state._closedWindows) {
      let cookies = winData.cookies || [];
      subsets.COOKIES.push(...cookies);
    }
  },

  






  walk: function(root, cb) {
    if (!root || typeof root !== "object") {
      return;
    }
    for (let k of Object.keys(root)) {
      let obj = root[k];
      let stepIn = cb(k, obj);
      if (stepIn) {
        this.walk(obj, cb);
      }
    }
  },

  


  gatherComplexData: function(state, subsets) {
    
    subsets.DOM_STORAGE = [];
    
    subsets.FORMDATA = [];
    
    subsets.HISTORY = [];


    this.walk(state, function(k, value) {
      let dest;
      switch (k) {
        case "entries":
          subsets.HISTORY.push(value);
          return true;
        case "storage":
          subsets.DOM_STORAGE.push(value);
          
          return false;
        case "formdata":
          subsets.FORMDATA.push(value);
          
          return false;
        case "cookies": 
        case "extData":
          return false;
        default:
          return true;
      }
    });

    return subsets;
  },

};
