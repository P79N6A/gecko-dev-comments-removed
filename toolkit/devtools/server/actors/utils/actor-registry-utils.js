





"use strict";

let { Cu, CC, Ci, Cc } = require("chrome");

const { DebuggerServer } = require("devtools/server/main");









exports.registerActor = function(sourceText, fileName, options) {
  const principal = CC("@mozilla.org/systemprincipal;1", "nsIPrincipal")();
  const sandbox = Cu.Sandbox(principal);
  const exports = sandbox.exports = {};
  sandbox.require = require;

  Cu.evalInSandbox(sourceText, sandbox, "1.8", fileName, 1);

  let { prefix, constructor, type } = options;

  if (type.global && !DebuggerServer.globalActorFactories.hasOwnProperty(prefix)) {
    DebuggerServer.addGlobalActor({
      constructorName: constructor,
      constructorFun: sandbox[constructor]
    }, prefix);
  }

  if (type.tab && !DebuggerServer.tabActorFactories.hasOwnProperty(prefix)) {
    DebuggerServer.addTabActor({
      constructorName: constructor,
      constructorFun: sandbox[constructor]
    }, prefix);
  }

  
  
  if (!DebuggerServer.isInChildProcess) {
    DebuggerServer.setupInChild({
      module: "devtools/server/actors/utils/actor-registry-utils",
      setupChild: "registerActor",
      args: [sourceText, fileName, options]
    });
  }
}

exports.unregisterActor = function(options) {
  if (options.tab) {
    DebuggerServer.removeTabActor(options);
  }

  if (options.global) {
    DebuggerServer.removeGlobalActor(options);
  }

  
  
  if (!DebuggerServer.isInChildProcess) {
    DebuggerServer.setupInChild({
      module: "devtools/server/actors/utils/actor-registry-utils",
      setupChild: "unregisterActor",
      args: [options]
    });
  }
}
