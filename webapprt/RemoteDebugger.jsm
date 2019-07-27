



"use strict";

this.EXPORTED_SYMBOLS = ["RemoteDebugger"];

let Cc = Components.classes;
let Ci = Components.interfaces;
let Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import('resource://gre/modules/devtools/dbg-server.jsm');

this.RemoteDebugger = {
  init: function(port) {
    if (!DebuggerServer.initialized) {
      DebuggerServer.init();
      DebuggerServer.addBrowserActors("webapprt:webapp");
      DebuggerServer.addActors("chrome://webapprt/content/dbg-webapp-actors.js");
    }
    let listener = DebuggerServer.createListener();
    listener.portOrPath = port;
    listener.open();
  }
}
