



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





const MAX_TAG_LENGTH = 18;

let liblog = ctypes.open("liblog.so"); 
let __android_log_write = liblog.declare("__android_log_write",
                                         ctypes.default_abi,
                                         ctypes.int, 
                                         ctypes.int, 
                                         ctypes.char.ptr, 
                                         ctypes.char.ptr); 

let AndroidLog = {
  MAX_TAG_LENGTH: MAX_TAG_LENGTH,
  v: (tag, msg) => __android_log_write(ANDROID_LOG_VERBOSE, "Gecko" + tag.substring(0, MAX_TAG_LENGTH), msg),
  d: (tag, msg) => __android_log_write(ANDROID_LOG_DEBUG, "Gecko" + tag.substring(0, MAX_TAG_LENGTH), msg),
  i: (tag, msg) => __android_log_write(ANDROID_LOG_INFO, "Gecko" + tag.substring(0, MAX_TAG_LENGTH), msg),
  w: (tag, msg) => __android_log_write(ANDROID_LOG_WARN, "Gecko" + tag.substring(0, MAX_TAG_LENGTH), msg),
  e: (tag, msg) => __android_log_write(ANDROID_LOG_ERROR, "Gecko" + tag.substring(0, MAX_TAG_LENGTH), msg),

  bind: function(tag) {
    return {
      MAX_TAG_LENGTH: MAX_TAG_LENGTH,
      v: AndroidLog.v.bind(null, tag),
      d: AndroidLog.d.bind(null, tag),
      i: AndroidLog.i.bind(null, tag),
      w: AndroidLog.w.bind(null, tag),
      e: AndroidLog.e.bind(null, tag),
    };
  },
};

if (typeof Components == "undefined") {
  
  module.exports = AndroidLog;
}
