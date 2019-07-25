






































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

const KEY_PROFILEDIR                  = "ProfD";
const FILE_EXTENSIONS_LOG             = "extensions.log";
const PREF_LOGGING_ENABLED            = "extensions.logging.enabled";

const NS_PREFBRANCH_PREFCHANGE_TOPIC_ID = "nsPref:changed";

Components.utils.import("resource://gre/modules/FileUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");

var EXPORTED_SYMBOLS = [ "LogManager" ];

var gDebugLogEnabled = false;

function formatLogMessage(aType, aName, aStr) {
  return aType.toUpperCase() + " " + aName + ": " + aStr;
}

function AddonLogger(aName) {
  this.name = aName;
}

AddonLogger.prototype = {
  name: null,

  error: function(aStr) {
    let message = formatLogMessage("error", this.name, aStr);

    let consoleMessage = Cc["@mozilla.org/scripterror;1"].
                         createInstance(Ci.nsIScriptError);
    consoleMessage.init(message, null, null, 0, 0, Ci.nsIScriptError.errorFlag,
                        "component javascript");
    Services.console.logMessage(consoleMessage);

    if (gDebugLogEnabled)
      dump("*** " + message + "\n");

    try {
      var tstamp = new Date();
      var logfile = FileUtils.getFile(KEY_PROFILEDIR, [FILE_EXTENSIONS_LOG]);
      var stream = Cc["@mozilla.org/network/file-output-stream;1"].
                   createInstance(Ci.nsIFileOutputStream);
      stream.init(logfile, 0x02 | 0x08 | 0x10, 0666, 0); 
      var writer = Cc["@mozilla.org/intl/converter-output-stream;1"].
                   createInstance(Ci.nsIConverterOutputStream);
      writer.init(stream, "UTF-8", 0, 0x0000);
      writer.writeString(tstamp.toLocaleFormat("%Y-%m-%d %H:%M:%S ") +
                         message + "\n");
      writer.close();
    }
    catch (e) { }
  },

  warn: function(aStr) {
    let message = formatLogMessage("warn", this.name, aStr);

    let consoleMessage = Cc["@mozilla.org/scripterror;1"].
                         createInstance(Ci.nsIScriptError);
    consoleMessage.init(message, null, null, 0, 0, Ci.nsIScriptError.warningFlag,
                        "component javascript");
    Services.console.logMessage(consoleMessage);

    if (gDebugLogEnabled)
      dump("*** " + message + "\n");
  },

  log: function(aStr) {
    if (gDebugLogEnabled) {
      let message = formatLogMessage("log", this.name, aStr);
      dump("*** " + message + "\n");
      Services.console.logStringMessage(message);
    }
  }
};

var LogManager = {
  getLogger: function(aName, aTarget) {
    let logger = new AddonLogger(aName);

    if (aTarget) {
      ["error", "warn", "log"].forEach(function(name) {
        let fname = name.toUpperCase();
        delete aTarget[fname];
        aTarget[fname] = function(aStr) {
          logger[name](aStr);
        };
      });
    }

    return logger;
  }
};

var PrefObserver = {
  init: function() {
    Services.prefs.addObserver(PREF_LOGGING_ENABLED, this, false);
    Services.obs.addObserver(this, "xpcom-shutdown", false);
    this.observe(null, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID, PREF_LOGGING_ENABLED);
  },

  observe: function(aSubject, aTopic, aData) {
    if (aTopic == "xpcom-shutdown") {
      Services.prefs.removeObserver(PREF_LOGGING_ENABLED, this);
      Services.obs.removeObserver(this, "xpcom-shutdown");
    }
    else if (aTopic == NS_PREFBRANCH_PREFCHANGE_TOPIC_ID) {
      try {
        gDebugLogEnabled = Services.prefs.getBoolPref(PREF_LOGGING_ENABLED);
      }
      catch (e) {
        gDebugLogEnabled = false;
      }
    }
  }
};

PrefObserver.init();
