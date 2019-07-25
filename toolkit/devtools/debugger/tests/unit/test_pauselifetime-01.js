






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
  gThreadClient.addOneTimeListener("paused", function(aEvent, aPacket) {
    let pauseActor = aPacket["actor"];

    
    
    gClient.request({ to: pauseActor, type: "bogusRequest" }, function(aResponse) {
      do_check_eq(aResponse.error, "unrecognizedPacketType");

      gThreadClient.resume(function() {
        
        
        gClient.request({ to: pauseActor, type: "bogusRequest" }, function(aResponse) {
          do_check_eq(aResponse.error, "noSuchActor");
          finishClient(gClient);
        });
      });

    });
  });

  gDebuggee.eval("(" + function() {
    function stopMe() {
      debugger;
    };
    stopMe();
    ")"
  } + ")()");
}
