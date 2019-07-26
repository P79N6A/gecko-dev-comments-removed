


















if (typeof Components != "undefined") {
  
  this.EXPORTED_SYMBOLS = ["OS"];
  Components.utils.import("resource://gre/modules/ctypes.jsm");
  Components.utils.import("resource://gre/modules/osfile/osfile_shared_allthreads.jsm", this);
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

  
  let libc;
  try {
    libc = ctypes.open("kernel32.dll");
  } catch (ex) {
    
    
    throw new Error("Could not open system library: " + ex.message);
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
      + this.operation + " (" + buf.readString() + ")";
  };

  



  Object.defineProperty(OSError.prototype, "becauseExists", {
    get: function becauseExists() {
      return this.winLastError == exports.OS.Constants.Win.ERROR_FILE_EXISTS ||
        this.winLastError == exports.OS.Constants.Win.ERROR_ALREADY_EXISTS;
    }
  });
  



  Object.defineProperty(OSError.prototype, "becauseNoSuchFile", {
    get: function becauseNoSuchFile() {
      return this.winLastError == exports.OS.Constants.Win.ERROR_FILE_NOT_FOUND;
    }
  });
  



  Object.defineProperty(OSError.prototype, "becauseNotEmpty", {
    get: function becauseNotEmpty() {
      return this.winLastError == OS.Constants.Win.ERROR_DIR_NOT_EMPTY;
    }
  });
  



  Object.defineProperty(OSError.prototype, "becauseClosed", {
    get: function becauseClosed() {
      return this.winLastError == exports.OS.Constants.Win.INVALID_HANDLE_VALUE;
    }
  });

  



  OSError.toMsg = function toMsg(error) {
    return {
      operation: error.operation,
     winLastError: error.winLastError
    };
  };

  


  OSError.fromMsg = function fromMsg(msg) {
    return new OSError(msg.operation, msg.winLastError);
  };

  exports.OS.Shared.Win.Error = OSError;

  




  let AbstractInfo = function AbstractInfo(isDir, isSymLink, size, winBirthDate,
                                           lastAccessDate) {
    this._isDir = isDir;
    this._isSymLink = isSymLink;
    this._size = size;
    this._winBirthDate = winBirthDate;
    this._lastAccessDate = lastAccessDate;
    this._lastModificationDate = lastAccessDate;
  };

  AbstractInfo.prototype = {
    


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
  exports.OS.Shared.Win.AbstractInfo = AbstractInfo;

  




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
  exports.OS.Shared.Win.AbstractEntry = AbstractEntry;

  

  Object.defineProperty(exports.OS.Shared, "POS_START", { value: exports.OS.Constants.Win.FILE_BEGIN });
  Object.defineProperty(exports.OS.Shared, "POS_CURRENT", { value: exports.OS.Constants.Win.FILE_CURRENT });
  Object.defineProperty(exports.OS.Shared, "POS_END", { value: exports.OS.Constants.Win.FILE_END });

  
  
  let Types = exports.OS.Shared.Type;

  




  Types.path = Types.wstring.withName("[in] path");
  Types.out_path = Types.out_wstring.withName("[out] path");

  
  OSError.closed = function closed(operation) {
    return new OSError(operation, exports.OS.Constants.Win.INVALID_HANDLE_VALUE);
  };

  OSError.exists = function exists(operation) {
    return new OSError(operation, exports.OS.Constants.Win.ERROR_FILE_EXISTS);
  };

  OSError.noSuchFile = function noSuchFile(operation) {
    return new OSError(operation, exports.OS.Constants.Win.ERROR_FILE_NOT_FOUND);
  };
})(this);
