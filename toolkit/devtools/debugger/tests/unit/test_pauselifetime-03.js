






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
    let args = aPacket.frame["arguments"];
    let objActor = args[0].actor;
    do_check_eq(args[0]["class"], "Object");
    do_check_true(!!objActor);

    let objClient = gThreadClient.pauseGrip(args[0]);
    do_check_true(objClient.valid);

    
    
    gClient.request({ to: objActor, type: "bogusRequest" }, function(aResponse) {
      do_check_eq(aResponse.error, "unrecognizedPacketType");
      do_check_true(objClient.valid);

      gThreadClient.resume(function() {
        
        
        gClient.request({ to: objActor, type: "bogusRequest" }, function(aResponse) {
          do_check_false(objClient.valid);
          do_check_eq(aResponse.error, "noSuchActor");
          finishClient(gClient);
        });
      });
    });
  });

  gDebuggee.eval("(" + function() {
    function stopMe(aObject) {
      debugger;
    };
    stopMe({ foo: "bar" });
    ")"
  } + ")()");
}
