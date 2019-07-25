



try {
  netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

  if (Cc === undefined) {
    var Cc = Components.classes;
    var Ci = Components.interfaces;
  }
} catch (ex) {} 

try {
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



var MozillaFileLogger = {};

var ipcMode = false;
try {
  if (typeof(TestRunner) != undefined)
    ipcMode = TestRunner.ipcMode;
} catch(e) { };

MozillaFileLogger.init = function(path) {
  if (ipcMode) {
    contentAsyncEvent("LoggerInit", {"filename": path});
    return;
  }

  try {
    netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
  } catch (ex) {} 

  MozillaFileLogger._file = Cc[LF_CID].createInstance(Ci.nsILocalFile);
  MozillaFileLogger._file.initWithPath(path);
  MozillaFileLogger._foStream = Cc[FOSTREAM_CID].createInstance(Ci.nsIFileOutputStream);
  MozillaFileLogger._foStream.init(this._file, PR_WRITE_ONLY | PR_CREATE_FILE | PR_APPEND,
                                   0664, 0);
}

MozillaFileLogger.getLogCallback = function() {
  if (ipcMode) {
    return function(msg) {
      contentAsyncEvent("Logger", {"num": msg.num, "level": msg.level, "info": msg.info.join(' ')});
    }
  }

  return function (msg) {
    try {
      netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
    } catch(ex) {} 

    var data = msg.num + " " + msg.level + " " + msg.info.join(' ') + "\n";
    if (MozillaFileLogger._foStream)
      MozillaFileLogger._foStream.write(data, data.length);

    if (data.indexOf("SimpleTest FINISH") >= 0) {
      MozillaFileLogger.close();
    }
  }
}


MozillaFileLogger.log = function(msg) {
  netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
  if (MozillaFileLogger._foStream)
    MozillaFileLogger._foStream.write(msg, msg.length);
}

MozillaFileLogger.close = function() {
  if (ipcMode) {
    contentAsyncEvent("LoggerClose");
    return;
  }

  try {
    netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
  } catch(ex) {} 

  if(MozillaFileLogger._foStream)
    MozillaFileLogger._foStream.close();
  
  MozillaFileLogger._foStream = null;
  MozillaFileLogger._file = null;
}
