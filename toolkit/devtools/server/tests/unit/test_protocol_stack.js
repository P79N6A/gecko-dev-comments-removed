


"use strict";








let protocol = devtools.require("devtools/server/protocol");
let {method, Arg, Option, RetVal} = protocol;
let events = devtools.require("sdk/event/core");

function simpleHello() {
  return {
    from: "root",
    applicationType: "xpcshell-tests",
    traits: [],
  };
}

let RootActor = protocol.ActorClass({
  typeName: "root",
  initialize: function(conn) {
    protocol.Actor.prototype.initialize.call(this, conn);
    
    this.manage(this);
    this.actorID = "root";
    this.sequence = 0;
  },

  sayHello: simpleHello,

  simpleReturn: method(function() {
    return this.sequence++;
  }, {
    response: { value: RetVal() },
  })
});

let RootFront = protocol.FrontClass(RootActor, {
  initialize: function(client) {
    this.actorID = "root";
    protocol.Front.prototype.initialize.call(this, client);
    
    this.manage(this);
  }
});

function run_test() {
  if (!Services.prefs.getBoolPref("javascript.options.asyncstack")) {
    do_print("Async stacks are disabled.");
    return;
  }

  DebuggerServer.createRootActor = RootActor;
  DebuggerServer.init();

  let trace = connectPipeTracing();
  let client = new DebuggerClient(trace);
  let rootClient;

  client.connect(function onConnect() {
    rootClient = RootFront(client);

    rootClient.simpleReturn().then(() => {
      let stack = Components.stack;
      while (stack) {
        do_print(stack.name);
        if (stack.name == "onConnect") {
          
          ok(true, "Complete stack");
          return;
        }
        stack = stack.asyncCaller || stack.caller;
      }
      ok(false, "Incomplete stack");
    }, () => {
      ok(false, "Request failed unexpectedly");
    }).then(() => {
      client.close(() => {
        do_test_finished();
      });
    })
  });

  do_test_pending();
}
