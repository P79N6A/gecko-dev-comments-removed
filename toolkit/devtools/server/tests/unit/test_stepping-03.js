






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
      test_simple_stepping();
    });
  });
}

function test_simple_stepping()
{
  gThreadClient.addOneTimeListener("paused", function (aEvent, aPacket) {
    gThreadClient.addOneTimeListener("paused", function (aEvent, aPacket) {
      
      do_check_eq(aPacket.type, "paused");
      do_check_eq(aPacket.frame.where.line, gDebuggee.line0 + 4);
      do_check_eq(aPacket.why.type, "resumeLimit");
      
      do_check_eq(gDebuggee.a, 1);
      do_check_eq(gDebuggee.b, 2);

      gThreadClient.resume(function () {
        gClient.close(gCallback);
      });
    });
    gThreadClient.stepOut();

  });

  gDebuggee.eval("var line0 = Error().lineNumber;\n" +
                 "function f() {\n" + 
                 "  debugger;\n" +    
                 "  this.a = 1;\n" +  
                 "  this.b = 2;\n" +  
                 "}\n" +              
                 "f();\n");           
}
