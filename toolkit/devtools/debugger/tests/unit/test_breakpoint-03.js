






var gDebuggee;
var gClient;
var gThreadClient;

function run_test()
{
  initTestDebuggerServer();
  gDebuggee = addTestGlobal("test-stack");
  gClient = new DebuggerClient(DebuggerServer.connectPipe());
  gClient.connect(function () {
    attachTestGlobalClientAndResume(gClient,
                                    "test-stack",
                                    function (aResponse, aThreadClient) {
      gThreadClient = aThreadClient;
      test_skip_breakpoint();
    });
  });
  do_test_pending();
}

function test_skip_breakpoint()
{
  gThreadClient.addOneTimeListener("paused", function (aEvent, aPacket) {
    let path = getFilePath('test_breakpoint-03.js');
    let location = { url: path, line: gDebuggee.line0 + 3};
    gThreadClient.setBreakpoint(location, function (aResponse, bpClient) {
      
      do_check_eq(aResponse.actualLocation.url, location.url);
      do_check_eq(aResponse.actualLocation.line, location.line + 1);
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
      
      gThreadClient.resume();

    });

  });

  gDebuggee.eval("var line0 = Error().lineNumber;\n" +
                 "debugger;\n" +      
                 "var a = 1;\n" +     
                 "// A comment.\n" +  
                 "var b = 2;\n");     
}
