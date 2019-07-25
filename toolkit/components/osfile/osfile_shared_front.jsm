










if (typeof Components != "undefined") {
  throw new Error("osfile_shared_front.jsm cannot be used from the main thread");
}
(function(exports) {







let AbstractFile = function AbstractFile(fd) {
  this._fd = fd;
};

AbstractFile.prototype = {
  




  get fd() {
    if (this._fd) {
      return this._fd;
    }
    throw OS.File.Error.closed();
  }
};

















AbstractFile.normalizeOpenMode = function normalizeOpenMode(mode) {
  let result = {
    read: false,
    write: false,
    trunc: false,
    create: false,
    existing: false
  };
  for (let key in mode) {
    if (!mode[key]) continue; 
    switch (key) {
    case "read":
      result.read = true;
      break;
    case "write":
      result.write = true;
      break;
    case "truncate": 
    case "trunc":
      result.trunc = true;
      result.write = true;
      break;
    case "create":
      result.create = true;
      result.write = true;
      break;
    case "existing": 
    case "exist":
      result.existing = true;
      break;
    default:
      throw new TypeError("Mode " + key + " not understood");
    }
  }
  
  if (result.existing && result.create) {
    throw new TypeError("Cannot specify both existing:true and create:true");
  }
  if (result.trunc && result.create) {
    throw new TypeError("Cannot specify both trunc:true and create:true");
  }
  
  if (!result.write) {
    result.read = true;
  }
  return result;
};

   exports.OS.Shared.AbstractFile = AbstractFile;
})(this);