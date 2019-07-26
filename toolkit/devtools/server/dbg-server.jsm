





"use strict";






const Ci = Components.interfaces;
const Cc = Components.classes;
const Cu = Components.utils;

const { devtools } = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});

this.EXPORTED_SYMBOLS = ["DebuggerServer", "ActorPool"];

let server = devtools.require("devtools/server/main");

this.DebuggerServer = server.DebuggerServer;
this.ActorPool = server.ActorPool;
