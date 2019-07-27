"use strict";
const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;


Cu.import("resource://testing-common/AppInfo.jsm");
updateAppInfo();

const { devtools } = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});
const { require } = devtools;

this.sourceMap = require("source-map");
this.acorn = require("acorn/acorn");
this.prettyFast = require("devtools/pretty-fast");
const { console } = Cu.import("resource://gre/modules/devtools/Console.jsm", {});



let errorCount = 0;
let listener = {
  observe: function (aMessage) {
    errorCount++;
    try {
      
      
      var scriptError = aMessage.QueryInterface(Ci.nsIScriptError);
      dump(aMessage.sourceName + ":" + aMessage.lineNumber + ": " +
           scriptErrorFlagsToKind(aMessage.flags) + ": " +
           aMessage.errorMessage + "\n");
      var string = aMessage.errorMessage;
    } catch (x) {
      
      
      try {
        var string = "" + aMessage.message;
      } catch (x) {
        var string = "<error converting error message to string>";
      }
    }

    do_throw("head_pretty-fast.js got console message: " + string + "\n");
  }
};

let consoleService = Cc["@mozilla.org/consoleservice;1"].getService(Ci.nsIConsoleService);
consoleService.registerListener(listener);

