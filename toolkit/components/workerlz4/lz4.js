



"use strict";

if (typeof Components != "undefined") {
  throw new Error("This file is meant to be loaded in a worker");
}
if (!module || !exports) {
  throw new Error("Please load this module with require()");
}

const SharedAll = require("resource://gre/modules/osfile/osfile_shared_allthreads.jsm");
const Internals = require("resource://gre/modules/workers/lz4_internal.js");

const MAGIC_NUMBER = new Uint8Array([109, 111, 122, 76, 122, 52, 48, 0]); 

const BYTES_IN_SIZE_HEADER = ctypes.uint32_t.size;

const HEADER_SIZE = MAGIC_NUMBER.byteLength + BYTES_IN_SIZE_HEADER;

const EXPECTED_HEADER_TYPE = new ctypes.ArrayType(ctypes.uint8_t, HEADER_SIZE);
const EXPECTED_SIZE_BUFFER_TYPE = new ctypes.ArrayType(ctypes.uint8_t, BYTES_IN_SIZE_HEADER);









function LZError(operation, reason, message) {
  SharedAll.OSError.call(this);
  this.operation = operation;
  this[reason] = true;
  this.message = message;
}
LZError.prototype = Object.create(SharedAll.OSError);
LZError.prototype.toString = function toString() {
  return this.message;
};
exports.Error = LZError;
























function compressFileContent(array, options = {}) {
  
  let inputBytes;
  if (SharedAll.isTypedArray(array) && !(options && "bytes" in options)) {
    inputBytes = array.byteLength;
  } else if (options && options.bytes) {
    inputBytes = options.bytes;
  } else {
    throw new TypeError("compressFileContent requires a size");
  }
  let maxCompressedSize = Internals.maxCompressedSize(inputBytes);
  let outputArray = new Uint8Array(HEADER_SIZE + maxCompressedSize);

  
  let payload = new Uint8Array(outputArray.buffer, outputArray.byteOffset + HEADER_SIZE);
  let compressedSize = Internals.compress(array, inputBytes, payload);

  
  outputArray.set(MAGIC_NUMBER);
  let view = new DataView(outputArray.buffer);
  view.setUint32(MAGIC_NUMBER.byteLength, inputBytes, true);

  return new Uint8Array(outputArray.buffer, 0, HEADER_SIZE + compressedSize);
}
exports.compressFileContent = compressFileContent;

function decompressFileContent(array, options = {}) {
  let {ptr, bytes} = SharedAll.normalizeToPointer(array, options.bytes);
  if (bytes < HEADER_SIZE) {
    throw new LZError("decompress", "becauseLZNoHeader", "Buffer is too short (no header)");
  }

  
  let expectMagicNumber = ctypes.cast(ptr, EXPECTED_HEADER_TYPE.ptr).contents;
  for (let i = 0; i < MAGIC_NUMBER.byteLength; ++i) {
    if (expectMagicNumber[i] != MAGIC_NUMBER[i]) {
      throw new LZError("decompress", "becauseLZWrongMagicNumber", "Invalid header (no magic number");
    }
  }

  let sizeBuf =
    ctypes.cast(
      SharedAll.offsetBy(ptr, MAGIC_NUMBER.byteLength),
      EXPECTED_SIZE_BUFFER_TYPE.ptr).contents;
  let expectDecompressedSize =
    sizeBuf[0] + (sizeBuf[1] << 8) + (sizeBuf[2] << 16) + (sizeBuf[3] << 24);
  if (expectDecompressedSize == 0) {
    
    return new Uint8Array(0);
  }

  
  let inputPtr = SharedAll.offsetBy(ptr, HEADER_SIZE);

  
  let outputBuffer = new Uint8Array(expectDecompressedSize);
  let decompressedBytes = (new SharedAll.Type.size_t.implementation(0));

  
  let success = Internals.decompress(inputPtr, bytes - HEADER_SIZE,
                                     outputBuffer, outputBuffer.byteLength,
                                     decompressedBytes.address());
  if (!success) {
    throw new LZError("decompress", "becauseLZInvalidContent", "Invalid content:Decompression stopped at " + decompressedBytes.value);
  }
  return new Uint8Array(outputBuffer.buffer, outputBuffer.byteOffset, decompressedBytes.value);
}
exports.decompressFileContent = decompressFileContent;
