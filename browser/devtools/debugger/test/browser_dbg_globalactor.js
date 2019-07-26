






const CHROME_URL = "chrome://mochitests/content/browser/browser/devtools/debugger/test/"
const ACTORS_URL = CHROME_URL + "testactors.js";

function test() {
  let gClient;

  if (!DebuggerServer.initialized) {
    DebuggerServer.init(() => true);
    DebuggerServer.addBrowserActors();
  }

  DebuggerServer.addActors(ACTORS_URL);

  let transport = DebuggerServer.connectPipe();
  gClient = new DebuggerClient(transport);
  gClient.connect((aType, aTraits) => {
    is(aType, "browser",
      "Root actor should identify itself as a browser.");

    gClient.listTabs(aResponse => {
      let globalActor = aResponse.testGlobalActor1;
      ok(globalActor, "Found the test tab actor.")
      ok(globalActor.contains("test_one"),
        "testGlobalActor1's actorPrefix should be used.");

      gClient.request({ to: globalActor, type: "ping" }, aResponse => {
        is(aResponse.pong, "pong", "Actor should respond to requests.");

        
        gClient.request({ to: globalActor, type: "ping" }, aResponse => {
          is(aResponse.pong, "pong", "Actor should respond to requests.");

          
          let conn = transport._serverConnection;

          
          let extraPools = conn._extraPools;
          let globalPool;

          for (let pool of extraPools) {
            if (Object.keys(pool._actors).some(e => {
              
              let re = new RegExp(conn._prefix + "tab", "g");
              return e.match(re) !== null;
            })) {
              globalPool = pool;
              break;
            }
          }

          
          let actorPrefix = conn._prefix + "test_one";
          let actors = Object.keys(globalPool._actors).join();
          info("Global actors: " + actors);

          isnot(actors.indexOf(actorPrefix), -1,
            "The test actor exists in the pool.");
          is(actors.indexOf(actorPrefix), actors.lastIndexOf(actorPrefix),
            "Only one actor exists in the pool.");

          gClient.close(finish);
        });
      });
    });
  });
}
