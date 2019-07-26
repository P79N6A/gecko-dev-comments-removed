


var gClient;
var gActors;








function run_test()
{
  DebuggerServer.addActors("resource://test/pre_init_global_actors.js");
  DebuggerServer.addActors("resource://test/pre_init_tab_actors.js");

  DebuggerServer.init(function () { return true; });
  DebuggerServer.addBrowserActors();

  DebuggerServer.addActors("resource://test/post_init_global_actors.js");
  DebuggerServer.addActors("resource://test/post_init_tab_actors.js");

  add_test(init);
  add_test(test_pre_init_global_actor);
  add_test(test_pre_init_tab_actor);
  add_test(test_post_init_global_actor);
  add_test(test_post_init_tab_actor);
  add_test(test_stable_global_actor_instances);
  add_test(close_client);
  run_next_test();
}

function init()
{
  gClient = new DebuggerClient(DebuggerServer.connectPipe());
  gClient.connect(function onConnect() {
    gClient.listTabs(function onListTabs(aResponse) {
      gActors = aResponse;
      run_next_test();
    });
  });
}

function test_pre_init_global_actor()
{
  gClient.request({ to: gActors.preInitGlobalActor, type: "ping" },
    function onResponse(aResponse) {
      do_check_eq(aResponse.message, "pong");
      run_next_test();
    }
  );
}

function test_pre_init_tab_actor()
{
  gClient.request({ to: gActors.preInitTabActor, type: "ping" },
    function onResponse(aResponse) {
      do_check_eq(aResponse.message, "pong");
      run_next_test();
    }
  );
}

function test_post_init_global_actor()
{
  gClient.request({ to: gActors.postInitGlobalActor, type: "ping" },
    function onResponse(aResponse) {
      do_check_eq(aResponse.message, "pong");
      run_next_test();
    }
  );
}

function test_post_init_tab_actor()
{
  gClient.request({ to: gActors.postInitTabActor, type: "ping" },
    function onResponse(aResponse) {
      do_check_eq(aResponse.message, "pong");
      run_next_test();
    }
  );
}


function getActorInstance(connID, actorID) {
  return DebuggerServer._connections[connID].getActor(actorID);
}

function test_stable_global_actor_instances()
{
  
  
  let connID = Object.keys(DebuggerServer._connections)[0];
  let postInitGlobalActor = getActorInstance(connID, gActors.postInitGlobalActor);
  let preInitGlobalActor = getActorInstance(connID, gActors.preInitGlobalActor);
  gClient.listTabs(function onListTabs(aResponse) {
    do_check_eq(postInitGlobalActor, getActorInstance(connID, aResponse.postInitGlobalActor));
    do_check_eq(preInitGlobalActor, getActorInstance(connID, aResponse.preInitGlobalActor));
    run_next_test();
  });
}

function close_client() {
  gClient.close(() => run_next_test());
}
