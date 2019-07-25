



































function stringify(arg) {
  try {
    return String(arg);
  }
  catch(ex) {
    return "<toString() error>";
  }
}

function stringifyArgs(args) {
  return Array.map(args, stringify).join(" ");
}

function message(print, level, args) {
  print(level + ": " + stringifyArgs(args) + "\n");
}

var Console = exports.PlainTextConsole = function PlainTextConsole(print) {
  if (!print)
    print = dump;
  if (print === dump) {
    
    
    var prefs = Cc["@mozilla.org/preferences-service;1"]
                .getService(Ci.nsIPrefBranch);
    prefs.setBoolPref("browser.dom.window.dump.enabled", true);
  }
  this.print = print;
};

Console.prototype = {
  log: function log() {
    message(this.print, "info", arguments);
  },

  info: function info() {
    message(this.print, "info", arguments);
  },

  warn: function warn() {
    message(this.print, "warning", arguments);
  },

  error: function error() {
    message(this.print, "error", arguments);
  },

  debug: function debug() {
    message(this.print, "debug", arguments);
  },

  exception: function exception(e) {
    var fullString = ("An exception occurred.\n" +
                      require("traceback").format(e) + "\n" + e);
    this.error(fullString);
  },

  trace: function trace() {
    var traceback = require("traceback");
    var stack = traceback.get();
    stack.splice(-1, 1);
    message(this.print, "info", [traceback.format(stack)]);
  }
};
