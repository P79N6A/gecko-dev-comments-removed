



"use strict";

module.metadata = {
  "stability": "unstable"
};

const { Cc, Ci } = require("chrome");
const self = require("../self");
const traceback = require("./traceback")
const prefs = require("../preferences/service");
const { merge } = require("../util/object");
const { curry } = require("../lang/functional");

const LEVELS = {
  "all": Number.MIN_VALUE,
  "debug": 20000,
  "info": 30000,
  "warn": 40000,
  "error": 50000,
  "off": Number.MAX_VALUE,
};
const DEFAULT_LOG_LEVEL = "error";
const ADDON_LOG_LEVEL_PREF = "extensions." + self.id + ".sdk.console.logLevel";
const SDK_LOG_LEVEL_PREF = "extensions.sdk.console.logLevel";

let logLevel;

function setLogLevel() {
  logLevel = prefs.get(ADDON_LOG_LEVEL_PREF, prefs.get(SDK_LOG_LEVEL_PREF,
                                                       DEFAULT_LOG_LEVEL));
}
setLogLevel();

let logLevelObserver = {
  observe: function(subject, topic, data) {
    setLogLevel();
  }
};
let branch = Cc["@mozilla.org/preferences-service;1"].
             getService(Ci.nsIPrefService).
             getBranch(null);
branch.addObserver(ADDON_LOG_LEVEL_PREF, logLevelObserver, false);
branch.addObserver(SDK_LOG_LEVEL_PREF, logLevelObserver, false);

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

function message(print, level) {
  if (LEVELS[level] < LEVELS[logLevel])
    return;

  let args = Array.slice(arguments, 2);

  print(level + ": " + self.name + ": " + stringifyArgs(args) + "\n", level);
}

function errorMessage(print, e) {
  
  
  var fullString = ("An exception occurred.\n" +
                   (e.name ? e.name + ": " : "") +
                   (e.message ? e.message : e.toString()) + "\n" +
                   (e.fileName ? traceback.sourceURI(e.fileName) + " " +
                                 e.lineNumber + "\n"
                               : "") +
                   traceback.format(e));

  message(print, "error", fullString);
}

function traceMessage(print) {
  var stack = traceback.get();
  stack.splice(-1, 1);

  message(print, "info", traceback.format(stack));
}

function PlainTextConsole(print) {
  if (!print)
    print = dump;

  if (print === dump) {
    
    
    var prefs = Cc["@mozilla.org/preferences-service;1"]
                .getService(Ci.nsIPrefBranch);
    prefs.setBoolPref("browser.dom.window.dump.enabled", true);
  }

  merge(this, {
    log: curry(message, print, "info"),
    info: curry(message, print, "info"),
    warn: curry(message, print, "warn"),
    error: curry(message, print, "error"),
    debug: curry(message, print, "debug"),
    exception: curry(errorMessage, print),
    trace: curry(traceMessage, print),

    dir: function dir() {},
    group: function group() {},
    groupCollapsed: function groupCollapsed() {},
    groupEnd: function groupEnd() {},
    time: function time() {},
    timeEnd: function timeEnd() {}
  });

  
  
  
  
  
  
  

  this.__exposedProps__ = Object.keys(this).reduce(function(exposed, prop) {
    exposed[prop] = "r";
    return exposed;
  }, {});

  Object.freeze(this);
};
exports.PlainTextConsole = PlainTextConsole;
