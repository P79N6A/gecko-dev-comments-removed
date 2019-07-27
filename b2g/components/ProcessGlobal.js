





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

function ConsoleMessage(aMsg, aLevel) {
  this.timeStamp = Date.now();
  this.msg = aMsg;

  switch (aLevel) {
    case 'error':
    case 'assert':
      this.logLevel = Ci.nsIConsoleMessage.error;
      break;
    case 'warn':
      this.logLevel = Ci.nsIConsoleMessage.warn;
      break;
    case 'log':
    case 'info':
      this.logLevel = Ci.nsIConsoleMessage.info;
      break;
    default:
      this.logLevel = Ci.nsIConsoleMessage.debug;
      break;
  }
}

ConsoleMessage.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIConsoleMessage]),
  toString: function() { return this.msg; }
};

const gFactoryResetFile = "__post_reset_cmd__";

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

    Cu.import("resource://gre/modules/osfile.jsm");
    let dir = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsIFile);
    dir.initWithPath("/persist");
    var postResetFile = dir.exists() ?
                        OS.Path.join("/persist", gFactoryResetFile):
                        OS.Path.join("/cache", gFactoryResetFile);
    let file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsIFile);
    file.initWithPath(postResetFile);
    if (!file.exists()) {
      debug("Nothing to wipe.")
      return;
    }

    let promise = OS.File.read(postResetFile);
    promise.then(
      (array) => {
        file.remove(false);
        let decoder = new TextDecoder();
        this.processWipeFile(decoder.decode(array));
      },
      function onError(error) {
        debug("Error: " + error);
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

      let msg = 'Content JS ' + message.level.toUpperCase() + ': ' + Array.join(args, ' ');
      Services.console.logMessage(new ConsoleMessage(msg, message.level));
      break;
    }
    }
  },
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([ProcessGlobal]);
