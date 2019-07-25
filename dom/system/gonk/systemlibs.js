



const SYSTEM_PROPERTY_KEY_MAX = 32;
const SYSTEM_PROPERTY_VALUE_MAX = 92;




let libcutils = (function() {
  let lib;
  try {
    lib = ctypes.open("libcutils.so");
  } catch(ex) {
    
    
    dump("Could not load libcutils.so. Using fake propdb.\n");
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





let libnetutils = (function () {
  let library;
  try {
    library = ctypes.open("libnetutils.so");
  } catch(ex) {
    dump("Could not load libnetutils.so!\n");
    
    
    library = {
      declare: function fake_declare() {
        return function fake_libnetutils_function() {};
      }
    };
  }

  return {
    ifc_enable: library.declare("ifc_enable", ctypes.default_abi,
                                ctypes.int,
                                ctypes.char.ptr),
    ifc_disable: library.declare("ifc_disable", ctypes.default_abi,
                                 ctypes.int,
                                 ctypes.char.ptr),
    ifc_add_host_route: library.declare("ifc_add_host_route",
                                        ctypes.default_abi,
                                        ctypes.int,
                                        ctypes.char.ptr,
                                        ctypes.int),
    ifc_remove_host_routes: library.declare("ifc_remove_host_routes",
                                            ctypes.default_abi,
                                            ctypes.int,
                                            ctypes.char.ptr),
    ifc_set_default_route: library.declare("ifc_set_default_route",
                                           ctypes.default_abi,
                                           ctypes.int,
                                           ctypes.char.ptr,
                                           ctypes.int),
    ifc_get_default_route: library.declare("ifc_get_default_route",
                                           ctypes.default_abi,
                                           ctypes.int,
                                           ctypes.char.ptr),
    ifc_remove_default_route: library.declare("ifc_remove_default_route",
                                              ctypes.default_abi,
                                              ctypes.int,
                                              ctypes.char.ptr),
    ifc_reset_connections: library.declare("ifc_reset_connections",
                                           ctypes.default_abi,
                                           ctypes.int,
                                           ctypes.char.ptr),
    ifc_configure: library.declare("ifc_configure", ctypes.default_abi,
                                   ctypes.int,
                                   ctypes.char.ptr,
                                   ctypes.int,
                                   ctypes.int,
                                   ctypes.int,
                                   ctypes.int,
                                   ctypes.int),
    dhcp_do_request: library.declare("dhcp_do_request", ctypes.default_abi,
                                     ctypes.int,
                                     ctypes.char.ptr,
                                     ctypes.int.ptr,
                                     ctypes.int.ptr,
                                     ctypes.int.ptr,
                                     ctypes.int.ptr,
                                     ctypes.int.ptr,
                                     ctypes.int.ptr,
                                     ctypes.int.ptr),
    dhcp_stop: library.declare("dhcp_stop", ctypes.default_abi,
                               ctypes.int,
                               ctypes.char.ptr),
    dhcp_release_lease: library.declare("dhcp_release_lease", ctypes.default_abi,
                                        ctypes.int,
                                        ctypes.char.ptr),
    dhcp_get_errmsg: library.declare("dhcp_get_errmsg", ctypes.default_abi,
                                     ctypes.char.ptr),
    dhcp_do_request_renew: library.declare("dhcp_do_request_renew",
                                           ctypes.default_abi,
                                           ctypes.int,
                                           ctypes.char.ptr,
                                           ctypes.int.ptr,
                                           ctypes.int.ptr,
                                           ctypes.int.ptr,
                                           ctypes.int.ptr,
                                           ctypes.int.ptr,
                                           ctypes.int.ptr,
                                           ctypes.int.ptr)
  };
})();
