







"use strict";

importScripts("resource://gre/modules/osfile.jsm");

let File = OS.File;
let Encoder = new TextEncoder();
let Decoder = new TextDecoder();











self.onmessage = function (msg) {
  let data = msg.data;
  if (!(data.fun in Agent)) {
    throw new Error("Cannot find method " + data.fun);
  }

  let result;
  let id = data.id;

  try {
    result = Agent[data.fun].apply(Agent, data.args) || {};
  } catch (ex if ex instanceof OS.File.Error) {
    
    
    
    self.postMessage({fail: OS.File.Error.toMsg(ex), id: id});
    return;
  }

  
  
  
  self.postMessage({
    ok: result.result,
    id: id,
    telemetry: result.telemetry || {}
  });
};

let Agent = {
  
  initialState: null,

  
  
  hasWrittenLoadStateOnce: false,

  
  
  
  
  hasWrittenState: false,

  
  path: OS.Path.join(OS.Constants.Path.profileDir, "sessionstore.js"),

  
  backupPath: OS.Path.join(OS.Constants.Path.profileDir, "sessionstore.bak"),

  



  read: function () {
    for (let path of [this.path, this.backupPath]) {
      try {
        let durationMs = Date.now();
        let bytes = File.read(path);
        durationMs = Date.now() - durationMs;
        this.initialState = Decoder.decode(bytes);

        return {
          result: this.initialState,
          telemetry: {FX_SESSION_RESTORE_READ_FILE_MS: durationMs}
        };
      } catch (ex if isNoSuchFileEx(ex)) {
        
      }
    }
    
    return {result: ""};
  },

  


  write: function (stateString) {
    let exn;
    let telemetry = {};

    if (!this.hasWrittenState) {
      try {
        let startMs = Date.now();
        File.move(this.path, this.backupPath);
        telemetry.FX_SESSION_RESTORE_BACKUP_FILE_MS = Date.now() - startMs;
      } catch (ex if isNoSuchFileEx(ex)) {
        
      } catch (ex) {
        
        
        exn = ex;
      }

      this.hasWrittenState = true;
    }

    let ret = this._write(stateString, telemetry);

    if (exn) {
      throw exn;
    }

    return ret;
  },

  




  writeLoadStateOnceAfterStartup: function (loadState) {
    if (this.hasWrittenLoadStateOnce) {
      throw new Error("writeLoadStateOnceAfterStartup() must only be called once.");
    }

    if (!this.initialState) {
      throw new Error("writeLoadStateOnceAfterStartup() must not be called " +
                      "without a valid session state or before it has been " +
                      "read from disk.");
    }

    
    this.hasWrittenLoadStateOnce = true;

    let state;
    try {
      state = JSON.parse(this.initialState);
    } finally {
      this.initialState = null;
    }

    state.session = state.session || {};
    state.session.state = loadState;
    return this._write(JSON.stringify(state));
  },

  


  _write: function (stateString, telemetry = {}) {
    let bytes = Encoder.encode(stateString);
    let startMs = Date.now();
    let result = File.writeAtomic(this.path, bytes, {tmpPath: this.path + ".tmp"});
    telemetry.FX_SESSION_RESTORE_WRITE_FILE_MS = Date.now() - startMs;
    return {result: result, telemetry: telemetry};
  },

  


  createBackupCopy: function (ext) {
    try {
      return {result: File.copy(this.path, this.backupPath + ext)};
    } catch (ex if isNoSuchFileEx(ex)) {
      
      return {result: true};
    }
  },

  


  removeBackupCopy: function (ext) {
    try {
      return {result: File.remove(this.backupPath + ext)};
    } catch (ex if isNoSuchFileEx(ex)) {
      
      return {result: true};
    }
  },

  


  wipe: function () {
    let exn;

    
    try {
      File.remove(this.path);
    } catch (ex if isNoSuchFileEx(ex)) {
      
    } catch (ex) {
      
      exn = ex;
    }

    
    let iter = new File.DirectoryIterator(OS.Constants.Path.profileDir);
    for (let entry in iter) {
      if (!entry.isDir && entry.path.startsWith(this.backupPath)) {
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

    return {result: true};
  }
};

function isNoSuchFileEx(aReason) {
  return aReason instanceof OS.File.Error && aReason.becauseNoSuchFile;
}
