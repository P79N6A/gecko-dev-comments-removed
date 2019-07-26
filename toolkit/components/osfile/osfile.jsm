







if (typeof Components != "undefined") {
  this.EXPORTED_SYMBOLS = ["OS"];
  Components.utils.import("resource://gre/modules/osfile/osfile_async_front.jsm", this);
} else {
#ifdef XP_WIN
  importScripts("resource://gre/modules/osfile/osfile_win_front.jsm");
#else
  importScripts("resource://gre/modules/osfile/osfile_unix_front.jsm");
#endif
}