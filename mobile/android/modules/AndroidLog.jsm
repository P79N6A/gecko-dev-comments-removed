



"use strict";





























if (typeof Components != "undefined") {
  
  this.EXPORTED_SYMBOLS = ["AndroidLog"];
  Components.utils.import("resource://gre/modules/ctypes.jsm");
}


const ANDROID_LOG_VERBOSE = 2;
const ANDROID_LOG_DEBUG = 3;
const ANDROID_LOG_INFO = 4;
const ANDROID_LOG_WARN = 5;
const ANDROID_LOG_ERROR = 6;

let liblog = ctypes.open("liblog.so"); 
let __android_log_write = liblog.declare("__android_log_write",
                                         ctypes.default_abi,
                                         ctypes.int, 
                                         ctypes.int, 
                                         ctypes.char.ptr, 
                                         ctypes.char.ptr); 

let AndroidLog = {
  v: (tag, msg) => __android_log_write(ANDROID_LOG_VERBOSE, "Gecko" + tag, msg),
  d: (tag, msg) => __android_log_write(ANDROID_LOG_DEBUG, "Gecko" + tag, msg),
  i: (tag, msg) => __android_log_write(ANDROID_LOG_INFO, "Gecko" + tag, msg),
  w: (tag, msg) => __android_log_write(ANDROID_LOG_WARN, "Gecko" + tag, msg),
  e: (tag, msg) => __android_log_write(ANDROID_LOG_ERROR, "Gecko" + tag, msg),
};

if (typeof Components == "undefined") {
  
  module.exports = AndroidLog;
}
