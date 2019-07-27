



"use strict";

this.EXPORTED_SYMBOLS = ["SessionFile"];




















const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/osfile.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/AsyncShutdown.jsm");
Cu.import("resource://gre/modules/Preferences.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "console",
  "resource://gre/modules/devtools/Console.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "RunState",
  "resource:///modules/sessionstore/RunState.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "TelemetryStopwatch",
  "resource://gre/modules/TelemetryStopwatch.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
  "resource://gre/modules/Task.jsm");
XPCOMUtils.defineLazyServiceGetter(this, "Telemetry",
  "@mozilla.org/base/telemetry;1", "nsITelemetry");
XPCOMUtils.defineLazyServiceGetter(this, "sessionStartup",
  "@mozilla.org/browser/sessionstartup;1", "nsISessionStartup");
XPCOMUtils.defineLazyModuleGetter(this, "SessionWorker",
  "resource:///modules/sessionstore/SessionWorker.jsm");

const PREF_UPGRADE_BACKUP = "browser.sessionstore.upgradeBackup.latestBuildID";
const PREF_MAX_UPGRADE_BACKUPS = "browser.sessionstore.upgradeBackup.maxUpgradeBackups";

const PREF_MAX_SERIALIZE_BACK = "browser.sessionstore.max_serialize_back";
const PREF_MAX_SERIALIZE_FWD = "browser.sessionstore.max_serialize_forward";

this.SessionFile = {
  


  read: function () {
    return SessionFileInternal.read();
  },
  


  write: function (aData) {
    return SessionFileInternal.write(aData);
  },
  












  gatherTelemetry: function(aData) {
    return SessionFileInternal.gatherTelemetry(aData);
  },
  


  wipe: function () {
    return SessionFileInternal.wipe();
  },

  



  get Paths() {
    return SessionFileInternal.Paths;
  }
};

Object.freeze(SessionFile);

let Path = OS.Path;
let profileDir = OS.Constants.Path.profileDir;

let SessionFileInternal = {
  Paths: Object.freeze({
    
    
    clean: Path.join(profileDir, "sessionstore.js"),

    
    
    cleanBackup: Path.join(profileDir, "sessionstore-backups", "previous.js"),

    
    backups: Path.join(profileDir, "sessionstore-backups"),

    
    
    
    
    
    recovery: Path.join(profileDir, "sessionstore-backups", "recovery.js"),

    
    
    
    
    
    
    
    recoveryBackup: Path.join(profileDir, "sessionstore-backups", "recovery.bak"),

    
    
    
    
    upgradeBackupPrefix: Path.join(profileDir, "sessionstore-backups", "upgrade.js-"),

    
    
    
    
    
    
    get upgradeBackup() {
      let latestBackupID = SessionFileInternal.latestUpgradeBackupID;
      if (!latestBackupID) {
        return "";
      }
      return this.upgradeBackupPrefix + latestBackupID;
    },

    
    
    
    get nextUpgradeBackup() {
      return this.upgradeBackupPrefix + Services.appinfo.platformBuildID;
    },

    


    get loadOrder() {
      
      
      
      
      
      
      
      
      
      let order = ["clean",
                   "recovery",
                   "recoveryBackup",
                   "cleanBackup"];
      if (SessionFileInternal.latestUpgradeBackupID) {
        
        order.push("upgradeBackup");
      }
      return order;
    },
  }),

  
  
  get latestUpgradeBackupID() {
    try {
      return Services.prefs.getCharPref(PREF_UPGRADE_BACKUP);
    } catch (ex) {
      return undefined;
    }
  },

  read: Task.async(function* () {
    let result;
    let noFilesFound = true;
    
    for (let key of this.Paths.loadOrder) {
      let corrupted = false;
      let exists = true;
      try {
        let path = this.Paths[key];
        let startMs = Date.now();
        let source = yield OS.File.read(path, { encoding: "utf-8" });
        let parsed = JSON.parse(source);
        result = {
          origin: key,
          source: source,
          parsed: parsed
        };
        Telemetry.getHistogramById("FX_SESSION_RESTORE_CORRUPT_FILE").
          add(false);
        Telemetry.getHistogramById("FX_SESSION_RESTORE_READ_FILE_MS").
          add(Date.now() - startMs);
        break;
      } catch (ex if ex instanceof OS.File.Error && ex.becauseNoSuchFile) {
        exists = false;
      } catch (ex if ex instanceof OS.File.Error) {
        
        
        console.error("Could not read session file ", ex, ex.stack);
        corrupted = true;
      } catch (ex if ex instanceof SyntaxError) {
        console.error("Corrupt session file (invalid JSON found) ", ex, ex.stack);
        
        corrupted = true;
      } finally {
        if (exists) {
          noFilesFound = false;
          Telemetry.getHistogramById("FX_SESSION_RESTORE_CORRUPT_FILE").
            add(corrupted);
        }
      }
    }

    
    let allCorrupt = !noFilesFound && !result;
    Telemetry.getHistogramById("FX_SESSION_RESTORE_ALL_FILES_CORRUPT").
      add(allCorrupt);

    if (!result) {
      
      result = {
        origin: "empty",
        source: "",
        parsed: null
      };
    }

    result.noFilesFound = noFilesFound;

    
    
    SessionWorker.post("init", [result.origin, this.Paths, {
      maxUpgradeBackups: Preferences.get(PREF_MAX_UPGRADE_BACKUPS, 3),
      maxSerializeBack: Preferences.get(PREF_MAX_SERIALIZE_BACK, 10),
      maxSerializeForward: Preferences.get(PREF_MAX_SERIALIZE_FWD, -1)
    }]);

    return result;
  }),

  gatherTelemetry: function(aStateString) {
    return Task.spawn(function() {
      let msg = yield SessionWorker.post("gatherTelemetry", [aStateString]);
      this._recordTelemetry(msg.telemetry);
      throw new Task.Result(msg.telemetry);
    }.bind(this));
  },

  write: function (aData) {
    if (RunState.isClosed) {
      return Promise.reject(new Error("SessionFile is closed"));
    }

    let isFinalWrite = false;
    if (RunState.isClosing) {
      
      
      isFinalWrite = true;
      RunState.setClosed();
    }

    let refObj = {};
    let name = "FX_SESSION_RESTORE_WRITE_FILE_LONGEST_OP_MS";

    let promise = new Promise(resolve => {
      
      TelemetryStopwatch.start(name, refObj);

      let performShutdownCleanup = isFinalWrite &&
        !sessionStartup.isAutomaticRestoreEnabled();

      let options = {isFinalWrite, performShutdownCleanup};

      try {
        resolve(SessionWorker.post("write", [aData, options]));
      } finally {
        
        TelemetryStopwatch.finish(name, refObj);
      }
    });

    
    promise = promise.then(msg => {
      
      this._recordTelemetry(msg.telemetry);

      if (msg.result.upgradeBackup) {
        
        
        Services.prefs.setCharPref(PREF_UPGRADE_BACKUP,
          Services.appinfo.platformBuildID);
      }
    }, err => {
      
      TelemetryStopwatch.cancel(name, refObj);
      console.error("Could not write session state file ", err, err.stack);
      
      
      
    });

    
    
    AsyncShutdown.profileBeforeChange.addBlocker(
      "SessionFile: Finish writing Session Restore data", promise);

    
    
    
    return promise.then(() => {
      
      AsyncShutdown.profileBeforeChange.removeBlocker(promise);

      if (isFinalWrite) {
        Services.obs.notifyObservers(null, "sessionstore-final-state-write-complete", "");
      }
    });
  },

  wipe: function () {
    return SessionWorker.post("wipe");
  },

  _recordTelemetry: function(telemetry) {
    for (let id of Object.keys(telemetry)){
      let value = telemetry[id];
      let samples = [];
      if (Array.isArray(value)) {
        samples.push(...value);
      } else {
        samples.push(value);
      }
      let histogram = Telemetry.getHistogramById(id);
      for (let sample of samples) {
        histogram.add(sample);
      }
    }
  }
};
