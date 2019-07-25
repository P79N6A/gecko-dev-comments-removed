






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
      test_syntax_error_eval();
    });
  });
  do_test_pending();
}

function test_syntax_error_eval()
{
  gThreadClient.addOneTimeListener("paused", function(aEvent, aPacket) {

    gThreadClient.getFrames(0, 2, function(aResponse) {
      let frame0 = aResponse.frames[0];
      let frame1 = aResponse.frames[1];

      
      gThreadClient.eval(frame0.actor, "arg", function(aResponse) {
        do_check_eq(aResponse.type, "resumed");
        gThreadClient.addOneTimeListener("paused", function(aEvent, aPacket) {
          
          do_check_eq(aPacket.type, "paused");
          do_check_eq(aPacket.why.type, "clientEvaluated");
          do_check_eq(aPacket.why.value, "arg0");

          
          gThreadClient.eval(frame1.actor, "arg", function(aResponse) {
            gThreadClient.addOneTimeListener("paused", function(aEvent, aPacket) {
              
              do_check_eq(aPacket.type, "paused");
              do_check_eq(aPacket.why.value, "arg1");

              gThreadClient.resume(function() {
                finishClient(gClient);
              });
            });
          });
        });
      });
    });
  });

  gDebuggee.eval("(" + function() {
    function frame0(arg) {
      debugger;
    }
    function frame1(arg) {
      frame0("arg0");
    }
    frame1("arg1");
  } + ")()");
}
