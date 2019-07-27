



"use strict";

module.metadata = {
  "stability": "stable"
};

const { Cc, Ci } = require("chrome");
const file = require("./io/file");
const prefs = require("./preferences/service");
const jpSelf = require("./self");
const timer = require("./timers");
const unload = require("./system/unload");
const { emit, on, off } = require("./event/core");

const WRITE_PERIOD_PREF = "extensions.addon-sdk.simple-storage.writePeriod";
const WRITE_PERIOD_DEFAULT = 300000; 

const QUOTA_PREF = "extensions.addon-sdk.simple-storage.quota";
const QUOTA_DEFAULT = 5242880; 

const JETPACK_DIR_BASENAME = "jetpack";

Object.defineProperties(exports, {
  storage: {
    enumerable: true,
    get: function() { return manager.root; },
    set: function(value) { manager.root = value; }
  },
  quotaUsage: {
    get: function() { return manager.quotaUsage; }
  }
});



function JsonStore(options) {
  this.filename = options.filename;
  this.quota = options.quota;
  this.writePeriod = options.writePeriod;
  this.onOverQuota = options.onOverQuota;
  this.onWrite = options.onWrite;

  unload.ensure(this);

  this.writeTimer = timer.setInterval(this.write.bind(this),
                                      this.writePeriod);
}

JsonStore.prototype = {
  
  get root() {
    return this.isRootInited ? this._root : {};
  },

  
  set root(val) {
    let types = ["array", "boolean", "null", "number", "object", "string"];
    if (types.indexOf(typeof(val)) < 0) {
      throw new Error("storage must be one of the following types: " +
                      types.join(", "));
    }
    this._root = val;
    return val;
  },

  
  
  get isRootInited() {
    return this._root !== undefined;
  },

  
  
  get quotaUsage() {
    return this.quota > 0 ?
           JSON.stringify(this.root).length / this.quota :
           undefined;
  },

  
  purge: function JsonStore_purge() {
    try {
      
      file.remove(this.filename);
      let parentPath = this.filename;
      do {
        parentPath = file.dirname(parentPath);
        
        file.rmdir(parentPath);
      } while (file.basename(parentPath) !== JETPACK_DIR_BASENAME);
    }
    catch (err) {}
  },

  
  read: function JsonStore_read() {
    try {
      let str = file.read(this.filename);

      
      
      
      this.root = JSON.parse(str);
    }
    catch (err) {
      this.root = {};
    }
  },

  
  
  write: function JsonStore_write() {
    if (this.quotaUsage > 1)
      this.onOverQuota(this);
    else
      this._write();
  },

  
  
  unload: function JsonStore_unload(reason) {
    timer.clearInterval(this.writeTimer);
    this.writeTimer = null;

    if (reason === "uninstall")
      this.purge();
    else
      this._write();
  },

  
  get _isEmpty() {
    if (this.root && typeof(this.root) === "object") {
      let empty = true;
      for (let key in this.root) {
        empty = false;
        break;
      }
      return empty;
    }
    return false;
  },

  
  
  
  _write: function JsonStore__write() {
    
    
    if (!this.isRootInited || (this._isEmpty && !file.exists(this.filename)))
      return;

    
    
    if (this.quotaUsage > 1)
      return;

    
    let stream = file.open(this.filename, "w");
    try {
      stream.writeAsync(JSON.stringify(this.root), function writeAsync(err) {
        if (err)
          console.error("Error writing simple storage file: " + this.filename);
        else if (this.onWrite)
          this.onWrite(this);
      }.bind(this));
    }
    catch (err) {
      
      stream.close();
    }
  }
};





let manager = ({
  jsonStore: null,

  
  get filename() {
    let storeFile = Cc["@mozilla.org/file/directory_service;1"].
                    getService(Ci.nsIProperties).
                    get("ProfD", Ci.nsIFile);
    storeFile.append(JETPACK_DIR_BASENAME);
    storeFile.append(jpSelf.id);
    storeFile.append("simple-storage");
    file.mkpath(storeFile.path);
    storeFile.append("store.json");
    return storeFile.path;
  },

  get quotaUsage() {
    return this.jsonStore.quotaUsage;
  },

  get root() {
    if (!this.jsonStore.isRootInited)
      this.jsonStore.read();
    return this.jsonStore.root;
  },

  set root(val) {
    return this.jsonStore.root = val;
  },

  unload: function manager_unload() {
    off(this);
  },

  new: function manager_constructor() {
    let manager = Object.create(this);
    unload.ensure(manager);

    manager.jsonStore = new JsonStore({
      filename: manager.filename,
      writePeriod: prefs.get(WRITE_PERIOD_PREF, WRITE_PERIOD_DEFAULT),
      quota: prefs.get(QUOTA_PREF, QUOTA_DEFAULT),
      onOverQuota: emit.bind(null, exports, "OverQuota")
    });

    return manager;
  }
}).new();

exports.on = on.bind(null, exports);
exports.removeListener = function(type, listener) {
  off(exports, type, listener);
};
