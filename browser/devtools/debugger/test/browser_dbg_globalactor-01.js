






var gClient = null;

function test()
{
  DebuggerServer.addActors("chrome://mochitests/content/browser/browser/devtools/debugger/test/testactors.js");

  let transport = DebuggerServer.connectPipe();
  gClient = new DebuggerClient(transport);
  gClient.connect(function(aType, aTraits) {
    is(aType, "browser", "Root actor should identify itself as a browser.");
    gClient.listTabs(function(aResponse) {
      let globalActor = aResponse.testGlobalActor1;
      ok(globalActor, "Found the test tab actor.")
      ok(globalActor.indexOf("testone") >= 0,
         "testTabActor's actorPrefix should be used.");
      gClient.request({ to: globalActor, type: "ping" }, function(aResponse) {
        is(aResponse.pong, "pong", "Actor should respond to requests.");
        
        gClient.request({ to: globalActor, type: "ping" }, function(aResponse) {
          is(aResponse.pong, "pong", "Actor should respond to requests.");

          
          let connections = Object.keys(DebuggerServer._connections);
          info(connections.length + " connections are established.");
          let connPrefix = connections[connections.length - 1];
          ok(DebuggerServer._connections[connPrefix],
             connPrefix + " is a valid connection.");
          
          let extraPools = DebuggerServer._connections[connPrefix]._extraPools;
          let globalPool;
          for (let pool of extraPools) {
            if (Object.keys(pool._actors).some(function(elem) {
              
              let re = new RegExp(connPrefix + "tab", "g");
              return elem.match(re) !== null;
            })) {
              globalPool = pool;
              break;
            }
          }
          
          let actorPrefix = connPrefix + "testone";
          let actors = Object.keys(globalPool._actors).join();
          info("Global actors: " + actors);
          isnot(actors.indexOf(actorPrefix), -1, "The test actor exists in the pool.");
          is(actors.indexOf(actorPrefix), actors.lastIndexOf(actorPrefix),
             "Only one actor exists in the pool.");

          finish_test();
        });
      });
    });
  });
}

function finish_test()
{
  gClient.close(function() {
    finish();
  });
}
