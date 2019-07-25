






var gDebuggee;
var gClient;
var gThreadClient;

function run_test()
{
  initTestDebuggerServer();
  gDebuggee = addTestGlobal("test-stack");
  gClient = new DebuggerClient(DebuggerServer.connectPipe());
  gClient.ready(function() {
    attachTestGlobalClientAndResume(gClient, "test-stack", function(aResponse, aThreadClient) {
      gThreadClient = aThreadClient;
      test_pause_frame();
    });
  });
  do_test_pending();
}

function test_pause_frame()
{
  gThreadClient.addOneTimeListener("paused", function(aEvent, aPacket1) {
    gThreadClient.addOneTimeListener("paused", function(aEvent, aPacket2) {
      do_check_eq(aPacket1.frame.actor, aPacket2.frame.actor);
      gThreadClient.resume(function() {
        finishClient(gClient);
      });
    });
    gThreadClient.resume();
  });

  gDebuggee.eval("(" + function() {
    function stopMe() {
      debugger;
      debugger;
    };
    stopMe();
    ")"
  } + ")()");
}
