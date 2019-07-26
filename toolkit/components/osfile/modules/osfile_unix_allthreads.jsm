


















"use strict";

let SharedAll;
if (typeof Components != "undefined") {
  let Cu = Components.utils;
  
  Cu.import("resource://gre/modules/ctypes.jsm", this);

  SharedAll = {};
  Cu.import("resource://gre/modules/osfile/osfile_shared_allthreads.jsm", SharedAll);
  this.exports = {};
} else if (typeof "module" != "undefined" && typeof "require" != "undefined") {
  
  SharedAll = require("resource://gre/modules/osfile/osfile_shared_allthreads.jsm");
} else {
  throw new Error("Please open this module with Component.utils.import or with require()");
}

let LOG = SharedAll.LOG.bind(SharedAll, "Unix", "allthreads");
let Const = SharedAll.Constants.libc;


let libc;
let libc_candidates =  [ "libSystem.B.dylib",
                         "libc.so.6",
                         "libc.so" ];
for (let i = 0; i < libc_candidates.length; ++i) {
  try {
    libc = ctypes.open(libc_candidates[i]);
    break;
  } catch (x) {
    LOG("Could not open libc ", libc_candidates[i]);
  }
}

if (!libc) {
  
  throw new Error("Could not open system library: no libc");
}
exports.libc = libc;


let declareFFI = SharedAll.declareFFI.bind(null, libc);
exports.declareFFI = declareFFI;


let strerror = libc.declare("strerror",
  ctypes.default_abi,
   ctypes.char.ptr,
   ctypes.int);
























let OSError = function OSError(operation, errno) {
  operation = operation || "unknown operation";
  SharedAll.OSError.call(this, operation);
  this.unixErrno = errno || ctypes.errno;
};
OSError.prototype = Object.create(SharedAll.OSError.prototype);
OSError.prototype.toString = function toString() {
  return "Unix error " + this.unixErrno +
    " during operation " + this.operation +
    " (" + strerror(this.unixErrno).readString() + ")";
};





Object.defineProperty(OSError.prototype, "becauseExists", {
  get: function becauseExists() {
    return this.unixErrno == Const.EEXIST;
  }
});




Object.defineProperty(OSError.prototype, "becauseNoSuchFile", {
  get: function becauseNoSuchFile() {
    return this.unixErrno == Const.ENOENT;
  }
});





 Object.defineProperty(OSError.prototype, "becauseNotEmpty", {
   get: function becauseNotEmpty() {
     return this.unixErrno == Const.ENOTEMPTY;
   }
 });




Object.defineProperty(OSError.prototype, "becauseClosed", {
  get: function becauseClosed() {
    return this.unixErrno == Const.EBADF;
  }
});




Object.defineProperty(OSError.prototype, "becauseAccessDenied", {
  get: function becauseAccessDenied() {
    return this.unixErrno == Const.EACCES;
  }
});





OSError.toMsg = function toMsg(error) {
  return {
    operation: error.operation,
    unixErrno: error.unixErrno
  };
};




OSError.fromMsg = function fromMsg(msg) {
  return new OSError(msg.operation, msg.unixErrno);
};
exports.Error = OSError;






let AbstractInfo = function AbstractInfo(isDir, isSymLink, size, lastAccessDate,
                                         lastModificationDate, unixLastStatusChangeDate,
                                         unixOwner, unixGroup, unixMode) {
  this._isDir = isDir;
  this._isSymlLink = isSymLink;
  this._size = size;
  this._lastAccessDate = lastAccessDate;
  this._lastModificationDate = lastModificationDate;
  this._unixLastStatusChangeDate = unixLastStatusChangeDate;
  this._unixOwner = unixOwner;
  this._unixGroup = unixGroup;
  this._unixMode = unixMode;
};

AbstractInfo.prototype = {
  


  get isDir() {
    return this._isDir;
  },
  


  get isSymLink() {
    return this._isSymlLink;
  },
  







  get size() {
    return this._size;
  },
  







  get lastAccessDate() {
    return this._lastAccessDate;
  },
  


  get lastModificationDate() {
    return this._lastModificationDate;
  },
  




  get unixLastStatusChangeDate() {
    return this._unixLastStatusChangeDate;
  },
  


  get unixOwner() {
    return this._unixOwner;
  },
  


  get unixGroup() {
    return this._unixGroup;
  },
  


  get unixMode() {
    return this._unixMode;
  }
};
exports.AbstractInfo = AbstractInfo;






let AbstractEntry = function AbstractEntry(isDir, isSymLink, name, path) {
  this._isDir = isDir;
  this._isSymlLink = isSymLink;
  this._name = name;
  this._path = path;
};

AbstractEntry.prototype = {
  


  get isDir() {
    return this._isDir;
  },
  


  get isSymLink() {
    return this._isSymlLink;
  },
  



  get name() {
    return this._name;
  },
  


  get path() {
    return this._path;
  }
};
exports.AbstractEntry = AbstractEntry;



exports.POS_START = Const.SEEK_SET;
exports.POS_CURRENT = Const.SEEK_CUR;
exports.POS_END = Const.SEEK_END;



let Type = Object.create(SharedAll.Type);
exports.Type = Type;






Type.path = Type.cstring.withName("[in] path");
Type.out_path = Type.out_cstring.withName("[out] path");


OSError.closed = function closed(operation) {
  return new OSError(operation, Const.EBADF);
};

OSError.exists = function exists(operation) {
  return new OSError(operation, Const.EEXIST);
};

OSError.noSuchFile = function noSuchFile(operation) {
  return new OSError(operation, Const.ENOENT);
};

let EXPORTED_SYMBOLS = [
  "declareFFI",
  "libc",
  "Error",
  "AbstractInfo",
  "AbstractEntry",
  "Type",
  "POS_START",
  "POS_CURRENT",
  "POS_END"
];


if (typeof Components != "undefined") {
  this.EXPORTED_SYMBOLS = EXPORTED_SYMBOLS;
  for (let symbol of EXPORTED_SYMBOLS) {
    this[symbol] = exports[symbol];
  }
}
