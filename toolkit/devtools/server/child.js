



"use strict";



(function () {
  let { devtools } = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});
  const DevToolsUtils = devtools.require("devtools/toolkit/DevToolsUtils.js");
  const {DebuggerServer, ActorPool} = Cu.import("resource://gre/modules/devtools/dbg-server.jsm", {});

  if (!DebuggerServer.initialized) {
    DebuggerServer.init();
  }

  
  
  
  
  DebuggerServer.addChildActors();

  let onConnect = DevToolsUtils.makeInfallible(function (msg) {
    removeMessageListener("debug:connect", onConnect);

    let mm = msg.target;

    let prefix = msg.data.prefix + docShell.appId;

    let conn = DebuggerServer.connectToParent(prefix, mm);

    let actor = new DebuggerServer.ContentAppActor(conn, content);
    let actorPool = new ActorPool(conn);
    actorPool.addActor(actor);
    conn.addActorPool(actorPool);

    sendAsyncMessage("debug:actor", {actor: actor.grip(),
                                     appId: docShell.appId,
                                     prefix: prefix});
  });

  addMessageListener("debug:connect", onConnect);
})();
