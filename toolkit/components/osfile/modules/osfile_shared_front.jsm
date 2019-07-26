










if (typeof Components != "undefined") {
  throw new Error("osfile_shared_front.jsm cannot be used from the main thread");
}
(function(exports) {

let SharedAll =
  require("resource://gre/modules/osfile/osfile_shared_allthreads.jsm");

let LOG = SharedAll.LOG.bind(SharedAll, "Shared front-end");
let clone = SharedAll.clone;







let AbstractFile = function AbstractFile(fd) {
  this._fd = fd;
};

AbstractFile.prototype = {
  




  get fd() {
    if (this._fd) {
      return this._fd;
    }
    throw OS.File.Error.closed();
  },
  








  read: function read(bytes, options = {}) {
    options = clone(options);
    options.bytes = bytes == null ? this.stat().size : bytes;
    let buffer = new Uint8Array(options.bytes);
    let size = this.readTo(buffer, options);
    if (size == options.bytes) {
      return buffer;
    } else {
      return buffer.subarray(0, size);
    }
  },

  

















  readTo: function readTo(buffer, options = {}) {
    let {ptr, bytes} = SharedAll.normalizeToPointer(buffer, options.bytes);
    let pos = 0;
    while (pos < bytes) {
      let chunkSize = this._read(ptr, bytes - pos, options);
      if (chunkSize == 0) {
        break;
      }
      pos += chunkSize;
      ptr = SharedAll.offsetBy(ptr, chunkSize);
    }

    return pos;
  },

  
















  write: function write(buffer, options = {}) {

    let {ptr, bytes} =
      SharedAll.normalizeToPointer(buffer, options.bytes || undefined);

    let pos = 0;
    while (pos < bytes) {
      let chunkSize = this._write(ptr, bytes - pos, options);
      pos += chunkSize;
      ptr = SharedAll.offsetBy(ptr, chunkSize);
    }
    return pos;
  }
};
















AbstractFile.openUnique = function openUnique(path, options = {}) {
  let mode = {
    create : true
  };

  let dirName = OS.Path.dirname(path);
  let leafName = OS.Path.basename(path);
  let lastDotCharacter = leafName.lastIndexOf('.');
  let fileName = leafName.substring(0, lastDotCharacter != -1 ? lastDotCharacter : leafName.length);
  let suffix = (lastDotCharacter != -1 ? leafName.substring(lastDotCharacter) : "");
  let uniquePath = "";
  let maxAttempts = options.maxAttempts || 99;
  let humanReadable = !!options.humanReadable;
  const HEX_RADIX = 16;
  
  const MAX_HEX_NUMBER = 16777215;

  try {
    return {
      path: path,
      file: OS.File.open(path, mode)
    };
  } catch (ex if ex instanceof OS.File.Error && ex.becauseExists) {
    for (let i = 0; i < maxAttempts; ++i) {
      try {
        if (humanReadable) {
          uniquePath = OS.Path.join(dirName, fileName + "-" + (i + 1) + suffix);
        } else {
          let hexNumber = Math.floor(Math.random() * MAX_HEX_NUMBER).toString(HEX_RADIX);
          uniquePath = OS.Path.join(dirName, fileName + "-" + hexNumber + suffix);
        }
        return {
          path: uniquePath,
          file: OS.File.open(uniquePath, mode)
        };
      } catch (ex if ex instanceof OS.File.Error && ex.becauseExists) {
        
      }
    }
    throw OS.File.Error.exists("could not find an unused file name.");
  }
};




AbstractFile.AbstractIterator = function AbstractIterator() {
};
AbstractFile.AbstractIterator.prototype = {
  


  __iterator__: function __iterator__() {
    return this;
  },
  









  forEach: function forEach(cb) {
    let index = 0;
    for (let entry in this) {
      cb(entry, index++, this);
    }
  },
  










  nextBatch: function nextBatch(length) {
    let array = [];
    let i = 0;
    for (let entry in this) {
      array.push(entry);
      if (++i >= length) {
        return array;
      }
    }
    return array;
  }
};
















AbstractFile.normalizeOpenMode = function normalizeOpenMode(mode) {
  let result = {
    read: false,
    write: false,
    trunc: false,
    create: false,
    existing: false,
    append: true
  };
  for (let key in mode) {
    let val = !!mode[key]; 
    switch (key) {
    case "read":
      result.read = val;
      break;
    case "write":
      result.write = val;
      break;
    case "truncate": 
    case "trunc":
      result.trunc = val;
      result.write |= val;
      break;
    case "create":
      result.create = val;
      result.write |= val;
      break;
    case "existing": 
    case "exist":
      result.existing = val;
      break;
    case "append":
      result.append = val;
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












AbstractFile.read = function read(path, bytes, options = {}) {
  let file = exports.OS.File.open(path);
  try {
    return file.read(bytes, options);
  } finally {
    file.close();
  }
};







































AbstractFile.writeAtomic =
     function writeAtomic(path, buffer, options = {}) {

  
  if (typeof path != "string" || path == "") {
    throw new TypeError("File path should be a (non-empty) string");
  }
  let noOverwrite = options.noOverwrite;
  if (noOverwrite && OS.File.exists(path)) {
    throw OS.File.Error.exists("writeAtomic");
  }

  if (typeof buffer == "string") {
    
    let encoding = options.encoding || "utf-8";
    buffer = new TextEncoder(encoding).encode(buffer);
  }

  let bytesWritten = 0;

  if (!options.tmpPath) {
    
    let dest = OS.File.open(path, {write: true, truncate: true});
    try {
      bytesWritten = dest.write(buffer, options);
      if (options.flush) {
        dest.flush();
      }
    } finally {
      dest.close();
    }
    return bytesWritten;
  }

  let tmpFile = OS.File.open(options.tmpPath, {write: true, truncate: true});
  try {
    bytesWritten = tmpFile.write(buffer, options);
    if (options.flush) {
      tmpFile.flush();
    }
  } catch (x) {
    OS.File.remove(options.tmpPath);
    throw x;
  } finally {
    tmpFile.close();
  }

  OS.File.move(options.tmpPath, path, {noCopy: true});
  return bytesWritten;
};














AbstractFile.removeDir = function(path, options = {}) {
  let iterator = new OS.File.DirectoryIterator(path);
  if (!iterator.exists() && options.ignoreAbsent) {
    return;
  }

  try {
    for (let entry in iterator) {
      if (entry.isDir) {
        OS.File.removeDir(entry.path, options);
      } else {
        OS.File.remove(entry.path, options);
      }
    }
  } finally {
    iterator.close();
  }

  OS.File.removeEmptyDir(path);
};

if (!exports.OS.Shared) {
  exports.OS.Shared = {};
}
exports.OS.Shared.AbstractFile = AbstractFile;
})(this);
