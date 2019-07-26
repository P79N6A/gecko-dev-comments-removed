







if (typeof Components != "undefined") {
  this.EXPORTED_SYMBOLS = ["OS"];
  Components.utils.import("resource://gre/modules/osfile/osfile_async_front.jsm", this);
} else {
  
  
  
#ifdef XP_WIN
  importScripts(
    "resource://gre/modules/workers/require.js",
    "resource://gre/modules/osfile/osfile_win_allthreads.jsm",
    "resource://gre/modules/osfile/osfile_win_back.jsm",
    "resource://gre/modules/osfile/osfile_shared_front.jsm",
    "resource://gre/modules/osfile/osfile_win_front.jsm"
  );
#else
  importScripts(
    "resource://gre/modules/workers/require.js",
    "resource://gre/modules/osfile/osfile_unix_allthreads.jsm",
    "resource://gre/modules/osfile/osfile_unix_back.jsm",
    "resource://gre/modules/osfile/osfile_shared_front.jsm",
    "resource://gre/modules/osfile/osfile_unix_front.jsm"
  );
#endif
  OS.Path = require("resource://gre/modules/osfile/ospath.jsm");
}
