







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
  
  
  
  
  hasWrittenState: false,

  
  path: OS.Path.join(OS.Constants.Path.profileDir, "sessionstore.js"),

  
  backupPath: OS.Path.join(OS.Constants.Path.profileDir, "sessionstore.bak"),

  


  init: function () {
    return {result: true};
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

  





  gatherTelemetry: function (stateString) {
    return Statistics.collect(stateString);
  },

  


  _write: function (stateString, telemetry = {}) {
    let bytes = Encoder.encode(stateString);
    let startMs = Date.now();
    let result = File.writeAtomic(this.path, bytes, {tmpPath: this.path + ".tmp"});
    telemetry.FX_SESSION_RESTORE_WRITE_FILE_MS = Date.now() - startMs;
    telemetry.FX_SESSION_RESTORE_FILE_SIZE_BYTES = bytes.byteLength;
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





function getByteLength(str) {
  return Encoder.encode(JSON.stringify(str)).byteLength;
}




let Statistics = {
  collect: function(stateString) {
    let start = Date.now();
    let TOTAL_PREFIX = "FX_SESSION_RESTORE_TOTAL_";
    let INDIVIDUAL_PREFIX = "FX_SESSION_RESTORE_INDIVIDUAL_";
    let SIZE_SUFFIX = "_SIZE_BYTES";

    let state = JSON.parse(stateString);

    
    let subsets = {};
    this.gatherSimpleData(state, subsets);
    this.gatherComplexData(state, subsets);

    
    let telemetry = {};
    for (let k of Object.keys(subsets)) {
      let obj = subsets[k];
      telemetry[TOTAL_PREFIX + k + SIZE_SUFFIX] = getByteLength(obj);

      if (Array.isArray(obj)) {
        let size = obj.map(getByteLength);
        telemetry[INDIVIDUAL_PREFIX + k + SIZE_SUFFIX] = size;
      }
    }

    let stop = Date.now();
    telemetry["FX_SESSION_RESTORE_EXTRACTING_STATISTICS_DURATION_MS"] = stop - start;
    return {
      telemetry: telemetry
    };
  },

  



  gatherSimpleData: function(state, subsets) {
    
    subsets.OPEN_WINDOWS = state.windows;

    
    subsets.CLOSED_WINDOWS = state._closedWindows;

    
    
    subsets.CLOSED_TABS_IN_OPEN_WINDOWS = [];

    
    
    subsets.COOKIES = [];

    for (let winData of state.windows) {
      let closedTabs = winData._closedTabs || [];
      subsets.CLOSED_TABS_IN_OPEN_WINDOWS.push(...closedTabs);

      let cookies = winData.cookies || [];
      subsets.COOKIES.push(...cookies);
    }

    for (let winData of state._closedWindows) {
      let cookies = winData.cookies || [];
      subsets.COOKIES.push(...cookies);
    }
  },

  






  walk: function(root, cb) {
    if (!root || typeof root !== "object") {
      return;
    }
    for (let k of Object.keys(root)) {
      let obj = root[k];
      let stepIn = cb(k, obj);
      if (stepIn) {
        this.walk(obj, cb);
      }
    }
  },

  


  gatherComplexData: function(state, subsets) {
    
    subsets.DOM_STORAGE = [];
    
    subsets.FORMDATA = [];
    
    subsets.HISTORY = [];


    this.walk(state, function(k, value) {
      let dest;
      switch (k) {
        case "entries":
          subsets.HISTORY.push(value);
          return true;
        case "storage":
          subsets.DOM_STORAGE.push(value);
          
          return false;
        case "formdata":
          subsets.FORMDATA.push(value);
          
          return false;
        case "cookies": 
        case "extData":
          return false;
        default:
          return true;
      }
    });

    return subsets;
  },

};
