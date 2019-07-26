


function PreInitGlobalActor(aConnection) {}

PreInitGlobalActor.prototype = {
  actorPrefix: "preInitGlobal",
  onPing: function onPing(aRequest) {
    return { message: "pong" };
  },
};

PreInitGlobalActor.prototype.requestTypes = {
  "ping": PreInitGlobalActor.prototype.onPing,
};

DebuggerServer.addGlobalActor(PreInitGlobalActor, "preInitGlobalActor");
