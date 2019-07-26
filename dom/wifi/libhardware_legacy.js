






"use strict";

let libhardware_legacy = (function () {
  let library = ctypes.open("libhardware_legacy.so");
  let sdkVersion = libcutils.property_get("ro.build.version.sdk", "0");
  sdkVersion = parseInt(sdkVersion, 10);

  let iface = {
    
    load_driver: library.declare("wifi_load_driver", ctypes.default_abi, ctypes.int),

    
    unload_driver: library.declare("wifi_unload_driver", ctypes.default_abi, ctypes.int),

    
    start_supplicant: library.declare("wifi_start_supplicant", ctypes.default_abi, ctypes.int),

    
    stop_supplicant: library.declare("wifi_stop_supplicant", ctypes.default_abi, ctypes.int),

    
    connect_to_supplicant: library.declare("wifi_connect_to_supplicant", ctypes.default_abi, ctypes.int),

    
    close_supplicant_connection: library.declare("wifi_close_supplicant_connection", ctypes.default_abi, ctypes.void_t),

    
    
    wait_for_event: library.declare("wifi_wait_for_event", ctypes.default_abi, ctypes.int, ctypes.char.ptr, ctypes.size_t),

    
    
    command: library.declare("wifi_command", ctypes.default_abi, ctypes.int, ctypes.char.ptr, ctypes.char.ptr, ctypes.size_t.ptr),
  };

  if (sdkVersion >= 16) {
    let c_start_supplicant =
      library.declare("wifi_start_supplicant",
                      ctypes.default_abi,
                      ctypes.int,
                      ctypes.int);
    iface.start_supplicant = function () {
      return c_start_supplicant(0);
    };

    let c_connect_to_supplicant =
      library.declare("wifi_connect_to_supplicant",
                      ctypes.default_abi,
                      ctypes.int,
                      ctypes.char.ptr);
    iface.connect_to_supplicant = function () {
      return c_connect_to_supplicant("wlan0");
    };

    let c_close_supplicant_connection =
      library.declare("wifi_close_supplicant_connection",
                      ctypes.default_abi,
                      ctypes.void_t,
                      ctypes.char.ptr);
    iface.close_supplicant_connection = function () {
      c_close_supplicant_connection("wlan0");
    };

    let c_wait_for_event =
      library.declare("wifi_wait_for_event",
                      ctypes.default_abi,
                      ctypes.int,
                      ctypes.char.ptr,
                      ctypes.char.ptr,
                      ctypes.size_t);
    iface.wait_for_event = function (cbuf, len) {
      return c_wait_for_event("wlan0", cbuf, len);
    };

    let c_command =
      library.declare("wifi_command",
                      ctypes.default_abi,
                      ctypes.int,
                      ctypes.char.ptr,
                      ctypes.char.ptr,
                      ctypes.char.ptr,
                      ctypes.size_t.ptr);
    iface.command = function (message, cbuf, len) {
      return c_command("wlan0", message, cbuf, len);
    };
  }

  return iface;
})();
