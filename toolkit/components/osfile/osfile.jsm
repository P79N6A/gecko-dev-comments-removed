







if (typeof Components != "undefined") {
  
  
  throw new Error("osfile.jsm cannot be used from the main thread yet");
}
#ifdef XP_WIN
importScripts("resource://gre/modules/osfile/osfile_win_front.jsm");
#else
importScripts("resource://gre/modules/osfile/osfile_unix_front.jsm");
#endif
