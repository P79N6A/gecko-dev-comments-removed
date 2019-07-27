"use strict"

loadSubScript("resource://gre/modules/devtools/worker-loader.js");

let { ActorPool } = worker.require("devtools/server/actors/common");
let { ThreadActor } = worker.require("devtools/server/actors/script");
let { TabSources } = worker.require("devtools/server/actors/utils/TabSources");
let makeDebugger = worker.require("devtools/server/actors/utils/make-debugger");
let { DebuggerServer } = worker.require("devtools/server/main");

DebuggerServer.init();
DebuggerServer.createRootActor = function () {
  throw new Error("Should never get here!");
};

let connections = Object.create(null);

this.addEventListener("message",  function (event) {
  let packet = JSON.parse(event.data);
  switch (packet.type) {
  case "connect":
    
    let connection = DebuggerServer.connectToParent(packet.id, this);
    connections[packet.id] = connection;

    
    let pool = new ActorPool(connection);
    connection.addActorPool(pool);

    let sources = null;

    let actor = new ThreadActor({
      makeDebugger: makeDebugger.bind(null, {
        findDebuggees: () => {
          return [this.global];
        },

        shouldAddNewGlobalAsDebuggee: () => {
          return true;
        },
      }),

      get sources() {
        if (sources === null) {
          sources = new TabSources(actor);
        }
        return sources;
      }
    }, global);

    pool.addActor(actor);

    
    
    
    
    
    
    actor.onAttach({});
    break;

  case "disconnect":
    connections[packet.id].close();
    break;
  };
});
