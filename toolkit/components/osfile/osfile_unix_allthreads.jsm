


















if (typeof Components != "undefined") {
  
  this.EXPORTED_SYMBOLS = ["OS"];
  Components.utils.import("resource://gre/modules/ctypes.jsm");
  Components.utils.import("resource://gre/modules/osfile/osfile_shared_allthreads.jsm", this);
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
  exports.OS.Shared.Unix.AbstractInfo = AbstractInfo;

  




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
  exports.OS.Shared.Unix.AbstractEntry = AbstractEntry;

  

   Object.defineProperty(exports.OS.Shared, "POS_START", { value: exports.OS.Constants.libc.SEEK_SET });
   Object.defineProperty(exports.OS.Shared, "POS_CURRENT", { value: exports.OS.Constants.libc.SEEK_CUR });
   Object.defineProperty(exports.OS.Shared, "POS_END", { value: exports.OS.Constants.libc.SEEK_END });

  
  
  let Types = exports.OS.Shared.Type;

   




  Types.path = Types.cstring.withName("[in] path");
  Types.out_path = Types.out_cstring.withName("[out] path");

  
  OSError.closed = function closed(operation) {
    return new OSError(operation, OS.Constants.libc.EBADF);
  };

  OSError.exists = function exists(operation) {
    return new OSError(operation, OS.Constants.libc.EEXIST);
  };
})(this);
