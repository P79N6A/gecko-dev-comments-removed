


"use strict";

let libhardware_legacy = (function () {
  let library = ctypes.open("/system/lib/libhardware_legacy.so");

  return {
    
    load_driver: library.declare("wifi_load_driver", ctypes.default_abi, ctypes.int),

    
    unload_driver: library.declare("wifi_unload_driver", ctypes.default_abi, ctypes.int),

    
    start_supplicant: library.declare("wifi_start_supplicant", ctypes.default_abi, ctypes.int),

    
    stop_supplicant: library.declare("wifi_stop_supplicant", ctypes.default_abi, ctypes.int),

    
    connect_to_supplicant: library.declare("wifi_connect_to_supplicant", ctypes.default_abi, ctypes.int),

    
    close_supplicant_connection: library.declare("wifi_close_supplicant_connection", ctypes.default_abi, ctypes.int),

    
    
    wait_for_event: library.declare("wifi_wait_for_event", ctypes.default_abi, ctypes.int, ctypes.char.ptr, ctypes.size_t),

    
    
    command: library.declare("wifi_command", ctypes.default_abi, ctypes.int, ctypes.char.ptr, ctypes.char.ptr, ctypes.size_t.ptr),
  };
})();
