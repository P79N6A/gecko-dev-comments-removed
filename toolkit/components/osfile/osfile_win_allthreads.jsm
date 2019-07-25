


















if (typeof Components != "undefined") {
  
  var EXPORTED_SYMBOLS = ["OS"];
  Components.utils.import("resource://gre/modules/ctypes.jsm");
  Components.utils.import("resource://gre/modules/osfile/osfile_shared_allthreads.jsm");
} else {
  
  importScripts("resource://gre/modules/osfile/osfile_shared_allthreads.jsm");
}

(function(exports) {
  "use strict";
  if (!exports.OS || !exports.OS.Shared) {
    throw new Error("osfile_win_allthreads.jsm must be loaded after osfile_shared_allthreads.jsm");
  }
  if (exports.OS.Shared.Win) {
    
    return;
  }
  exports.OS.Shared.Win = {};

  let LOG = OS.Shared.LOG.bind(OS.Shared, "Win", "allthreads");

  
  let libc = ctypes.open("kernel32.dll");
  if (!libc) {
    throw new Error("Could not open kernel32.dll");
  }
  exports.OS.Shared.Win.libc = libc;

  
  let declareFFI = OS.Shared.declareFFI.bind(null, libc);
  exports.OS.Shared.Win.declareFFI = declareFFI;

  
  let FormatMessage = libc.declare("FormatMessageW", ctypes.winapi_abi,
     ctypes.uint32_t,
      ctypes.uint32_t,
     ctypes.voidptr_t,
      ctypes.uint32_t,
     ctypes.uint32_t,
        ctypes.jschar.ptr,
       ctypes.uint32_t,
    ctypes.voidptr_t
  );

  






















  let OSError = function OSError(operation, lastError) {
    operation = operation || "unknown operation";
    exports.OS.Shared.Error.call(this, operation);
    this.winLastError = lastError || ctypes.winLastError;
  };
  OSError.prototype = new exports.OS.Shared.Error();
  OSError.prototype.toString = function toString() {
    let buf = new (ctypes.ArrayType(ctypes.jschar, 1024))();
    let result = FormatMessage(
      exports.OS.Constants.Win.FORMAT_MESSAGE_FROM_SYSTEM |
      exports.OS.Constants.Win.FORMAT_MESSAGE_IGNORE_INSERTS,
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
      + this.operation + " (" + buf.readString() + " )";
  };

  



  Object.defineProperty(OSError.prototype, "becauseExists", {
    get: function becauseExists() {
      return this.winLastError == exports.OS.Constants.Win.ERROR_FILE_EXISTS;
    }
  });
  



  Object.defineProperty(OSError.prototype, "becauseNoSuchFile", {
    get: function becauseNoSuchFile() {
      return this.winLastError == exports.OS.Constants.Win.ERROR_FILE_NOT_FOUND;
    }
  });

  exports.OS.Shared.Win.Error = OSError;
})(this);