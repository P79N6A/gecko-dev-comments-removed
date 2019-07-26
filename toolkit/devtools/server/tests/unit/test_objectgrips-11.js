




var gDebuggee;
var gClient;
var gThreadClient;

function run_test()
{
  initTestDebuggerServer();
  gDebuggee = addTestGlobal("test-grips");
  gDebuggee.eval(function stopMe(arg1) {
    debugger;
  }.toString());

  gClient = new DebuggerClient(DebuggerServer.connectPipe());
  gClient.connect(function() {
    attachTestTabAndResume(gClient, "test-grips", function(aResponse, aTabClient, aThreadClient) {
      gThreadClient = aThreadClient;
      test_object_grip();
    });
  });
  do_test_pending();
}

function test_object_grip()
{
  gThreadClient.addOneTimeListener("paused", function(aEvent, aPacket) {
    let args = aPacket.frame.arguments;

    let objClient = gThreadClient.pauseGrip(args[0]);
    objClient.getOwnPropertyNames(function(aResponse) {
      do_check_eq(aResponse.ownPropertyNames.length, 5);
      do_check_eq(aResponse.ownPropertyNames[0], "message");
      do_check_eq(aResponse.ownPropertyNames[1], "stack");
      do_check_eq(aResponse.ownPropertyNames[2], "fileName");
      do_check_eq(aResponse.ownPropertyNames[3], "lineNumber");
      do_check_eq(aResponse.ownPropertyNames[4], "columnNumber");

      gThreadClient.resume(function() {
        finishClient(gClient);
      });
    });

  });

  gDebuggee.eval("stopMe(new TypeError('error message text'))");
}

