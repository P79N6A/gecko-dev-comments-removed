







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
    result = Agent[data.fun].apply(Agent, data.args);
  } catch (ex if ex instanceof OS.File.Error) {
    
    
    
    self.postMessage({fail: OS.File.Error.toMsg(ex), id: id});
    return;
  }

  
  
  
  self.postMessage({ok: result, id: id});
};

let Agent = {
  
  initialState: null,

  
  
  hasWrittenLoadStateOnce: false,

  
  
  
  
  hasWrittenState: false,

  
  path: OS.Path.join(OS.Constants.Path.profileDir, "sessionstore.js"),

  
  backupPath: OS.Path.join(OS.Constants.Path.profileDir, "sessionstore.bak"),

  






  setInitialState: function (aState) {
    
    
    
    if (this.hasWrittenLoadStateOnce) {
      throw new Error("writeLoadStateOnceAfterStartup() must only be called once.");
    }

    
    
    
    
    if (!this.initialState) {
      this.initialState = aState;
    }
  },

  



  read: function () {
    for (let path of [this.path, this.backupPath]) {
      try {
        return this.initialState = Decoder.decode(File.read(path));
      } catch (ex if isNoSuchFileEx(ex)) {
        
      }
    }

    
    return "";
  },

  


  write: function (stateString, options) {
    if (!this.hasWrittenState) {
      if (options && options.backupOnFirstWrite) {
        try {
          File.move(this.path, this.backupPath);
        } catch (ex if isNoSuchFileEx(ex)) {
          
        }
      }

      this.hasWrittenState = true;
    }

    let bytes = Encoder.encode(stateString);
    return File.writeAtomic(this.path, bytes, {tmpPath: this.path + ".tmp"});
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
    let bytes = Encoder.encode(JSON.stringify(state));
    return File.writeAtomic(this.path, bytes, {tmpPath: this.path + ".tmp"});
  },

  


  createBackupCopy: function (ext) {
    try {
      return File.copy(this.path, this.backupPath + ext);
    } catch (ex if isNoSuchFileEx(ex)) {
      
      return true;
    }
  },

  


  removeBackupCopy: function (ext) {
    try {
      return File.remove(this.backupPath + ext);
    } catch (ex if isNoSuchFileEx(ex)) {
      
      return true;
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

    return true;
  }
};

function isNoSuchFileEx(aReason) {
  return aReason instanceof OS.File.Error && aReason.becauseNoSuchFile;
}
