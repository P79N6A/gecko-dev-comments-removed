



const SYSTEM_PROPERTY_KEY_MAX = 32;
const SYSTEM_PROPERTY_VALUE_MAX = 92;




let libcutils = (function() {
  let lib;
  try {
    lib = ctypes.open("libcutils.so");
  } catch(ex) {
    
    
    dump("Could not load libcutils.so. Using fake propdb.");
    let fake_propdb = Object.create(null);
    return {
      property_get: function fake_property_get(key, defaultValue) {
        if (key in fake_propdb) {
          return fake_propdb[key];
        }
        return defaultValue === undefined ? null : defaultValue;
      },
      property_set: function fake_property_set(key, value) {
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

    







    property_get: function property_get(key, defaultValue) {
      if (defaultValue === undefined) {
        defaultValue = null;
      }
      c_property_get(key, c_value_buf, defaultValue);
      return c_value_buf.readString();
    },

    







    property_set: function property_set(key, value) {
      let rv = c_property_set(key, value);
      if (rv) {
        throw Error('libcutils.property_set("' + key + '", "' + value +
                    '") failed with error ' + rv);
      }
    }

  };
})();
