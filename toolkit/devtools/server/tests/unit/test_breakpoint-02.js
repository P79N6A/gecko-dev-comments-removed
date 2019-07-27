






var gDebuggee;
var gClient;
var gThreadClient;
var gCallback;

function run_test()
{
  run_test_with_server(DebuggerServer, function () {
    run_test_with_server(WorkerDebuggerServer, do_test_finished);
  });
  do_test_pending();
};

function run_test_with_server(aServer, aCallback)
{
  gCallback = aCallback;
  initTestDebuggerServer(aServer);
  gDebuggee = addTestGlobal("test-stack", aServer);
  gClient = new DebuggerClient(aServer.connectPipe());
  gClient.connect(function () {
    attachTestTabAndResume(gClient, "test-stack", function (aResponse, aTabClient, aThreadClient) {
      gThreadClient = aThreadClient;
      test_breakpoint_running();
    });
  });
}

function test_breakpoint_running()
{
  gThreadClient.addOneTimeListener("paused", function (aEvent, aPacket) {
    let location = { line: gDebuggee.line0 + 3 };

    gThreadClient.resume();

    
    gThreadClient.addOneTimeListener("paused", function (aEvent, aPacket) {
      do_check_eq(aPacket.type, "paused");
      do_check_eq(aPacket.why.type, "interrupted");
    });

    let source = gThreadClient.source(aPacket.frame.where.source);
    source.setBreakpoint(location, function(aResponse) {
      
      
      do_check_neq(aResponse.error, "noScript");

      do_execute_soon(function() {
        gClient.close(gCallback);
      });
    });
  });

  gDebuggee.eval("var line0 = Error().lineNumber;\n" +
                 "debugger;\n" +
                 "var a = 1;\n" +  
                 "var b = 2;\n");  
}
