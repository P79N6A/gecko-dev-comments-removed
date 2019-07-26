





"use strict";






const Ci = Components.interfaces;
const Cc = Components.classes;
const Cu = Components.utils;

this.EXPORTED_SYMBOLS = ["DebuggerServer"];

var loadSubScript =
  "function loadSubScript(aURL)\n" +
  "{\n" +
  "const Ci = Components.interfaces;\n" +
  "const Cc = Components.classes;\n" +
  "  try {\n" +
  "    let loader = Cc[\"@mozilla.org/moz/jssubscript-loader;1\"]\n" +
  "      .getService(Ci.mozIJSSubScriptLoader);\n" +
  "    loader.loadSubScript(aURL, this);\n" +
  "  } catch(e) {\n" +
  "    dump(\"Error loading: \" + aURL + \": \" + e + \" - \" + e.stack + \"\\n\");\n" +
  "    throw e;\n" +
  "  }\n" +
  "}";

Cu.import("resource://gre/modules/devtools/dbg-client.jsm");


var systemPrincipal = Cc["@mozilla.org/systemprincipal;1"]
                      .createInstance(Ci.nsIPrincipal);

var gGlobal = Cu.Sandbox(systemPrincipal);
Cu.evalInSandbox(loadSubScript, gGlobal, "1.8");
gGlobal.loadSubScript("chrome://global/content/devtools/dbg-server.js");

this.DebuggerServer = gGlobal.DebuggerServer;
