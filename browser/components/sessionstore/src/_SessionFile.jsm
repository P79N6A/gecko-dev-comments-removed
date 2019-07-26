



"use strict";

this.EXPORTED_SYMBOLS = ["_SessionFile"];




















const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/osfile.jsm");
Cu.import("resource://gre/modules/commonjs/sdk/core/promise.js");

XPCOMUtils.defineLazyModuleGetter(this, "TelemetryStopwatch",
  "resource://gre/modules/TelemetryStopwatch.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
  "resource://gre/modules/NetUtil.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "FileUtils",
  "resource://gre/modules/FileUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
  "resource://gre/modules/Task.jsm");


XPCOMUtils.defineLazyGetter(this, "gEncoder", function () {
  return new TextEncoder();
});

this._SessionFile = {
  



  promiseInitialized: function SessionFile_initialized() {
    return SessionFileInternal.promiseInitialized;
  },
  


  read: function SessionFile_read() {
    return SessionFileInternal.read();
  },
  


  syncRead: function SessionFile_syncRead() {
    return SessionFileInternal.syncRead();
  },
  


  write: function SessionFile_write(aData) {
    return SessionFileInternal.write(aData);
  },
  


  createBackupCopy: function SessionFile_createBackupCopy() {
    return SessionFileInternal.createBackupCopy();
  },
  


  wipe: function SessionFile_wipe() {
    return SessionFileInternal.wipe();
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
  


  promiseInitialized: Promise.defer(),

  


  path: OS.Path.join(OS.Constants.Path.profileDir, "sessionstore.js"),

  


  backupPath: OS.Path.join(OS.Constants.Path.profileDir, "sessionstore.bak"),

  






  syncRead: function ssfi_syncRead() {
    let text;
    let exn;
    TelemetryStopwatch.start("FX_SESSION_RESTORE_SYNC_READ_FILE_MS");
    try {
      let file = new FileUtils.File(this.path);
      let chan = NetUtil.newChannel(file);
      let stream = chan.open();
      text = NetUtil.readInputStreamToString(stream, stream.available(), {charset: "utf-8"});
    } catch (e if e.result == Components.results.NS_ERROR_FILE_NOT_FOUND) {
      return "";
    } catch(ex) {
      exn = ex;
    } finally {
      TelemetryStopwatch.finish("FX_SESSION_RESTORE_SYNC_READ_FILE_MS");
    }
    if (exn) {
      Cu.reportError(exn);
      return "";
    }
    return text;
  },

  read: function ssfi_read() {
    let refObj = {};
    let self = this;
    return TaskUtils.spawn(function task() {
      TelemetryStopwatch.start("FX_SESSION_RESTORE_READ_FILE_MS", refObj);
      let text;
      try {
        let bytes = yield OS.File.read(self.path);
        text = new TextDecoder().decode(bytes);
        TelemetryStopwatch.finish("FX_SESSION_RESTORE_READ_FILE_MS", refObj);
      } catch (ex) {
        if (self._isNoSuchFile(ex)) {
          
          TelemetryStopwatch.finish("FX_SESSION_RESTORE_READ_FILE_MS", refObj);
        } else {
          
          TelemetryStopwatch.cancel("FX_SESSION_RESTORE_READ_FILE_MS", refObj);
          Cu.reportError(ex);
        }
        text = "";
      }
      throw new Task.Result(text);
    });
  },

  write: function ssfi_write(aData) {
    let refObj = {};
    let self = this;
    return TaskUtils.spawn(function task() {
      TelemetryStopwatch.start("FX_SESSION_RESTORE_WRITE_FILE_MS", refObj);

      let bytes = gEncoder.encode(aData);

      try {
        yield OS.File.writeAtomic(self.path, bytes, {tmpPath: self.path + ".tmp"});
        TelemetryStopwatch.finish("FX_SESSION_RESTORE_WRITE_FILE_MS", refObj);
      } catch (ex) {
        TelemetryStopwatch.cancel("FX_SESSION_RESTORE_WRITE_FILE_MS", refObj);
        Cu.reportError("Could not write session state file " + self.path
                       + ": " + aReason);
      }
    });
  },

  createBackupCopy: function ssfi_createBackupCopy() {
    let self = this;
    return TaskUtils.spawn(function task() {
      try {
        yield OS.File.copy(self.path, self.backupPath);
      } catch (ex if self._isNoSuchFile(ex)) {
        
      } catch (ex) {
        Cu.reportError("Could not backup session state file: " + ex);
        throw ex;
      }
    });
  },

  wipe: function ssfi_wipe() {
    let self = this;
    return TaskUtils.spawn(function task() {
      try {
        yield OS.File.remove(self.path);
      } catch (ex if self._isNoSuchFile(ex)) {
        
      } catch (ex) {
        Cu.reportError("Could not remove session state file: " + ex);
        throw ex;
      }

      try {
        yield OS.File.remove(self.backupPath);
      } catch (ex if self._isNoSuchFile(ex)) {
        
      } catch (ex) {
        Cu.reportError("Could not remove session state backup file: " + ex);
        throw ex;
      }
    });
  },

  _isNoSuchFile: function ssfi_isNoSuchFile(aReason) {
    return aReason instanceof OS.File.Error && aReason.becauseNoSuchFile;
  }
};
