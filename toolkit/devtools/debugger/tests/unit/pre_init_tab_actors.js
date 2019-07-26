


function PreInitTabActor(aConnection) {}

PreInitTabActor.prototype = {
  actorPrefix: "preInitTab",
  onPing: function onPing(aRequest) {
    return { message: "pong" };
  },
};

PreInitTabActor.prototype.requestTypes = {
  "ping": PreInitTabActor.prototype.onPing,
};

DebuggerServer.addGlobalActor(PreInitTabActor, "preInitTabActor");
