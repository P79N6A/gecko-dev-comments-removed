






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
    aPacket.frame["arguments"][0];
    let grips = [];

    let handler = function(aResponse) {
      grips.push(aResponse.threadGrip);
      if (grips.length == 3) {
        test_release_many(grips);
      }
    };
    for (let i = 0; i < 3; i++) {
      gClient.request({ to: aPacket.frame["arguments"][i].actor, type: "threadGrip" },
                      handler);
    }
  });

  gDebuggee.eval("(" + function() {
    function stopMe(arg1, arg2, arg3) {
      debugger;
    };
    stopMe({obj: 1}, {obj: 2}, {obj: 3});
    ")"
  } + ")()");
}

function test_release_many(grips)
{
  

  let release = [grips[0].actor, grips[1].actor];

  gClient.request({ to: gThreadClient.actor, type: "releaseMany", "actors": release }, function(aResponse) {
    
    
    gClient.request({ to: grips[0].actor, type: "bogusRequest" }, function(aResponse) {
      do_check_eq(aResponse.error, "noSuchActor");
      gClient.request({ to: grips[1].actor, type: "bogusRequest" }, function(aResponse) {
        do_check_eq(aResponse.error, "noSuchActor");

        
        
        gClient.request({ to: grips[2].actor, type: "bogusRequest" }, function(aResponse) {
          do_check_eq(aResponse.error, "unrecognizedPacketType");
          gThreadClient.resume(function() {
            finishClient(gClient);
          });
        });
      });
    });
  });
}
