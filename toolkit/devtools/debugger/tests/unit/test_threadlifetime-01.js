






var gDebuggee;
var gClient;
var gThreadClient;

function run_test()
{
  initTestDebuggerServer();
  gDebuggee = addTestGlobal("test-grips");
  gClient = new DebuggerClient(DebuggerServer.connectPipe());
  gClient.connect(function() {
    attachTestGlobalClientAndResume(gClient, "test-grips", function(aResponse, aThreadClient) {
      gThreadClient = aThreadClient;
      test_thread_lifetime();
    });
  });
  do_test_pending();
}

function test_thread_lifetime()
{
  gThreadClient.addOneTimeListener("paused", function(aEvent, aPacket) {
    let pauseGrip = aPacket.frame["arguments"][0];

    gClient.request({ to: pauseGrip.actor, type: "threadGrip" }, function(aResponse) {
      let threadGrip = aResponse.threadGrip;

      do_check_neq(pauseGrip.actor, threadGrip.actor);

      

      gThreadClient.addOneTimeListener("paused", function(aEvent, aPacket) {
        
        
        gClient.request({ to: pauseGrip.actor, type: "bogusRequest" }, function(aResponse) {
          do_check_eq(aResponse.error, "noSuchActor");
          gClient.request({ to: threadGrip.actor, type: "bogusRequest"}, function(aResponse) {
            do_check_eq(aResponse.error, "unrecognizedPacketType");
            gThreadClient.resume(function() {
              finishClient(gClient);
            });
          });
        });
      });
      gThreadClient.resume();
    });
  });

  gDebuggee.eval("(" + function() {
    function stopMe(arg1) {
      debugger;
      debugger;
    };
    stopMe({obj: true});
    ")"
  } + ")()");
}
