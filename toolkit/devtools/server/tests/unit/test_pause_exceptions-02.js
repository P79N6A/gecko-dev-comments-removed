







var gDebuggee;
var gClient;
var gThreadClient;

function run_test()
{
  initTestDebuggerServer();
  gDebuggee = addTestGlobal("test-stack");
  gClient = new DebuggerClient(DebuggerServer.connectPipe());
  gClient.connect(function() {
    attachTestTabAndResume(gClient, "test-stack", function(aResponse, aTabClient, aThreadClient) {
      gThreadClient = aThreadClient;
      test_pause_frame();
    });
  });
  do_test_pending();
}

function test_pause_frame()
{
  gThreadClient.pauseOnExceptions(true, function () {
    gThreadClient.addOneTimeListener("paused", function(aEvent, aPacket) {
      do_check_eq(aPacket.why.type, "exception");
      do_check_eq(aPacket.why.exception, 42);
      gThreadClient.resume(function () {
        finishClient(gClient);
      });
    });

    gDebuggee.eval("(" + function() {
      function stopMe() {
        throw 42;
      };
      try {
        stopMe();
      } catch (e) {}
      ")"
    } + ")()");
  });
}
