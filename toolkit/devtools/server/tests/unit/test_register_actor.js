


"use strict";

const Profiler = Cc["@mozilla.org/tools/profiler;1"].getService(Ci.nsIProfiler);

function check_actors(expect) {
  do_check_eq(expect, DebuggerServer.tabActorFactories.hasOwnProperty("registeredActor1"));
  do_check_eq(expect, DebuggerServer.tabActorFactories.hasOwnProperty("registeredActor2"));

  do_check_eq(expect, DebuggerServer.globalActorFactories.hasOwnProperty("registeredActor2"));
  do_check_eq(expect, DebuggerServer.globalActorFactories.hasOwnProperty("registeredActor1"));
}

function run_test()
{
  
  DebuggerServer.init(function () { return true; });
  DebuggerServer.addBrowserActors();

  add_test(test_deprecated_api);
  add_test(test_lazy_api);
  add_test(cleanup);
  run_next_test();
}

function test_deprecated_api() {
  
  DebuggerServer.registerModule("xpcshell-test/registertestactors-01");
  DebuggerServer.registerModule("xpcshell-test/registertestactors-02");

  check_actors(true);

  check_except(() => {
    DebuggerServer.registerModule("xpcshell-test/registertestactors-01");
  });
  check_except(() => {
    DebuggerServer.registerModule("xpcshell-test/registertestactors-02");
  });

  DebuggerServer.unregisterModule("xpcshell-test/registertestactors-01");
  DebuggerServer.unregisterModule("xpcshell-test/registertestactors-02");
  check_actors(false);

  DebuggerServer.registerModule("xpcshell-test/registertestactors-01");
  DebuggerServer.registerModule("xpcshell-test/registertestactors-02");
  check_actors(true);

  run_next_test();
}


function test_lazy_api() {
  let isActorLoaded = false;
  let isActorInstanciated = false;
  function onActorEvent(subject, topic, data) {
    if (data == "loaded") {
      isActorLoaded = true;
    } else if (data == "instantiated") {
      isActorInstanciated = true;
    }
  }
  Services.obs.addObserver(onActorEvent, "actor", false);
  DebuggerServer.registerModule("xpcshell-test/registertestactors-03", {
    prefix: "lazy",
    constructor: "LazyActor",
    type: { global: true, tab: true }
  });
  
  do_check_true(DebuggerServer.tabActorFactories.hasOwnProperty("lazyActor"));
  do_check_true(DebuggerServer.globalActorFactories.hasOwnProperty("lazyActor"));
  do_check_false(isActorLoaded);
  do_check_false(isActorInstanciated);

  let client = new DebuggerClient(DebuggerServer.connectPipe());
  client.connect(function onConnect() {
    client.listTabs(onListTabs);
  });
  function onListTabs(aResponse) {
    
    
    do_check_false(isActorLoaded);
    do_check_false(isActorInstanciated);
    do_check_true("lazyActor" in aResponse);

    let {LazyFront} = devtools.require("xpcshell-test/registertestactors-03");
    let front = LazyFront(client, aResponse);
    front.hello().then(onRequest);
  }
  function onRequest(aResponse) {
    do_check_eq(aResponse, "world");

    
    do_check_true(isActorLoaded);
    do_check_true(isActorInstanciated);

    Services.obs.removeObserver(onActorEvent, "actor", false);
    client.close(() => run_next_test());
  }
}

function cleanup() {
  DebuggerServer.destroy();

  
  check_actors(false);
  do_check_false(DebuggerServer.tabActorFactories.hasOwnProperty("lazyActor"));
  do_check_false(DebuggerServer.globalActorFactories.hasOwnProperty("lazyActor"));

  run_next_test();
}

