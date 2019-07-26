










if (typeof Components != "undefined") {
  throw new Error("osfile_shared_front.jsm cannot be used from the main thread");
}
(function(exports) {

let LOG = exports.OS.Shared.LOG.bind(OS.Shared, "Shared front-end");

const noOptions = {};







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
  









  read: function read(bytes) {
    if (bytes == null) {
      bytes = this.stat().size;
    }
    let buffer = new ArrayBuffer(bytes);
    let size = this.readTo(buffer, bytes);
    return {
      buffer: buffer,
      bytes: size
    };
  },

  




















  readTo: function readTo(buffer, options) {
    options = options || noOptions;

    let {ptr, bytes} = AbstractFile.normalizeToPointer(buffer, options.bytes,
      options.offset);
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

  



















  write: function write(buffer, options) {
    options = options || noOptions;

    let {ptr, bytes} = AbstractFile.normalizeToPointer(buffer, options.bytes,
      options.offset);

    let pos = 0;
    while (pos < bytes) {
      let chunkSize = this._write(ptr, bytes - pos, options);
      pos += chunkSize;
      ptr = exports.OS.Shared.offsetBy(ptr, chunkSize);
    }
    return pos;
  }
};

















AbstractFile.normalizeToPointer = function normalizeToPointer(candidate, bytes, offset) {
  if (!candidate) {
    throw new TypeError("Expecting a C pointer or an ArrayBuffer");
  }
  if (offset == null) {
    offset = 0;
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
  } else if ("byteLength" in candidate) {
    ptr = exports.OS.Shared.Type.uint8_t.out_ptr.implementation(candidate);
    if (bytes == null) {
      bytes = candidate.byteLength - offset;
    }
    if (candidate.byteLength < offset + bytes) {
      throw new TypeError("Buffer is too short. I need at least " +
                         (offset + bytes) +
                         " bytes but I have only " +
                         buffer.byteLength +
                          "bytes");
    }
  } else {
    throw new TypeError("Expecting a C pointer or an ArrayBuffer");
  }
  if (offset != 0) {
    ptr = exports.OS.Shared.offsetBy(ptr, offset);
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