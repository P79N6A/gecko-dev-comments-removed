


















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


let libc = new SharedAll.Library("libc",
                                 "libc.so", "libSystem.B.dylib", "a.out");
exports.libc = libc;


let declareFFI = SharedAll.declareFFI.bind(null, libc);
exports.declareFFI = declareFFI;


let LazyBindings = {};
libc.declareLazy(LazyBindings, "strerror",
                 "strerror", ctypes.default_abi,
                  ctypes.char.ptr,
                  ctypes.int);


























let OSError = function OSError(operation = "unknown operation",
                               errno = ctypes.errno, path = "") {
  SharedAll.OSError.call(this, operation, path);
  this.unixErrno = errno;
};
OSError.prototype = Object.create(SharedAll.OSError.prototype);
OSError.prototype.toString = function toString() {
  return "Unix error " + this.unixErrno +
    " during operation " + this.operation +
    (this.path? " on file " + this.path : "") +
    " (" + LazyBindings.strerror(this.unixErrno).readString() + ")";
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




Object.defineProperty(OSError.prototype, "becauseInvalidArgument", {
  get: function becauseInvalidArgument() {
    return this.unixErrno == Const.EINVAL;
  }
});





OSError.toMsg = function toMsg(error) {
  return {
    operation: error.operation,
    unixErrno: error.unixErrno,
    path: error.path
  };
};




OSError.fromMsg = function fromMsg(msg) {
  return new OSError(msg.operation, msg.unixErrno, msg.path);
};
exports.Error = OSError;






let AbstractInfo = function AbstractInfo(path, isDir, isSymLink, size, lastAccessDate,
                                         lastModificationDate, unixLastStatusChangeDate,
                                         unixOwner, unixGroup, unixMode) {
  this._path = path;
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
  




  get path() {
    return this._path;
  },
  


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


OSError.closed = function closed(operation, path) {
  return new OSError(operation, Const.EBADF, path);
};

OSError.exists = function exists(operation, path) {
  return new OSError(operation, Const.EEXIST, path);
};

OSError.noSuchFile = function noSuchFile(operation, path) {
  return new OSError(operation, Const.ENOENT, path);
};

OSError.invalidArgument = function invalidArgument(operation) {
  return new OSError(operation, Const.EINVAL);
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
