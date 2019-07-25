


var gDebuggee;
var gClient;
var gThreadClient;

function run_test()
{
  initTestDebuggerServer();
  gDebuggee = addTestGlobal("test-stack");
  gClient = new DebuggerClient(DebuggerServer.connectPipe());
  gClient.connect(function() {
    attachTestGlobalClientAndResume(gClient, "test-stack", function(aResponse, aThreadClient) {
      gThreadClient = aThreadClient;
      test_pause_frame();
    });
  });
  do_test_pending();
}

function test_pause_frame()
{
  gThreadClient.addOneTimeListener("paused", function(aEvent, aPacket) {
    
    gThreadClient.addOneTimeListener("framesadded", function() {
      do_check_false(gThreadClient.moreFrames);
      gThreadClient.resume(function() {
        finishClient(gClient);
      });
    });
    do_check_true(gThreadClient.fillFrames(3));
  });

  gDebuggee.eval("(" + function() {
    var recurseLeft = 1;
    function recurse() {
      if (--recurseLeft == 0) {
        debugger;
        return;
      }
      recurse();
    };
    recurse();
    ")"
  } + ")()");
}
