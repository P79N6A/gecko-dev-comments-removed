







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
    let actors = [];
    let last;
    for (let aGrip of aPacket.frame.arguments) {
      actors.push(aGrip.actor);
      last = aGrip.actor;
    }

    
    gThreadClient.threadGrips(actors, function(aResponse) {
      
      do_check_eq(aResponse.error, undefined);

      gThreadClient.addOneTimeListener("paused", function(aEvent, aPacket) {
        
        actors.forEach(function(actor, i) {
          do_check_eq(actor, aPacket.frame.arguments[i].actor);
        });
        
        gThreadClient.releaseMany(actors, function(aResponse) {
          
          do_check_eq(aResponse.error, undefined);

          gClient.request({ to: last, type: "bogusRequest" }, function(aResponse) {
            do_check_eq(aResponse.error, "noSuchActor");
            gThreadClient.resume(function(aResponse) {
              finishClient(gClient);
            });
          });
        });
      });
      gThreadClient.resume();
    });
  });

  gDebuggee.eval("(" + function() {
    function stopMe(arg1, arg2, arg3) {
      debugger;
      debugger;
    };
    stopMe({obj: 1}, {obj: 2}, {obj: 3});
    ")"
  } + ")()");
}
