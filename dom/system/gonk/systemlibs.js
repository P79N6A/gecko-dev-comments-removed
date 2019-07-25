














const SYSTEM_PROPERTY_KEY_MAX = 32;
const SYSTEM_PROPERTY_VALUE_MAX = 92;




let DEBUG;




let libcutils = (function() {
  let lib;
  try {
    lib = ctypes.open("libcutils.so");
  } catch(ex) {
    
    
    if (DEBUG) {
      dump("Could not load libcutils.so. Using fake propdb.\n");
    }
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
    if (DEBUG) {
      dump("Could not load libnetutils.so!\n");
    }
    
    
    library = {
      declare: function fake_declare() {
        return function fake_libnetutils_function() {};
      }
    };
  }

  let iface = {
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
    ifc_configure: library.declare("ifc_configure", ctypes.default_abi,
                                   ctypes.int,
                                   ctypes.char.ptr,
                                   ctypes.int,
                                   ctypes.int,
                                   ctypes.int,
                                   ctypes.int,
                                   ctypes.int),
    dhcp_stop: library.declare("dhcp_stop", ctypes.default_abi,
                               ctypes.int,
                               ctypes.char.ptr),
    dhcp_release_lease: library.declare("dhcp_release_lease", ctypes.default_abi,
                                        ctypes.int,
                                        ctypes.char.ptr),
    dhcp_get_errmsg: library.declare("dhcp_get_errmsg", ctypes.default_abi,
                                     ctypes.char.ptr),

    
    
    RESET_IPV4_ADDRESSES: 0x01,
    RESET_IPV6_ADDRESSES: 0x02,
  };

  iface.RESET_ALL_ADDRESSES = iface.RESET_IPV4_ADDRESSES |
                              iface.RESET_IPV6_ADDRESSES

  
  

  let sdkVersion = libcutils.property_get("ro.build.version.sdk") || "0";
  sdkVersion = parseInt(sdkVersion, 10);
  if (sdkVersion >= 15) {
    let ipaddrbuf = ctypes.char.array(4096)();
    let gatewaybuf = ctypes.char.array(4096)();
    let prefixLen = ctypes.int();
    let dns1buf = ctypes.char.array(4096)();
    let dns2buf = ctypes.char.array(4096)();
    let serverbuf = ctypes.char.array(4096)();
    let lease = ctypes.int();
    let c_dhcp_do_request =
      library.declare("dhcp_do_request", ctypes.default_abi,
                      ctypes.int,      
                      ctypes.char.ptr, 
                      ctypes.char.ptr, 
                      ctypes.char.ptr, 
                      ctypes.int.ptr,  
                      ctypes.char.ptr, 
                      ctypes.char.ptr, 
                      ctypes.char.ptr, 
                      ctypes.int.ptr); 


    iface.dhcp_do_request = function dhcp_do_request(ifname) {
      let ret = c_dhcp_do_request(ifname,
                                  ipaddrbuf,
                                  gatewaybuf,
                                  prefixLen.address(),
                                  dns1buf,
                                  dns2buf,
                                  serverbuf,
                                  lease.address());

      if (ret && DEBUG) {
        let error = iface.dhcp_get_errmsg();
        dump("dhcp_do_request failed - " + error.readString());
      }
      let obj = {
        ret: ret | 0,
        ipaddr_str: ipaddrbuf.readString(),
        mask: netHelpers.makeMask(prefixLen.value),
        gateway_str: gatewaybuf.readString(),
        dns1_str: dns1buf.readString(),
        dns2_str: dns2buf.readString(),
        server_str: serverbuf.readString(),
        lease: lease.value | 0
      };
      obj.ipaddr = netHelpers.stringToIP(obj.ipaddr_str);
      obj.mask_str = netHelpers.ipToString(obj.mask);
      obj.broadcast_str = netHelpers.ipToString((obj.ipaddr & obj.mask) + ~obj.mask);
      obj.gateway = netHelpers.stringToIP(obj.gateway_str);
      obj.dns1 = netHelpers.stringToIP(obj.dns1_str);
      obj.dns2 = netHelpers.stringToIP(obj.dns2_str);
      obj.server = netHelpers.stringToIP(obj.server_str);
      return obj;
    };
    
    iface.dhcp_do_request_renew = iface.dhcp_do_request;

    
    let c_ifc_reset_connections =
      library.declare("ifc_reset_connections",
                      ctypes.default_abi,
                      ctypes.int,
                      ctypes.char.ptr,
                      ctypes.int);
    iface.ifc_reset_connections = function(ifname, reset_mask) {
      return c_ifc_reset_connections(ifname, reset_mask) | 0;
    }
  } else {
    let ints = ctypes.int.array(8)();
    let c_dhcp_do_request =
      library.declare("dhcp_do_request", ctypes.default_abi,
                      ctypes.int,      
                      ctypes.char.ptr, 
                      ctypes.int.ptr,  
                      ctypes.int.ptr,  
                      ctypes.int.ptr,  
                      ctypes.int.ptr,  
                      ctypes.int.ptr,  
                      ctypes.int.ptr,  
                      ctypes.int.ptr); 
    let c_dhcp_do_request_renew =
      library.declare("dhcp_do_request_renew", ctypes.default_abi,
                      ctypes.int,      
                      ctypes.char.ptr, 
                      ctypes.int.ptr,  
                      ctypes.int.ptr,  
                      ctypes.int.ptr,  
                      ctypes.int.ptr,  
                      ctypes.int.ptr,  
                      ctypes.int.ptr,  
                      ctypes.int.ptr); 

    let wrapCFunc = function wrapCFunc(c_fn) {
      return function (ifname) {
        let ret = c_fn(ifname,
                       ints.addressOfElement(0),
                       ints.addressOfElement(1),
                       ints.addressOfElement(2),
                       ints.addressOfElement(3),
                       ints.addressOfElement(4),
                       ints.addressOfElement(5),
                       ints.addressOfElement(6));
        if (ret && DEBUG) {
          let error = iface.dhcp_get_errmsg();
          dump("dhcp_do_request_* failed - " + error.readString());
        }
        return {ret: ret | 0,
                ipaddr: ints[0] | 0,
                gateway: ints[1] | 0,
                mask: ints[2] | 0,
                dns1: ints[3] | 0,
                dns2: ints[4] | 0,
                server: ints[5] | 0,
                lease: ints[6] | 0};
      };
    };
    iface.dhcp_do_request = wrapCFunc(c_dhcp_do_request);
    iface.dhcp_do_request_renew = wrapCFunc(c_dhcp_do_request_renew);
    let c_ifc_reset_connections =
      library.declare("ifc_reset_connections",
                      ctypes.default_abi,
                      ctypes.int,
                      ctypes.char.ptr);
    iface.ifc_reset_connections = function(ifname, reset_mask) {
      return c_ifc_reset_connections(ifname) | 0;
    }
  }

  return iface;
})();




let netHelpers = {

  


  swap32: function swap32(n) {
    return (((n >> 24) & 0xFF) <<  0) |
           (((n >> 16) & 0xFF) <<  8) |
           (((n >>  8) & 0xFF) << 16) |
           (((n >>  0) & 0xFF) << 24);
  },

  



  ntohl: function ntohl(n) {
    return this.swap32(n);
  },

  



  htonl: function htonl(n) {
    return this.swap32(n);
  },

  






  ipToString: function ipToString(ip) {
    return ((ip >>  0) & 0xFF) + "." +
           ((ip >>  8) & 0xFF) + "." +
           ((ip >> 16) & 0xFF) + "." +
           ((ip >> 24) & 0xFF);
  },

  






  stringToIP: function stringToIP(string) {
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

  


  makeMask: function makeMask(len) {
    let mask = 0;
    for (let i = 0; i < len; ++i) {
      mask |= (0x80000000 >> i);
    }
    return this.ntohl(mask);
  }
};
