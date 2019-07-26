


const { RootActor } = require("devtools/server/actors/root");
const { DebuggerServer } = require("devtools/server/main");




function createRootActor(aConnection) {
  let root = new RootActor(aConnection, {
    globalActorFactories: DebuggerServer.globalActorFactories
  });
  root.applicationType = "xpcshell-tests";
  root.traits = {
    bulk: false
  };
  return root;
}

exports.register = function(handle) {
  handle.setRootActor(createRootActor);
};

exports.unregister = function(handle) {
  handle.setRootActor(null);
};
