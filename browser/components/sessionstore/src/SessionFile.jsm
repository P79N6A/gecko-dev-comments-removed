



"use strict";

this.EXPORTED_SYMBOLS = ["SessionFile"];




















const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/osfile.jsm");
Cu.import("resource://gre/modules/osfile/_PromiseWorker.jsm", this);
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/AsyncShutdown.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "console",
  "resource://gre/modules/devtools/Console.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "TelemetryStopwatch",
  "resource://gre/modules/TelemetryStopwatch.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
  "resource://gre/modules/Task.jsm");
XPCOMUtils.defineLazyServiceGetter(this, "Telemetry",
  "@mozilla.org/base/telemetry;1", "nsITelemetry");

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
  



  createBackupCopy: function (ext) {
    return SessionFileInternal.createBackupCopy(ext);
  },
  



  removeBackupCopy: function (ext) {
    return SessionFileInternal.removeBackupCopy(ext);
  },
  


  wipe: function () {
    SessionFileInternal.wipe();
  }
};

Object.freeze(SessionFile);




let SessionFileInternal = {
  


  path: OS.Path.join(OS.Constants.Path.profileDir, "sessionstore.js"),

  


  backupPath: OS.Path.join(OS.Constants.Path.profileDir, "sessionstore.bak"),

  




  _latestWrite: null,

  


  _isClosed: false,

  read: function () {
    
    
    
    
    SessionWorker.post("init");

    return Task.spawn(function*() {
      for (let filename of [this.path, this.backupPath]) {
        try {
          let startMs = Date.now();

          let data = yield OS.File.read(filename, { encoding: "utf-8" });

          Telemetry.getHistogramById("FX_SESSION_RESTORE_READ_FILE_MS")
                   .add(Date.now() - startMs);

          return data;
        } catch (ex if ex instanceof OS.File.Error && ex.becauseNoSuchFile) {
          
        }
      }

      return "";
    }.bind(this));
  },

  gatherTelemetry: function(aStateString) {
    return Task.spawn(function() {
      let msg = yield SessionWorker.post("gatherTelemetry", [aStateString]);
      this._recordTelemetry(msg.telemetry);
      throw new Task.Result(msg.telemetry);
    }.bind(this));
  },

  write: function (aData) {
    if (this._isClosed) {
      return Promise.reject(new Error("SessionFile is closed"));
    }
    let refObj = {};

    let isFinalWrite = false;
    if (Services.startup.shuttingDown) {
      
      
      isFinalWrite = this._isClosed = true;
    }

    return this._latestWrite = Task.spawn(function task() {
      TelemetryStopwatch.start("FX_SESSION_RESTORE_WRITE_FILE_LONGEST_OP_MS", refObj);

      try {
        let promise = SessionWorker.post("write", [aData]);
        
        TelemetryStopwatch.finish("FX_SESSION_RESTORE_WRITE_FILE_LONGEST_OP_MS", refObj);

        
        let msg = yield promise;
        this._recordTelemetry(msg.telemetry);
      } catch (ex) {
        TelemetryStopwatch.cancel("FX_SESSION_RESTORE_WRITE_FILE_LONGEST_OP_MS", refObj);
        console.error("Could not write session state file ", this.path, ex);
      }

      if (isFinalWrite) {
        Services.obs.notifyObservers(null, "sessionstore-final-state-write-complete", "");
      }
    }.bind(this));
  },

  createBackupCopy: function (ext) {
    return SessionWorker.post("createBackupCopy", [ext]);
  },

  removeBackupCopy: function (ext) {
    return SessionWorker.post("removeBackupCopy", [ext]);
  },

  wipe: function () {
    SessionWorker.post("wipe");
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


let SessionWorker = (function () {
  let worker = new PromiseWorker("resource:///modules/sessionstore/SessionWorker.js",
    OS.Shared.LOG.bind("SessionWorker"));
  return {
    post: function post(...args) {
      let promise = worker.post.apply(worker, args);
      return promise.then(
        null,
        function onError(error) {
          
          if (error instanceof PromiseWorker.WorkerError) {
            throw OS.File.Error.fromMsg(error.data);
          }
          
          if (error instanceof ErrorEvent) {
            throw new Error(error.message, error.filename, error.lineno);
          }
          throw error;
        }
      );
    }
  };
})();



AsyncShutdown.profileBeforeChange.addBlocker(
  "SessionFile: Finish writing the latest sessionstore.js",
  function() {
    return SessionFileInternal._latestWrite;
  });
