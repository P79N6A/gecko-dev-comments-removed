





'use strict';

this.EXPORTED_SYMBOLS = ['LogCapture'];









let readLogFile = function(logLocation) {
  if (!this.ctypes) {
    
    Components.utils.import('resource://gre/modules/ctypes.jsm', this);

    this.lib = this.ctypes.open(this.ctypes.libraryName('c'));

    this.read = this.lib.declare('read',
      this.ctypes.default_abi,
      this.ctypes.int,       
      this.ctypes.int,       
      this.ctypes.voidptr_t, 
      this.ctypes.size_t     
    );

    this.open = this.lib.declare('open',
      this.ctypes.default_abi,
      this.ctypes.int,      
      this.ctypes.char.ptr, 
      this.ctypes.int       
    );

    this.close = this.lib.declare('close',
      this.ctypes.default_abi,
      this.ctypes.int, 
      this.ctypes.int  
    );
  }

  const O_READONLY = 0;
  const O_NONBLOCK = 1 << 11;

  const BUF_SIZE = 2048;

  let BufType = this.ctypes.ArrayType(this.ctypes.char);
  let buf = new BufType(BUF_SIZE);
  let logArray = [];

  let logFd = this.open(logLocation, O_READONLY | O_NONBLOCK);
  if (logFd === -1) {
    return null;
  }

  let readStart = Date.now();
  let readCount = 0;
  while (true) {
    let count = this.read(logFd, buf, BUF_SIZE);
    readCount += 1;

    if (count <= 0) {
      
      break;
    }
    for(let i = 0; i < count; i++) {
      logArray.push(buf[i]);
    }
  }

  let logTypedArray = new Uint8Array(logArray);

  this.close(logFd);

  return logTypedArray;
};

let cleanup = function() {
  this.lib.close();
  this.read = this.open = this.close = null;
  this.lib = null;
  this.ctypes = null;
};

this.LogCapture = { readLogFile: readLogFile, cleanup: cleanup };
