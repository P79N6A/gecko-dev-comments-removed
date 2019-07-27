


"use strict";
const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;
const CC = Components.Constructor;

const { devtools } =
  Cu.import("resource://gre/modules/devtools/Loader.jsm", {});
const { Promise: promise } =
  Cu.import("resource://gre/modules/Promise.jsm", {});
const { Task } = Cu.import("resource://gre/modules/Task.jsm", {});

const Services = devtools.require("Services");
const DevToolsUtils = devtools.require("devtools/toolkit/DevToolsUtils.js");
const xpcInspector = devtools.require("xpcInspector");








Services.prefs.setBoolPref("devtools.debugger.remote-enabled", true);

Services.prefs.setIntPref("devtools.remote.tls-handshake-timeout", 1000);

function tryImport(url) {
  try {
    Cu.import(url);
  } catch (e) {
    dump("Error importing " + url + "\n");
    dump(DevToolsUtils.safeErrorString(e) + "\n");
    throw e;
  }
}

tryImport("resource://gre/modules/devtools/dbg-server.jsm");
tryImport("resource://gre/modules/devtools/dbg-client.jsm");


function scriptErrorFlagsToKind(aFlags) {
  var kind;
  if (aFlags & Ci.nsIScriptError.warningFlag)
    kind = "warning";
  if (aFlags & Ci.nsIScriptError.exceptionFlag)
    kind = "exception";
  else
    kind = "error";

  if (aFlags & Ci.nsIScriptError.strictFlag)
    kind = "strict " + kind;

  return kind;
}



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

    
    while (xpcInspector.eventLoopNestLevel > 0) {
      xpcInspector.exitNestedEventLoop();
    }

    
    if (!(aMessage.flags & Ci.nsIScriptError.strictFlag)) {
      do_print("head_dbg.js got console message: " + string + "\n");
    }
  }
};

let consoleService = Cc["@mozilla.org/consoleservice;1"]
                     .getService(Ci.nsIConsoleService);
consoleService.registerListener(listener);




function initTestDebuggerServer() {
  DebuggerServer.registerModule("xpcshell-test/testactors");
  DebuggerServer.init();
}
