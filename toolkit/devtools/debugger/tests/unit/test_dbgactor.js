


Cu.import("resource://gre/modules/devtools/dbg-server.jsm");
Cu.import("resource://gre/modules/devtools/dbg-client.jsm");

var gClient;
var gDebuggee;

function run_test()
{
  DebuggerServer.addActors("resource://test/testactors.js");

  
  DebuggerServer.init(function () { return true; });
  gDebuggee = testGlobal("test-1");
  DebuggerServer.addTestGlobal(gDebuggee);

  let transport = DebuggerServer.connectPipe();
  gClient = new DebuggerClient(transport);
  gClient.addListener("connected", function(aEvent, aType, aTraits) {
    gClient.request({ to: "root", type: "listContexts" }, function(aResponse) {
      do_check_true('contexts' in aResponse);
      for each (let context in aResponse.contexts) {
        if (context.global == "test-1") {
          test_attach(context);
          return false;
        }
      }
      do_check_true(false);
    });
  });
  gClient.connect();

  do_test_pending();
}

function test_attach(aContext)
{
  gClient.request({ to: aContext.actor, type: "attach" }, function(aResponse) {
    do_check_true(!aResponse.error);
    do_check_eq(aResponse.type, "paused");

    
    gClient.request({ to: aContext.actor, type: "resume" }, function() {
      test_debugger_statement(aContext);
    });
  });
}

function test_debugger_statement(aContext)
{
  gClient.addListener("paused", function(aName, aPacket) {
    
    
    do_check_true(gDebuggee.a);
    do_check_false(gDebuggee.b);

    let xpcInspector = Cc["@mozilla.org/jsinspector;1"].getService(Ci.nsIJSInspector);
    do_check_eq(xpcInspector.eventLoopNestLevel, 1);

    gClient.request({ to: aContext.actor, type: "resume" }, function() {
      cleanup();
    });
  });

  Cu.evalInSandbox("var a = true; var b = false; debugger; var b = true;", gDebuggee);
  
  do_check_true(gDebuggee.b);
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
