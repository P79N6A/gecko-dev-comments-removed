


"use strict";

let libcutils = (function () {
  let library = ctypes.open("libcutils.so");

  return {
    property_get: library.declare("property_get", ctypes.default_abi, ctypes.int, ctypes.char.ptr, ctypes.char.ptr, ctypes.char.ptr),
    property_set: library.declare("property_set", ctypes.default_abi, ctypes.int, ctypes.char.ptr, ctypes.char.ptr)
  };
})();
