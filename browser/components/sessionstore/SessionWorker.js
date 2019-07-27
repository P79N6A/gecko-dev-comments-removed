







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

  







  init(origin, paths, prefs = {}) {
    if (!(origin in paths || origin == STATE_EMPTY)) {
      throw new TypeError("Invalid origin: " + origin);
    }

    
    for (let pref of ["maxUpgradeBackups", "maxSerializeBack", "maxSerializeForward"]) {
      if (!prefs.hasOwnProperty(pref)) {
        throw new TypeError(`Missing preference value for ${pref}`);
      }
    }

    this.state = origin;
    this.Paths = paths;
    this.maxUpgradeBackups = prefs.maxUpgradeBackups;
    this.maxSerializeBack = prefs.maxSerializeBack;
    this.maxSerializeForward = prefs.maxSerializeForward;
    this.upgradeBackupNeeded = paths.nextUpgradeBackup != paths.upgradeBackup;
    return {result: true};
  },

  












  write: function (state, options = {}) {
    let exn;
    let telemetry = {};

    
    if (options.isFinalWrite) {
      for (let window of state.windows) {
        for (let tab of window.tabs) {
          let lower = 0;
          let upper = tab.entries.length;

          if (this.maxSerializeBack > -1) {
            lower = Math.max(lower, tab.index - this.maxSerializeBack - 1);
          }
          if (this.maxSerializeForward > -1) {
            upper = Math.min(upper, tab.index + this.maxSerializeForward);
          }

          tab.entries = tab.entries.slice(lower, upper);
          tab.index -= lower;
        }
      }
    }

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
