


function PostInitTabActor(aConnection) {}

PostInitTabActor.prototype = {
  actorPostfix: "postInitTab",
  onPing: function onPing(aRequest) {
    return { message: "pong" };
  },
};

PostInitTabActor.prototype.requestTypes = {
  "ping": PostInitTabActor.prototype.onPing,
};

DebuggerServer.addGlobalActor(PostInitTabActor, "postInitTabActor");
