


function PostInitGlobalActor(aConnection) {}

PostInitGlobalActor.prototype = {
  actorPrefix: "postInitGlobal",
  onPing: function onPing(aRequest) {
    return { message: "pong" };
  },
};

PostInitGlobalActor.prototype.requestTypes = {
  "ping": PostInitGlobalActor.prototype.onPing,
};

DebuggerServer.addGlobalActor(PostInitGlobalActor, "postInitGlobalActor");
