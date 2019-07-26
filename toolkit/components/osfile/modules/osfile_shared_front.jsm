










if (typeof Components != "undefined") {
  throw new Error("osfile_shared_front.jsm cannot be used from the main thread");
}
(function(exports) {

exports.OS = require("resource://gre/modules/osfile/osfile_shared_allthreads.jsm").OS;

let LOG = exports.OS.Shared.LOG.bind(OS.Shared, "Shared front-end");
let clone = exports.OS.Shared.clone;







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
    let {ptr, bytes} = AbstractFile.normalizeToPointer(buffer, options.bytes);
    let pos = 0;
    while (pos < bytes) {
      let chunkSize = this._read(ptr, bytes - pos, options);
      if (chunkSize == 0) {
        break;
      }
      pos += chunkSize;
      ptr = exports.OS.Shared.offsetBy(ptr, chunkSize);
    }

    return pos;
  },

  
















  write: function write(buffer, options = {}) {

    let {ptr, bytes} =
      AbstractFile.normalizeToPointer(buffer, options.bytes || undefined);

    let pos = 0;
    while (pos < bytes) {
      let chunkSize = this._write(ptr, bytes - pos, options);
      pos += chunkSize;
      ptr = exports.OS.Shared.offsetBy(ptr, chunkSize);
    }
    return pos;
  }
};















AbstractFile.normalizeToPointer = function normalizeToPointer(candidate, bytes) {
  if (!candidate) {
    throw new TypeError("Expecting  a Typed Array or a C pointer");
  }
  let ptr;
  if ("isNull" in candidate) {
    if (candidate.isNull()) {
      throw new TypeError("Expecting a non-null pointer");
    }
    ptr = exports.OS.Shared.Type.uint8_t.out_ptr.cast(candidate);
    if (bytes == null) {
      throw new TypeError("C pointer missing bytes indication.");
    }
  } else if (exports.OS.Shared.isTypedArray(candidate)) {
    
    ptr = exports.OS.Shared.Type.uint8_t.out_ptr.implementation(candidate.buffer);
    if (bytes == null) {
      bytes = candidate.byteLength;
    } else if (candidate.byteLength < bytes) {
      throw new TypeError("Buffer is too short. I need at least " +
                         bytes +
                         " bytes but I have only " +
                         candidate.byteLength +
                          "bytes");
    }
  } else {
    throw new TypeError("Expecting  a Typed Array or a C pointer");
  }
  return {ptr: ptr, bytes: bytes};
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

   exports.OS.Shared.AbstractFile = AbstractFile;
})(this);
