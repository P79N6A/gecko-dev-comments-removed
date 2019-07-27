














if (!this.ctypes) {
  
  this.EXPORTED_SYMBOLS = [ "libcutils", "netHelpers" ];
  Components.utils.import("resource://gre/modules/ctypes.jsm");
}

const SYSTEM_PROPERTY_KEY_MAX = 32;
const SYSTEM_PROPERTY_VALUE_MAX = 92;




let DEBUG;




this.libcutils = (function() {
  let lib;
  try {
    lib = ctypes.open("libcutils.so");
  } catch(ex) {
    
    
    if (DEBUG) {
      dump("Could not load libcutils.so. Using fake propdb.\n");
    }
    let fake_propdb = Object.create(null);
    return {
      property_get: function(key, defaultValue) {
        if (key in fake_propdb) {
          return fake_propdb[key];
        }
        return defaultValue === undefined ? null : defaultValue;
      },
      property_set: function(key, value) {
        fake_propdb[key] = value;
      }
    };
  }

  let c_property_get = lib.declare("property_get", ctypes.default_abi,
                                   ctypes.int,       
                                   ctypes.char.ptr,  
                                   ctypes.char.ptr,  
                                   ctypes.char.ptr); 
  let c_property_set = lib.declare("property_set", ctypes.default_abi,
                                   ctypes.int,       
                                   ctypes.char.ptr,  
                                   ctypes.char.ptr); 
  let c_value_buf = ctypes.char.array(SYSTEM_PROPERTY_VALUE_MAX)();

  return {

    







    property_get: function(key, defaultValue) {
      if (defaultValue === undefined) {
        defaultValue = null;
      }
      c_property_get(key, c_value_buf, defaultValue);
      return c_value_buf.readString();
    },

    







    property_set: function(key, value) {
      let rv = c_property_set(key, value);
      if (rv) {
        throw Error('libcutils.property_set("' + key + '", "' + value +
                    '") failed with error ' + rv);
      }
    }

  };
})();




this.netHelpers = {

  


  swap32: function(n) {
    return (((n >> 24) & 0xFF) <<  0) |
           (((n >> 16) & 0xFF) <<  8) |
           (((n >>  8) & 0xFF) << 16) |
           (((n >>  0) & 0xFF) << 24);
  },

  



  ntohl: function(n) {
    return this.swap32(n);
  },

  



  htonl: function(n) {
    return this.swap32(n);
  },

  






  ipToString: function(ip) {
    return ((ip >>  0) & 0xFF) + "." +
           ((ip >>  8) & 0xFF) + "." +
           ((ip >> 16) & 0xFF) + "." +
           ((ip >> 24) & 0xFF);
  },

  






  stringToIP: function(string) {
    if (!string) {
      return null;
    }
    let ip = 0;
    let start, end = -1;
    for (let i = 0; i < 4; i++) {
      start = end + 1;
      end = string.indexOf(".", start);
      if (end == -1) {
        end = string.length;
      }
      let num = parseInt(string.slice(start, end), 10);
      if (isNaN(num)) {
        return null;
      }
      ip |= num << (i * 8);
    }
    return ip;
  },

  


  makeMask: function(len) {
    let mask = 0;
    for (let i = 0; i < len; ++i) {
      mask |= (0x80000000 >> i);
    }
    return this.ntohl(mask);
  },

  


  getMaskLength: function(mask) {
    let len = 0;
    let netmask = this.ntohl(mask);
    while (netmask & 0x80000000) {
        len++;
        netmask = netmask << 1;
    }
    return len;
  }
};
