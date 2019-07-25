






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
      test_throw_eval();
    });
  });
  do_test_pending();
}

function test_throw_eval()
{
  gThreadClient.addOneTimeListener("paused", function(aEvent, aPacket) {
    gThreadClient.eval(null, "throw 'failure'", function(aResponse) {
      do_check_eq(aResponse.type, "resumed");
      
      gThreadClient.addOneTimeListener("paused", function(aEvent, aPacket) {
        
        do_check_eq(aPacket.type, "paused");
        do_check_eq(aPacket.why.type, "clientEvaluated");
        do_check_eq(aPacket.why.exception, "failure");
        gThreadClient.resume(function() {
          finishClient(gClient);
        });
      });
    });
  });

  gDebuggee.eval("(" + function() {
    function stopMe(arg1) { debugger; };
    stopMe({obj: true});
  } + ")()");
}
