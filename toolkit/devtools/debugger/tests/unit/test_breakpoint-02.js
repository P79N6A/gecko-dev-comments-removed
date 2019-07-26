






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
      test_breakpoint_running();
    });
  });
  do_test_pending();
}

function test_breakpoint_running()
{
  gDebuggee.eval("var line0 = Error().lineNumber;\n" +
                 "var a = 1;\n" +  
                 "var b = 2;\n");  

  let path = getFilePath('test_breakpoint-02.js');
  let location = { url: path, line: gDebuggee.line0 + 2};

  
  gThreadClient.addOneTimeListener("paused", function (aEvent, aPacket) {
    do_check_eq(aPacket.type, "paused");
    do_check_eq(aPacket.why.type, "interrupted");
  });

  gThreadClient.setBreakpoint(location, function(aResponse) {
    
    
    do_check_neq(aResponse.error, "noScript");

    do_execute_soon(function() {
      finishClient(gClient);
    });
  });
}
