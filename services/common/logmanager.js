


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





function FlushableStorageAppender(formatter) {
  Log.StorageStreamAppender.call(this, formatter);
  this.sawError = false;
}

FlushableStorageAppender.prototype = {
  __proto__: Log.StorageStreamAppender.prototype,

  append(message) {
    if (message.level >= Log.Level.Error) {
      this.sawError = true;
    }
    Log.StorageStreamAppender.prototype.append.call(this, message);
  },

  reset() {
    Log.StorageStreamAppender.prototype.reset.call(this);
    this.sawError = false;
  },

  
  
  flushToFile: Task.async(function* (subdirArray, filename, log) {
    let inStream = this.getInputStream();
    this.reset();
    if (!inStream) {
      log.debug("Failed to flush log to a file - no input stream");
      return;
    }
    log.debug("Flushing file log");
    log.trace("Beginning stream copy to " + filename + ": " + Date.now());
    try {
      yield this._copyStreamToFile(inStream, subdirArray, filename, log);
      log.trace("onCopyComplete", Date.now());
    } catch (ex) {
      log.error("Failed to copy log stream to file", ex);
    }
  }),

  







  _copyStreamToFile: Task.async(function* (inputStream, subdirArray, outputFileName, log) {
    
    
    const BUFFER_SIZE = 8192;

    
    let binaryStream = Cc["@mozilla.org/binaryinputstream;1"].createInstance(Ci.nsIBinaryInputStream);
    binaryStream.setInputStream(inputStream);

    let outputDirectory = OS.Path.join(OS.Constants.Path.profileDir, ...subdirArray);
    yield OS.File.makeDir(outputDirectory, { ignoreExisting: true, from: OS.Constants.Path.profileDir });
    let fullOutputFileName = OS.Path.join(outputDirectory, outputFileName);
    let output = yield OS.File.open(fullOutputFileName, { write: true} );
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
        log.error("Failed to close the input stream", ex);
      }
    }
    log.trace("finished copy to", fullOutputFileName);
  }),
}


function LogManager(prefRoot, logNames, logFilePrefix) {
  this.init(prefRoot, logNames, logFilePrefix);
}

LogManager.prototype = {
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

    this._observeConsolePref = setupAppender(consoleAppender, "log.appender.console", Log.Level.Fatal, true);
    this._observeDumpPref = setupAppender(dumpAppender, "log.appender.dump", Log.Level.Error, true);

    
    let fapp = this._fileAppender = new FlushableStorageAppender(formatter);
    
    
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

  
  SUCCESS_LOG_WRITTEN: "success-log-written",
  ERROR_LOG_WRITTEN: "error-log-written",

  








  resetFileLog: Task.async(function* () {
    try {
      let flushToFile;
      let reasonPrefix;
      let reason;
      if (this._fileAppender.sawError) {
        reason = this.ERROR_LOG_WRITTEN;
        flushToFile = this._prefs.get("log.appender.file.logOnError", true);
        reasonPrefix = "error";
      } else {
        reason = this.SUCCESS_LOG_WRITTEN;
        flushToFile = this._prefs.get("log.appender.file.logOnSuccess", false);
        reasonPrefix = "success";
      }

      
      if (!flushToFile) {
        this._fileAppender.reset();
        return null;
      }

      
      
      let filename = reasonPrefix + "-" + this.logFilePrefix + "-" + Date.now() + ".txt";
      yield this._fileAppender.flushToFile(this._logFileSubDirectoryEntries, filename, this._log);

      
      
      
      
      
      if (reason == this.ERROR_LOG_WRITTEN && !this._cleaningUpFileLogs) {
        this._log.trace("Scheduling cleanup.");
        
        
        this.cleanupLogs().catch(err => {
          this._log.error("Failed to cleanup logs", err);
        });
      }
      return reason;
    } catch (ex) {
      this._log.error("Failed to resetFileLog", ex);
      return null;
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
