






var gDebuggee;
var gClient;
var gThreadClient;

function run_test()
{
  initTestDebuggerServer();
  gDebuggee = addTestGlobal("test-stack");
  gClient = new DebuggerClient(DebuggerServer.connectPipe());
  gClient.ready(function () {
    attachTestGlobalClientAndResume(gClient, "test-stack", function (aResponse, aThreadClient) {
      gThreadClient = aThreadClient;
      test_simple_breakpoint();
    });
  });
  do_test_pending();
}

function test_simple_breakpoint()
{
  gThreadClient.addOneTimeListener("paused", function (aEvent, aPacket) {
    let path = getFilePath('test_breakpoint-01.js');
    gThreadClient.setBreakpoint({ url: path, line: gDebuggee.line0 + 3}, function (aResponse, bpClient) {
      gThreadClient.addOneTimeListener("paused", function (aEvent, aPacket) {
        
        do_check_eq(aPacket.type, "paused");
        do_check_eq(aPacket.why.type, "breakpoint");
        do_check_eq(aPacket.why.actors[0], bpClient.actor);
        
        do_check_eq(gDebuggee.a, 1);
        do_check_eq(gDebuggee.b, undefined);

        
        bpClient.remove(function (aResponse) {
          gThreadClient.resume(function () {
            finishClient(gClient);
          });
        });

      });
      
      gThreadClient.resume(function () {
      });

    });

  });

  gDebuggee.eval("var line0 = Error().lineNumber;\n" +
                 "debugger;\n" +   
                 "var a = 1;\n" +  
                 "var b = 2;\n");  
}
