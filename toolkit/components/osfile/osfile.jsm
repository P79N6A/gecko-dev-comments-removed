







if (typeof Components != "undefined") {
  
  
  throw new Error("osfile.jsm cannot be used from the main thread yet");
}
#ifdef XP_WIN
throw new Error("osfile.jsm not implemented for Windows platforms yet");
#else
importScripts("resource://gre/modules/osfile/osfile_unix_back.jsm");
#endif
