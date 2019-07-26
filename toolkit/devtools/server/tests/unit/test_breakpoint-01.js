





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
      test_simple_breakpoint();
    });
  });
}

function test_simple_breakpoint()
{
  gThreadClient.addOneTimeListener("paused", function (aEvent, aPacket) {
    let path = getFilePath('test_breakpoint-01.js');
    let location = {
      url: path,
      line: gDebuggee.line0 + 3
    };
    gThreadClient.setBreakpoint(location, function (aResponse, bpClient) {
      gThreadClient.addOneTimeListener("paused", function (aEvent, aPacket) {
        
        do_check_eq(aPacket.type, "paused");
        do_check_eq(aPacket.frame.where.url, path);
        do_check_eq(aPacket.frame.where.line, location.line);
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

  Components.utils.evalInSandbox("var line0 = Error().lineNumber;\n" +
                                 "debugger;\n" +   
                                 "var a = 1;\n" +  
                                 "var b = 2;\n",   
                                 gDebuggee);
}
