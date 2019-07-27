


















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

let LOG = SharedAll.LOG.bind(SharedAll, "Win", "allthreads");
let Const = SharedAll.Constants.Win;


let libc = new SharedAll.Library("libc", "kernel32.dll");
exports.libc = libc;


let declareFFI = SharedAll.declareFFI.bind(null, libc);
exports.declareFFI = declareFFI;

let Scope = {};


libc.declareLazy(Scope, "FormatMessage",
                 "FormatMessageW", ctypes.winapi_abi,
                  ctypes.uint32_t,
                   ctypes.uint32_t,
                  ctypes.voidptr_t,
                   ctypes.uint32_t,
                  ctypes.uint32_t,
                     ctypes.char16_t.ptr,
                    ctypes.uint32_t,
                 ctypes.voidptr_t);


























let OSError = function OSError(operation = "unknown operation",
                               lastError = ctypes.winLastError, path = "") {
  operation = operation;
  SharedAll.OSError.call(this, operation, path);
  this.winLastError = lastError;
};
OSError.prototype = Object.create(SharedAll.OSError.prototype);
OSError.prototype.toString = function toString() {
  let buf = new (ctypes.ArrayType(ctypes.char16_t, 1024))();
  let result = Scope.FormatMessage(
    Const.FORMAT_MESSAGE_FROM_SYSTEM |
    Const.FORMAT_MESSAGE_IGNORE_INSERTS,
    null,
     this.winLastError,
     0,
         buf,
     1024,
           null
  );
  if (!result) {
    buf = "additional error " +
      ctypes.winLastError +
      " while fetching system error message";
  }
  return "Win error " + this.winLastError + " during operation "
    + this.operation + (this.path? " on file " + this.path : "") +
    " (" + buf.readString() + ")";
};
OSError.prototype.toMsg = function toMsg() {
  return OSError.toMsg(this);
};





Object.defineProperty(OSError.prototype, "becauseExists", {
  get: function becauseExists() {
    return this.winLastError == Const.ERROR_FILE_EXISTS ||
      this.winLastError == Const.ERROR_ALREADY_EXISTS;
  }
});




Object.defineProperty(OSError.prototype, "becauseNoSuchFile", {
  get: function becauseNoSuchFile() {
    return this.winLastError == Const.ERROR_FILE_NOT_FOUND ||
      this.winLastError == Const.ERROR_PATH_NOT_FOUND;
  }
});




Object.defineProperty(OSError.prototype, "becauseNotEmpty", {
  get: function becauseNotEmpty() {
    return this.winLastError == Const.ERROR_DIR_NOT_EMPTY;
  }
});




Object.defineProperty(OSError.prototype, "becauseClosed", {
  get: function becauseClosed() {
    return this.winLastError == Const.ERROR_INVALID_HANDLE;
  }
});




Object.defineProperty(OSError.prototype, "becauseAccessDenied", {
  get: function becauseAccessDenied() {
    return this.winLastError == Const.ERROR_ACCESS_DENIED;
  }
});




Object.defineProperty(OSError.prototype, "becauseInvalidArgument", {
  get: function becauseInvalidArgument() {
    return this.winLastError == Const.ERROR_NOT_SUPPORTED ||
           this.winLastError == Const.ERROR_BAD_ARGUMENTS;
  }
});





OSError.toMsg = function toMsg(error) {
  return {
    exn: "OS.File.Error",
    fileName: error.moduleName,
    lineNumber: error.lineNumber,
    stack: error.moduleStack,
    operation: error.operation,
    winLastError: error.winLastError,
    path: error.path
  };
};




OSError.fromMsg = function fromMsg(msg) {
  let error = new OSError(msg.operation, msg.winLastError, msg.path);
  error.stack = msg.stack;
  error.fileName = msg.fileName;
  error.lineNumber = msg.lineNumber;
  return error;
};
exports.Error = OSError;






let AbstractInfo = function AbstractInfo(path, isDir, isSymLink, size,
                                         winBirthDate,
                                         lastAccessDate, lastWriteDate) {
  this._path = path;
  this._isDir = isDir;
  this._isSymLink = isSymLink;
  this._size = size;
  this._winBirthDate = winBirthDate;
  this._lastAccessDate = lastAccessDate;
  this._lastModificationDate = lastWriteDate;
};

AbstractInfo.prototype = {
  




  get path() {
    return this._path;
  },
  


  get isDir() {
    return this._isDir;
  },
  


  get isSymLink() {
    return this._isSymLink;
  },
  







  get size() {
    return this._size;
  },
  
  get creationDate() {
    return this._winBirthDate;
  },
  




  get winBirthDate() {
    return this._winBirthDate;
  },
  







  get lastAccessDate() {
    return this._lastAccessDate;
  },
  







  get lastModificationDate() {
    return this._lastModificationDate;
  }
};
exports.AbstractInfo = AbstractInfo;






let AbstractEntry = function AbstractEntry(isDir, isSymLink, name,
                                           winCreationDate, winLastWriteDate,
                                           winLastAccessDate, path) {
  this._isDir = isDir;
  this._isSymLink = isSymLink;
  this._name = name;
  this._winCreationDate = winCreationDate;
  this._winLastWriteDate = winLastWriteDate;
  this._winLastAccessDate = winLastAccessDate;
  this._path = path;
};

AbstractEntry.prototype = {
  


  get isDir() {
    return this._isDir;
  },
  


  get isSymLink() {
    return this._isSymLink;
  },
  



  get name() {
    return this._name;
  },
  



  get winCreationDate() {
    return this._winCreationDate;
  },
  



  get winLastWriteDate() {
    return this._winLastWriteDate;
  },
  



  get winLastAccessDate() {
    return this._winLastAccessDate;
  },
  



  get path() {
    return this._path;
  }
};
exports.AbstractEntry = AbstractEntry;



exports.POS_START = Const.FILE_BEGIN;
exports.POS_CURRENT = Const.FILE_CURRENT;
exports.POS_END = Const.FILE_END;



let Type = Object.create(SharedAll.Type);
exports.Type = Type;






Type.path = Type.wstring.withName("[in] path");
Type.out_path = Type.out_wstring.withName("[out] path");


OSError.closed = function closed(operation, path) {
  return new OSError(operation, Const.ERROR_INVALID_HANDLE, path);
};

OSError.exists = function exists(operation, path) {
  return new OSError(operation, Const.ERROR_FILE_EXISTS, path);
};

OSError.noSuchFile = function noSuchFile(operation, path) {
  return new OSError(operation, Const.ERROR_FILE_NOT_FOUND, path);
};

OSError.invalidArgument = function invalidArgument(operation) {
  return new OSError(operation, Const.ERROR_NOT_SUPPORTED);
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
