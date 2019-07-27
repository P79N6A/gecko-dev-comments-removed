



"use strict";

try {

let chromeGlobal = this;



(function () {
  let Cu = Components.utils;
  let { devtools } = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});
  const DevToolsUtils = devtools.require("devtools/toolkit/DevToolsUtils.js");
  const {DebuggerServer, ActorPool} = Cu.import("resource://gre/modules/devtools/dbg-server.jsm", {});
  if (!DebuggerServer.childID) {
    DebuggerServer.childID = 1;
  }

  if (!DebuggerServer.initialized) {
    DebuggerServer.init();

    
    DebuggerServer.parentMessageManager = {
      sendSyncMessage: sendSyncMessage,
      addMessageListener: addMessageListener
    };
  }

  
  
  
  
  DebuggerServer.addChildActors();

  let connections = new Map();

  let onConnect = DevToolsUtils.makeInfallible(function (msg) {
    removeMessageListener("debug:connect", onConnect);

    let mm = msg.target;
    let prefix = msg.data.prefix;
    let id = DebuggerServer.childID++;

    let conn = DebuggerServer.connectToParent(prefix, mm);
    connections.set(id, conn);

    let actor = new DebuggerServer.ContentActor(conn, chromeGlobal);
    let actorPool = new ActorPool(conn);
    actorPool.addActor(actor);
    conn.addActorPool(actorPool);

    sendAsyncMessage("debug:actor", {actor: actor.form(), childID: id});
  });

  addMessageListener("debug:connect", onConnect);

  let onDisconnect = DevToolsUtils.makeInfallible(function (msg) {
    removeMessageListener("debug:disconnect", onDisconnect);

    
    
    
    let childID = msg.data.childID;
    let conn = connections.get(childID);
    if (conn) {
      conn.close();
      connections.delete(childID);
    }
  });
  addMessageListener("debug:disconnect", onDisconnect);

  let onInspect = DevToolsUtils.makeInfallible(function(msg) {
    
    
    
    
    let inspector = devtools.require("devtools/server/actors/inspector");
    inspector.setInspectingNode(msg.objects.node);
  });
  addMessageListener("debug:inspect", onInspect);
})();

} catch(e) {
  dump("Exception in app child process: " + e + "\n");
}
