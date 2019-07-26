









var gDebuggee;
var gClient;
var gThreadClient;

function run_test()
{
  initTestDebuggerServer();
  gDebuggee = addTestGlobal("test-new-source-before-new-script");
  gClient = new DebuggerClient(DebuggerServer.connectPipe());
  gClient.connect(function() {
    attachTestGlobalClientAndResume(
      gClient,
      "test-new-source-before-new-script",
      function(aResponse, aThreadClient) {
        gThreadClient = aThreadClient;
        test_newSource_before_newScript();
      }
    );
  });
  do_test_pending();
}

function test_newSource_before_newScript()
{
  let newSourceFired = false;
  let newScriptFired = false;

  gClient.addOneTimeListener("newSource", function (aEvent, aPacket) {
    do_check_true(!aPacket.error);
    do_check_eq(aEvent, "newSource");
    do_check_eq(aPacket.type, "newSource");
    do_check_eq(newScriptFired, false);
    do_check_eq(newSourceFired, false);
    newSourceFired = true;
  });

  gClient.addOneTimeListener("newScript", function (aEvent, aPacket) {
    do_check_true(!aPacket.error);
    do_check_eq(aEvent, "newScript");
    do_check_eq(aPacket.type, "newScript");
    do_check_eq(newScriptFired, false);
    do_check_eq(newSourceFired, true);
    finishClient(gClient);
  });

  gDebuggee.eval(function inc(n) {
    return n+1;
  }.toString());
}
