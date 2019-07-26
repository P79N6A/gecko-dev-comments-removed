



"use strict";

this.EXPORTED_SYMBOLS = ["SessionFile"];




















const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/osfile.jsm");
Cu.import("resource://gre/modules/osfile/_PromiseWorker.jsm", this);
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/AsyncShutdown.jsm");

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
  



  writeLoadStateOnceAfterStartup: function (aLoadState) {
    SessionFileInternal.writeLoadStateOnceAfterStartup(aLoadState);
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




const TaskUtils = {
  






  captureErrors: function captureErrors(promise) {
    return promise.then(
      null,
      function onError(reason) {
        Cu.reportError("Uncaught asynchronous error: " + reason + " at\n" + reason.stack);
        throw reason;
      }
    );
  },
  










  spawn: function spawn(gen) {
    return this.captureErrors(Task.spawn(gen));
  }
};

let SessionFileInternal = {
  


  path: OS.Path.join(OS.Constants.Path.profileDir, "sessionstore.js"),

  


  backupPath: OS.Path.join(OS.Constants.Path.profileDir, "sessionstore.bak"),

  




  _latestWrite: null,

  


  _isClosed: false,

  read: function () {
    return SessionWorker.post("read").then(msg => {
      this._recordTelemetry(msg.telemetry);
      return msg.ok;
    });
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

    return this._latestWrite = TaskUtils.spawn(function task() {
      TelemetryStopwatch.start("FX_SESSION_RESTORE_WRITE_FILE_LONGEST_OP_MS", refObj);

      try {
        let promise = SessionWorker.post("write", [aData]);
        
        TelemetryStopwatch.finish("FX_SESSION_RESTORE_WRITE_FILE_LONGEST_OP_MS", refObj);

        
        let msg = yield promise;
        this._recordTelemetry(msg.telemetry);
      } catch (ex) {
        TelemetryStopwatch.cancel("FX_SESSION_RESTORE_WRITE_FILE_LONGEST_OP_MS", refObj);
        Cu.reportError("Could not write session state file " + this.path
                       + ": " + ex);
      }

      if (isFinalWrite) {
        Services.obs.notifyObservers(null, "sessionstore-final-state-write-complete", "");
      }
    }.bind(this));
  },

  writeLoadStateOnceAfterStartup: function (aLoadState) {
    SessionWorker.post("writeLoadStateOnceAfterStartup", [aLoadState]).then(msg => {
      this._recordTelemetry(msg.telemetry);
      return msg;
    }, Cu.reportError);
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
    for (let histogramId in telemetry){
      Telemetry.getHistogramById(histogramId).add(telemetry[histogramId]);
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
          } else {
            throw error;
          }
        }
      );
    }
  };
})();



AsyncShutdown.profileBeforeChange.addBlocker(
  "SessionFile: Finish writing the latest sessionstore.js",
  function() {
    return SessionFile._latestWrite;
  });
