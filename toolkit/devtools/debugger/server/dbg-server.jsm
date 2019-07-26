





"use strict";






const Ci = Components.interfaces;
const Cc = Components.classes;
const Cu = Components.utils;

this.EXPORTED_SYMBOLS = ["DebuggerServer"];

function loadSubScript(aURL)
{
  try {
    let loader = Cc["@mozilla.org/moz/jssubscript-loader;1"]
      .getService(Ci.mozIJSSubScriptLoader);
    loader.loadSubScript(aURL, this);
  } catch(e) {
    dump("Error loading: " + aURL + ": " + e + " - " + e.stack + "\n");

    throw e;
  }
}

Cu.import("resource://gre/modules/devtools/dbg-client.jsm");


var systemPrincipal = Cc["@mozilla.org/systemprincipal;1"]
                      .createInstance(Ci.nsIPrincipal);

var gGlobal = Cu.Sandbox(systemPrincipal);
gGlobal.importFunction(loadSubScript);
gGlobal.loadSubScript("chrome://global/content/devtools/dbg-server.js");

this.DebuggerServer = gGlobal.DebuggerServer;
