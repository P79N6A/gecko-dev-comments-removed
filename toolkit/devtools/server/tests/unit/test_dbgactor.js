


Cu.import("resource://gre/modules/devtools/dbg-server.jsm");
Cu.import("resource://gre/modules/devtools/dbg-client.jsm");

var gClient;
var gDebuggee;

const xpcInspector = Cc["@mozilla.org/jsinspector;1"].getService(Ci.nsIJSInspector);

function run_test()
{
  initTestDebuggerServer();
  gDebuggee = testGlobal("test-1");
  DebuggerServer.addTestGlobal(gDebuggee);

  let transport = DebuggerServer.connectPipe();
  gClient = new DebuggerClient(transport);
  gClient.addListener("connected", function(aEvent, aType, aTraits) {
    gClient.listTabs((aResponse) => {
      do_check_true('tabs' in aResponse);
      for (let tab of aResponse.tabs) {
        if (tab.title == "test-1") {
          test_attach_tab(tab.actor);
          return false;
        }
      }
      do_check_true(false); 
    });
  });

  gClient.connect();

  do_test_pending();
}


function test_attach_tab(aTabActor)
{
  gClient.request({ to: aTabActor, type: "attach" }, function(aResponse) {
    do_check_false("error" in aResponse);
    do_check_eq(aResponse.from, aTabActor);
    do_check_eq(aResponse.type, "tabAttached");
    do_check_true(typeof aResponse.threadActor === "string");

    test_attach_thread(aResponse.threadActor);
  });
}


function test_attach_thread(aThreadActor)
{
  gClient.request({ to: aThreadActor, type: "attach" }, function(aResponse) {
    do_check_false("error" in aResponse);
    do_check_eq(aResponse.from, aThreadActor);
    do_check_eq(aResponse.type, "paused");
    do_check_true("why" in aResponse);
    do_check_eq(aResponse.why.type, "attached");

    test_resume_thread(aThreadActor);
  });
}



function test_resume_thread(aThreadActor)
{
  
  gClient.request({ to: aThreadActor, type: "resume" }, function (aResponse) {
    do_check_false("error" in aResponse);
    do_check_eq(aResponse.from, aThreadActor);
    do_check_eq(aResponse.type, "resumed");

    do_check_eq(xpcInspector.eventLoopNestLevel, 0);

    
    Cu.evalInSandbox("var a = true; var b = false; debugger; var b = true;", gDebuggee);
    
    do_check_true(gDebuggee.b);
  });

  gClient.addListener("paused", function(aName, aPacket) {
    do_check_eq(aName, "paused");
    do_check_false("error" in aPacket);
    do_check_eq(aPacket.from, aThreadActor);
    do_check_eq(aPacket.type, "paused");
    do_check_true("actor" in aPacket);
    do_check_true("why" in aPacket)
    do_check_eq(aPacket.why.type, "debuggerStatement");

    
    
    do_check_true(gDebuggee.a);
    do_check_false(gDebuggee.b);

    do_check_eq(xpcInspector.eventLoopNestLevel, 1);

    
    gClient.request({ to: aThreadActor, type: "resume" }, cleanup);
  });
}

function cleanup()
{
  gClient.addListener("closed", function(aEvent, aResult) {
    do_test_finished();
  });

  try {
    let xpcInspector = Cc["@mozilla.org/jsinspector;1"].getService(Ci.nsIJSInspector);
    do_check_eq(xpcInspector.eventLoopNestLevel, 0);
  } catch(e) {
    dump(e);
  }

  gClient.close();
}
