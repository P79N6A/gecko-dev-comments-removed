


















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
    throw new Error("osfile_unix_allthreads.jsm must be loaded after osfile_shared_allthreads.jsm");
  }
  if (exports.OS.Shared.Unix) {
    
    return;
  }
  exports.OS.Shared.Unix = {};

  let LOG = OS.Shared.LOG.bind(OS.Shared, "Unix", "allthreads");

  
  let libc;
  let libc_candidates =  [ "libsystem.B.dylib",
                           "libc.so.6",
                           "libc.so" ];
  for (let i = 0; i < libc_candidates.length; ++i) {
    try {
      libc = ctypes.open(libc_candidates[i]);
      break;
    } catch (x) {
      if (exports.OS.Shared.DEBUG) {
        LOG("Could not open libc "+libc_candidates[i]);
      }
    }
  }
  if (!libc) {
    throw new Error("Could not open any libc.");
  }
  exports.OS.Shared.Unix.libc = libc;

  
  let declareFFI = OS.Shared.declareFFI.bind(null, libc);
  exports.OS.Shared.Unix.declareFFI = declareFFI;

  
  let strerror = libc.declare("strerror",
    ctypes.default_abi,
     ctypes.char.ptr,
     ctypes.int);

  






















  let OSError = function OSError(operation, errno) {
    operation = operation || "unknown operation";
    exports.OS.Shared.Error.call(this, operation);
    this.unixErrno = errno || ctypes.errno;
  };
  OSError.prototype = new exports.OS.Shared.Error();
  OSError.prototype.toString = function toString() {
    return "Unix error " + this.unixErrno +
      " during operation " + this.operation +
      " (" + strerror(this.unixErrno).readString() + ")";
  };

  



  Object.defineProperty(OSError.prototype, "becauseExists", {
    get: function becauseExists() {
      return this.unixErrno == OS.Constants.libc.EEXIST;
    }
  });
  



  Object.defineProperty(OSError.prototype, "becauseNoSuchFile", {
    get: function becauseNoSuchFile() {
      return this.unixErrno == OS.Constants.libc.ENOENT;
    }
  });

  



   Object.defineProperty(OSError.prototype, "becauseNotEmpty", {
     get: function becauseNotEmpty() {
       return this.unixErrno == OS.Constants.libc.ENOTEMPTY;
     }
   });
  



  Object.defineProperty(OSError.prototype, "becauseClosed", {
    get: function becauseClosed() {
      return this.unixErrno == OS.Constants.libc.EBADF;
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

  exports.OS.Shared.Unix.Error = OSError;

  

   Object.defineProperty(exports.OS.Shared, "POS_START", { value: exports.OS.Constants.libc.SEEK_SET });
   Object.defineProperty(exports.OS.Shared, "POS_CURRENT", { value: exports.OS.Constants.libc.SEEK_CUR });
   Object.defineProperty(exports.OS.Shared, "POS_END", { value: exports.OS.Constants.libc.SEEK_END });

  
  
  let Types = exports.OS.Shared.Type;

   




  Types.path = Types.cstring.withName("[in] path");
  Types.out_path = Types.out_cstring.withName("[out] path");

  
  OSError.closed = function closed(operation) {
    return new OSError(operation, OS.Constants.libc.EBADF);
  };
})(this);
