







var gDebuggee;
var gClient;
var gThreadClient;

function run_test()
{
  initTestDebuggerServer();
  gDebuggee = addTestGlobal("test-stack");
  gClient = new DebuggerClient(DebuggerServer.connectPipe());
  gClient.connect(function () {
    attachTestTabAndResume(gClient, "test-stack", function (aResponse, aTabClient, aThreadClient) {
      gThreadClient = aThreadClient;
      test_child_breakpoint();
    });
  });
  do_test_pending();
}

function test_child_breakpoint()
{
  gThreadClient.addOneTimeListener("paused", function (aEvent, aPacket) {
    let path = getFilePath('test_breakpoint-11.js');
    let location = { url: path, line: gDebuggee.line0 + 2};
    gThreadClient.setBreakpoint(location, function (aResponse, bpClient) {
      
      do_check_eq(aResponse.actualLocation, undefined);
      gThreadClient.addOneTimeListener("paused", function (aEvent, aPacket) {
        
        do_check_eq(aPacket.type, "paused");
        do_check_eq(aPacket.why.type, "breakpoint");
        do_check_eq(aPacket.why.actors[0], bpClient.actor);
        
        do_check_eq(gDebuggee.a, undefined);

        gThreadClient.addOneTimeListener("paused", function (aEvent, aPacket) {
          
          do_check_eq(aPacket.type, "paused");
          do_check_eq(aPacket.why.type, "breakpoint");
          do_check_eq(aPacket.why.actors[0], bpClient.actor);
          
          do_check_eq(gDebuggee.a.b, 1);
          do_check_eq(gDebuggee.res, undefined);

          
          bpClient.remove(function (aResponse) {
            gThreadClient.resume(function () {
              finishClient(gClient);
            });
          });
        });

        
        gThreadClient.resume();

      });
      
      gThreadClient.resume();

    });

  });


  gDebuggee.eval("var line0 = Error().lineNumber;\n" +
                 "debugger;\n" +                      
                 "var a = { b: 1, f: function() { return 2; } };\n" + 
                 "var res = a.f();\n");               
}
