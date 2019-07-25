






































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

var EXPORTED_SYMBOLS = ["DebuggerServer"]

function loadSubScript(aURL)
{
  try {
    let loader = Components.classes["@mozilla.org/moz/jssubscript-loader;1"]
      .getService(Components.interfaces.mozIJSSubScriptLoader);
    loader.loadSubScript(aURL, this);
  } catch(e) {
    dump("Error loading: " + aURL + ": " + e + " - " + e.stack + "\n");

    throw e;
  }
}

Cu.import("resource:///modules/devtools/dbg-client.jsm");


var systemPrincipal = Cc["@mozilla.org/systemprincipal;1"]
  .createInstance(Ci.nsIPrincipal);

var gGlobal = Cu.Sandbox(systemPrincipal);
gGlobal.importFunction(loadSubScript);
gGlobal.loadSubScript("chrome://global/content/devtools/dbg-server.js");

var DebuggerServer = gGlobal.DebuggerServer;
