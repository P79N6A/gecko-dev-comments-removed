







var gDebuggee;
var gClient;
var gThreadClient;

function run_test()
{
  initTestDebuggerServer();
  gDebuggee = addTestGlobal("test-grips");
  gClient = new DebuggerClient(DebuggerServer.connectPipe());
  gClient.connect(function() {
    attachTestGlobalClientAndResume(gClient, "test-grips", function (aResponse, aThreadClient) {
      gThreadClient = aThreadClient;
      test_thread_lifetime();
    });
  });
  do_test_pending();
}

function test_thread_lifetime()
{
  gThreadClient.addOneTimeListener("paused", function (aEvent, aPacket) {
    let pauseGrip = aPacket.frame.arguments[0];

    gClient.request({ to: pauseGrip.actor, type: "threadGrip" }, function (aResponse) {
      
      do_check_eq(aResponse.error, undefined);

      let threadGrip1 = aResponse.from;

      gClient.request({ to: pauseGrip.actor, type: "threadGrip" }, function (aResponse) {
        do_check_eq(threadGrip1, aResponse.from);
        gThreadClient.resume(function() {
          finishClient(gClient);
        });
      });
    });
  });

  gDebuggee.eval("(" + function() {
    function stopMe(arg1) {
      debugger;
    };
    stopMe({obj: true});
    ")"
  } + ")()");
}
