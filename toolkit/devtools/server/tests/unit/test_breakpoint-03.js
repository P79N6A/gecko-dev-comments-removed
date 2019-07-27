






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
    attachTestTabAndResume(gClient,
                           "test-stack",
                           function (aResponse, aTabClient, aThreadClient) {
                             gThreadClient = aThreadClient;
                             test_skip_breakpoint();
                           });
  });
}

function test_skip_breakpoint()
{
  gThreadClient.addOneTimeListener("paused", function (aEvent, aPacket) {
    let location = { line: gDebuggee.line0 + 3 };
    let source = gThreadClient.source(aPacket.frame.where.source);

    source.setBreakpoint(location, function (aResponse, bpClient) {
      
      do_check_true(!!aResponse.actualLocation);
      do_check_eq(aResponse.actualLocation.source.actor, source.actor);
      do_check_eq(aResponse.actualLocation.line, location.line + 1);

      gThreadClient.addOneTimeListener("paused", function (aEvent, aPacket) {
        
        do_check_eq(aPacket.type, "paused");
        do_check_eq(aPacket.frame.where.source.actor, source.actor);
        do_check_eq(aPacket.frame.where.line, location.line + 1);
        do_check_eq(aPacket.why.type, "breakpoint");
        do_check_eq(aPacket.why.actors[0], bpClient.actor);
        
        do_check_eq(gDebuggee.a, 1);
        do_check_eq(gDebuggee.b, undefined);

        
        bpClient.remove(function (aResponse) {
          gThreadClient.resume(function () {
            gClient.close(gCallback);
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
                 "var b = 2;");     
}
