Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/devtools/dbg-server.jsm");
Cu.import("resource://gre/modules/devtools/DevToolsUtils.jsm");

if (!DebuggerServer.initialized) {
  DebuggerServer.init();
}





DebuggerServer.addChildActors();

let onConnect = DevToolsUtils.makeInfallible(function (msg) {
  removeMessageListener("debug:connect", onConnect);

  let mm = msg.target;

  let prefix = msg.data.prefix + docShell.appId;

  let conn = DebuggerServer.connectToParent(prefix, mm);

  let actor = new DebuggerServer.ContentTabActor(conn, content);
  let actorPool = new ActorPool(conn);
  actorPool.addActor(actor);
  conn.addActorPool(actorPool);

  sendAsyncMessage("debug:actor", {actor: actor.grip(),
                                   appId: docShell.appId,
                                   prefix: prefix});
});

addMessageListener("debug:connect", onConnect);
