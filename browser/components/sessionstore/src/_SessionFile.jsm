



"use strict";

this.EXPORTED_SYMBOLS = ["_SessionFile"];




















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
XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
  "resource://gre/modules/NetUtil.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "FileUtils",
  "resource://gre/modules/FileUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
  "resource://gre/modules/Task.jsm");
XPCOMUtils.defineLazyServiceGetter(this, "Telemetry",
  "@mozilla.org/base/telemetry;1", "nsITelemetry");
XPCOMUtils.defineLazyModuleGetter(this, "Deprecated",
  "resource://gre/modules/Deprecated.jsm");

this._SessionFile = {
  


  read: function () {
    return SessionFileInternal.read();
  },
  


  syncRead: function () {
    Deprecated.warning(
      "syncRead is deprecated and will be removed in a future version",
      "https://bugzilla.mozilla.org/show_bug.cgi?id=532150")
    return SessionFileInternal.syncRead();
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

Object.freeze(_SessionFile);




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

  





  readAuxSync: function (aPath) {
    let text;
    try {
      let file = new FileUtils.File(aPath);
      let chan = NetUtil.newChannel(file);
      let stream = chan.open();
      text = NetUtil.readInputStreamToString(stream, stream.available(),
        {charset: "utf-8"});
    } catch (e if e.result == Components.results.NS_ERROR_FILE_NOT_FOUND) {
      
    } catch (ex) {
      
      Cu.reportError(ex);
    } finally {
      return text;
    }
  },

  










  syncRead: function () {
    
    TelemetryStopwatch.start("FX_SESSION_RESTORE_SYNC_READ_FILE_MS");
    
    let text = this.readAuxSync(this.path);
    if (typeof text === "undefined") {
      
      text = this.readAuxSync(this.backupPath);
    }
    
    TelemetryStopwatch.finish("FX_SESSION_RESTORE_SYNC_READ_FILE_MS");
    text = text || "";

    
    
    SessionWorker.post("setInitialState", [text]);
    return text;
  },

  read: function () {
    return SessionWorker.post("read").then(msg => {
      this._recordTelemetry(msg.telemetry);
      return msg.ok;
    });
  },

  write: function (aData) {
    if (this._isClosed) {
      return Promise.reject(new Error("_SessionFile is closed"));
    }
    let refObj = {};
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
      
      
      if (Services.startup.shuttingDown) {
        this._isClosed = true;
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
    return _SessionFile._latestWrite;
  });
