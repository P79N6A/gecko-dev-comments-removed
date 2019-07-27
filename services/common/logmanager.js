


"use strict;"

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Services",
  "resource://gre/modules/Services.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "FileUtils",
  "resource://gre/modules/FileUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Log",
  "resource://gre/modules/Log.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "OS",
  "resource://gre/modules/osfile.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "CommonUtils",
  "resource://services-common/utils.js");

Cu.import("resource://gre/modules/Preferences.jsm");
Cu.import("resource://gre/modules/Task.jsm");

this.EXPORTED_SYMBOLS = [
  "LogManager",
];

const DEFAULT_MAX_ERROR_AGE = 20 * 24 * 60 * 60; 













let formatter;
let dumpAppender;
let consoleAppender;


let allBranches = new Set();


function LogManager(prefRoot, logNames, logFilePrefix) {
  this.init(prefRoot, logNames, logFilePrefix);
}

LogManager.prototype = {
  REASON_SUCCESS: "success",
  REASON_ERROR: "error",

  _cleaningUpFileLogs: false,

  _prefObservers: [],

  init(prefRoot, logNames, logFilePrefix) {
    if (prefRoot instanceof Preferences) {
      this._prefs = prefRoot;
    } else {
      this._prefs = new Preferences(prefRoot);
    }

    this.logFilePrefix = logFilePrefix;
    if (!formatter) {
      
      formatter = new Log.BasicFormatter();
      consoleAppender = new Log.ConsoleAppender(formatter);
      dumpAppender = new Log.DumpAppender(formatter);
    }

    allBranches.add(this._prefs._branchStr);
    
    
    let setupAppender = (appender, prefName, defaultLevel, findSmallest = false) => {
      let observer = newVal => {
        let level = Log.Level[newVal] || defaultLevel;
        if (findSmallest) {
          
          
          
          
          
          for (let branch of allBranches) {
            let lookPrefBranch = new Preferences(branch);
            let lookVal = Log.Level[lookPrefBranch.get(prefName)];
            if (lookVal && lookVal < level) {
              level = lookVal;
            }
          }
        }
        appender.level = level;
      }
      this._prefs.observe(prefName, observer, this);
      this._prefObservers.push([prefName, observer]);
      
      observer(this._prefs.get(prefName));
      return observer;
    }

    this._observeConsolePref = setupAppender(consoleAppender, "log.appender.console", Log.Level.Error, true);
    this._observeDumpPref = setupAppender(dumpAppender, "log.appender.dump", Log.Level.Error, true);

    
    let fapp = this._fileAppender = new Log.StorageStreamAppender(formatter);
    
    
    this._observeStreamPref = setupAppender(fapp, "log.appender.file.level", Log.Level.Debug);

    
    for (let logName of logNames) {
      let log = Log.repository.getLogger(logName);
      
      
      
      
      
      log.level = Log.Level.All;
      for (let appender of [fapp, dumpAppender, consoleAppender]) {
        log.addAppender(appender);
      }
    }
    
    this._log = Log.repository.getLogger(logNames[0] + ".LogManager");
  },

  


  finalize() {
    for (let [name, pref] of this._prefObservers) {
      this._prefs.ignore(name, pref, this);
    }
    this._prefObservers = [];
    try {
      allBranches.delete(this._prefs._branchStr);
    } catch (e) {}
    this._prefs = null;
  },

  get _logFileSubDirectoryEntries() {
    
    
    
    
    return ["weave", "logs"];
  },

  







  _copyStreamToFile: Task.async(function* (inputStream, outputFileName) {
    
    
    const BUFFER_SIZE = 8192;

    
    let binaryStream = Cc["@mozilla.org/binaryinputstream;1"].createInstance(Ci.nsIBinaryInputStream);
    binaryStream.setInputStream(inputStream);
    
    let profd = FileUtils.getDir("ProfD", []);
    let outputFile = FileUtils.getDir("ProfD", this._logFileSubDirectoryEntries);
    yield OS.File.makeDir(outputFile.path, { ignoreExisting: true, from: profd.path });
    outputFile.append(outputFileName);
    let output = yield OS.File.open(outputFile.path, { write: true} );
    try {
      while (true) {
        let available = binaryStream.available();
        if (!available) {
          break;
        }
        let chunk = binaryStream.readByteArray(Math.min(available, BUFFER_SIZE));
        yield output.write(new Uint8Array(chunk));
      }
    } finally {
      try {
        binaryStream.close(); 
        yield output.close();
      } catch (ex) {
        this._log.error("Failed to close the input stream", ex);
      }
    }
    this._log.trace("finished copy to", outputFile.path);
  }),

  





  resetFileLog: Task.async(function* (reason) {
    try {
      let flushToFile;
      let reasonPrefix;
      switch (reason) {
        case this.REASON_SUCCESS:
          flushToFile = this._prefs.get("log.appender.file.logOnSuccess", false);
          reasonPrefix = "success";
          break;
        case this.REASON_ERROR:
          flushToFile = this._prefs.get("log.appender.file.logOnError", true);
          reasonPrefix = "error";
          break;
        default:
          throw new Error("Invalid reason");
      }

      
      if (!flushToFile) {
        this._fileAppender.reset();
        return;
      }

      let inStream = this._fileAppender.getInputStream();
      this._fileAppender.reset();
      if (inStream) {
        this._log.debug("Flushing file log");
        
        
        let filename = reasonPrefix + "-" + this.logFilePrefix + "-" + Date.now() + ".txt";
        this._log.trace("Beginning stream copy to " + filename + ": " +
                        Date.now());
        try {
          yield this._copyStreamToFile(inStream, filename);
          this._log.trace("onCopyComplete", Date.now());
        } catch (ex) {
          this._log.error("Failed to copy log stream to file", ex);
          return;
        }
        
        
        
        
        
        if (reason == this.REASON_ERROR && !this._cleaningUpFileLogs) {
          this._log.trace("Scheduling cleanup.");
          
          
          this.cleanupLogs().catch(err => {
            this._log.error("Failed to cleanup logs", err);
          });
        }
      }
    } catch (ex) {
      this._log.error("Failed to resetFileLog", ex)
    }
  }),

  


  cleanupLogs: Task.async(function* () {
    this._cleaningUpFileLogs = true;
    let logDir = FileUtils.getDir("ProfD", this._logFileSubDirectoryEntries);
    let iterator = new OS.File.DirectoryIterator(logDir.path);
    let maxAge = this._prefs.get("log.appender.file.maxErrorAge", DEFAULT_MAX_ERROR_AGE);
    let threshold = Date.now() - 1000 * maxAge;

    this._log.debug("Log cleanup threshold time: " + threshold);
    yield iterator.forEach(Task.async(function* (entry) {
      if (!entry.name.startsWith("error-" + this.logFilePrefix + "-") &&
          !entry.name.startsWith("success-" + this.logFilePrefix + "-")) {
        return;
      }
      try {
        
        let info = yield OS.File.stat(entry.path);
        if (info.lastModificationDate.getTime() >= threshold) {
          return;
        }
        this._log.trace(" > Cleanup removing " + entry.name +
                        " (" + info.lastModificationDate.getTime() + ")");
        yield OS.File.remove(entry.path);
        this._log.trace("Deleted " + entry.name);
      } catch (ex) {
        this._log.debug("Encountered error trying to clean up old log file "
                        + entry.name, ex);
      }
    }.bind(this)));
    this._cleaningUpFileLogs = false;
    this._log.debug("Done deleting files.");
    
    Services.obs.notifyObservers(null, "services-tests:common:log-manager:cleanup-logs", null);
  }),
}
