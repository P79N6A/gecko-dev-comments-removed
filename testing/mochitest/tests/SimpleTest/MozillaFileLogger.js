


try {
  netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
  const Cc = Components.classes;
  const Ci = Components.interfaces;
  const FOSTREAM_CID = "@mozilla.org/network/file-output-stream;1";
  const LF_CID = "@mozilla.org/file/local;1";
  
  
  
  const PR_READ_ONLY    = 0x01; 
  const PR_WRITE_ONLY   = 0x02; 
  const PR_READ_WRITE   = 0x04; 
  
  
  
  const PR_CREATE_FILE  = 0x08;
  
  
  const PR_APPEND       = 0x10;
  
  
  const PR_TRUNCATE     = 0x20;
  
  
  
  const PR_SYNC         = 0x40;
  
  
  
  const PR_EXCL         = 0x80;
} catch (ex) {
  
}



var MozillaFileLogger = {}

MozillaFileLogger.init = function(path) {
  netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
  MozillaFileLogger._file = Cc[LF_CID].createInstance(Ci.nsILocalFile);
  MozillaFileLogger._file.initWithPath(path);
  MozillaFileLogger._foStream = Cc[FOSTREAM_CID].createInstance(Ci.nsIFileOutputStream);
  MozillaFileLogger._foStream.init(this._file, PR_WRITE_ONLY | PR_CREATE_FILE | PR_APPEND,
                                   0664, 0);
}

MozillaFileLogger.getLogCallback = function() {
  return function (msg) {
    netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
    var data = msg.num + " " + msg.level + " " + msg.info.join(' ') + "\n";
    MozillaFileLogger._foStream.write(data, data.length);
    if (data.indexOf("SimpleTest FINISH") >= 0) {
      MozillaFileLogger.close();
    }
  }
}

MozillaFileLogger.close = function() {
  netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
  MozillaFileLogger._foStream.close();
  MozillaFileLogger._foStream = null;
  MozillaFileLogger._file = null;
}
