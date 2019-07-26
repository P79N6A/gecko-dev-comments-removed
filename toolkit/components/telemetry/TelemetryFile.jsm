"use strict";

this.EXPORTED_SYMBOLS = ["TelemetryFile"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

let imports = {};
Cu.import("resource://gre/modules/Services.jsm", imports);
Cu.import("resource://gre/modules/Deprecated.jsm", imports);
Cu.import("resource://gre/modules/NetUtil.jsm", imports);

let {Services, Deprecated, NetUtil} = imports;


const PR_WRONLY = 0x2;
const PR_CREATE_FILE = 0x8;
const PR_TRUNCATE = 0x20;
const PR_EXCL = 0x80;
const RW_OWNER = parseInt("0600", 8);
const RWX_OWNER = parseInt("0700", 8);



const MAX_PING_FILE_AGE = 14 * 24 * 60 * 60 * 1000; 



const OVERDUE_PING_FILE_AGE = 7 * 24 * 60 * 60 * 1000; 



let pingsLoaded = 0;


let pingLoadsCompleted = 0;



let pingsDiscarded = 0;



let pingsOverdue = 0;



let shouldNotifyUponSave = false;


let pendingPings = [];

this.TelemetryFile = {

  get MAX_PING_FILE_AGE() {
    return MAX_PING_FILE_AGE;
  },

  get OVERDUE_PING_FILE_AGE() {
    return OVERDUE_PING_FILE_AGE;
  },

  









  savePingToFile: function(ping, file, sync, overwrite) {
    let pingString = JSON.stringify(ping);

    let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
                    .createInstance(Ci.nsIScriptableUnicodeConverter);
    converter.charset = "UTF-8";

    let ostream = Cc["@mozilla.org/network/file-output-stream;1"]
                  .createInstance(Ci.nsIFileOutputStream);
    let initFlags = PR_WRONLY | PR_CREATE_FILE | PR_TRUNCATE;
    if (!overwrite) {
      initFlags |= PR_EXCL;
    }
    try {
      ostream.init(file, initFlags, RW_OWNER, 0);
    } catch (e) {
      
      return;
    }

    if (sync) {
      let utf8String = converter.ConvertFromUnicode(pingString);
      utf8String += converter.Finish();
      let success = false;
      try {
        let amount = ostream.write(utf8String, utf8String.length);
        success = amount == utf8String.length;
      } catch (e) {
      }
      finishTelemetrySave(success, ostream);
    } else {
      let istream = converter.convertToInputStream(pingString);
      let self = this;
      NetUtil.asyncCopy(istream, ostream,
                        function(result) {
                          finishTelemetrySave(Components.isSuccessCode(result),
                              ostream);
                        });
    }
  },

  






  savePing: function(ping, overwrite) {
    this.savePingToFile(ping,
      getSaveFileForPing(ping), true, overwrite);
  },

  




  savePendingPings: function(sessionPing) {
    this.savePing(sessionPing, true);
    pendingPings.forEach(function sppcb(e, i, a) {
      this.savePing(e, false);
    }, this);
    pendingPings = [];
  },

  




  cleanupPingFile: function(ping) {
    
    let file = getSaveFileForPing(ping);
    try {
      file.remove(true); 
    } catch(e) {
    }
  },

  










  loadSavedPings: function(sync, onLoad = null, onDone = null) {
    let directory = ensurePingDirectory();
    let entries = directory.directoryEntries
                           .QueryInterface(Ci.nsIDirectoryEnumerator);
    pingsLoaded = 0;
    pingLoadsCompleted = 0;
    try {
      while (entries.hasMoreElements()) {
        this.loadHistograms(entries.nextFile, sync, onLoad, onDone);
      }
    } finally {
      entries.close();
    }
  },

  











  loadHistograms: function loadHistograms(file, sync, onLoad = null, onDone = null) {
    let now = Date.now();
    if (now - file.lastModifiedTime > MAX_PING_FILE_AGE) {
      
      file.remove(true);
      pingsDiscarded++;
      return;
    }

    
    if (now - file.lastModifiedTime > OVERDUE_PING_FILE_AGE) {
      pingsOverdue++;
    }

    pingsLoaded++;
    if (sync) {
      let stream = Cc["@mozilla.org/network/file-input-stream;1"]
                   .createInstance(Ci.nsIFileInputStream);
      stream.init(file, -1, -1, 0);
      addToPendingPings(file, stream, onLoad, onDone);
    } else {
      let channel = NetUtil.newChannel(file);
      channel.contentType = "application/json";

      NetUtil.asyncFetch(channel, (function(stream, result) {
        if (!Components.isSuccessCode(result)) {
          return;
        }
        addToPendingPings(file, stream, onLoad, onDone);
      }).bind(this));
    }
  },

  


  get pingsLoaded() {
    return pingsLoaded;
  },

  



  get pingsOverdue() {
    return pingsOverdue;
  },

  



  get pingsDiscarded() {
    return pingsDiscarded;
  },

  




  popPendingPings: function(reason) {
    while (pendingPings.length > 0) {
      let data = pendingPings.pop();
      
      if (reason == "test-ping") {
        data.reason = reason;
      }
      yield data;
    }
  },

  set shouldNotifyUponSave(value) {
    shouldNotifyUponSave = value;
  },

  testLoadHistograms: function(file, sync, onLoad) {
    pingsLoaded = 0;
    pingLoadsCompleted = 0;
    this.loadHistograms(file, sync, onLoad);
  }
};



function getSaveFileForPing(ping) {
  let file = ensurePingDirectory();
  file.append(ping.slug);
  return file;
};

function ensurePingDirectory() {
  let directory = Services.dirsvc.get("ProfD", Ci.nsILocalFile).clone();
  directory.append("saved-telemetry-pings");
  try {
    directory.create(Ci.nsIFile.DIRECTORY_TYPE, RWX_OWNER);
  } catch (e) {
    
  }
  return directory;
};

function addToPendingPings(file, stream, onLoad, onDone) {
  let success = false;

  try {
    let string = NetUtil.readInputStreamToString(stream, stream.available(),
      { charset: "UTF-8" });
    stream.close();
    let ping = JSON.parse(string);
    
    if (typeof(ping.payload) == "string") {
      ping.payload = JSON.parse(ping.payload);
    }
    pingLoadsCompleted++;
    pendingPings.push(ping);
    success = true;
  } catch (e) {
    
    stream.close();           
    file.remove(true); 
  }

  if (onLoad) {
    onLoad(success);
  }

  if (pingLoadsCompleted == pingsLoaded) {
    if (onDone) {
      onDone();
    }
    if (shouldNotifyUponSave) {
      Services.obs.notifyObservers(null, "telemetry-test-load-complete", null);
    }
  }
};

function finishTelemetrySave(ok, stream) {
  stream.close();
  if (shouldNotifyUponSave && ok) {
    Services.obs.notifyObservers(null, "telemetry-test-save-complete", null);
  }
};
