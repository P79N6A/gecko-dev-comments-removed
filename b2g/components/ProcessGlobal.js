





'use strict';










const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import('resource://gre/modules/Services.jsm');
Cu.import('resource://gre/modules/XPCOMUtils.jsm');

function debug(msg) {
  log(msg);
}
function log(msg) {
  
  
}

function formatStackFrame(aFrame) {
  let functionName = aFrame.functionName || '<anonymous>';
  return '    at ' + functionName +
         ' (' + aFrame.filename + ':' + aFrame.lineNumber +
         ':' + aFrame.columnNumber + ')';
}

const gFactoryResetFile = "/persist/__post_reset_cmd__";

function ProcessGlobal() {}
ProcessGlobal.prototype = {
  classID: Components.ID('{1a94c87a-5ece-4d11-91e1-d29c29f21b28}'),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference]),

  wipeDir: function(path) {
    log("wipeDir " + path);
    let dir = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsIFile);
    dir.initWithPath(path);
    if (!dir.exists() || !dir.isDirectory()) {
      return;
    }
    let entries = dir.directoryEntries;
    while (entries.hasMoreElements()) {
      let file = entries.getNext().QueryInterface(Ci.nsIFile);
      log("Deleting " + file.path);
      try {
        file.remove(true);
      } catch(e) {}
    }
  },

  processWipeFile: function(text) {
    log("processWipeFile " + text);
    let lines = text.split("\n");
    lines.forEach((line) => {
      log(line);
      let params = line.split(" ");
      if (params[0] == "wipe") {
        this.wipeDir(params[1]);
      }
    });
  },

  cleanupAfterFactoryReset: function() {
    log("cleanupAfterWipe start");

    let file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsIFile);
    file.initWithPath(gFactoryResetFile);
    if (!file.exists()) {
      debug("Nothing to wipe.")
      return;
    }

    Cu.import("resource://gre/modules/osfile.jsm");
    let promise = OS.File.read(gFactoryResetFile);
    promise.then(
      (array) => {
        file.remove(false);
        let decoder = new TextDecoder();
        this.processWipeFile(decoder.decode(array));
      }
    );

    log("cleanupAfterWipe end.");
  },

  observe: function pg_observe(subject, topic, data) {
    switch (topic) {
    case 'app-startup': {
      Services.obs.addObserver(this, 'console-api-log-event', false);
      let inParent = Cc["@mozilla.org/xre/app-info;1"]
                       .getService(Ci.nsIXULRuntime)
                       .processType == Ci.nsIXULRuntime.PROCESS_TYPE_DEFAULT;
      if (inParent) {
        let ppmm = Cc["@mozilla.org/parentprocessmessagemanager;1"]
                     .getService(Ci.nsIMessageListenerManager);
        ppmm.addMessageListener("getProfD", function(message) {
          return Services.dirsvc.get("ProfD", Ci.nsIFile).path;
        });

        this.cleanupAfterFactoryReset();
      }
      break;
    }
    case 'console-api-log-event': {
      
      
      let message = subject.wrappedJSObject;
      let args = message.arguments;
      let stackTrace = '';

      if (message.level == 'assert' || message.level == 'error' || message.level == 'trace') {
        stackTrace = Array.map(message.stacktrace, formatStackFrame).join('\n');
      } else {
        stackTrace = formatStackFrame(message);
      }

      if (stackTrace) {
        args.push('\n' + stackTrace);
      }

      let prefix = 'Content JS ' + message.level.toUpperCase() + ': ';
      Services.console.logStringMessage(prefix + Array.join(args, ' '));
      break;
    }
    }
  },
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([ProcessGlobal]);
