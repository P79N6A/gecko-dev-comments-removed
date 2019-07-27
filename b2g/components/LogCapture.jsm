





'use strict';

this.EXPORTED_SYMBOLS = ['LogCapture'];

const SYSTEM_PROPERTY_KEY_MAX = 32;
const SYSTEM_PROPERTY_VALUE_MAX = 92;

function debug(msg) {
  dump('LogCapture.jsm: ' + msg + '\n');
}

let LogCapture = {
  ensureLoaded: function() {
    if (!this.ctypes) {
      this.load();
    }
  },

  load: function() {
    
    Components.utils.import('resource://gre/modules/ctypes.jsm', this);

    this.libc = this.ctypes.open(this.ctypes.libraryName('c'));

    this.read = this.libc.declare('read',
      this.ctypes.default_abi,
      this.ctypes.int,       
      this.ctypes.int,       
      this.ctypes.voidptr_t, 
      this.ctypes.size_t     
    );

    this.open = this.libc.declare('open',
      this.ctypes.default_abi,
      this.ctypes.int,      
      this.ctypes.char.ptr, 
      this.ctypes.int       
    );

    this.close = this.libc.declare('close',
      this.ctypes.default_abi,
      this.ctypes.int, 
      this.ctypes.int  
    );

    this.property_find_nth =
      this.libc.declare("__system_property_find_nth",
                        this.ctypes.default_abi,
                        this.ctypes.voidptr_t,     
                        this.ctypes.unsigned_int); 

    this.property_read =
      this.libc.declare("__system_property_read",
                        this.ctypes.default_abi,
                        this.ctypes.void_t,     
                        this.ctypes.voidptr_t,  
                        this.ctypes.char.ptr,   
                        this.ctypes.char.ptr);  

    this.key_buf   = this.ctypes.char.array(SYSTEM_PROPERTY_KEY_MAX)();
    this.value_buf = this.ctypes.char.array(SYSTEM_PROPERTY_VALUE_MAX)();
  },

  cleanup: function() {
    this.libc.close();

    this.read = null;
    this.open = null;
    this.close = null;
    this.property_find_nth = null;
    this.property_read = null;
    this.key_buf = null;
    this.value_buf = null;

    this.libc = null;
    this.ctypes = null;
  },

  







  readLogFile: function(logLocation) {
    this.ensureLoaded();

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
  },

 


  readProperties: function() {
    this.ensureLoaded();
    let n = 0;
    let propertyDict = {};

    while(true) {
      let prop_info = this.property_find_nth(n);
      if(prop_info.isNull()) {
        break;
      }

      
      this.property_read(prop_info, this.key_buf, this.value_buf);
      let key = this.key_buf.readString();;
      let value = this.value_buf.readString()

      propertyDict[key] = value;
      n++;
    }

    return propertyDict;
  }
};

this.LogCapture = LogCapture;
